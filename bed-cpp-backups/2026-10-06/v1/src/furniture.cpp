#include "jerico/scene.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace jerico {
namespace {
constexpr double pi=3.141592653589793238462643383279502884;
// Python-compatible shortest decimal IDs; fixture parity covers float-derived IDs.
std::string decimal(double x){char text[64];auto r=std::to_chars(text,text+sizeof(text),x);if(r.ec!=std::errc())throw std::runtime_error("number formatting failed");std::string s(text,r.ptr);if(s.find_first_of(".eE")==std::string::npos)s+=".0";return s;}
std::string integer(int x){return std::to_string(x);}
void reverse_faces(Json& fs){for(auto&f:fs)std::reverse(f.begin(),f.end());}
double signed_face_volume(const Json&vs,const Json&fs){double v=0;for(const auto&f:fs)v+=dot(vs.at(f[0].get<size_t>()).get<V3>(),cross(vs.at(f[1].get<size_t>()).get<V3>(),vs.at(f[2].get<size_t>()).get<V3>()))/6;return v;}

struct FurnitureBuilder:Builder {
 explicit FurnitureBuilder(const Json&r):Builder(r){
  scene["metadata"].update(Json{{"generator","JericoProceduralCppLab/furniture.cpp"},{"leaf_envelopes",Json::array()},{"frame_envelopes",Json::array()},{"drawers",Json::array()},{"static_envelopes",Json::array()},{"references",{"cajonera4cajones","bancocajon+balda"}},{"clearance_scope","Conservative complete drawer shell AABB swept through full -Y travel against fixed structural envelopes. Hardware is constrained within its front envelope. Mortise and fastener overlaps are intentional."}});
  node("frame",{0,0,0},"Furniture",nullptr);
 }
 Json& board(const std::string&id,const std::string&owner,V3 lo,V3 hi,const std::string&role="Plank",const std::string&face="front",bool groove=false){
  V3 origin,U,V,N;double L,W,T;
  if(face=="front"){origin=lo;U={1,0,0};V={0,0,1};N={0,-1,0};L=hi[0]-lo[0];W=hi[2]-lo[2];T=hi[1]-lo[1];}
  else if(face=="side"){origin={hi[0],lo[1],lo[2]};U={0,1,0};V={0,0,1};N={1,0,0};L=hi[1]-lo[1];W=hi[2]-lo[2];T=hi[0]-lo[0];}
  else{origin={lo[0],lo[1],hi[2]};U={1,0,0};V={0,1,0};N={0,0,1};L=hi[0]-lo[0];W=hi[1]-lo[1];T=hi[2]-lo[2];}
  if(W>L){origin=add(origin,mul(U,L));auto oldU=U;U=V;V=mul(oldU,-1);std::swap(L,W);}
  const double wear=r["wear"],c=std::max(.0002,std::min({.006*wear,L*.06,W*.09}));
  const double a=rng.uniform(.24,.41),end=rng.uniform(.64,.83);
  std::vector<double> uk={0,c/L,.1,a,(a+end)/2,end,.91,1-c/L,1};std::sort(uk.begin(),uk.end());uk.erase(std::unique(uk.begin(),uk.end()),uk.end());
  const double center=rng.uniform(.37,.63),half=std::min(.009/W,.045);
  const std::vector<double> vk={0,.035,center-half,center,center+half,.965,1};
  const double d=groove?std::min(T*.15,.0035)*wear:0;
  const size_t rows=vk.size(),cols=uk.size();Json verts=Json::array(),faces=Json::array(),mats=Json::array();
  for(int side:{0,1})for(double u:uk){
   const double edge=c*std::max({0.,1-u*L/c,1-(1-u)*L/c});
   for(size_t j=0;j<rows;++j){
    const double notch=groove?std::min(.008,W*.07)*wear*std::max(0.,1-std::abs(u-end)/.075):0;
    double s=edge+notch+(W-2*edge-notch)*vk[j];
    if(j==2||j==3||j==4)s+=std::sin(u*18+1)*std::min(.0015,W*.01);
    const double k=a<u&&u<end?std::max(0.,std::sin(pi*(u-a)/(end-a))):0;
    const double depth=side?-T:(j==3?-d*k:0);V3 vertex;
    for(size_t i=0;i<3;++i)vertex[i]=origin[i]+L*u*U[i]+s*V[i]+depth*N[i];
    verts.push_back(vertex);
   }
  }
  for(size_t side:{0u,1u}){size_t off=side*rows*cols;for(size_t i=0;i<cols-1;++i)for(size_t j=0;j<rows-1;++j){
   std::array<size_t,4> f={off+i*rows+j,off+(i+1)*rows+j,off+(i+1)*rows+j+1,off+i*rows+j+1};if(side)std::reverse(f.begin(),f.end());
   faces.push_back({f[0],f[1],f[2]});faces.push_back({f[0],f[2],f[3]});mats.push_back("wood");mats.push_back("wood");
  }}
  std::vector<size_t> ring;for(size_t i=0;i<cols;++i)ring.push_back(i*rows);for(size_t j=1;j<rows;++j)ring.push_back((cols-1)*rows+j);for(int i=int(cols)-2;i>=0;--i)ring.push_back(i*rows+rows-1);for(int j=int(rows)-2;j>0;--j)ring.push_back(j);
  for(size_t i=0;i<ring.size();++i){size_t p=ring[i],q=ring[(i+1)%ring.size()];faces.push_back({q,p,p+rows*cols,q+rows*cols});mats.push_back("wood_edge");}
  auto&part=mesh(id,owner,verts,faces,"wood",std::min({.0025,T*.08,W*.03}),role);
  part.update(Json{{"face_materials",mats},{"grain_axis",U},{"groove_depth_m",d},{"outline_chamfer_m",c},{"bevel_material","wood_edge"},{"bounds",{lo,hi}}});return part;
 }
 Json& fixed_board(const std::string&id,V3 lo,V3 hi,const std::string&role="Structure",const std::string&face="front",bool groove=false){
  auto&part=board(id,"frame",lo,hi,role,face,groove);scene["metadata"]["static_envelopes"].push_back({{"id",id},{"lo",lo},{"hi",hi}});return part;
 }
 void bolt(const std::string&id,const std::string&owner,double x,double y,double z,double radius=.005){
  cylinder(id,owner,{x,y,z},{x,y-.0035,z},radius,"iron",12,.0006,"Bolt");
  box(id+".slot",owner,x-radius*.6,x+radius*.6,y-.0038,y-.0035,z-.0007,z+.0007,"grime",0,"ScrewSlot");
 }
 void cup(const std::string&id,const std::string&owner,double x,double y,double z){
  constexpr double rx=.064,rz=.029,dep=.028,th=.0025;constexpr int nt=16,nr=5,ns=1+nr*(nt+1);Json verts=Json::array(),faces=Json::array();
  for(bool inner:{false,true}){double R=inner?rx-th:rx,Z=inner?rz-th:rz,D=inner?dep-th:dep;verts.push_back({x,y-D,z});for(int j=1;j<=nr;++j){double phi=double(j)/nr*pi/2;for(int i=0;i<=nt;++i){double theta=double(i)/nt*pi;verts.push_back({x+R*std::cos(theta)*std::sin(phi),y-D*std::cos(phi),z+Z*std::sin(theta)*std::sin(phi)});}}}
  for(int inner:{0,1}){int off=inner*ns;for(int i=0;i<nt;++i){Json f={off,off+1+i,off+2+i};if(inner)std::reverse(f.begin(),f.end());faces.push_back(f);}for(int j=0;j<nr-1;++j)for(int i=0;i<nt;++i){Json f={off+1+j*(nt+1)+i,off+1+(j+1)*(nt+1)+i,off+2+(j+1)*(nt+1)+i,off+2+j*(nt+1)+i};if(inner)std::reverse(f.begin(),f.end());faces.push_back(f);}}
  reverse_faces(faces);std::vector<int> ring={0};for(int j=0;j<nr;++j)ring.push_back(1+j*(nt+1));for(int i=1;i<=nt;++i)ring.push_back(1+(nr-1)*(nt+1)+i);for(int j=nr-2;j>=0;--j)ring.push_back(1+j*(nt+1)+nt);
  for(size_t i=0;i<ring.size();++i){int a=ring[i],b=ring[(i+1)%ring.size()];faces.push_back({a,b,b+ns,a+ns});}reverse_faces(faces);
  mesh(id,owner,verts,faces,"iron",.0005,"CupHandle");
  for(int sx:{-1,1}){box(id+".ear."+integer(sx),owner,x+sx*.072-.011,x+sx*.072+.011,y-.003,y+.001,z-.006,z+.015,"iron",.003,"HandleMount");bolt(id+".bolt."+integer(sx),owner,x+sx*.073,y-.003,z+.004,.005);}
  auto&s=socket(id+".socket",owner,"HandleFront","mount.cup_handle.v1",{x,y,z},{{"support_part_ids",{owner+".front"}},{"footprint",{.18,.055}},{"max_projection",dep}});
  scene["grips"].push_back({{"id",id+".grip"},{"node",owner},{"socket_id",s["id"]},{"position",verts[0]},{"approach",{0,1,0}},{"basis",{{"x",{1,0,0}},{"y",{0,0,1}},{"z",{0,-1,0}}}},{"reach_radius",.15},{"enabled",true}});
  scene["metadata"]["grip_supports"][id+".grip"]=id;
 }
 void label(const std::string&id,const std::string&owner,double y,double z){
  box(id+".top",owner,-.066,.066,y-.003,y,z+.016,z+.022,"iron",.001,"LabelFrame");
  box(id+".bottom",owner,-.066,.066,y-.003,y,z-.022,z-.016,"iron",.001,"LabelFrame");
  box(id+".left",owner,-.072,-.066,y-.003,y,z-.022,z+.022,"iron",.001,"LabelFrame");
  box(id+".right",owner,.066,.072,y-.003,y,z-.022,z+.022,"iron",.001,"LabelFrame");
  box(id+".paper",owner,-.064,.064,y-.0007,y,z-.014,z+.014,"paper",.0003,"LabelInsert");
  for(double x:{-.069,.069})bolt(id+".bolt."+decimal(x),owner,x,y-.003,z,.0028);
 }
 void top_fixing(const std::string&id,double x,double y,double z){
  cylinder(id,"frame",{x,y,z-.001},{x,y,z+.0017},.005,"iron",12,.00035,"TopFixing");
  box(id+".slot","frame",x-.003,x+.003,y-.00065,y+.00065,z+.0017,z+.0019,"grime",0,"ScrewSlot");
 }
 void splinter(const std::string&id,double x,double y,double z){
  Json vs={{x,y+.007,z-.006},{x+.062,y-.006,z+.003},{x+.016,y+.008,z-.003}};for(size_t i=0;i<3;++i){V3 p=vs[i].get<V3>();p[2]+=.002;vs.push_back(p);}
  Json fs={{2,1,0},{3,4,5},{0,1,4,3},{1,2,5,4},{2,0,3,5}};if(signed_face_volume(vs,fs)<0)reverse_faces(fs);
  mesh(id,"frame",vs,fs,"wood_edge",.0002,"AttachedSplinter")["attachment"]="Embedded root, decorative authoring geometry; no detached fragment physics";
 }
 void drawer_box(const std::string&id,double cx,double front_y,double z,double width,double height,double depth,bool labels=true,bool open_it=false){
  const double travel=depth*.67,open_fraction=r["open_fraction"];V3 position={cx,open_it?front_y-travel*open_fraction:front_y,z};node(id,position,"Drawer");
  const double t=.017,front=.03,gap=.008,sideh=height-.032;
  board(id+".front",id,{-width/2,-front,0},{width/2,0,height},"DrawerFront","front",true);
  double boxw=width-.018,x0=-boxw/2,x1=boxw/2;
  board(id+".left",id,{x0,0,.008},{x0+t,depth,sideh},"DrawerLeft","side");
  board(id+".right",id,{x1-t,0,.008},{x1,depth,sideh},"DrawerRight","side",true);
  board(id+".back",id,{x0+t,depth-t,.008},{x1-t,depth,sideh},"DrawerBack");
  board(id+".bottom",id,{x0+t,0,.008},{x1-t,depth-t,.021},"DrawerBottom","top",true);
  for(int side:{-1,1})for(int j=0;j<3;++j)box(id+".peg."+integer(side)+"."+integer(j),id,side*(width/2-.011)-.004,side*(width/2-.011)+.004,-front-.0005,-front+.001,height*(.12+j*.30),height*(.12+j*.30)+std::min(.007,height*.05),"wood_edge",.0003,"JoineryPlug");
  double hz=height*.40;cup(id+".cup",id,0,-front,hz);
  if(labels){label(id+".label",id,-front,height*.75);double kx=width*.35;
   box(id+".keyplate",id,kx-.011,kx+.011,-front-.004,-front+.001,hz-.019,hz+.033,"iron",.003,"KeyEscutcheon");
   cylinder(id+".keyhole.round",id,{kx,-front-.0041,hz+.012},{kx,-front-.0045,hz+.012},.0045,"grime",12,0,"Keyhole");
   prism(id+".keyhole.slot",id,{{kx-.004,hz-.008},{kx+.004,hz-.008},{kx+.002,hz+.008},{kx-.002,hz+.008}},-front-.0045,-front-.0041,"grime",0,"Keyhole");
  }
  auto&guide=socket(id+".guide",id,"DrawerGuide","drawer.linear.v1",{0,depth/2,.025},{{"support_part_ids",{id+".left",id+".right"}},{"travel_m",travel},{"lateral_gap_m",gap},{"vertical_gap_m",.008},{"vertical_gap_scope","minimum nominal front-to-bay clearance"},{"runner_gap_m",.003}});
  guide.update(Json{{"axis",{0,-1,0}},{"limits_m",{0,travel}},{"closed_translation",{cx,front_y,z}}});
  socket(id+".mount","frame","DrawerMount","drawer.bay.v1",{cx,front_y,z},{{"support_part_ids",Json::array()},{"drawer_node",id},{"opening_m",{width+.016,height+.016,depth+.025}}});
  scene["metadata"]["drawers"].push_back({{"node",id},{"closed_origin",{cx,front_y,z}},{"travel_m",travel},{"outer_lo",{-width/2,-front-.04,0}},{"outer_hi",{width/2,depth,height}},{"interior_lo",{x0+t,0,.021}},{"interior_hi",{x1-t,depth-t,sideh}},{"clearance_m",.008}});
 }
};

void cabinet(FurnitureBuilder&b){
 const double w=b.r["width"],d=b.r["depth"],h=b.r["height"],s=.054,front=-d/2,back=d/2,leg=.10,low=.15,top=h-.057;const int n=b.r["drawer_count"];
 for(int sx:{-1,1}){
  for(int sy:{-1,1}){double x0=sx*(w/2-s/2)-s/2,y0=sy*(d/2-s/2)-s/2;b.fixed_board("post."+integer(sx)+"."+integer(sy),{x0,y0,0},{x0+s,y0+s,h-.045},"Post","front",sx==1);}
  double length=d-2*s;int count=3;for(int j=0;j<count;++j){double y0=front+s+j*length/count+.002,x0=sx<0?-w/2+.010:w/2-.029;b.fixed_board("side."+integer(sx)+"."+integer(j),{x0,y0,leg},{x0+.019,y0+length/count-.004,h-.05},"SidePlank","side",j==1);}
 }
 for(int sx:{-1,1}){int k=0;for(double z:{leg+.025,h-.14}){double x0=sx<0?-w/2-.005:w/2-.011;b.fixed_board("side.rail."+integer(sx)+"."+integer(k),{x0,front+s,z},{x0+.016,back-s,z+.042},"SideRail","side");for(double yy:{front+s+.025,back-s-.025})b.cylinder("side.pin."+integer(sx)+"."+integer(k)+"."+decimal(yy),"frame",{sx*(w/2-.003),yy,z+.021},{sx*(w/2+.007),yy,z+.021},.0045,"iron",10,.0003,"SideFixing");++k;}}
 for(int j=0;j<5;++j){double x0=-w/2+s+j*(w-2*s)/5+.001;b.fixed_board("back."+integer(j),{x0,back-.025,leg},{x0+(w-2*s)/5-.002,back-.01,h-.05},"BackPlank","front",j==3);}
 for(double z:{leg+.08,h*.54,h-.13})b.fixed_board("backbrace."+decimal(z),{-w/2+s,back-.008,z},{w/2-s,back+.018,z+.047},"BackBrace");
 for(int j=0;j<4;++j){double y0=front-.018+j*(d+.036)/4+.0025;b.fixed_board("cap."+integer(j),{-w/2-.02+(j%2)*.002,y0,h-.052},{w/2+.02-(j%3)*.0015,y0+(d+.036)/4-.005,h-(j%2)*.0008},"TopPlank","top",j==0||j==2);}
 b.splinter("cap.splinter",w*.20,front-.0155,h);
 for(int j=0;j<4;++j){double yy=front-.018+(j+.5)*(d+.036)/4;for(int sx:{-1,1})b.top_fixing("cap.fix."+integer(j)+"."+integer(sx),sx*(w/2-.045),yy,h-(j%2)*.0008);}
 double bay=(top-low)/n,dw=w-2*s-.016,dd=d-.105;
 for(int i=0;i<n+1;++i){double z=low+i*bay-.014;b.fixed_board("rail.front."+integer(i),{-w/2+s,front,z},{w/2-s,front+.034,z+.019},"FrontRail");}
 for(int i=0;i<n;++i){double z=low+i*bay+.013,dh=bay-.039;for(int sx:{-1,1}){double x=sx*(dw/2-.015);b.fixed_board("runner."+integer(i)+"."+integer(sx),{x-.016,front+.031,z-.012},{x+.016,back-.044,z+.005},"DrawerRunner");}b.drawer_box("drawer."+integer(i),0,front+.004,z,dw,dh,dd,true,i==std::max(0,n-2));}
}

void bench(FurnitureBuilder&b){
 const double w=b.r["width"],d=b.r["depth"],h=b.r["height"],s=.086,front=-d/2,back=d/2,top=h-.065,legx=w/2-.095,legy=d/2-.072,shelfz=.22;
 for(int sx:{-1,1})for(int sy:{-1,1}){
  double x=sx*legx,y=sy*legy;b.fixed_board("leg."+integer(sx)+"."+integer(sy),{x-s/2,y-s/2,0},{x+s/2,y+s/2,top},"Leg","front",sx==1);
  for(const auto&z_and_h:std::array<V2,2>{{{0,.048},{top-.125,.105}}}){double zz=z_and_h[0],hh=z_and_h[1];std::string zz_id=zz==0?"0":decimal(zz);
   b.box("strap."+integer(sx)+"."+integer(sy)+"."+zz_id,"frame",x-s/2-.002,x+s/2+.002,y-s/2-.004,y-s/2,zz,zz+hh,"iron",.002,"CornerStrap");for(double z:{zz+.015,zz+hh-.015})b.bolt("strapbolt."+integer(sx)+"."+integer(sy)+"."+decimal(z),"frame",x,y-s/2-.004,z,.006);
  }
 }
 for(int j=0;j<5;++j){double y0=front-.018+j*(d+.036)/5+.003;b.fixed_board("top."+integer(j),{-w/2+(j%2)*.003,y0,top},{w/2-(j%3)*.002,y0+(d+.036)/5-.006,h-(j%2)*.001},"TopPlank","top",j==0||j==3);}
 b.splinter("top.splinter",w*.27,front-.015,h);
 for(int j=0;j<5;++j){double yy=front-.018+(j+.5)*(d+.036)/5;for(int sx:{-1,1})b.top_fixing("top.fix."+integer(j)+"."+integer(sx),sx*(w/2-.08),yy,h-(j%2)*.001);}
 b.fixed_board("apron.back",{-legx,back-.086,top-.15},{legx,back-.05,top},"Apron");for(int sx:{-1,1}){double x=sx*legx;b.fixed_board("apron.side."+integer(sx),{x-.023,-legy,top-.15},{x+.023,legy,top},"Apron","side");}
 double drawerw=std::min(.46,w*.31),cx=legx-.071-drawerw/2,dh=.135,z=top-.166,dd=d-.18;
 if(b.r["drawer"]){
  b.fixed_board("apron.front.left",{-legx,front+.015,top-.151},{cx-drawerw/2-.012,front+.051,top},"Apron","front",true);
  b.fixed_board("apron.front.header",{cx-drawerw/2-.008,front+.015,top-.016},{legx,front+.051,top},"Apron");
  for(double x:{cx-drawerw/2-.025,cx+drawerw/2+.025}){b.fixed_board("drawer.support."+decimal(x),{x-.01,front+.022,z-.024},{x+.01,front+dd+.01,top-.019},"DrawerSupport");double rx=cx+(x<cx?-1:1)*drawerw/2;b.fixed_board("drawer.runner."+decimal(x),{rx-.028,front+.024,z-.012},{rx+.028,front+dd+.01,z+.005},"DrawerRunner");}
  b.drawer_box("drawer.0",cx,front+.012,z,drawerw,dh,dd,false,true);
 }else b.fixed_board("apron.front",{-legx,front+.015,top-.151},{legx,front+.051,top},"Apron");
 for(int sy:{-1,1}){double y=sy*legy;b.fixed_board("shelf.rail."+integer(sy),{-legx,y-.023,shelfz-.052},{legx,y+.023,shelfz-.019},"ShelfSupport");}
 for(int sx:{-1,1}){double x=sx*(legx-.030);b.fixed_board("shelf.cross."+integer(sx),{x-.026,-legy,shelfz-.052},{x+.026,legy,shelfz-.019},"ShelfSupport");}
 if(b.r["shelf"]){int count=4;double dep=2*legy-.03;for(int j=0;j<count;++j){double y0=-dep/2+j*dep/count+.0025;b.fixed_board("shelf."+integer(j),{-legx+.043,y0,shelfz-.019},{legx-.043,y0+dep/count-.005,shelfz+.011},"ShelfPlank","top",j==0);}}
 for(int sx:{-1,1}){double x=sx*(legx+.045);V3 a={x,-legy+.043,shelfz+.05},bb={x,legy-.043,top-.18};double length=std::hypot(bb[0]-a[0],bb[1]-a[1],bb[2]-a[2]);V3 direction;for(size_t i=0;i<3;++i)direction[i]=(bb[i]-a[i])/length;V3 perp={0,-direction[2]*.013,direction[1]*.013};Json vs=Json::array();for(double dx:{-.0025,.0025}){for(int i=0;i<4;++i){const V3&p=(i==1||i==2)?bb:a;int sg=i<2?-1:1;vs.push_back({p[0]+dx,p[1]+sg*perp[1],p[2]+sg*perp[2]});}}
  Json fs={{3,2,1,0},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}};if(signed_face_volume(vs,fs)<0)reverse_faces(fs);b.mesh("diagonal."+integer(sx),"frame",vs,fs,"iron",.001,"DiagonalBrace");
 }
}
} // namespace

