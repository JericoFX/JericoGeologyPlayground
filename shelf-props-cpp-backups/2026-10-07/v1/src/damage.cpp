// Native ports of the frozen R2 damage_math.py and crack_math.py.
// Insertion-ordered maps below preserve Python vertex/face emission order.
#include "jerico/damage.hpp"
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>
namespace jerico {
namespace {
using Polygon=std::vector<V2>;using Cells=std::vector<Polygon>;using Bounds=std::array<double,6>;using Face=std::vector<size_t>;
constexpr double eps=1e-10,area_eps=1e-12,pi=3.14159265358979323846264338327950288;
[[noreturn]] void fail(const std::string&message){throw std::invalid_argument(message);}
template<class K,class V>class OrderedMap {
 std::map<K,size_t> index;std::vector<std::optional<std::pair<K,V>>> records;
 public:
 bool contains(const K&k)const{return index.count(k)!=0;}
 size_t size()const{return index.size();}
 bool empty()const{return index.empty();}
 V&at(const K&k){return records.at(index.at(k))->second;}
 const V&at(const K&k)const{return records.at(index.at(k))->second;}
 void set(const K&k,const V&v){auto found=index.find(k);if(found!=index.end()){records[found->second]->second=v;return;}index.emplace(k,records.size());records.emplace_back(std::make_pair(k,v));}
 void setdefault(const K&k,const V&v){if(!contains(k))set(k,v);}
 void erase(const K&k){auto found=index.find(k);if(found!=index.end()){records[found->second].reset();index.erase(found);}}
 std::vector<std::pair<K,V>> items()const{std::vector<std::pair<K,V>> result;for(const auto&record:records)if(record)result.push_back(*record);return result;}
};
// CPython round(x,11) rounds the exact binary input to decimal then converts
// that decimal back to binary64. Multiplying by 1e11 would add a rounding step.
double round11(double value){char text[384];const auto written=std::to_chars(text,text+sizeof(text),value,std::chars_format::fixed,11);if(written.ec!=std::errc())fail("Coordinate rounding failed");double result=0;const auto parsed=std::from_chars(text,written.ptr,result);if(parsed.ec!=std::errc())fail("Coordinate rounding failed");return result;}
template<size_t N>std::array<double,N> key(const std::array<double,N>&v){auto result=v;for(double&x:result)x=round11(x);return result;}
double dist(V2 a,V2 b){return std::hypot(a[0]-b[0],a[1]-b[1]);}
double cross2(V2 a,V2 b,V2 c){return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);}
double area(const Polygon&p){double s=0;for(size_t i=1;i+1<p.size();++i)s+=cross2(p[0],p[i],p[i+1]);return s/2;}
V2 center(const Polygon&p){V2 result{0,0};for(int k=0;k<2;++k){for(auto v:p)result[k]+=v[k];result[k]/=p.size();}return result;}
bool close(double a,double b,double relative,double absolute){return std::abs(a-b)<=std::max(absolute,relative*std::max(std::abs(a),std::abs(b)));}
bool on_segment(V2 p,V2 a,V2 b,double tolerance=eps){double length=dist(a,b);return std::abs(cross2(a,b,p))<=tolerance*std::max(length,tolerance)&&std::min(a[0],b[0])-tolerance<=p[0]&&p[0]<=std::max(a[0],b[0])+tolerance&&std::min(a[1],b[1])-tolerance<=p[1]&&p[1]<=std::max(a[1],b[1])+tolerance;}
bool intersects(V2 a,V2 b,V2 c,V2 d){return (cross2(a,b,c)*cross2(a,b,d)<0&&cross2(c,d,a)*cross2(c,d,b)<0)||on_segment(c,a,b)||on_segment(d,a,b)||on_segment(a,c,d)||on_segment(b,c,d);}
bool contains(const Polygon&p,V2 point,bool boundary=true){bool inside=false;for(size_t i=0;i<p.size();++i){auto a=p[i],b=p[(i+1)%p.size()];if(on_segment(point,a,b))return boundary;if((a[1]>point[1])!=(b[1]>point[1])){double crossing=a[0]+(point[1]-a[1])*(b[0]-a[0])/(b[1]-a[1]);if(point[0]<crossing)inside=!inside;}}return inside;}
template<size_t N>std::array<double,N> finite_vector(const Json&v,const std::string&label){if(!v.is_array()||v.size()!=N)fail(label+" must have "+std::to_string(N)+" coordinates");std::array<double,N> result;for(size_t i=0;i<N;++i){if(!v[i].is_number()||!std::isfinite(v[i].get<double>()))fail(label+" must contain finite numeric coordinates");result[i]=v[i].get<double>();}return result;}
Polygon profile(const Json&value){
 if(!value.is_array()||value.size()<3||value.size()>64)fail("Cutter must contain 3..64 vertices");
 Polygon p;for(const auto&v:value)p.push_back(finite_vector<2>(v,"Cutter point"));
 for(size_t i=0;i<p.size();++i)for(size_t j=i+1;j<p.size();++j)if(dist(p[i],p[j])<=eps)fail("Cutter has repeated or coincident vertices");
 for(size_t i=0;i<p.size();++i){auto a=p[i],b=p[(i+1)%p.size()],prev=p[(i+p.size()-1)%p.size()];
  for(size_t j=i+1;j<p.size();++j){if(j==i+1||(i==0&&j==p.size()-1))continue;if(intersects(a,b,p[j],p[(j+1)%p.size()]))fail("Cutter must be a simple, non-self-intersecting polygon");}
  if(std::abs(cross2(prev,a,b))<=eps*dist(prev,b)&&dot(sub({a[0],0,a[1]},{prev[0],0,prev[1]}),sub({b[0],0,b[1]},{a[0],0,a[1]}))<0)fail("Cutter doubles back along an edge");
 }
 double ar=area(p);if(std::abs(ar)<=area_eps)fail("Cutter has zero or unsupported tiny area");if(ar<0)std::reverse(p.begin(),p.end());return p;
}
Bounds bounds(const Json&part,bool check_mesh){
 if(!part.is_object()||!part.contains("vertices")||part["vertices"].empty()||!part.contains("faces")||part["faces"].empty())fail("Panel must be a nonempty mesh part");
 if(part["vertices"].size()>60000||part["faces"].size()>100000)fail("Input mesh exceeds panel budget");
 std::vector<V3> vertices;for(const auto&v:part["vertices"])vertices.push_back(finite_vector<3>(v,"Panel vertex"));
 Bounds result;
 if(part.contains("panel_bounds")&&!part["panel_bounds"].is_null())result=finite_vector<6>(part["panel_bounds"],"panel_bounds");
 else for(size_t axis=0;axis<3;++axis){double low=vertices[0][axis],high=low;for(auto v:vertices){low=std::min(low,v[axis]);high=std::max(high,v[axis]);}result[axis*2]=low;result[axis*2+1]=high;}
 if(std::min({result[1]-result[0],result[3]-result[2],result[5]-result[4]})<=eps||std::any_of(result.begin(),result.end(),[](double v){return std::abs(v)>1e5;}))fail("Panel bounds are degenerate or outside supported metric range");
 for(auto v:vertices)for(size_t k=0;k<3;++k)if(v[k]<result[k*2]-eps||v[k]>result[k*2+1]+eps)fail("Panel vertices lie outside panel_bounds");
 if(check_mesh){
  std::map<std::pair<size_t,size_t>,int> edges;double volume=0;bool original=!part.contains("cuts")||part["cuts"].empty();
  for(const auto&face:part["faces"]){
   if(!face.is_array()||face.size()<3)fail("Invalid panel face");
   Face indices;std::vector<V3> points;
   for(const auto&i:face){if(!i.is_number_integer()||i.get<int64_t>()<0||i.get<size_t>()>=vertices.size())fail("Invalid panel face");indices.push_back(i.get<size_t>());points.push_back(vertices[i.get<size_t>()]);}
   if(original){bool exterior=false;for(size_t axis=0;axis<3;++axis)for(size_t end=0;end<2;++end)if(std::all_of(points.begin(),points.end(),[&](V3 p){return std::abs(p[axis]-result[axis*2+end])<=eps;}))exterior=true;if(!exterior)fail("Only axis-aligned rectangular panel prisms are supported");}
   for(size_t i=0;i<indices.size();++i)++edges[{indices[i],indices[(i+1)%indices.size()]}];
   for(size_t k=1;k+1<points.size();++k){auto normal=cross(sub(points[k],points[0]),sub(points[k+1],points[0]));if(dot(normal,normal)<=1e-20)fail("Panel contains degenerate triangles");volume+=dot(points[0],cross(points[k],points[k+1]))/6;}
  }
  bool valid=true;for(const auto&e:edges)if(e.second!=1||edges[{e.first.second,e.first.first}]!=1)valid=false;
  if(!valid||volume<=area_eps)fail("Panel must be closed, consistently oriented and positive-volume");
  if(original&&!close(volume,(result[1]-result[0])*(result[3]-result[2])*(result[5]-result[4]),1e-8,1e-12))fail("Mesh is not a complete rectangular panel prism");
 }
 return result;
}
struct Cut {Polygon polygon;std::string side;Json depth;double low,high;Json json()const{return {{"polygon_xz",polygon},{"side",side},{"depth",depth},{"through",depth.is_null()},{"y_range",{low,high}}};}};
Cut cut_record(const Json&polygon,const std::string&side,Json depth,const Bounds&b){
 if(side!="front"&&side!="back")fail("side must be 'front' or 'back'");
 double low=b[2],high=b[3];
 if(!depth.is_null()){if(!depth.is_number()||!std::isfinite(depth.get<double>())||!(eps<depth.get<double>()&&depth.get<double>()<b[3]-b[2]-eps))fail("Numeric depth must be positive and strictly less than thickness; use None for through");depth=depth.get<double>();if(side=="front")high=b[2]+depth.get<double>();else low=b[3]-depth.get<double>();}
 return {profile(polygon),side,depth,low,high};
}
std::vector<Cut> read_cuts(const Json&part,const Bounds&b){std::vector<Cut> cuts;const Json raw=part.value("cuts",Json::array());if(!raw.is_array()||raw.size()>12)fail("Panel exceeds cut budget");for(const auto&cut:raw){if(!cut.is_object())fail("Malformed cut metadata");cuts.push_back(cut_record(cut.value("polygon_xz",Json(nullptr)),cut.value("side",Json(nullptr)).is_string()?cut["side"].get<std::string>():"",cut.value("depth",Json(nullptr)),b));}return cuts;}
Polygon clean(const Polygon&p){Polygon result;for(auto point:p)if(result.empty()||dist(point,result.back())>eps)result.push_back(point);if(result.size()>1&&dist(result.front(),result.back())<=eps)result.pop_back();return result;}
Cells split(const Polygon&cell,V2 a,V2 b){
 double length=dist(a,b);std::vector<double> distances;for(auto p:cell)distances.push_back(cross2(a,b,p)/length);
 if(*std::min_element(distances.begin(),distances.end())>=-eps||*std::max_element(distances.begin(),distances.end())<=eps)return {cell};
 Polygon positive,negative;
 for(size_t i=0;i<cell.size();++i){size_t j=(i+1)%cell.size();double d=distances[i],other=distances[j];auto point=cell[i];if(d>=-eps)positive.push_back(point);if(d<=eps)negative.push_back(point);
  if((d>eps&&other<-eps)||(d<-eps&&other>eps)){double t=d/(d-other);V2 hit={point[0]+t*(cell[j][0]-point[0]),point[1]+t*(cell[j][1]-point[1])};positive.push_back(hit);negative.push_back(hit);}}
 Cells result;for(auto polygon:{positive,negative}){polygon=clean(polygon);if(polygon.size()>=3&&area(polygon)>0)result.push_back(polygon);else fail("Cutter produces a numerically degenerate cell");}return result;
}
Cells partition(const Bounds&b,const std::vector<Cut>&cuts){
 Cells cells={{{b[0],b[4]},{b[1],b[4]},{b[1],b[5]},{b[0],b[5]}}};std::set<std::array<V2,2>> lines;
 for(const auto&cut:cuts)for(size_t i=0;i<cut.polygon.size();++i){auto a=cut.polygon[i],b=cut.polygon[(i+1)%cut.polygon.size()];std::array<V2,2> line={key(a),key(b)};std::sort(line.begin(),line.end());if(!lines.insert(line).second)continue;Cells divided;
  for(const auto&cell:cells){bool touched=contains(cell,a)||contains(cell,b);for(size_t j=0;j<cell.size()&&!touched;++j)touched=intersects(a,b,cell[j],cell[(j+1)%cell.size()]);if(touched){auto pieces=split(cell,a,b);divided.insert(divided.end(),pieces.begin(),pieces.end());}else divided.push_back(cell);if(divided.size()>4096)fail("Cut arrangement exceeds cell budget");}cells=std::move(divided);
 }return cells;
}
std::vector<Face> triangles(const Polygon&p){
 Face remaining(p.size());std::iota(remaining.begin(),remaining.end(),size_t(0));std::vector<Face> result;
 while(remaining.size()>3){bool found=false;for(size_t k=0;k<remaining.size();++k){size_t current=remaining[k],previous=remaining[(k+remaining.size()-1)%remaining.size()],following=remaining[(k+1)%remaining.size()];auto a=p[previous],b=p[current],c=p[following];if(cross2(a,b,c)<=1e-10)continue;bool occupied=false;for(size_t j:remaining)if(j!=previous&&j!=current&&j!=following&&cross2(a,b,p[j])>=-area_eps&&cross2(b,c,p[j])>=-area_eps&&cross2(c,a,p[j])>=-area_eps){occupied=true;break;}if(occupied)continue;result.push_back({previous,current,following});remaining.erase(remaining.begin()+k);found=true;break;}if(!found)fail("Unsupported tiny or ill-conditioned surface triangulation");}
 if(cross2(p[remaining[0]],p[remaining[1]],p[remaining[2]])<=1e-10)fail("Unsupported tiny surface triangle");
 result.push_back(remaining);return result;
}
std::pair<Polygon,std::vector<Face>> cap_mesh(const Polygon&cell){try{return {cell,triangles(cell)};}catch(const std::invalid_argument&){auto midpoint=center(cell);for(size_t i=0;i<cell.size();++i)if(cross2(cell[i],cell[(i+1)%cell.size()],midpoint)<=1e-10)fail("Cell needs a larger compatible triangulation domain");Polygon points=cell;points.push_back(midpoint);std::vector<Face> faces;for(size_t i=0;i<cell.size();++i)faces.push_back({i,(i+1)%cell.size(),cell.size()});return {points,faces};}}
std::optional<Polygon> union_convex(const Polygon&first,const Polygon&second){
 std::map<V2,V2> positions;OrderedMap<std::pair<V2,V2>,bool> edges;
 for(const auto&cell:{first,second}){std::vector<V2> keys;for(auto point:cell){auto k=key(point);positions.emplace(k,point);keys.push_back(k);}for(size_t i=0;i<keys.size();++i){auto a=keys[i],b=keys[(i+1)%keys.size()];if(edges.contains({b,a}))edges.erase({b,a});else edges.set({a,b},true);}}
 if(edges.size()==first.size()+second.size())return std::nullopt;
 std::map<V2,V2> outgoing;auto entries=edges.items();if(entries.empty())return std::nullopt;V2 start=entries.front().first.first;
 for(const auto&entry:entries){const auto&a=entry.first.first;const auto&b=entry.first.second;if(outgoing.count(a))return std::nullopt;outgoing.emplace(a,b);}
 V2 current=start;std::vector<V2> outline;
 for(size_t i=0;i<edges.size();++i){if(!outgoing.count(current)||std::find(outline.begin(),outline.end(),current)!=outline.end())return std::nullopt;outline.push_back(current);current=outgoing.at(current);}if(current!=start)return std::nullopt;
 Polygon merged;for(auto k:outline)merged.push_back(positions.at(k));for(size_t i=0;i<merged.size();++i)if(cross2(merged[(i+merged.size()-1)%merged.size()],merged[i],merged[(i+1)%merged.size()])<-area_eps)return std::nullopt;
 if(!close(area(merged),area(first)+area(second),1e-9,1e-14))return std::nullopt;
 return merged;
}
Cells coalesce_slivers(Cells cells,const std::vector<Cut>&cuts){
 using Signature=std::vector<std::pair<bool,bool>>;std::vector<Signature> signatures;std::set<double> unique;for(const auto&cut:cuts){unique.insert(cut.low);unique.insert(cut.high);}std::vector<double> levels(unique.begin(),unique.end()),middles;for(size_t i=0;i+1<levels.size();++i)middles.push_back((levels[i]+levels[i+1])/2);
 for(const auto&cell:cells){auto midpoint=center(cell);std::vector<bool> inside;for(const auto&cut:cuts)inside.push_back(contains(cut.polygon,midpoint));Signature sig;for(double y:middles){bool before=false,after=false;for(size_t i=0;i<cuts.size();++i)if(inside[i]&&cuts[i].low<y&&y<cuts[i].high){after=true;if(i+1<cuts.size())before=true;}sig.push_back({before,after});}signatures.push_back(sig);}
 size_t index=0;while(index<cells.size()){
  try{cap_mesh(cells[index]);}
  catch(const std::invalid_argument&){bool found=false,best_usable=false;double best_area=0;size_t best_other=0;Polygon best_merged;
   for(size_t other=0;other<cells.size();++other){if(other==index||signatures[other]!=signatures[index])continue;auto merged=union_convex(cells[index],cells[other]);if(!merged)continue;bool usable=true;try{cap_mesh(*merged);}catch(const std::invalid_argument&){usable=false;}double candidate_area=area(*merged);if(!found||usable>best_usable||(usable==best_usable&&candidate_area>best_area)){found=true;best_usable=usable;best_area=candidate_area;best_other=other;best_merged=*merged;}}
   if(!found)fail("Cut creates an unsupported genuinely tiny material region");
   cells[index]=std::move(best_merged);cells.erase(cells.begin()+best_other);signatures.erase(signatures.begin()+best_other);if(best_other<index)--index;continue;
  }++index;
 }return cells;
}
Cells conform(const Cells&cells){
 OrderedMap<V2,V2> positions;for(const auto&cell:cells)for(auto point:cell)positions.setdefault(key(point),point);Polygon points;for(const auto&entry:positions.items())points.push_back(entry.second);Cells result;
 for(const auto&cell:cells){Polygon boundary;for(size_t i=0;i<cell.size();++i){auto a=cell[i],b=cell[(i+1)%cell.size()];Polygon segment;for(auto p:points)if(on_segment(p,a,b))segment.push_back(p);double dx=b[0]-a[0],dz=b[1]-a[1];std::stable_sort(segment.begin(),segment.end(),[&](V2 p,V2 q){return (p[0]-a[0])*dx+(p[1]-a[1])*dz<(q[0]-a[0])*dx+(q[1]-a[1])*dz;});if(!segment.empty())boundary.insert(boundary.end(),segment.begin(),segment.end()-1);}result.push_back(clean(boundary));}return result;
}
} // namespace
Json irregular_polygon(V2 center_xz,V2 radii_xz,int64_t seed,int vertices,double jaggedness,uint64_t positive_offset){
 auto c=finite_vector<2>(center_xz,"Center"),r=finite_vector<2>(radii_xz,"Radii");if(std::min(r[0],r[1])<=eps)fail("Radii must be positive");if(vertices<3||vertices>64)fail("vertices outside supported 3..64 range");if(!std::isfinite(jaggedness)||jaggedness<0||jaggedness>.85)fail("jaggedness must be finite in 0..0.85");PythonRandom rng(seed,positive_offset);Polygon result;
 for(int i=0;i<vertices;++i){double angle=(i+rng.uniform(-.15,.15)*jaggedness)*2*pi/vertices;double radius=1-rng.uniform(0,jaggedness);result.push_back({c[0]+r[0]*radius*std::cos(angle),c[1]+r[1]*radius*std::sin(angle)});}return profile(result);
}
bool point_in_panel(const Json&part,V3 point){auto p=finite_vector<3>(point,"Point");auto b=bounds(part,true);for(size_t k=0;k<3;++k)if(p[k]<b[k*2]-eps||p[k]>b[k*2+1]+eps)return false;for(const auto&cut:read_cuts(part,b))if(cut.low-eps<=p[1]&&p[1]<=cut.high+eps&&contains(cut.polygon,{p[0],p[2]}))return false;return true;}
Json carve_panel(const Json&part,const Json&polygon_xz,std::string side,Json depth){
 auto b=bounds(part,true);auto cuts=read_cuts(part,b);if(cuts.size()>=12)fail("Panel exceeds 12 cut budget");cuts.push_back(cut_record(polygon_xz,side,depth,b));auto cells=coalesce_slivers(conform(partition(b,cuts)),cuts);
 std::set<double> unique={b[2],b[3]};for(const auto&cut:cuts){unique.insert(cut.low);unique.insert(cut.high);}std::vector<double> ys(unique.begin(),unique.end());
 Json output=part;auto&vertices=output["vertices"];std::map<V3,size_t> vertex_map;for(size_t i=vertices.size();i-->0;)vertex_map[key(vertices[i].get<V3>())]=i;
 auto vertex=[&](V3 point){auto k=key(point);auto found=vertex_map.find(k);if(found!=vertex_map.end())return found->second;if(vertices.size()>=60000)fail("Generated mesh exceeds vertex budget");size_t index=vertices.size();vertex_map[k]=index;vertices.push_back(point);return index;};
 OrderedMap<Face,Face> surfaces;auto surface=[&](Face face){auto k=face;std::sort(k.begin(),k.end());if(surfaces.contains(k))surfaces.erase(k);else{surfaces.set(k,face);if(surfaces.size()>100000)fail("Generated mesh exceeds face budget");}};
 double removed_volume=0,remaining_volume=0;
 for(const auto&cell:cells){auto midpoint=center(cell);std::vector<size_t> in_cuts;for(size_t i=0;i<cuts.size();++i)if(contains(cuts[i].polygon,midpoint))in_cuts.push_back(i);double cell_area=area(cell);auto cap=cap_mesh(cell);const auto&cap_points=cap.first;const auto&tris=cap.second;
  for(size_t interval=0;interval+1<ys.size();++interval){double low=ys[interval],high=ys[interval+1],mid=(low+high)/2;std::vector<size_t> removed;for(size_t i:in_cuts)if(cuts[i].low<mid&&mid<cuts[i].high)removed.push_back(i);
   if(!removed.empty()){if(removed.size()==1&&removed[0]+1==cuts.size())removed_volume+=cell_area*(high-low);continue;}remaining_volume+=cell_area*(high-low);
   Face front,back;for(auto p:cap_points)front.push_back(vertex({p[0],low,p[1]}));for(auto p:cap_points)back.push_back(vertex({p[0],high,p[1]}));
   for(const auto&triangle:tris){Face f,r;for(size_t i:triangle)f.push_back(front[i]);for(auto i=triangle.rbegin();i!=triangle.rend();++i)r.push_back(back[*i]);surface(f);surface(r);}
   for(size_t i=0;i<cell.size();++i){size_t j=(i+1)%cell.size();surface({front[i],back[i],back[j],front[j]});}
  }
 }
 if(removed_volume<=1e-12)fail("Cutter does not remove new panel material");
 if(remaining_volume<=1e-12||surfaces.empty())fail("Cutter would erase the entire panel");
 Json faces=Json::array(),materials=Json::array(),provenance=Json::array();const Json raw_material=part.value("material",Json(""));std::string material=raw_material.is_string()?raw_material.get<std::string>():raw_material.dump();std::transform(material.begin(),material.end(),material.begin(),[](unsigned char c){return std::tolower(c);});std::string cut_material=material.find("glass")!=std::string::npos?"glass_edge":"cut_metal";
 for(const auto&entry:surfaces.items()){const auto&face=entry.second;faces.push_back(face);std::vector<V3> points;for(size_t i:face)points.push_back(vertices[i].get<V3>());bool exterior=false;for(size_t axis=0;axis<3;++axis)for(size_t end=0;end<2;++end)if(std::all_of(points.begin(),points.end(),[&](V3 p){return std::abs(p[axis]-b[axis*2+end])<=eps;}))exterior=true;
  materials.push_back(exterior?part.value("material",Json("metal")):Json(cut_material));
  if(exterior)provenance.push_back({{"kind","original_surface"},{"source_part_id",part.value("id",Json(nullptr))}});
  else{V3 midpoint{0,0,0};for(size_t k=0;k<3;++k){for(auto p:points)midpoint[k]+=p[k];midpoint[k]/=points.size();}Json owners=Json::array();for(size_t i=0;i<cuts.size();++i)if(cuts[i].low-eps<=midpoint[1]&&midpoint[1]<=cuts[i].high+eps&&contains(cuts[i].polygon,{midpoint[0],midpoint[2]}))owners.push_back(i);provenance.push_back({{"kind","cut_surface"},{"source_part_id",part.value("id",Json(nullptr))},{"cut_indices",owners}});}
 }
 output["faces"]=faces;output["uvs"]=metric_uvs(vertices,faces);output["face_materials"]=materials;output["face_provenance"]=provenance;output["panel_bounds"]=b;output["cuts"]=Json::array();for(const auto&cut:cuts)output["cuts"].push_back(cut.json());output["cut_math"]={{"method","original_convex_arrangement_extrusion"},{"units","m"},{"cells",cells.size()},{"remaining_volume",remaining_volume},{"last_removed_volume",removed_volume}};bounds(output,true);return output;
}

