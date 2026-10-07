#include "jerico/dock_deform.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace jerico {
namespace {
void need(bool condition,const std::string& reason){
 if(!condition)throw std::invalid_argument("timber edge deformation: "+reason);
}
double number(const Json& value,const std::string& name){
 need(value.is_number(),name+" must be numeric");double result=value.get<double>();
 need(std::isfinite(result),name+" must be finite");return result;
}
V3 vec(const Json& value,const std::string& name){
 need(value.is_array()&&value.size()==3,name+" must be a three-vector");
 return {number(value[0],name),number(value[1],name),number(value[2],name)};
}
double smooth(double t){t=std::clamp(t,0.,1.);return t*t*t*(t*(t*6-15)+10);}
uint64_t mix(uint64_t h){h^=h>>30;h*=0xbf58476d1ce4e5b9ULL;h^=h>>27;h*=0x94d049bb133111ebULL;return h^(h>>31);}
double random(uint64_t key,int64_t index){return double(mix(key^mix(uint64_t(index))))/18446744073709551616.;}
double noise(double coordinate,uint64_t key){
 const auto index=static_cast<int64_t>(std::floor(coordinate));double t=smooth(coordinate-double(index));
 return (1-t)*random(key,index)+t*random(key,index+1);
}
struct Geometry{std::vector<V3> vertices;std::vector<V3> normals;double volume=0;};
Geometry geometry(const Json& part,V3 origin){
 need(part.contains("vertices")&&part["vertices"].is_array()&&part["vertices"].size()>=4&&part["vertices"].size()<=60000,"invalid vertex budget");
 need(part.contains("faces")&&part["faces"].is_array()&&part["faces"].size()>=4&&part["faces"].size()<=100000,"invalid face budget");
 need(part.contains("uvs")&&part["uvs"].is_array()&&part["uvs"].size()==part["faces"].size(),"UV face count mismatch");
 Geometry result;for(const auto& vertex:part["vertices"])result.vertices.push_back(vec(vertex,"vertex"));
 std::map<std::pair<size_t,size_t>,unsigned> edges;
 size_t fi=0;
 for(const auto& face:part["faces"]){
  need(face.is_array()&&face.size()==3,"mesh must be triangulated before deformation");
  std::array<size_t,3> ids{};
  for(size_t j=0;j<3;++j){need(face[j].is_number_integer()&&face[j].get<double>()>=0&&face[j].get<double>()<double(result.vertices.size()),"invalid vertex index");ids[j]=face[j].get<size_t>();}
  const auto a=sub(result.vertices[ids[0]],origin),b=sub(result.vertices[ids[1]],origin),c=sub(result.vertices[ids[2]],origin);
  auto ab=sub(b,a),ac=sub(c,a),n=cross(ab,ac);
  need(dot(ab,ab)>0&&dot(ac,ac)>0&&dot(n,n)>1e-20*dot(ab,ab)*dot(ac,ac),"degenerate triangle");
  result.normals.push_back(n);result.volume+=dot(a,cross(b,c))/6;
  for(size_t j=0;j<3;++j)++edges[{ids[j],ids[(j+1)%3]}];
  const auto& uv=part["uvs"][fi++];need(uv.is_array()&&uv.size()==3,"triangle UV count mismatch");
  for(const auto& corner:uv){need(corner.is_array()&&corner.size()==2,"UV must be a two-vector");number(corner[0],"UV");number(corner[1],"UV");}
 }
 for(const auto& edge:edges){auto reverse=edges.find({edge.first.second,edge.first.first});need(edge.second==1&&reverse!=edges.end()&&reverse->second==1,"mesh must be closed and consistently oriented");}
 need(std::isfinite(result.volume)&&result.volume>1e-14,"mesh must have positive volume");
 return result;
}
}