Json normalize_furniture_recipe(const Json&recipe){
 if(!recipe.is_object())throw std::invalid_argument("Recipe must be an object");
 Json r={{"preset","cabinet"},{"width",.76},{"depth",.52},{"height",1.32},{"drawer_count",4},{"drawer",true},{"shelf",true},{"seed",41},{"open_fraction",0.},{"wear",.7}};
 if(recipe.value("preset",Json())=="workbench")r.update(Json{{"width",1.65},{"depth",.68},{"height",.91},{"drawer_count",1}});
 for(auto it=recipe.begin();it!=recipe.end();++it){
  if(!r.contains(it.key()))throw std::invalid_argument("Unknown field: "+it.key());
 }
 r.update(recipe);
 if(r["preset"]!="cabinet"&&r["preset"]!="workbench")throw std::invalid_argument("Unknown preset");
 struct Range{const char*key;double lo,hi;};
 for(const auto&range:std::array<Range,5>{{{"width",.55,2.4},{"depth",.4,.95},{"height",.65,2.1},{"open_fraction",0,1},{"wear",0,1}}}){const auto&v=r[range.key];if(!v.is_number())throw std::invalid_argument(std::string(range.key)+" must be numeric");double x=v.get<double>();if(!std::isfinite(x)||x<range.lo||x>range.hi)throw std::invalid_argument(std::string(range.key)+" outside supported range");r[range.key]=x;}
 for(const std::string k:{"drawer_count","seed"})if(!r[k].is_number_integer())throw std::invalid_argument(k+" must be integer");
 recipe_seed(r["seed"]);
 if(r["drawer_count"]<1||r["drawer_count"]>8)throw std::invalid_argument("drawer_count must be 1-8");
 for(const std::string k:{"drawer","shelf"})if(!r[k].is_boolean())throw std::invalid_argument(k+" must be boolean");
 if(r["preset"]=="cabinet"&&(r["height"].get<double>()-.22)/r["drawer_count"].get<int>()<.15)throw std::invalid_argument("Drawer bays must be at least 150 mm");
 return r;
}

