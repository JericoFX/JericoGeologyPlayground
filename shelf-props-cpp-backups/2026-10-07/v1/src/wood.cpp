#include "jerico/scene.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>

namespace jerico {
namespace {
constexpr double pi=3.141592653589793238462643383279502884;
constexpr double tau=2*pi;
constexpr double rebate=.012,edge_clearance=.001,glass_thickness=.004,frame_member=.135;
std::string id(int n){return std::to_string(n);}
double radians(double degrees){return degrees*(pi/180.);}
double python_mod(double x,double divisor){double r=std::fmod(x,divisor);if(r<0)r+=divisor;return r;}
uint32_t randbelow(PythonRandom&rng,uint32_t n){unsigned k=0;for(uint32_t v=n;v;v>>=1)++k;uint32_t result;do{result=rng.next()>>(32-k);}while(result>=n);return result;}
V2 extrema(const Json&vertices,size_t axis){V2 b={INFINITY,-INFINITY};for(const auto&v:vertices){double x=v[axis];b[0]=std::min(b[0],x);b[1]=std::max(b[1],x);}return b;}

bool rectangles_overlap(const Json&a,const Json&b){
 for(const Json* polygon:{&a,&b})for(size_t i=0;i<polygon->size();++i){
  V2 p=(*polygon)[i].get<V2>(),q=(*polygon)[(i+1)%polygon->size()].get<V2>();
  double dx=q[0]-p[0],dy=q[1]-p[1],length=std::hypot(dx,dy);V2 axis={-dy/length,dx/length};
  auto projection=[&](const Json&poly){V2 bounds={INFINITY,-INFINITY};for(const auto&v:poly){double x=v[0].get<double>()*axis[0]+v[1].get<double>()*axis[1];bounds[0]=std::min(bounds[0],x);bounds[1]=std::max(bounds[1],x);}return bounds;};
  auto pa=projection(a),pb=projection(b);if(std::min(pa[1],pb[1])-std::max(pa[0],pb[0])<=1e-10)return false;
 }
 return true;
}
Json clearance_sweep_impl(const Json&scene,double step=.5){
 if(!std::isfinite(step)||step<=0)throw std::invalid_argument("Positive finite step required");
 const auto&leaves=scene["metadata"]["leaf_envelopes"];const auto&frames=scene["metadata"]["frame_envelopes"];
 if(leaves.empty())return {{"method","not_applicable_fixed_frame"},{"applicable",false},{"samples",0},{"leaf_pair_samples",0},{"collisions",Json::array()},{"scope","Fixed frame has no moving leaf; no motion sweep was evaluated."}};
 auto transformed=[&](const Json&leaf,double angle){
  Json node;for(const auto&n:scene["nodes"])if(n["id"]==leaf["node"])node=n;
  double rad=radians(angle)*leaf["angle_sign"].get<double>(),c=std::cos(rad),s=std::sin(rad);Json result=Json::array();
  for(const auto&p:leaf["rect"]){double x=p[0],y=p[1];result.push_back({x*c-y*s+node["translation"][0].get<double>(),x*s+y*c+node["translation"][1].get<double>()});}return result;
 };
 auto z_overlap=[](const Json&a,const Json&b){return std::min(a["z"][1].get<double>(),b["z"][1].get<double>())-std::max(a["z"][0].get<double>(),b["z"][0].get<double>())>1e-10;};
 Json collisions=Json::array();size_t samples=0,pairs=0;double range=INFINITY;
 for(const auto&leaf:leaves){double maximum=leaf["maximum_degrees"];range=std::min(range,maximum);int steps=static_cast<int>(std::ceil(maximum/step));for(int i=0;i<=steps;++i){double angle=std::min(i*step,maximum);auto rect=transformed(leaf,angle);++samples;for(const auto&frame:frames)if(z_overlap(leaf,frame)&&rectangles_overlap(rect,frame["rect"]))collisions.push_back({{"node",leaf["node"]},{"frame",frame["id"]},{"angle_degrees",angle}});}}
 for(size_t index=0;index<leaves.size();++index)for(size_t j=index+1;j<leaves.size();++j){const auto&leaf=leaves[index];const auto&other=leaves[j];double maximum=std::min(leaf["maximum_degrees"].get<double>(),other["maximum_degrees"].get<double>());int steps=static_cast<int>(std::ceil(maximum/step));for(int i=0;i<=steps;++i){double angle=std::min(i*step,maximum);++pairs;if(z_overlap(leaf,other)&&rectangles_overlap(transformed(leaf,angle),transformed(other,angle)))collisions.push_back({{"node",leaf["node"]},{"other_node",other["node"]},{"angle_degrees",angle}});}}
 return {{"method","2D rectangle SAT + Z-interval separation; frame and synchronized leaf pairs"},{"step_degrees",step},{"samples",samples},{"leaf_pair_samples",pairs},{"range_degrees",{0,range}},{"collisions",collisions},{"passed",collisions.empty()}};
}
void hinge(Builder&b,const std::string&node,const std::string&prefix,double x,double z,double front_y,int mirror=1,double strap_length=.34){
 std::vector<V2> profile={{x,z-.022},{x+mirror*(strap_length-.035),z-.022},{x+mirror*strap_length,z},{x+mirror*(strap_length-.035),z+.022},{x,z+.022}};if(mirror<0)std::reverse(profile.begin(),profile.end());
 b.prism(prefix+".strap",node,profile,front_y-.009,front_y-.003,"iron",.001,"HingeStrap");
 int i=0;for(double fraction:{.15,.55,.86})b.rivet(prefix+".rivet."+id(i++),node,x+mirror*strap_length*fraction,front_y-.009,z,true);
 auto pivot=world_point(b.scene,node,{0,0,z});i=0;
 for(V2 ends:std::array<V2,3>{{{-.033,-.013},{-.011,.011},{.013,.033}}}){b.cylinder(prefix+".barrel."+id(i),"frame",{pivot[0],pivot[1],z+ends[0]},{pivot[0],pivot[1],z+ends[1]},.014,i==1?"rust":"iron",12,.001,"HingeBarrel");++i;}
 double frame_x=pivot[0]-mirror*.068;b.box(prefix+".frame_plate","frame",std::min(frame_x,pivot[0]),std::max(frame_x,pivot[0]),pivot[1]+.004,pivot[1]+.011,z-.023,z+.023,"iron",.001,"HingePlate");
 b.socket(prefix+".socket",node,"Hinge","hinge.pin.v1",{0,0,z},{{"footprint",{.035,.07}},{"pin_diameter",.028},{"capabilities",{"rotate_z"}},{"range_degrees",{0,b.r["kind"]=="door"?105:85}}});
}
void wood_door(Builder&b){
 double w=b.r["width"],h=b.r["height"],t=b.r["thickness"],p=.024,swept_depth=p+t+.047;
 double g=std::max(.014,(2*w-std::sqrt(4*w*w-12*swept_depth*swept_depth))/6+.003),lw=w-2*g;
 b.node("door.leaf",{-w/2+g,-t/2-p,0},"Leaf","frame",-radians(b.r["angle"]));
 const std::string node="door.leaf",damage=b.r["damage"];double y0=p,y1=p+t,severity=damage=="aged"?1:damage=="deteriorated"?2.1:3;bool broken=damage=="broken";
 int n=6;double pitch=lw/n,gap=.0035;std::vector<double> weights;for(int i=0;i<n;++i)weights.push_back(b.rng.uniform(.93,1.07));double total=std::accumulate(weights.begin(),weights.end(),0.);std::vector<double> boundaries={0};for(double weight:weights)boundaries.push_back(boundaries.back()+lw*weight/total);
 for(int i=0;i<n;++i){std::string name="door.plank."+id(i);if(broken&&i==4){b.scene["metadata"]["missing_parts"].push_back(name);continue;}double bottom=g+(damage=="deteriorated"&&i==2?std::min(.09,h*.05):0);if(broken&&i==3)bottom=h*.39;
  b.chipped_board(name,node,boundaries[i]+gap/2,boundaries[i+1]-gap/2,y0,y1,bottom,h-g,severity);
  if(i%2==0){double crackx=boundaries[i]+.42*(boundaries[i+1]-boundaries[i]);b.box(name+".check",node,crackx,crackx+.0015,y0-.0004,y0+.0003,std::max(bottom+.07,h*.16),h*.72,"grime",.0001,"WoodCheck");}
 }
 int i=0;for(double z:{h*.18,h*.5,h*.82}){b.chipped_board("door.rail."+id(i),node,.018,lw-.018,y1-.001,y1+.026,z-.059,z+.059,severity,"CrossRail");for(int j=0;j<6;++j)b.rivet("door.rail."+id(i)+".bolt."+id(j),node,(j+.5)*pitch,y1+.026,z,false,.007);++i;}
 double xa=.055,xb=lw-.055,za=h*.22,zb=h*.79,dz=.037;b.prism("door.diagonal",node,{{xa,za-dz},{xb,zb-dz},{xb,zb+dz},{xa,za+dz}},y1+.026,y1+.047,"wood",.002,"DiagonalBrace");
 i=0;for(double z:{h*.13,h*.51,h*.88})hinge(b,node,"door.hinge."+id(i++),0,z,y0,1,std::min(.40,lw*.48));
 double hx=(boundaries[4]+boundaries[5])/2,hz=h*.48,support_width=boundaries[5]-boundaries[4]-gap-.010;
 b.handle("door.handle.front",node,hx,y0,hz,true,!broken,"","door.plank.4",support_width);
 b.handle("door.handle.back",node,hx,y1,hz,false,!broken,"","door.plank.4",support_width);
 b.socket("door.lock",node,"Lock","lock.mortise.v1",{hx,y0,hz-.16},{{"footprint",{.047,.079}},{"capabilities",{"keyed"}},{"support_thickness",t}},!broken);
 if(!broken){b.prism("door.lock.escutcheon",node,{{hx-.022,hz-.205},{hx+.022,hz-.205},{hx+.025,hz-.14},{hx,hz-.12},{hx-.025,hz-.14}},y0-.006,y0-.001,"iron",.001,"Escutcheon");b.cylinder("door.lock.keyhole.round",node,{hx,y0-.0065,hz-.155},{hx,y0-.007,hz-.155},.006,"grime",12,0,"Keyhole");b.prism("door.lock.keyhole.slot",node,{{hx-.0035,hz-.181},{hx+.0035,hz-.181},{hx+.002,hz-.156},{hx-.002,hz-.156}},y0-.0072,y0-.0068,"grime",0,"Keyhole");}
 b.socket("door.leaf.mount",node,"Leaf","leaf.hinged.v1",{lw/2,(y0+y1)/2,h/2},{{"footprint",{lw,h-2*g}},{"capabilities",{"hinged_leaf"}},{"damage",damage}});
 b.scene["metadata"]["leaf_envelopes"]=Json::array({{{"node",node},{"rect",{{0,y0},{lw,y0},{lw,y1+.047},{0,y1+.047}}},{"z",{g,h-g}},{"angle_sign",-1},{"maximum_degrees",105}}});
 b.scene["metadata"]["dimensions"]={{"clear_opening",{w,h,t}},{"leaf_width",lw},{"frame_member",.115},{"edge_gap",g},{"pivot_front_offset",p}};
}
void wood_window(Builder&b){
 double w=b.r["width"],h=b.r["height"],t=b.r["thickness"],g=.014,p=.024,pivot_span=w-2*g,depth=p+t;
 double center_gap=std::max(.022,pivot_span-std::sqrt(pivot_span*pivot_span-4*depth*depth)+.006),lw=w/2-g-center_gap/2,stile=std::min(.062,lw*.20);
 const std::string damage=b.r["damage"];bool broken=damage=="broken";double severity=damage=="aged"?1:damage=="deteriorated"?2:3;Json envelopes=Json::array();
 for(int mirror:{1,-1}){
  std::string side=mirror==1?"left":"right",node="window."+side;double px=mirror==1?-w/2+g:w/2-g;
  b.node(node,{px,-t/2-p,0},"Leaf","frame",-mirror*radians(b.r["angle"]));
  auto xx=[&](double a,double c){return V2{std::min(mirror*a,mirror*c),std::max(mirror*a,mirror*c)};};
  struct Stile {const char*tag;double xa,xb,za,zb;};
  for(const auto&s:std::array<Stile,4>{{{"outer",0,stile,g,h-g},{"inner",lw-stile,lw,g,h-g},{"top",stile,lw-stile,h-g-stile,h-g},{"bottom",stile,lw-stile,g,g+stile}}}){
   std::string name=node+".stile."+s.tag;if(broken&&side=="left"&&std::string(s.tag)=="inner"){b.scene["metadata"]["missing_parts"].push_back(name);continue;}auto x=xx(s.xa,s.xb);b.chipped_board(name,node,x[0],x[1],p,p+t,s.za,s.zb,severity,"CasementFrame");
  }
  double mx=lw/2,mw=.018;int rows=3;auto x=xx(mx-mw/2,mx+mw/2);b.box(node+".mullion.vertical",node,x[0],x[1],p+.009,p+t-.008,g+stile,h-g-stile,"wood",.001,"Mullion");
  double iz0=g+stile,iz1=h-g-stile,row=(iz1-iz0)/rows;
  for(int j:{1,2}){double z=iz0+row*j;x=xx(stile,lw-stile);b.box(node+".mullion.horizontal."+id(j),node,x[0],x[1],p+.009,p+t-.008,z-.009,z+.009,"wood",.001,"Mullion");}
  int col=0;for(V2 span:std::array<V2,2>{{{stile,mx-mw/2},{mx+mw/2,lw-stile}}}){for(int j=0;j<rows;++j){std::string pane=node+".glass."+id(col)+"."+id(j);bool missing=broken&&side=="left"&&(j==1||(col==1&&j==0));x=xx(span[0]+.002,span[1]-.002);double za=iz0+row*j+.012,zb=iz0+row*(j+1)-.012;
   b.socket(pane+".socket",node,"Glass","pane.rect.v1",{(x[0]+x[1])/2,p+t*.5,(za+zb)/2},{{"footprint",{x[1]-x[0],zb-za}},{"thickness",.004},{"capabilities",{"glazing"}},{"max_thickness",.006}},!missing);
   if(missing){b.scene["metadata"]["missing_parts"].push_back(pane);continue;}b.box(pane,node,x[0],x[1],p+t*.5-.002,p+t*.5+.002,za,zb,"glass",.0002,"GlassPane");b.box(pane+".grime",node,x[0],x[0]+.006,p+t*.5-.0025,p+t*.5-.002,za,zb,"grime",.0001,"GlazingGrime");}++col;}
  int i=0;for(double z:{h*.2,h*.8})hinge(b,node,node+".hinge."+id(i++),0,z,p,mirror,std::min(.21,lw*.6));
  double hx=mirror*(lw-stile/2),hz=h*.48;bool enabled=!(broken&&side=="left");
  b.handle(node+".handle",node,hx,p,hz,true,enabled,"",node+".stile.inner",stile-.004,-mirror);
  b.socket(node+".latch",node,"Lock","latch.casement.v1",{hx,p,hz-.15},{{"footprint",{.035,.07}},{"capabilities",{"latch"}},{"support_thickness",t}},enabled);
  if(enabled){x=xx(lw-stile,lw+.005);b.box(node+".latch.plate",node,x[0],x[1],p-.009,p-.004,hz-.165,hz-.14,"iron",.001,"Latch");b.rivet(node+".latch.rivet",node,hx,p-.01,hz-.152,true,.007);}
  x=xx(0,lw);b.socket(node+".mount",node,"Leaf","leaf.casement.v1",{(x[0]+x[1])/2,p+t/2,h/2},{{"footprint",{lw,h-2*g}},{"capabilities",{"hinged_leaf"}},{"damage",damage}});
  envelopes.push_back({{"node",node},{"rect",{{x[0],p},{x[1],p},{x[1],p+t},{x[0],p+t}}},{"z",{g,h-g}},{"angle_sign",-mirror},{"maximum_degrees",85}});
 }
 b.scene["metadata"]["leaf_envelopes"]=envelopes;
 b.scene["metadata"]["dimensions"]={{"clear_opening",{w,h,t}},{"leaf_width",lw},{"frame_member",.09},{"edge_gap",g},{"center_gap",center_gap},{"pivot_front_offset",p}};
}
Json base_wood(const Json&recipe){
 Builder b(normalize_recipe(recipe));wood_frame(b);if(b.r["kind"]=="door")wood_door(b);else wood_window(b);
 b.scene["metadata"]["clearance"]=clearance_sweep(b.scene);b.scene["metadata"]["clearance"]["scope"]="Conservative leaf wood envelopes versus fixed frame and synchronized leaf-leaf motion; sampled SAT, not continuous physics. Hardware contact is not certified.";
 b.scene["metadata"]["damage_policy"]="Broken supporting wood removes its handle/lock assemblies and disables matching sockets and grips. Missing glass disables glazing socket.";return b.scene;
}
void reinforce(Builder&b){
 auto&r=b.r;auto&scene=b.scene;double w=r["width"],h=r["height"],t=r["thickness"],f=r["kind"]=="door"?.115:.09,scale=1.5,ff=f*scale;PythonRandom rng(recipe_seed(r["seed"]),9021);
 auto widen=[&](double x){return std::abs(x)<=w/2?x:std::copysign(w/2+(std::abs(x)-w/2)*scale,x);};
 for(auto&part:scene["parts"]){if(part["material"]=="wood")part["material"]="wood_rough";if(part["role"]=="Frame"){for(auto&v:part["vertices"]){v[0]=widen(v[0]);double z=v[2];if(z>h)z=h+(z-h)*scale;if(z<0)z*=scale;v[2]=z;}part["uvs"]=metric_uvs(part["vertices"],part["faces"]);}}
 for(auto&envelope:scene["metadata"]["frame_envelopes"]){for(auto&p:envelope["rect"])p[0]=widen(p[0]);if(envelope["z"][1].get<double>()>h)envelope["z"][1]=h+(envelope["z"][1].get<double>()-h)*scale;if(envelope["z"][0].get<double>()<0)envelope["z"][0]=envelope["z"][0].get<double>()*scale;}
 double fy=-t/2-.01;
 for(int sign:{-1,1}){std::string side=sign<0?"left":"right";double cx=sign*(w/2+ff*.51);
  for(int corner:{0,1}){std::string tag=corner==0?"upper":"foot";double z0=corner==0?h-.13:.026,z1=corner==0?h+.045:.19;std::string prefix="reinforced.frame."+side+"."+tag;b.box(prefix,"frame",cx-.046,cx+.046,fy-.008,fy-.001,z0,z1,"iron",.002,"CornerReinforcement");int j=0;for(double z:{z0+.025,z1-.025})b.rivet(prefix+".bolt."+id(j++),"frame",cx,fy-.008,z,true,.012);}
  b.box("reinforced.frame."+side+".head","frame",cx-.067,cx+.067,fy-.01,fy-.002,h+.025,h+.087,"iron",.002,"CornerReinforcement");int j=0;for(double x:{cx-.044,cx+.044})b.rivet("reinforced.frame."+side+".head.bolt."+id(j++),"frame",x,fy-.01,h+.057,true,.011);
 }
 if(r["kind"]=="door"){
  const std::string node="door.leaf";double lw=scene["metadata"]["dimensions"]["leaf_width"],py=scene["metadata"]["dimensions"]["pivot_front_offset"];
  for(auto&part:scene["parts"]){const std::string name=part["id"];if(name.rfind("door.hinge.",0)==0&&name.find(".rivet.")!=std::string::npos)for(auto&v:part["vertices"])v[1]=v[1].get<double>()-.006;}
  int i=0;for(double z:{h*.13,h*.51,h*.88}){const std::string old="door.hinge."+id(i)+".strap";auto&parts=scene["parts"];parts.erase(std::remove_if(parts.begin(),parts.end(),[&](const Json&p){return p["id"]==old;}),parts.end());b.box(old,node,.006,lw-.006,py-.014,py-.004,z-.041,z+.041,"iron",.0018,"HingeStrap");int j=0;for(double fraction:{.13,.31,.50,.69,.87})b.rivet("reinforced.band."+id(i)+".bolt."+id(j++),node,lw*fraction,py-.014,z,true,.011);++i;}
  for(int diagonal:{0,1}){V2 a={.042,h*(diagonal==0?.855:.155)},c={lw-.042,h*(diagonal==0?.535:.485)};double dx=c[0]-a[0],dz=c[1]-a[1],length=std::hypot(dx,dz),ox=-dz/length*.025,oz=dx/length*.025;
   b.prism("reinforced.diagonal."+id(diagonal),node,{{a[0]-ox,a[1]-oz},{c[0]-ox,c[1]-oz},{c[0]+ox,c[1]+oz},{a[0]+ox,a[1]+oz}},py-.012,py-.003,"iron",.0014,"DiagonalReinforcement");int j=0;for(double q:{.12,.34,.56,.78,.93})b.rivet("reinforced.diagonal."+id(diagonal)+".bolt."+id(j++),node,a[0]+dx*q,py-.012,a[1]+dz*q,true,.009);
  }
  int j=0;for(double x:{.018,lw-.048})b.box("reinforced.leaf.edge."+id(j++),node,x,x+.03,py-.012,py-.005,.035,h-.038,"iron",.0015,"EdgeReinforcement");
 }else{
  for(const auto&n:scene["nodes"])if(n["role"]=="Leaf"){
   std::string node=n["id"];auto parts=scene["parts"];for(const auto&part:parts){if(part["node"]!=node||part["role"]!="CasementFrame")continue;auto xs=extrema(part["vertices"],0),zs=extrema(part["vertices"],2),ys=extrema(part["vertices"],1);if(zs[1]-zs[0]<xs[1]-xs[0])continue;double cx=(xs[0]+xs[1])/2;int k=0;for(double z:{h*.17,h*.83}){std::string prefix="reinforced."+part["id"].get<std::string>()+".clip."+id(k++);b.box(prefix,node,cx-.019,cx+.019,ys[0]-.007,ys[0]-.001,z-.045,z+.045,"iron",.001,"CasementClip");b.rivet(prefix+".bolt",node,cx,ys[0]-.007,z,true,.0065);}}
  }
 }
 auto parts=scene["parts"];
 for(const auto&part:parts){const std::string role=part["role"];if(part["material"]!="wood_rough"||(role!="Plank"&&role!="Frame"&&role!="CasementFrame"))continue;auto xs=extrema(part["vertices"],0),zs=extrema(part["vertices"],2),ys=extrema(part["vertices"],1);double xmin=xs[0],xmax=xs[1],zmin=zs[0],zmax=zs[1],y=ys[0];if(zmax-zmin<.3||xmax-xmin<.035)continue;
  for(int j=0;j<(role=="Plank"?4:2);++j){double x=rng.uniform(xmin+.01,xmax-.01),z=rng.uniform(zmin+.035,zmax-.21),length=std::min(rng.uniform(.13,.63),zmax-z-.015),wide=rng.uniform(.0008,.002);b.prism("wear."+part["id"].get<std::string>()+".check."+id(j),part["node"],{{x-wide,z},{x+wide*.65,z+length*.12},{x+wide*.35,z+length},{x-wide*.55,z+length*.77}},y-.0004,y+.0002,"grime",.0001,"WeatherCheck");}
 }
 scene["metadata"]["preset"]="reinforced_timber";scene["metadata"]["art_direction"]="Original reinforced mine timber: deep joints, bolted iron bands, visible diagonal bracing, localized longitudinal checking and corrosion. Guided by user reference, not a pixel/texture copy.";
 scene["recipe"]["preset"]="reinforced_timber";scene["metadata"]["clearance"]=clearance_sweep(scene);
}

Json fixed_frame(Builder&b){
 double w=b.r["width"],h=b.r["height"],t=b.r["thickness"];Json recipe=b.r;b.r["width"]=w+2*rebate;b.r["height"]=h+2*rebate;wood_frame(b);b.r=recipe;
 double timber_half=w/2+rebate,timber_top=h+2*rebate;
 auto widen=[&](double x){return std::abs(x)>timber_half?std::copysign(timber_half+(std::abs(x)-timber_half)*1.5,x):x;};
 auto height=[&](double z){if(z>timber_top)z=timber_top+(z-timber_top)*1.5;if(z<0)z*=1.5;return z-rebate;};
 for(auto&part:b.scene["parts"]){for(auto&v:part["vertices"]){v[0]=widen(v[0]);v[2]=part["role"]=="Frame"?height(v[2]):v[2].get<double>()-rebate;}if(part["material"]=="wood")part["material"]="wood_rough";part["uvs"]=metric_uvs(part["vertices"],part["faces"]);}
 for(auto&envelope:b.scene["metadata"]["frame_envelopes"]){for(auto&p:envelope["rect"])p[0]=widen(p[0]);for(auto&z:envelope["z"])z=height(z);}
 b.scene["sockets"][0]["coverage"]={{"opening",{w,h,t}},{"capabilities",{"fixed_glazing"}},{"max_pane_count",1},{"support_part_ids",{"frame.left","frame.right","frame.head","frame.sill"}}};
 double bead=rebate+.018;struct Profile{const char*name;double x0,x1,z0,z1;};const std::array<Profile,4> profiles={{{"left",-w/2-bead,-w/2,0,h},{"right",w/2,w/2+bead,0,h},{"bottom",-w/2-bead,w/2+bead,-bead,0},{"top",-w/2-bead,w/2+bead,h,h+bead}}};Json supports=Json::array();
 for(bool front:{true,false}){std::string face=front?"front":"back";double y0=front?-glass_thickness/2-.012:glass_thickness/2,y1=front?-glass_thickness/2:glass_thickness/2+.012;for(const auto&p:profiles){std::string name="frame.bead."+face+"."+p.name;b.box(name,"frame",p.x0,p.x1,y0,y1,p.z0,p.z1,"wood_rough",.0005,"GlazingBead");supports.push_back(name);}}
 double front_y=-t/2-.010;for(int sign:{-1,1}){std::string side=sign<0?"left":"right";double cx=sign*(w/2+rebate+frame_member*.52);for(bool bottom:{true,false}){std::string corner=bottom?"bottom":"top",name="frame.reinforcement."+side+"."+corner;double z=bottom?.09:h-.09;b.box(name,"frame",cx-.034,cx+.034,front_y-.006,front_y,z-.065,z+.065,"iron",.0015,"CornerReinforcement");int i=0;for(double dz:{-.043,.043})b.rivet(name+".bolt."+id(i++),"frame",cx,front_y-.006,z+dz,true,.010);}}
 return supports;
}
double polygon_area(const std::vector<V2>&points){double sum=0;for(size_t i=0;i<points.size();++i){const auto&a=points[i];const auto&b=points[(i+1)%points.size()];sum+=a[0]*b[1]-b[0]*a[1];}return std::abs(sum)/2;}
Json broken_pane(Builder&b,const std::array<double,4>&bounds){
 double x0=bounds[0],x1=bounds[1],z0=bounds[2],z1=bounds[3];PythonRandom rng(recipe_seed(b.r["seed"]),40193);double width=x1-x0,height=z1-z0;
 int side=randbelow(rng,2)==0?-1:1;double cx=side*rng.uniform(.075,.12)*width;side=randbelow(rng,2)==0?-1:1;double cz=(z0+z1)/2+side*rng.uniform(.035,.09)*height;
 std::vector<double> angles;for(int i=0;i<10;++i)angles.push_back(python_mod((i+rng.uniform(-.35,.35))*tau/10,tau));
 for(V2 corner:std::array<V2,4>{{{x0,z0},{x1,z0},{x1,z1},{x0,z1}}}){double angle=python_mod(std::atan2(corner[1]-cz,corner[0]-cx),tau);angles.erase(std::remove_if(angles.begin(),angles.end(),[&](double a){return std::abs(python_mod(a-angle+pi,tau)-pi)<=.14;}),angles.end());angles.push_back(angle);}std::sort(angles.begin(),angles.end());
 std::vector<size_t> candidates(angles.size());std::iota(candidates.begin(),candidates.end(),0);for(size_t i=candidates.size()-1;i>0;--i)std::swap(candidates[i],candidates[randbelow(rng,static_cast<uint32_t>(i+1))]);
 std::vector<size_t> long_shards;for(size_t index:candidates){bool distant=true;for(size_t j:long_shards)if(std::abs(python_mod(angles[index]-angles[j]+pi,tau)-pi)<=.90)distant=false;if(distant)long_shards.push_back(index);if(long_shards.size()==3)break;}
 std::vector<V2> outer,inner;for(size_t index=0;index<angles.size();++index){double angle=angles[index],dx=std::cos(angle),dz=std::sin(angle),tx=std::abs(dx)>1e-14?((dx>0?x1:x0)-cx)/dx:INFINITY,tz=std::abs(dz)>1e-14?((dz>0?z1:z0)-cz)/dz:INFINITY,length=std::min(tx,tz);outer.push_back({cx+dx*length,cz+dz*length});bool long_shard=std::find(long_shards.begin(),long_shards.end(),index)!=long_shards.end();double fraction=long_shard?rng.uniform(.32,.46):rng.uniform(.76,.94);inner.push_back({cx+dx*length*fraction,cz+dz*length*fraction});}
 size_t count=outer.size();Json vertices=Json::array(),faces=Json::array();for(double y:{-glass_thickness/2,glass_thickness/2}){for(const auto&p:outer)vertices.push_back({p[0],y,p[1]});for(const auto&p:inner)vertices.push_back({p[0],y,p[1]});}
 for(size_t i=0;i<count;++i){size_t j=(i+1)%count,a=i,bb=j,c=count+j,d=count+i;faces.push_back({a,bb,c});faces.push_back({a,c,d});faces.push_back({c+2*count,bb+2*count,a+2*count});faces.push_back({d+2*count,c+2*count,a+2*count});faces.push_back({i,2*count+i,2*count+j,j});faces.push_back({count+j,3*count+j,3*count+i,count+i});}
 b.mesh("window.glass","frame",vertices,faces,"glass",.00015,"GlassPane");V2 lo={INFINITY,INFINITY},hi={-INFINITY,-INFINITY};for(const auto&v:inner)for(size_t k=0;k<2;++k){lo[k]=std::min(lo[k],v[k]);hi[k]=std::max(hi[k],v[k]);}double area=polygon_area(inner);
 return {{"retained_glass_fraction",1-area/(width*height)},{"glass_hole",{{"center_xz",{cx,cz}},{"polygon_xz",inner},{"bounds_xz",{lo,hi}},{"area_m2",area},{"construction","Closed connected annular glass shell with inner fracture walls; physical through-hole"}}}};
}
} // namespace

Json clearance_sweep(const Json&scene,double step){return clearance_sweep_impl(scene,step);}

void wood_frame(Builder&b){
 double w=b.r["width"],h=b.r["height"],t=b.r["thickness"],f=b.r["kind"]=="door"?.115:.09,ymin=-t/2-.010,ymax=t/2+.042;b.node("frame",{0,0,0},"Frame",nullptr);
 for(int side:{-1,1}){std::string name=side<0?"left":"right";double x0=side<0?-w/2-f:w/2,x1=side<0?-w/2:w/2+f;b.chipped_board("frame."+name,"frame",x0,x1,ymin,ymax,0,h,1.1,"Frame");int i=0;for(double z:{.14,h*.51,h-.10})b.rivet("frame."+name+".bolt."+id(i++),"frame",(x0+x1)/2,ymin,z,true,.014);b.box("frame."+name+".grime","frame",x0+.005,x0+.013,ymin-.001,ymin+.001,.045,h-.03,"grime",.0003,"DirtSeam");}
 b.chipped_board("frame.head","frame",-w/2-f,w/2+f,ymin,ymax,h,h+f,1.2,"Frame");if(b.r["kind"]=="window")b.chipped_board("frame.sill","frame",-w/2-f-.025,w/2+f+.025,ymin-.027,ymax,-f,0,1.3,"Frame");
 b.socket("frame.opening","frame","FrameOpening","opening.rect.v1",{0,0,0},{{"opening",{w,h,t}},{"capabilities",{"hinged_leaf"}},{"max_leaf_count",b.r["kind"]=="door"?1:2}});
 b.scene["metadata"]["frame_envelopes"]=Json::array({{{"id","frame.left"},{"rect",{{-w/2-f,ymin},{-w/2,ymin},{-w/2,ymax},{-w/2-f,ymax}}},{"z",{0,h}}},{{"id","frame.right"},{"rect",{{w/2,ymin},{w/2+f,ymin},{w/2+f,ymax},{w/2,ymax}}},{"z",{0,h}}},{{"id","frame.head"},{"rect",{{-w/2-f,ymin},{w/2+f,ymin},{w/2+f,ymax},{-w/2-f,ymax}}},{"z",{h,h+f}}}});
 if(b.r["kind"]=="window")b.scene["metadata"]["frame_envelopes"].push_back({{"id","frame.sill"},{"rect",{{-w/2-f,ymin-.027},{w/2+f,ymin-.027},{w/2+f,ymax},{-w/2-f,ymax}}},{"z",{-f,0}}});
}
Json build_wood(const Json&recipe){
 if(!recipe.is_object())throw std::invalid_argument("Recipe must be an object");
 Json local=recipe;auto preset=local.value("preset",Json("mine_timber"));local.erase("preset");if(preset!="mine_timber"&&preset!="reinforced_timber")throw std::invalid_argument("Unknown timber preset");auto scene=base_wood(local);if(preset=="mine_timber")return scene;Builder b(scene["recipe"]);b.scene=std::move(scene);reinforce(b);return b.scene;
}
Json build_singlepane(const Json&recipe){
 if(!recipe.is_object())throw std::invalid_argument("Recipe must be an object");
 Json local=recipe;auto glazing_json=local.value("glazing",Json("intact")),preset=local.value("preset",Json("single_pane_fixed"));local.erase("glazing");local.erase("preset");if(glazing_json!="intact"&&glazing_json!="broken"&&glazing_json!="empty")throw std::invalid_argument("Unknown glazing state");if(preset!="single_pane_fixed")throw std::invalid_argument("Unknown fixed-window preset");std::string glazing=glazing_json;
 Json defaults={{"kind","window"},{"width",1.12},{"height",1.28},{"thickness",.06},{"damage","aged"},{"seed",17}};defaults.update(local);auto normalized=normalize_recipe(defaults);if(normalized["kind"]!="window")throw std::invalid_argument("The single-pane fixed preset requires kind=window");if(normalized["angle"]!=0)throw std::invalid_argument("A fixed frame cannot have a moving leaf angle");
 Builder b(normalized);double w=normalized["width"],h=normalized["height"],t=normalized["thickness"];auto supports=fixed_frame(b);double bearing=rebate-edge_clearance,x0=-w/2-bearing,x1=w/2+bearing,z0=-bearing,z1=h+bearing;
 b.socket("window.glass.socket","frame","Glass","pane.rect.v1",{0,0,h/2},{{"footprint",{x1-x0,z1-z0}},{"opening",{w,h}},{"thickness",glass_thickness},{"max_thickness",glass_thickness},{"capabilities",{"glazing","fixed_glazing"}},{"support_part_ids",supports},{"support_overlap",bearing},{"edge_clearance",edge_clearance},{"glazing_state",glazing}},glazing!="empty");
 auto&metadata=b.scene["metadata"];metadata.update(Json{
  {"generator","JericoProceduralCppLab/wood.cpp"},{"preset","single_pane_fixed"},{"glazing_state",glazing},
  {"dimensions",{{"clear_opening",{w,h,t}},{"structural_opening",{w+2*rebate,h+2*rebate}},{"frame_member",frame_member},{"rebate_depth",rebate},{"glass_size",{x1-x0,z1-z0,glass_thickness}},{"glass_edge_clearance",edge_clearance}}},
  {"leaf_envelopes",Json::array()},{"retained_glass_fraction",glazing=="intact"?1.:0.},
  {"grasp_policy","Not applicable: fixed glazing has no moving leaf or hand-grasp interaction."},
  {"damage_policy","Glazing state is independent of the stable aged timber frame. Broken glazing retains a supported closed perimeter shell. Empty removes the glass and disables its socket."},
  {"support_policy","Glass sits in a two-sided stop/rebate pocket with 1 mm clearance from structural timber. Front/back retaining beads overlap all four edges by 11 mm; no floating fragments."},
  {"socket_coverage_convention","Glass footprint is the full pane cut size, opening is visible clear width/height. support_part_ids identifies both sets of retaining beads. Existing glass and the modeled stop pocket are 4 mm thick. The replacement interface is capped at 4 mm; no adjustable-stop mechanism is modeled."},
  {"art_direction","Dark aged timber and compact bolted iron perimeter reinforcement. Existing chipped frame outlines only; no new wood-edge damage or bars."}});
 if(glazing=="intact")b.box("window.glass","frame",x0,x1,-glass_thickness/2,glass_thickness/2,z0,z1,"glass",.00015,"GlassPane");else if(glazing=="broken")metadata.update(broken_pane(b,{x0,x1,z0,z1}));else metadata["missing_parts"].push_back("window.glass");
 metadata["clearance"]=clearance_sweep(b.scene);b.scene["recipe"]["preset"]="single_pane_fixed";b.scene["recipe"]["glazing"]=glazing;return b.scene;
}
} // namespace jerico