namespace {
constexpr double min_width=.001;
bool crack_on_segment(V2 p,V2 a,V2 b){return std::abs(cross2(a,b,p))<=1e-11*dist(a,b)&&std::min(a[0],b[0])-1e-11<=p[0]&&p[0]<=std::max(a[0],b[0])+1e-11&&std::min(a[1],b[1])-1e-11<=p[1]&&p[1]<=std::max(a[1],b[1])+1e-11;}
Polygon crack_outline(const Polygon&left,const Polygon&right){Polygon points=left;points.insert(points.end(),right.rbegin(),right.rend());if(area(points)<0)std::reverse(points.begin(),points.end());if(points.size()>64||area(points)<=1e-12)fail("Crack outline exceeds the geometric budget");for(size_t i=0;i<points.size();++i){auto a=points[i],b=points[(i+1)%points.size()];for(size_t j=i+1;j<points.size();++j)if(dist(a,points[j])<=1e-10)fail("Crack outline contains coincident points");for(size_t j=i+1;j<points.size();++j){if(j==i+1||(i==0&&j==points.size()-1))continue;auto c=points[j],d=points[(j+1)%points.size()];if((cross2(a,b,c)*cross2(a,b,d)<0&&cross2(c,d,a)*cross2(c,d,b)<0)||crack_on_segment(c,a,b)||crack_on_segment(d,a,b)||crack_on_segment(a,c,d)||crack_on_segment(b,c,d))fail("Crack outline would self-intersect");}}return points;}
std::pair<Polygon,Polygon> sweep(const Polygon&points,const std::vector<double>&widths){Polygon normals;for(size_t i=0;i+1<points.size();++i){auto a=points[i],b=points[i+1];double distance=dist(a,b);normals.push_back({-(b[1]-a[1])/distance,(b[0]-a[0])/distance});}Polygon left,right;for(size_t i=0;i<points.size();++i){auto before=normals[i?i-1:0],after=normals[std::min(i,normals.size()-1)];V2 miter={before[0]+after[0],before[1]+after[1]};double norm=std::hypot(miter[0],miter[1]);for(double&v:miter)v/=norm;double cosine=miter[0]*after[0]+miter[1]*after[1],distance=std::min(widths[i]*.5/std::max(cosine,1e-12),1.5*widths[i]*.5);left.push_back({points[i][0]+miter[0]*distance,points[i][1]+miter[1]*distance});right.push_back({points[i][0]-miter[0]*distance,points[i][1]-miter[1]*distance});}return {left,right};}
std::pair<Polygon,std::vector<double>> path(V2 origin,V2 direction,double length,double width,int segments,PythonRandom&rng,bool centered){double quotient=length/(2*width);int count=quotient>=segments?segments:std::min(segments,std::max(2,static_cast<int>(quotient)));double step=length/count;V2 normal={-direction[1],direction[0]};double amplitude=std::min(.055*length,.30*step);Polygon points;std::vector<double> widths;for(int i=0;i<=count;++i){double t=double(i)/count,offset=(i==0||i==count)?0:rng.uniform(-amplitude,amplitude),along=length*(centered?t-.5:t);points.push_back({origin[0]+direction[0]*along+normal[0]*offset,origin[1]+direction[1]*along+normal[1]*offset});double taper=.26+.74*std::sin(pi*t),variation=i==count/2?1:rng.uniform(.84,1);widths.push_back(std::max(min_width,width*taper*variation));}return {points,widths};}
Json record(const Polygon&left,const Polygon&right,const Polygon&points,double depth,const std::string&role){return {{"polygon_xz",crack_outline(left,right)},{"depth",depth},{"role",role},{"centerline_xz",points}};}
}
Json crack_profiles(V2 center_xz,double length,double width,double depth,int64_t seed,V2 direction_xz,int branches,int segments,uint64_t positive_offset){
 auto center=center_xz,direction=direction_xz;for(double value:center)if(!std::isfinite(value))fail("center_xz must be finite and numeric");for(double value:direction)if(!std::isfinite(value))fail("direction_xz must be finite and numeric");for(const auto&v:std::vector<std::pair<double,std::string>>{{length,"length"},{width,"width"},{depth,"depth"}})if(!std::isfinite(v.first))fail(v.second+" must be finite and numeric");if(width<min_width)fail("width must be at least 0.001 m");if(length<4*width)fail("length must be at least four times width");if(depth<=0||depth*.45==0)fail("depth must be positive and representable for every section");if(branches<0||branches>2)fail("branches must be an integer in 0..2");if(segments<2||segments>12)fail("segments must be an integer in 2..12");double norm=std::hypot(direction[0],direction[1]);if(norm==0||!std::isfinite(norm))fail("direction_xz must have finite nonzero length");for(double&v:direction)v/=norm;if(std::max(std::abs(center[0]),std::abs(center[1]))+2*length>1e5)fail("Crack exceeds supported local metric coordinate range");PythonRandom rng(seed,positive_offset);auto main=path(center,direction,length,width,segments,rng,true);auto edges=sweep(main.first,main.second);Json result=Json::array({record(edges.first,edges.second,main.first,depth*.65,"main")});int count=static_cast<int>(main.first.size())-1,start=std::max(0,count/2-1),end=std::min(count,count/2+1);if(end-start==count)--end;result.push_back(record(Polygon(edges.first.begin()+start,edges.first.begin()+end+1),Polygon(edges.second.begin()+start,edges.second.begin()+end+1),Polygon(main.first.begin()+start,main.first.begin()+end+1),depth,"deep_section"));V2 normal={-direction[1],direction[0]};
 for(int branch=0;branch<branches;++branch){int root_index=std::max(1,std::min(count-1,static_cast<int>(std::nearbyint(count*(branch==0?.34:.68)))));auto root=main.first[root_index];int sign=branch==0?1:-1;double angle=rng.uniform(.65,.95);V2 branch_direction={direction[0]*std::cos(angle)+sign*normal[0]*std::sin(angle),direction[1]*std::cos(angle)+sign*normal[1]*std::sin(angle)};double branch_width=std::max(min_width,width*.48),branch_length=std::max(4*branch_width,length*rng.uniform(.24,.34));auto branch_path=path(root,branch_direction,branch_length,branch_width,2,rng,false);auto branch_edges=sweep(branch_path.first,branch_path.second);result.push_back(record(branch_edges.first,branch_edges.second,branch_path.first,depth*.45,"branch"));}return result;
}
} // namespace jerico