Json deform_timber_edges(const Json& part,const Json& operation){
 need(part.is_object()&&operation.is_object(),"part and operation must be objects");
 const std::set<std::string> allowed={"amount_m","seed","edge_band_m","protected_intervals","start_protection_m","end_protection_m"};
 for(auto it=operation.begin();it!=operation.end();++it)need(allowed.count(it.key())!=0,"unknown operation field "+it.key());
 for(const char* field:{"normals","tangents","textures","texture_uvs","native_bevel"})need(!part.contains(field),std::string("apply before evaluation and bake; found ")+field);
 need(number(part.value("bevel",Json(0)),"bevel")==0,"explicit prebuilt bevel is required; pending bevel is unsupported");
 need(part.contains("timber_frame")&&part["timber_frame"].is_object(),"missing timber_frame");const auto& f=part["timber_frame"];
 for(const char* field:{"origin","u","v","w"})need(f.contains(field),std::string("missing frame ")+field);
 const auto origin=vec(f["origin"],"origin"),u=vec(f["u"],"u"),v=vec(f["v"],"v"),w=vec(f["w"],"w");
 for(auto axis:{u,v,w})need(std::abs(dot(axis,axis)-1)<1e-8,"frame axes must be unit length");
 need(std::abs(dot(u,v))<1e-8&&std::abs(dot(u,w))<1e-8&&std::abs(dot(v,w))<1e-8&&dot(cross(u,v),w)>1-1e-8,"frame must be orthonormal and right-handed");
 for(const char* field:{"timber_length","timber_width","timber_thickness"})need(part.contains(field),std::string("missing ")+field);
 const double length=number(part["timber_length"],"length"),width=number(part["timber_width"],"width"),thickness=number(part["timber_thickness"],"thickness");
 need(length>=.01&&length<=10000&&width>0&&thickness>0,"invalid timber dimensions");
 const double amount=number(operation.value("amount_m",Json(0)),"amount_m");
 const double band=number(operation.value("edge_band_m",Json(std::min(width*.25,thickness*.45))),"edge_band_m");
 need(band>0&&band<=width*.5,"edge band must be positive and at most half the width");
 // The quintic mask has maximum derivative 1.875. This bound makes the
 // transverse map strictly increasing at every u, independently of noise.
 need(amount>=0&&amount<=std::min({thickness*.12,width*.08,band*.20}),"amount exceeds thickness, width or monotonic edge-mask bound");
 const int64_t seed=recipe_seed(operation.value("seed",Json(0)));
 const double start=number(operation.value("start_protection_m",Json(std::min(.10,length*.10))),"start_protection_m");
 const double end=number(operation.value("end_protection_m",Json(std::min(.10,length*.10))),"end_protection_m");
 need(start>=0&&end>=0&&start<=length&&end<=length,"end protection outside timber length");
 const Json ranges=operation.value("protected_intervals",Json::array());need(ranges.is_array()&&ranges.size()<=4096,"invalid protection interval budget");
 std::vector<V2> intervals;
 for(const auto& range:ranges){need(range.is_array()&&range.size()==2,"protection interval needs two metric coordinates");double a=number(range[0],"interval start"),b=number(range[1],"interval end");need(a>=0&&b<=length&&a<=b,"invalid protection interval");intervals.push_back({a,b});}
 need(!part.contains("protected_support")||part["protected_support"].is_boolean(),"protected_support must be boolean");
 const auto source=geometry(part,origin);
 if(amount==0||part.value("protected_support",false))return part;
 uint64_t key=1469598103934665603ULL^uint64_t(seed);need(part.value("id",Json("")).is_string(),"id must be a string");
 for(unsigned char c:part.value("id",std::string(""))){key^=c;key*=1099511628211ULL;}
 const double falloff=std::max(.04,2*band),wavelength=std::max(.30,8*band),tolerance=1e-10;
 Json result=part;size_t changed=0,protected_vertices=0;
 for(size_t i=0;i<source.vertices.size();++i){
  const auto p=source.vertices[i],relative=sub(p,origin);const double x=dot(relative,u),y=dot(relative,v);
  need(x>=-tolerance&&x<=length+tolerance,"vertex outside authored longitudinal domain");
  double weight=smooth((x-start-tolerance)/falloff)*smooth((length-end-x-tolerance)/falloff);
  for(auto interval:intervals){double distance=x<interval[0]?interval[0]-x:(x>interval[1]?x-interval[1]:0);weight*=smooth((distance-tolerance)/falloff);}
  if(weight==0){++protected_vertices;continue;}
  double edge=smooth(1-std::max(0.,width*.5-std::abs(y))/band);if(edge==0)continue;
  const uint64_t side=key^(y<0?0x9e3779b97f4a7c15ULL:0xd1b54a32d192ed03ULL);
  const double field=.35+.65*(.72*noise(x/wavelength,side)+.28*noise(x*2/wavelength,side^0x94d049bb133111ebULL));
  const double displacement=(y<0?1:-1)*amount*weight*edge*field;
  const auto q=add(p,mul(v,displacement));if(q!=p){result["vertices"][i]=q;++changed;}
 }
 if(changed==0)return part;
 const auto deformed=geometry(result,origin);
 need(deformed.volume>=source.volume*.5,"deformation removes excessive volume");
 for(size_t i=0;i<source.normals.size();++i)need(dot(source.normals[i],deformed.normals[i])>0,"deformation would invert a triangle");
 result["timber_edge_deformation"]={{"method","grain_aligned_continuous_transverse_edge_field"},{"operation",operation},{"amount_m",amount},{"edge_band_m",band},{"falloff_m",falloff},{"wavelength_m",wavelength},{"changed_vertices",changed},{"protected_vertices",protected_vertices},{"uvs_preserved",true},{"connectivity_preserved",true},{"height_preserved",true},{"transverse_jacobian_lower_bound",1-1.875*amount/band},{"source_volume_m3",source.volume},{"result_volume_m3",deformed.volume},{"limitation","Static edge shape only; no strength, fragment physics or inter-part collision solve"}};
 return result;
}
}
