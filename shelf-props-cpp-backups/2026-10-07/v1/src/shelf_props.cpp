#include "jerico/shelf_props.hpp"
#include "jerico/evaluate.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
namespace jerico {
namespace {
constexpr double pi=3.14159265358979323846;
void need(bool b,const std::string&m){if(!b)throw std::invalid_argument("Shelf props: "+m);}
struct Maker {
 Builder b; V3 origin{}; double scale=1,wear=.8; int sides=64; int64_t seed=417; std::string item;
 Maker():b(Json{{"seed",417}}){b.node("frame",{0,0,0},"ShelfProps",nullptr);b.scene["metadata"]["items"]=Json::array();}
 V3 point(V3 p)const{return add(origin,mul(p,scale));}
 Json& mesh(const std::string&name,const Json&vs,const Json&fs,const Json&uv,const std::string&mat,const std::string&role){auto&p=b.mesh(item+"."+name,"frame",vs,fs,mat,0,role);p["item_id"]=item;p["prop_uvs"]=uv;p["original_surface"]=true;return p;}
 Json& lathe(const std::string&name,const std::vector<V2>&profile,const std::string&mat,const std::string&role,bool closed=false,bool dent=false){
  Json vs=Json::array(),fs=Json::array(),uvs=Json::array();std::vector<std::vector<size_t>>rings;double zlo=1e9,zhi=-1e9,rmax=0;for(auto q:profile){zlo=std::min(zlo,q[1]);zhi=std::max(zhi,q[1]);rmax=std::max(rmax,q[0]);}
  for(auto q:profile){std::vector<size_t> ring;int count=q[0]<1e-12?1:sides;for(int k=0;k<count;k++){double a=2*pi*k/sides;double zz=(q[1]-zlo)/std::max(1e-6,zhi-zlo);double d=dent?.00028*wear*std::sin(a*3+double(seed%31))*(.4+.6*std::sin(a*7+zz*9))*std::pow(std::sin(pi*zz),2):0;ring.push_back(vs.size());vs.push_back(point({(q[0]+d)*std::cos(a),(q[0]+d)*std::sin(a),q[1]}));}rings.push_back(ring);}
  auto emit=[&](std::vector<size_t>f,int k){Json face=f,uv=Json::array();auto a=vs[f[0]].get<V3>(),bb=vs[f[1]].get<V3>(),c=vs[f[2]].get<V3>();auto n=norm(cross(sub(bb,a),sub(c,a)));bool planar=std::abs(n[2])>.93;
   for(size_t index:f){auto q=mul(sub(vs[index].get<V3>(),origin),1/scale);if(planar)uv.push_back({.5+q[0]/(rmax*2.05),.5+q[1]/(rmax*2.05)});else{double u=std::atan2(q[1],q[0])/(2*pi);if(u<-.000001)u+=1;if(k==sides-1&&u<.2)u+=1;uv.push_back({u,.035+.93*(q[2]-zlo)/std::max(1e-6,zhi-zlo)});}}
   fs.push_back(face);uvs.push_back(uv);
  };
  size_t pairs=closed?profile.size():profile.size()-1;for(size_t j=0;j<pairs;j++){auto&A=rings[j];auto&B=rings[(j+1)%rings.size()];if(A.size()==1&&B.size()==1)continue;for(int k=0;k<sides;k++){size_t l=(k+1)%sides;if(A.size()==1)emit({A[0],B[l],B[k]},k);else if(B.size()==1)emit({A[k],A[l],B[0]},k);else emit({A[k],A[l],B[l],B[k]},k);}}
  auto&p=mesh(name,vs,fs,uvs,mat,role);p["surface_type"]="lathe";p["wall_surface_closed"]=true;return p;
 }
 Json& tube(const std::string&name,const std::vector<V3>&path,double radius,const std::string&mat,const std::string&role,int n=12){
  Json vs=Json::array(),fs=Json::array(),uvs=Json::array();std::vector<double>dist{0};for(size_t j=1;j<path.size();j++)dist.push_back(dist.back()+std::sqrt(dot(sub(path[j],path[j-1]),sub(path[j],path[j-1]))));
  for(size_t j=0;j<path.size();j++){auto tangent=norm(sub(path[j==path.size()-1?j:j+1],path[j?j-1:j]));V3 ref=std::abs(tangent[1])<.90?V3{0,1,0}:V3{1,0,0};auto u=norm(cross(ref,tangent)),v=cross(tangent,u);for(int k=0;k<n;k++){double a=k*2*pi/n;vs.push_back(point(add(path[j],add(mul(u,radius*std::cos(a)),mul(v,radius*std::sin(a))))));}}
  for(size_t j=0;j+1<path.size();j++)for(int k=0;k<n;k++){int l=(k+1)%n;fs.push_back({j*n+k,j*n+l,(j+1)*n+l,(j+1)*n+k});uvs.push_back({{dist[j]/dist.back(),double(k)/n},{dist[j]/dist.back(),double(k+1)/n},{dist[j+1]/dist.back(),double(k+1)/n},{dist[j+1]/dist.back(),double(k)/n}});}
  for(int end:{0,1}){size_t j=end?path.size()-1:0,ci=vs.size();vs.push_back(point(path[j]));for(int k=0;k<n;k++){int l=(k+1)%n;if(end)fs.push_back({ci,j*n+k,j*n+l});else fs.push_back({ci,j*n+l,j*n+k});double a=k*2*pi/n,bb=(k+1)*2*pi/n;Json ua={.5+.45*std::cos(a),.5+.45*std::sin(a)},ub={.5+.45*std::cos(bb),.5+.45*std::sin(bb)};uvs.push_back(end?Json::array({{.5,.5},ua,ub}):Json::array({{.5,.5},ub,ua}));}}
  auto&p=mesh(name,vs,fs,uvs,mat,role);p["surface_type"]="continuous_capped_tube";return p;
 }
 void torus(const std::string&name,double r,double z,double minor,const std::string&mat){std::vector<V2>pr;for(int k=0;k<12;k++){double a=-pi*.5+k*2*pi/12;pr.push_back({r+minor*std::cos(a),z+minor*std::sin(a)});}lathe(name,pr,mat,"RolledRim",true);}
 void finish(const std::string&type,V3 grab){V3 lo{1e9,1e9,1e9},hi{-1e9,-1e9,-1e9};Json ids=Json::array();for(auto&p:b.scene["parts"])if(p["item_id"]==item){ids.push_back(p["id"]);for(auto&v:p["vertices"]){auto q=v.get<V3>();for(int k=0;k<3;k++){lo[k]=std::min(lo[k],q[k]);hi[k]=std::max(hi[k],q[k]);}}}b.scene["metadata"]["items"].push_back({{"id",item},{"type",type},{"pivot",origin},{"bottom_plane_z",origin[2]},{"obb",{{"center",mul(add(lo,hi),.5)},{"axes",{V3{1,0,0},V3{0,1,0},V3{0,0,1}}},{"half",mul(sub(hi,lo),.5)}}},{"grab",{{"id",item+".grab"},{"position",point(grab)},{"group_part_ids",ids},{"physics_implemented",false}}}});}
};
std::vector<V3> bezier(V3 a,V3 b,V3 c,V3 d,int n){std::vector<V3>p;for(int k=0;k<=n;k++){double t=double(k)/n,s=1-t;p.push_back(add(add(mul(a,s*s*s),mul(b,3*s*s*t)),add(mul(c,3*s*t*t),mul(d,t*t*t))));}return p;}
void mug(Maker&m){
 m.lathe("shell",{{0,0},{.037,0},{.042,.0015},{.044,.006},{.0445,.017},{.046,.080},{.047,.091},{.047,.095},{.0446,.096},{.0436,.093},{.043,.084},{.041,.020},{.040,.006},{0,.005}},"tin","OpenMugShell",false,true);
 m.torus("rolled_lip",.0456,.0945,.0018,"tin");m.torus("foot_seam",.0426,.0045,.0012,"tin");
 auto path=bezier({.043,0,.079},{.080,0,.088},{.082,0,.015},{.042,0,.022},32);m.tube("handle",path,.0040,"iron","MugHandle",12);
 for(double z:{.022,.079}){m.tube("weld_collar."+std::to_string(int(z*1000)),{{.042,0,z},{.049,0,z}},.0052,"tin","WeldedHandleCollar",12);}
 m.b.scene["metadata"]["mug"]={{"open_mouth",true},{"wall_thickness_m",.0025*m.scale},{"interior_floor_m",.005*m.scale},{"liquid",false},{"handle_attachment","collars overlap shell by 1 mm nominal"}};m.finish("mug",{.067,0,.052});
}
void bottle(Maker&m){
 m.lathe("shell",{{0,.004},{.030,.004},{.035,0},{.041,0},{.044,.004},{.045,.012},{.045,.096},{.044,.112},{.041,.124},{.034,.136},{.021,.150},{.016,.160},{.016,.181},{.019,.183},{.019,.189},{.015,.191},{.0122,.189},{.0122,.160},{.017,.149},{.031,.134},{.038,.120},{.040,.108},{.040,.014},{.034,.008},{0,.010}},"glass_amber","HollowBottleShell",false,true);
 m.torus("neck_bead",.016,.177,.0012,"glass_amber");
 m.b.scene["metadata"]["bottle"]={{"open_mouth",true},{"wall_thickness_min_m",.0038*m.scale},{"empty",true},{"glass_ior",1.51}};m.finish("bottle",{0,0,.166});
}
void lantern(Maker&m){
 m.lathe("reservoir",{{0,0},{.054,0},{.057,.003},{.057,.010},{.052,.015},{.050,.027},{.046,.035},{.031,.042},{.025,.043},{0,.043}},"iron","LanternReservoir",false,true);
 m.torus("base_seam",.054,.011,.0017,"iron");
 m.lathe("burner_cup",{{0,.040},{.027,.040},{.030,.044},{.031,.048},{.028,.052},{.024,.054},{.023,.059},{0,.059}},"iron","UnlitBurnerCup");
 m.lathe("wick_holder",{{0,.055},{.009,.055},{.009,.066},{.008,.068},{0,.068}},"iron","UnlitWickHolder");
 m.lathe("globe",{{.024,.053},{.030,.057},{.037,.071},{.040,.095},{.039,.125},{.034,.150},{.026,.171},{.025,.180},{.023,.180},{.024,.171},{.032,.149},{.037,.125},{.038,.095},{.035,.072},{.028,.058},{.023,.056}},"glass_clear","HollowLanternGlobe",true);
 m.lathe("hood",{{0,.178},{.025,.178},{.031,.181},{.038,.183},{.039,.187},{.036,.191},{.027,.194},{.022,.201},{.020,.207},{.020,.217},{.024,.219},{.025,.223},{.023,.227},{0,.227}},"iron","VentilatedHood",false,true);
 m.torus("hood_roll",.037,.186,.0017,"iron");
 // Large continuous side rails are embedded in reservoir and hood supports.
 for(int side:{-1,1}){std::vector<V3>path;auto a=bezier({side*.034,0,.026},{side*.060,0,.024},{side*.060,0,.035},{side*.059,0,.062},10);auto b=bezier({side*.059,0,.062},{side*.056,0,.110},{side*.051,0,.180},{side*.048,0,.187},16);auto c=bezier({side*.048,0,.187},{side*.046,0,.198},{side*.031,0,.197},{side*.023,0,.196},10);path=a;path.insert(path.end(),b.begin()+1,b.end());path.insert(path.end(),c.begin()+1,c.end());m.tube("side_rail."+std::to_string(side),path,.0047,"iron","LanternSideRail");}
 // Front and rear protective diagonals wrap around rather than cut through the glass.
 for(int face:{-1,1})for(int slope:{-1,1}){std::vector<V3>path;for(int k=0;k<=32;k++){double t=double(k)/32,a=(-1.18+2.36*t)*slope;double z=.050+.134*t;const std::vector<V2> envelope{{.029,.050},{.035,.060},{.043,.080},{.045,.100},{.043,.130},{.039,.150},{.032,.170},{.030,.178},{.035,.184}};double r=envelope.back()[0];for(size_t j=1;j<envelope.size();j++)if(z<=envelope[j][1]){double f=(z-envelope[j-1][1])/(envelope[j][1]-envelope[j-1][1]);r=envelope[j-1][0]+f*(envelope[j][0]-envelope[j-1][0]);break;}if(slope==1)r+=.0026*std::pow(std::sin(pi*t),12);path.push_back({r*std::sin(a),face*r*std::cos(a),z});}m.tube("guard."+std::to_string(face)+"."+std::to_string(slope),path,.00125,"iron","GlobeGuard",8);}
 // A foldable carrying loop, represented in the upright rest pose only.
 std::vector<V3> handle;for(int k=0;k<=36;k++){double a=pi*k/36;handle.push_back({.048*std::cos(a),0,.191+.089*std::sin(a)});}m.tube("carry_handle",handle,.00215,"iron","CarryHandle",10);
 for(int side:{-1,1})m.tube("handle_pivot."+std::to_string(side),{{side*.048,-.006,.191},{side*.048,.006,.191}},.0056,"iron","HandlePivot",12);
 // Six short hood ribs supply stepped mechanical detail without text or branding.
 for(int k=0;k<6;k++){double a=2*pi*k/6;std::vector<V3>path;for(int j=0;j<=5;j++){double b=a+.08+j*.07;path.push_back({.0207*std::cos(b),.0207*std::sin(b),.211});}m.tube("hood_band_rib."+std::to_string(k),path,.0011,"iron","HoodRib",8);}
 m.b.scene["metadata"]["lantern"]={{"unlit",true},{"emissive",false},{"glass_rail_clearance_m",.006*m.scale},{"glass_guard_clearance_design_target_m",.001*m.scale},{"carry_loop_apex_m",.28215*m.scale},{"combustion",false}};m.finish("lantern",{0,0,.278});
}
}
Json default_shelf_props_recipe(){return {{"version",1},{"seed",417},{"wear",.8},{"segments",64},{"mug_scale",1.0},{"lantern_scale",1.0},{"bottle_scale",1.0}};}
Json build_shelf_props(const Json&input){need(input.is_object(),"recipe must be an object");auto r=default_shelf_props_recipe();for(auto it=input.begin();it!=input.end();++it){need(r.contains(it.key()),"unknown recipe field "+it.key());r[it.key()]=it.value();}Maker m;m.seed=recipe_seed(r["seed"]);auto scalar=[&](const char*k,double lo,double hi){need(r[k].is_number(),std::string(k)+" must be numeric");double v=r[k];need(std::isfinite(v)&&v>=lo&&v<=hi,std::string(k)+" outside range");return v;};m.wear=scalar("wear",0,1);double sides=scalar("segments",32,128);need(sides==std::floor(sides),"segments must be integral");m.sides=int(sides);need(r["version"]==1,"unsupported version");
 m.item="prop.mug";m.origin={-.20,0,0};m.scale=scalar("mug_scale",.70,1.30);mug(m);
 m.item="prop.lantern";m.origin={0,0,0};m.scale=scalar("lantern_scale",.70,1.30);lantern(m);
 m.item="prop.bottle";m.origin={.20,0,0};m.scale=scalar("bottle_scale",.70,1.30);bottle(m);
 m.b.scene["recipe"]=r;m.b.scene["metadata"]["family"]="shelf_props";m.b.scene["metadata"]["generator"]="JericoShelfPropsCppLab C++17";m.b.scene["metadata"]["engine_physics"]=false;m.b.scene["metadata"]["contact_policy"]="Individually closed components, intentional embedded assembly joints, exactly planar object support bases";return m.b.scene;
}
Json evaluate_shelf_props(const Json&s){Json out=s;for(auto&p:out["parts"]){auto original=p["material"];p["material"]="iron";p=evaluate_part(p);p["material"]=original;p["texture_uvs"]=p["prop_uvs"];p.erase("prop_uvs");p["native_normals"]["source"]="core03 angle-weighted smooth surface evaluation";}out["metadata"]["native_evaluation"]={{"generator","C++17"},{"corner_normals",true},{"blender_generation",false}};return out;}

}
