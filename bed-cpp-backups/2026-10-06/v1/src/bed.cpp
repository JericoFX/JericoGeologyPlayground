#include "jerico/bed.hpp"
#include "jerico/evaluate.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
namespace jerico {
namespace {
constexpr double pi=3.14159265358979323846;
double len(V3 p){return std::sqrt(dot(p,p));}
void require(bool b,const std::string&m){if(!b)throw std::invalid_argument("Bed: "+m);}
std::string num(int i){return std::to_string(i);}
void smooth_mesh(Json&p){
 std::vector<V3> ns(p["vertices"].size());
 for(auto&f:p["faces"]){V3 a=p["vertices"][f[0].get<size_t>()],b=p["vertices"][f[1].get<size_t>()],c=p["vertices"][f[2].get<size_t>()];V3 n=cross(sub(b,a),sub(c,a));for(auto i:f)ns[i.get<size_t>()]=add(ns[i.get<size_t>()],n);}
 Json loops=Json::array();for(auto&f:p["faces"]){Json q=Json::array();V3 a=p["vertices"][f[0].get<size_t>()],b=p["vertices"][f[1].get<size_t>()],c=p["vertices"][f[2].get<size_t>()],ng=norm(cross(sub(b,a),sub(c,a)));for(auto i:f){V3 n=norm(ns[i.get<size_t>()]);q.push_back(dot(n,ng)>.08?n:ng);}loops.push_back(q);}p["normals"]=loops;p["native_normals"]={{"method","native_area_weighted_surface_normals_with_hemisphere_guard"}};
}
Json& tube(Builder&b,const std::string&id,const std::string&owner,const std::vector<V3>&path,double radius,const std::string&material,const std::string&role,bool closed=false,double wear=0){
 require(path.size()>=2,"tube needs at least two samples");const int sides=12;Json vs=Json::array(),fs=Json::array();double along=0;std::vector<double> arc(path.size());
 for(size_t j=0;j<path.size();j++){if(j)along+=len(sub(path[j],path[j-1]));arc[j]=along;}
 for(size_t j=0;j<path.size();j++){
  V3 tangent=j==0?(closed?sub(path[1],path.back()):sub(path[1],path[0])):j+1==path.size()?(closed?sub(path[0],path[j-1]):sub(path[j],path[j-1])):sub(path[j+1],path[j-1]);tangent=norm(tangent);V3 ref=closed?V3{0,0,1}:(std::abs(tangent[1])<.92?V3{0,1,0}:V3{1,0,0});V3 u=norm(sub(ref,mul(tangent,dot(ref,tangent)))),v=cross(tangent,u);
  for(int k=0;k<sides;k++){double angle=2*pi*k/sides;double local=std::exp(-std::pow((double(j)/std::max(size_t(1),path.size()-1)-.37)/.055,2))*std::exp(-std::pow((angle-2.1)/.45,2));double rr=radius-std::min(.0013,radius*.08)*wear*local;vs.push_back(add(path[j],add(mul(u,rr*std::cos(angle)),mul(v,rr*std::sin(angle)))));}
 }
 size_t segments=closed?path.size():path.size()-1;
 for(size_t j=0;j<segments;j++)for(int k=0;k<sides;k++){size_t a=j*sides+k,c=j*sides+(k+1)%sides,d=((j+1)%path.size())*sides+(k+1)%sides,e=((j+1)%path.size())*sides+k;fs.push_back({a,c,d});fs.push_back({a,d,e});}
 if(!closed)for(size_t end:{size_t(0),path.size()-1}){size_t center=vs.size();vs.push_back(path[end]);for(int k=0;k<sides;k++){size_t a=end*sides+k,c=end*sides+(k+1)%sides;fs.push_back(end==0?Json{center,c,a}:Json{center,a,c});}}
 auto&p=b.mesh(id,owner,vs,fs,material,0,role);p["bed_smooth"]=true;p["tube_radius_m"]=radius;p["centerline"]=path;p["tube_closed_path"]=closed;p["localized_dent_m_max"]=std::min(.0013,radius*.08)*wear;return p;
}
std::vector<V3> line(V3 a,V3 c,int n=5){std::vector<V3> p;for(int i=0;i<=n;i++)p.push_back(add(a,mul(sub(c,a),double(i)/n)));return p;}
double mattress_top(double x,double y,double w,double l){double u=std::clamp(x/(w/2),-1.,1.),v=std::clamp(y/(l/2),-1.,1.);double f=std::pow(std::max(0.,1-std::pow(u,16)),.38)*std::pow(std::max(0.,1-std::pow(v,16)),.38);return .548+.029*f+.0012*f*std::sin(x*21+y*13);}
std::vector<size_t> boundary(int nx,int ny){std::vector<size_t> q;for(int x=0;x<=nx;x++)q.push_back(x);for(int y=1;y<=ny;y++)q.push_back(y*(nx+1)+nx);for(int x=nx-1;x>=0;x--)q.push_back(ny*(nx+1)+x);for(int y=ny-1;y>0;y--)q.push_back(y*(nx+1));return q;}
template<class F> Json& skin(Builder&b,const std::string&id,const std::string&node,int nx,int ny,F position,const std::string&material,const std::string&role){
 Json vs=Json::array(),fs=Json::array();size_t layer=(nx+1)*(ny+1);
 for(int side=0;side<2;side++)for(int y=0;y<=ny;y++)for(int x=0;x<=nx;x++)vs.push_back(position(2.*x/nx-1,2.*y/ny-1,side));
 for(int side=0;side<2;side++)for(int y=0;y<ny;y++)for(int x=0;x<nx;x++){size_t a=side*layer+y*(nx+1)+x,c=a+1,d=c+nx+1,e=a+nx+1;if(!side){fs.push_back({a,c,d});fs.push_back({a,d,e});}else{fs.push_back({d,c,a});fs.push_back({e,d,a});}}
 auto ring=boundary(nx,ny);for(size_t i=0;i<ring.size();i++){size_t a=ring[i],c=ring[(i+1)%ring.size()];fs.push_back({a,a+layer,c+layer});fs.push_back({a,c+layer,c});}
 auto&p=b.mesh(id,node,vs,fs,material,0,role);p["bed_smooth"]=true;return p;
}
void structure(Builder&b){
 double w=b.r["width"],l=b.r["length"],r=b.r["tube_radius"],wear=b.r["wear"];double x=w/2,y=l/2,z=.36;int bars=b.r["bar_count"];
 for(int end:{-1,1}){std::string id=end<0?"foot":"head";double h=b.r[end<0?"foot_height":"head_height"],bend=std::min(.115,w*.15);std::vector<V3> p;
  auto append=[&](V3 a,V3 c,int n){auto ps=line(a,c,n);if(!p.empty())ps.erase(ps.begin());p.insert(p.end(),ps.begin(),ps.end());};
  append({-x,end*y,.034},{-x,end*y,h-bend},12);
  for(int j=1;j<=10;j++){double a=pi-pi/2*j/10;p.push_back({-x+bend+bend*std::cos(a),end*y,h-bend+bend*std::sin(a)});}
  append({-x+bend,end*y,h},{x-bend,end*y,h},12);
  for(int j=1;j<=10;j++){double a=pi/2-pi/2*j/10;p.push_back({x-bend+bend*std::cos(a),end*y,h-bend+bend*std::sin(a)});}
  append({x,end*y,h-bend},{x,end*y,.034},12);tube(b,id+".bow","frame",p,r,"iron","CurvedHeadFootRail",false,wear);
  tube(b,id+".lower","frame",line({-x,end*y,z},{x,end*y,z}),r*.67,"iron","CrossRail");
  for(int j=1;j<=bars;j++){double xx=-x+bend+(w-2*bend)*j/(bars+1);tube(b,id+".bar."+num(j),"frame",line({xx,end*y,z},{xx,end*y,h},7),r*.34,"iron","VerticalBar",false,wear);for(int top:{0,1})tube(b,id+".weld."+num(j)+"."+num(top),"frame",line({xx,end*y,(top?h:z)-.002},{xx,end*y,(top?h:z)+.002},1),r*.39,"iron","WeldCollar");}
  for(int sx:{-1,1}){double xx=sx*x;b.cylinder(id+".foot."+num(sx),"frame",{xx,end*y,0},{xx,end*y,.044},r*1.30,"iron",16,.002,"FootFerrule");b.box(id+".plate."+num(sx),"frame",xx-.031,xx+.031,end*y-.031,end*y+.031,.315,.402,"iron",.003,"CornerMount");
   b.cylinder(id+".bolt."+num(sx),"frame",{xx,end*y+end*.031,.357},{xx,end*y+end*.040,.357},.011,"iron",6,.001,"HexBolt");
   b.box(id+".slot."+num(sx),"frame",xx-.006,xx+.006,std::min(end*y+end*.040,end*y+end*.0405),std::max(end*y+end*.040,end*y+end*.0405),.356,.358,"iron",.0002,"BoltSlot");
   // Gussets carry side rail into the post; overlaps are intentional connections.
   double yi=end*y;Json profile={{xx-.009,.28},{xx-.009,.38},{xx-sx*.14,.38}};if(sx<0)std::reverse(profile.begin(),profile.end());std::vector<V2> poly=profile.get<std::vector<V2>>();b.prism(id+".gusset."+num(sx),"frame",poly,yi-.006,yi+.006,"iron",.001,"TriangularGusset");
  }
 }
 for(int sx:{-1,1}){double xx=sx*x;b.box("side.rail."+num(sx),"frame",xx-.019,xx+.019,-y,y,.327,.393,"iron",.004,"SideRail");
  // Retaining tabs stop below the rounded mattress top and outside its footprint.
  for(int j=0;j<3;j++){double yy=(-.60+j*.60)*y;b.box("retainer."+num(sx)+"."+num(j),"frame",xx-.025,xx+.025,yy-.017,yy+.017,.365,.465,"iron",.003,"MattressRetainer");b.cylinder("retainer.bolt."+num(sx)+"."+num(j),"frame",{xx+sx*.025,yy,.390},{xx+sx*.032,yy,.390},.008,"iron",12,.0005,"RetainerBolt");}}
 for(int j=0;j<8;j++){double yy=-y+.09+(l-.18)*j/7;b.box("support.slat."+num(j),"frame",-x+.01,x-.01,yy-.025,yy+.025,.381,.405,"iron",.003,"MattressSupport");}
 b.socket("mattress.mount","frame","BeddingMount","bed.bedding.v1",{0,0,.405},{{"support_part_ids",{"support.slat.0","support.slat.3","support.slat.7"}},{"footprint",{w-.07,l-.10}},{"authoring_only",true}});
 for(int end:{-1,1}){std::string id=end<0?"foot":"head";double h=b.r[end<0?"foot_height":"head_height"];auto& railSocket=b.socket(id+".grab","frame","BedRail","bed.rail.grab.v1",{0,end*y,h},{{"support_part_ids",{id+".bow"}},{"authoring_only",true}});railSocket["normal"]={0,end,0};railSocket["basis"]={{"x",{1,0,0}},{"y",{0,0,-end}},{"z",{0,end,0}}};b.scene["grips"].push_back({{"id",id+".grip"},{"node","frame"},{"socket_id",id+".grab"},{"position",{0,end*y+end*r,h}},{"approach",{0,-end,0}},{"basis",{{"x",{1,0,0}},{"y",{0,0,-end}},{"z",{0,end,0}}}},{"reach_radius",.2},{"enabled",true}});b.scene["metadata"]["grip_supports"][id+".grip"]=id+".bow";}
}
void textiles(Builder&b){
 double w=b.r["width"].get<double>()-.080,l=b.r["length"].get<double>()-.11;const double seed=double(recipe_seed(b.r["seed"])%1000),phase=seed*.137;
 b.node("bedding",{0,0,0},"SeparateTextiles");
 auto mattress=[&](double u,double v,int side){double x=w*.5*u*std::sqrt(1-.06*std::pow(v,12)),y=l*.5*v*std::sqrt(1-.018*std::pow(u,12));double z=side?.405+.020*std::max(std::pow(u,64),std::pow(v,64)):mattress_top(x,y,w,l);return V3{x,y,z};};
 auto&mp=skin(b,"mattress","bedding",32,64,mattress,"linen","RoundedMattress");mp["contact_surface"]="support.slat.*";mp["native_textile"]="Analytical closed static mattress; no cloth simulation";
 auto ring=boundary(32,64);for(int top:{0,1}){std::vector<V3> points;for(auto index:ring){double u=2.*(index%33)/32-1,v=2.*(index/33)/64-1;V3 p=mattress(u,v,0);p[2]=top?p[2]-.007:.429;points.push_back(p);}tube(b,"mattress.seam."+num(top),"bedding",points,.0017,"linen","PerimeterPiping",true);}
 double pw=w*.72,pl=.45,cy=l/2-.30;
 auto pillow=[&](double u,double v,int side){double x=pw*.5*u*std::sqrt(1-.15*std::pow(v,4)),y=cy+pl*.5*v*std::sqrt(1-.12*std::pow(u,4));double dome=std::sqrt(std::max(0.,1-u*u))*std::sqrt(std::max(0.,1-v*v));x+=.014*v;y+=.007*std::sin(u*3);double support=mattress_top(x,y,w,l);double wrinkle=.007*std::sin(u*24+v*4+phase)*std::exp(-std::pow((std::abs(v)-.82)/.18,2))*dome;return V3{x,y,support+(side?.001+.009*(1-dome):.015+.097*dome+wrinkle)};};
 auto&pp=skin(b,"pillow","bedding",32,24,pillow,"linen","IrregularPillow");pp["native_textile"]="Analytical volumetric pillow, follows mattress support";
 std::vector<V3> perimeter;for(auto i:boundary(32,24)){double u=2.*(i%33)/32-1,v=2.*(i/33)/24-1;V3 p=pillow(u,v,0);p[2]-=.003;perimeter.push_back(p);}tube(b,"pillow.seam","bedding",perimeter,.0012,"linen","PillowPiping",true);
 if(b.r["preset"]!="complete")return;
 double half=w/2,yl=-l/2+.065,yh=l*.12;
 auto blanket=[&](double u,double v,int side){double s=-half-.32+(w+.46)*(u+1)/2,y=yl+(yh-yl)*(v+1)/2;double clamped=std::clamp(s,-half+.009,half-.009);double ridge=0;
  for(int j=0;j<5;j++){double center=yl+(yh-yl)*(.10+j*.188)+.021*std::sin(s*5+j*1.1+phase);ridge+=(.014+.009*std::sin(j*1.7+phase)*std::sin(j*1.7+phase))*std::exp(-std::pow((y-center)/(.027+.005*j),2));}
  double excess=std::max(0.,std::abs(s)-(half-.01)),x=s,z=mattress_top(clamped,y,w,l)+.003+ridge;double edge=(.006*std::sin(y*37+phase)+.004*std::sin(y*71));
  if(excess>0){x=std::copysign(half-.01+.092*(1-std::exp(-excess/.032))+ridge*.55*(1-std::exp(-excess/.06)),s);z-=excess*.91;z+=edge*std::pow(excess/.32,2);}
  y+=.009*std::sin(s*16+phase)*(std::abs(v)>.9?1:std::pow(std::abs(v),6));z+=.004*std::pow(std::abs(v),12)*(1+std::sin(s*22+phase));if(side)z-=.0025;return V3{x,y,z};};
 auto&bp=skin(b,"blanket","bedding",72,64,blanket,"blanket","StaticAnalyticalBlanket");bp["native_textile"]="Closed 2.5 mm analytical surface with static folds/drape; not physical cloth simulation";bp["minimum_bar_end_clearance_m"]=.025;
 // Worn edge threads remain sparse geometry, not alpha polygons or hundreds of textures.
 for(int j=0;j<32;j++){double v=-.97+1.94*j/31;V3 a=blanket(-1,v,0),c=add(a,{.002*std::sin(j),.003*std::sin(j*2),-.012-.015*(.5+.5*std::sin(j*2.3+phase))});tube(b,"blanket.fray."+num(j),"bedding",{a,add(mul(add(a,c),.5),{.003,0,0}),c},.00055,"blanket","AttachedEdgeThread");}
}
}
Json normalize_bed_recipe(const Json&input){
 require(input.is_object(),"recipe must be object");Json r={{"preset","complete"},{"width",1.02},{"length",2.04},{"head_height",1.12},{"foot_height",.81},{"tube_radius",.021},{"bar_count",5},{"seed",71},{"wear",.8}};
 for(auto i=input.begin();i!=input.end();++i)require(r.contains(i.key()),"unknown field "+i.key());r.update(input);require(r["preset"]=="iron_only"||r["preset"]=="bedding"||r["preset"]=="complete","unknown preset");
 for(auto range:std::vector<std::pair<std::string,V2>>{{"width",{.7,1.6}},{"length",{1.7,2.4}},{"head_height",{.85,1.45}},{"foot_height",{.69,1.08}},{"tube_radius",{.014,.032}},{"wear",{0,1}}}){auto value=r[range.first];require(value.is_number(),range.first+" must be numeric");double v=value;require(std::isfinite(v)&&v>=range.second[0]&&v<=range.second[1],range.first+" outside range");r[range.first]=v;}
 recipe_seed(r["seed"]);require(r["bar_count"].is_number_integer(),"bar_count must be integer");int n=r["bar_count"];require(n>=3&&n<=12,"bar_count 3..12");double w=r["width"],rad=r["tube_radius"];require((w-2*.115)/(n+1)>rad*2.9,"bar count/tube radius leave insufficient rail clearance");require(r["head_height"].get<double>()>=r["foot_height"].get<double>(),"head cannot be below foot");return r;
}
Json build_bed(const Json&recipe){Builder b(normalize_bed_recipe(recipe));b.node("frame",{0,0,0},"IronBed",nullptr);structure(b);if(b.r["preset"]!="iron_only")textiles(b);auto&m=b.scene["metadata"];m["generator"]="JericoBedCppLab/native C++17";m["family"]="aged_iron_bed";m["dimensions"]={{"nominal_width_m",b.r["width"]},{"nominal_length_m",b.r["length"]},{"mattress_support_z_m",.405},{"mattress_nominal_thickness_m",.172}};m["clearance_contract"]={{"mattress_to_end_bar_m",.020},{"blanket_to_end_bar_m",.025},{"scope","Static generated bed. Tube/collar and bolt/plate penetrations are intentional joints. No runtime simulation."}};m["engine_ownership"]="Engine owns colliders, rigid bodies, cloth, constraints and gameplay. Export contains only authored geometry, IDs and optional rail grip sockets.";m["reference_policy"]="Original procedural interpretation of an aged iron bed; no source asset or measured dimensions copied.";return b.scene;}
Json evaluate_bed(const Json&scene){auto s=evaluate_scene(scene);for(auto&p:s["parts"])if(p.value("bed_smooth",false))smooth_mesh(p);return s;}
Json validate_bed(const Json&s){Json errors=Json::array();double w=s["recipe"]["width"],l=s["recipe"]["length"],r=s["recipe"]["tube_radius"];std::set<std::string> ids;for(auto&p:s["parts"])ids.insert(p["id"]);for(auto id:{"head.bow","foot.bow","side.rail.-1","side.rail.1","support.slat.0","support.slat.7"})if(!ids.count(id))errors.push_back("missing structure "+std::string(id));
 for(auto&p:s["parts"]){std::string id=p["id"];if(id=="mattress"||id=="pillow"||id=="blanket")for(auto&v:p["vertices"]){V3 q=v;if(std::abs(q[1])>l/2-r-.008)errors.push_back(id+": intersects end-bar envelope");if(q[2]<.075)errors.push_back(id+": insufficient floor clearance");if(id=="mattress"&&std::abs(q[0])>w/2-.023)errors.push_back("mattress intersects side retainers");}
  if(id=="mattress"){double low=100;for(auto&v:p["vertices"])low=std::min(low,v[2].get<double>());if(std::abs(low-.405)>.000001)errors.push_back("mattress does not contact slats");}
 }
 return errors;}
}