Json build_furniture(const Json&recipe){FurnitureBuilder b(normalize_furniture_recipe(recipe));if(b.r["preset"]=="cabinet")cabinet(b);else bench(b);b.scene["metadata"]["engine_ownership"]="Engine owns mechanisms, constraints, colliders, mass, damage response and interaction runtime.";return b.scene;}

Json sweep_furniture_clearance(const Json&scene){
 Json collisions=Json::array();size_t pairs=0;double minimum=std::numeric_limits<double>::infinity();
 for(const auto&dr:scene.at("metadata").at("drawers")){
  auto o=dr.at("closed_origin").get<V3>();double travel=dr.at("travel_m");
  for(const auto&part:scene.at("parts"))if(part.at("node")==dr.at("node")){
   V3 lo={INFINITY,INFINITY,INFINITY},hi={-INFINITY,-INFINITY,-INFINITY};
   for(const auto&v:part.at("vertices")){
    for(size_t k=0;k<3;++k){lo[k]=std::min(lo[k],v[k].get<double>());hi[k]=std::max(hi[k],v[k].get<double>());}
   }
   for(size_t k=0;k<3;++k){lo[k]+=o[k];hi[k]+=o[k];}
   lo[1]-=travel;
   for(const auto&fixed:scene.at("metadata").at("static_envelopes")){
    ++pairs;bool overlap[3];for(size_t k=0;k<3;++k)overlap[k]=std::min(hi[k],fixed.at("hi")[k].get<double>())-std::max(lo[k],fixed.at("lo")[k].get<double>())>1e-7;
    if(overlap[0]&&overlap[1]&&overlap[2])collisions.push_back({{"drawer",dr["node"]},{"moving",part["id"]},{"part",fixed["id"]},{"interval_m",{0,travel}}});
    const std::string role=part.at("role"),id=fixed.at("id");if(id.find("runner")!=std::string::npos&&(role=="DrawerLeft"||role=="DrawerRight"||role=="DrawerBottom")&&overlap[0]&&overlap[1]){double gap=lo[2]-fixed.at("hi")[2].get<double>();if(gap>=0)minimum=std::min(minimum,gap);}
   }
  }
 }
 return Json{{"passed",collisions.empty()},{"collisions",collisions},{"samples",pairs},{"method","Continuous full-travel swept AABB of every rigid drawer part against every fixed structural envelope; conservative for curved parts; includes runners"},{"runner_gap_m",std::isfinite(minimum)?Json(minimum):Json(nullptr)}};
}

Json validate_furniture(const Json&scene){
 Json errors=validate_scene(scene);
 for(const auto&p:scene.at("parts"))if(p.contains("face_materials")&&!p["face_materials"].empty()&&p["face_materials"].size()!=p["faces"].size())errors.push_back(p["id"].get<std::string>()+": material assignment count");
 for(const auto&dr:scene.at("metadata").at("drawers"))for(size_t k=0;k<3;++k)if(dr["interior_lo"][k].get<double>()>=dr["interior_hi"][k].get<double>()){errors.push_back(dr["node"].get<std::string>()+": invalid interior");break;}
 auto sweep=sweep_furniture_clearance(scene);if(!sweep["passed"].get<bool>())errors.push_back("Drawer swept clearance collision: "+sweep["collisions"].dump());return errors;
}
} // namespace jerico
