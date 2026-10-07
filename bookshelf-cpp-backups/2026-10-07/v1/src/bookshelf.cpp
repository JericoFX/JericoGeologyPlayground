#include "jerico/bookshelf.hpp"
#include "jerico/dock_geometry.hpp"
#include "jerico/dock_deform.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
namespace jerico {
namespace {
constexpr double pi=3.14159265358979323846;
double random01(int64_t seed,const std::string& id,int salt=0){uint64_t h=1469598103934665603ULL^uint64_t(seed)^uint64_t(salt)*0x9e3779b97f4a7c15ULL;for(unsigned char c:id){h^=c;h*=1099511628211ULL;}h^=h>>30;h*=0xbf58476d1ce4e5b9ULL;h^=h>>27;h*=0x94d049bb133111ebULL;h^=h>>31;return double(h>>11)/9007199254740992.;}
struct Pose{V3 origin{},x{1,0,0},y{0,1,0},z{0,0,1};V3 point(V3 p)const{return add(origin,add(mul(x,p[0]),add(mul(y,p[1]),mul(z,p[2]))));}Json frame()const{return {{"origin",origin},{"u",x},{"v",y},{"w",z}};}};
void need(bool value,const std::string& why){if(!value)throw std::invalid_argument("Bookshelf: "+why);}
void item_tags(Json& p,const std::string& item,const Pose& pose,int family){p["item_id"]=item;p["book_family"]=family;p["book_frame"]=pose.frame();}
Json& ring_solid(Builder& b,const std::string&id,const Pose& pose,const std::vector<std::vector<V2>>& rings,const std::vector<double>& zs,const std::string&material,const std::string&role){
 Json vs=Json::array(),fs=Json::array();size_t n=rings[0].size();for(size_t j=0;j<rings.size();++j)for(auto xy:rings[j])vs.push_back(pose.point({xy[0],xy[1],zs[j]}));
 for(size_t j=0;j+1<rings.size();++j)for(size_t k=0;k<n;++k){size_t a=j*n+k,bb=j*n+(k+1)%n,c=(j+1)*n+(k+1)%n,d=(j+1)*n+k;fs.push_back({a,bb,c});fs.push_back({a,c,d});}
 for(size_t j:{size_t(0),rings.size()-1}){V3 c{};for(size_t k=0;k<n;++k)c=add(c,vs[j*n+k].get<V3>());c=mul(c,1./n);size_t ci=vs.size();vs.push_back(c);for(size_t k=0;k<n;++k){if(j==0)fs.push_back({ci,(k+1)%n,k});else fs.push_back({ci,j*n+k,j*n+(k+1)%n});}}
 return b.mesh(id,"frame",vs,fs,material,0,role);
}
std::vector<V2> rectangle(double x0,double x1,double y0,double y1,double c){return {{x0+c,y0},{x1-c,y0},{x1,y0+c},{x1,y1-c},{x1-c,y1},{x0+c,y1},{x0,y1-c},{x0,y0+c}};}
Json& posed_box(Builder&b,const std::string&id,const Pose&p,V3 lo,V3 hi,const std::string&mat,double bevel,const std::string&role){auto&q=b.box(id,"frame",lo[0],hi[0],lo[1],hi[1],lo[2],hi[2],mat,bevel,role);for(auto&v:q["vertices"])v=p.point(v.get<V3>());q["uvs"]=metric_uvs(q["vertices"],q["faces"]);return q;}
Json& wood(Builder&b,const std::string&id,V3 a,V3 c,V3 v,double width,double thick,const std::string&role,int64_t seed,double wear,bool flat=false){
 auto&p=dock_timber(b,id,a,c,v,width,thick,role,seed,false,false,wear,flat);
 const double length=p["timber_length"];auto u=norm(sub(c,a));int n=11;int seg=int(p["timber_profile"].size())-1;
 // Exact end planes preserve contact with the plinth/cap and declared tenons.
 for(int end:{0,seg}){for(int k=0;k<n;++k){auto q=p["vertices"][end*n+k].get<V3>();p["vertices"][end*n+k]=add(q,mul(u,(end?length:0)-dot(sub(q,a),u)));}size_t ci=p["vertices"].size()-2+(end?1:0);auto q=p["vertices"][ci].get<V3>();p["vertices"][ci]=add(q,mul(u,(end?length:0)-dot(sub(q,a),u)));}
 p["uvs"]=metric_uvs(p["vertices"],p["faces"]);p["wood_finish"]={{"age",.85},{"wetness",.34},{"grain_axis","Z"}};
 double band=std::min(width*.30,.025),amount=std::min({thick*.045,width*.025,band*.12})*wear;
 p=deform_timber_edges(p,{{"seed",seed},{"amount_m",amount},{"edge_band_m",band},{"start_protection_m",std::min(.08,length*.22)},{"end_protection_m",std::min(.08,length*.22)}});
 // Exposed chamfers are real surface groups, never assigned cut_metal.
 Json mats=Json::array();for(size_t i=0;i<p["faces"].size();++i){size_t edge=(i/2)%11;bool exposed=i<size_t(seg*22)&&(edge==0||edge==2||edge==4||edge==9);mats.push_back(exposed?"wood_exposed":"wood");}p["face_materials"]=mats;return p;
}
void fastener(Builder&b,const std::string&id,double x,double y,double z){auto&p=b.cylinder(id,"frame",{x,y+.007,z},{x,y-.002,z},.0045,"iron",10,.00045,"ForgedNail");p["protected_support"]=true;}
Json obb(Pose pose,double t,double d,double h){return {{"center",pose.point({0,0,h*.5})},{"axes",{pose.x,pose.y,pose.z}},{"half",{t*.5,d*.5,h*.5}}};}
std::array<V3,8> corners(const Json&o){auto c=o["center"].get<V3>();auto axes=o["axes"].get<std::array<V3,3>>();auto h=o["half"].get<V3>();std::array<V3,8> out;int k=0;for(int a:{-1,1})for(int b:{-1,1})for(int d:{-1,1})out[k++]=add(c,add(mul(axes[0],a*h[0]),add(mul(axes[1],b*h[1]),mul(axes[2],d*h[2]))));return out;}
V3 item_min(const Json&o){V3 p{1e9,1e9,1e9};for(auto q:corners(o))for(int k=0;k<3;++k)p[k]=std::min(p[k],q[k]);return p;}
V3 item_max(const Json&o){V3 p{-1e9,-1e9,-1e9};for(auto q:corners(o))for(int k=0;k<3;++k)p[k]=std::max(p[k],q[k]);return p;}
void register_item(Builder&b,const std::string&id,const std::string&type,Pose pose,double t,double d,double h,int bay,const std::string&support,const std::string&mode,const std::string&lean=""){
 Json item={{"id",id},{"type",type},{"compartment",bay},{"support_id",support},{"pose_mode",mode},{"obb",obb(pose,t,d,h)},{"dimensions_m",{t,d,h}},{"grab",{{"enabled",true},{"id",id+".grab"},{"center",pose.point({0,0,h*.5})},{"approach",mul(pose.y,-1)},{"group_part_ids",Json::array()},{"physics_implemented",false}}}};
 if(!lean.empty())item["lean_support_id"]=lean;
 for(auto&p:b.scene["parts"])if(p.value("item_id","")==id)item["grab"]["group_part_ids"].push_back(p["id"]);
 b.scene["metadata"]["items"].push_back(item);
}
void book(Builder&b,const std::string&id,Pose pose,double t,double d,double h,int bay,const std::string&support,const std::string&mode,int64_t seed,bool binder=false,const std::string&lean=""){
 int family=int(random01(seed,id,99)*6);double cover=.0025;double age=random01(seed,id,4);const double front=-d*.5,rear=d*.5;
 for(int side:{-1,1}){std::vector<std::vector<V2>> rings;std::vector<double> zs;int count=10;for(int k=0;k<=count;++k){double f=double(k)/count;double inset=.0006*age*std::pow(std::sin(f*pi),2);double warp=.0007*std::sin(f*5.4+random01(seed,id,7)*6)*std::sin(f*pi);double outer=side*(t*.5-inset);double inner=outer-side*cover;double x0=std::min(inner,outer),x1=std::max(inner,outer);double y0=front+.001+std::abs(warp),y1=rear-.0009*age*std::pow(std::sin(f*11),2);rings.push_back(rectangle(x0,x1,y0,y1,.00045));zs.push_back(h*f);}auto&p=ring_solid(b,id+(side<0?".cover.left":".cover.right"),pose,rings,zs,"book_cover","BookCover");item_tags(p,id,pose,family);p["cover_warp_m_max"]=.0013;}
 std::vector<std::vector<V2>> pageRings;std::vector<double> pageZ;const int layers=32;for(int k=0;k<=layers;++k){double f=double(k)/layers;double ripple=.0009*(.5+.5*std::sin(k*2.49+age*5));double x=t*.5-cover-.0015;pageRings.push_back(rectangle(-x,x,front+.010,rear-.006-ripple,.0008));pageZ.push_back(.005+(h-.010)*f);}auto&pages=ring_solid(b,id+".pages",pose,pageRings,pageZ,"book_pages","PageBlock");item_tags(pages,id,pose,family);pages["page_layers_geometry"]=layers;pages["individual_pages_simulated"]=false;
 auto spineProfile=[&](double bulge){std::vector<V2> p{{-t*.5+.0002,front+.008}};for(int k=0;k<=10;++k){double x=(-.5+double(k)/10)*t;p.push_back({x,front+.0002+bulge*(1-std::sqrt(std::max(0.,1-4*x*x/(t*t))))});}p.push_back({t*.5-.0002,front+.008});return p;};
 std::vector<std::vector<V2>> sr;std::vector<double> sz;for(int k=0;k<=12;++k){double f=double(k)/12;sr.push_back(spineProfile(.004+.0015*std::sin(f*pi)));sz.push_back(f*h);}auto&spine=ring_solid(b,id+".spine",pose,sr,sz,"book_cover","RoundedSpine");item_tags(spine,id,pose,family);
 // Raised binding cords and narrow gilt borders lie on the actual convex spine.
 for(int n=0;n<(binder?2:4);++n){double z=h*(binder?(.15+n*.70):(.11+n*.255));std::vector<std::vector<V2>> rr{spineProfile(.004),spineProfile(.004)};for(auto&r:rr)for(auto&xy:r){xy[0]*=.94;xy[1]-=.0006;}auto&p=ring_solid(b,id+".binding."+std::to_string(n),pose,rr,{z,z+.004},"book_cover","SpineCord");item_tags(p,id,pose,family);
 auto&line=posed_box(b,id+".gilt."+std::to_string(n),pose,{-t*.32,front-.0002,z+.006},{t*.32,front+.0003,z+.0068},"book_gilt",0,"GiltLine");item_tags(line,id,pose,family);}
 if(binder||random01(seed,id,5)>.55){double hh=h*(binder?.23:.12),zc=h*(binder?.61:.70);auto&label=posed_box(b,id+".label",pose,{-t*.31,front-.00045,zc-hh*.5},{t*.31,front+.0002,zc+hh*.5},"book_label",.00013,"ArchiveLabel");item_tags(label,id,pose,family);}
 if(binder){auto&ring=b.cylinder(id+".binder.ring","frame",pose.point({0,front+.0002,h*.17}),pose.point({0,front-.0007,h*.17}),t*.115,"iron",14,0,"BinderFingerRing");item_tags(ring,id,pose,family);auto&hole=b.cylinder(id+".binder.inset","frame",pose.point({0,front-.0007,h*.17}),pose.point({0,front-.00085,h*.17}),t*.078,"book_cover",14,0,"BinderFingerRecess");item_tags(hole,id,pose,5);}
 register_item(b,id,binder?"archive_binder":"book",pose,t,d,h,bay,support,mode,lean);
}
void archive_box(Builder&b,const std::string&id,double x,double z,double w,double d,double h,int bay,const std::string&support,int64_t seed,bool crate){
 Pose pose;pose.origin={x,-.002,z};double f=-d*.5;
 if(crate){double th=.012;for(int side:{-1,1})for(int row=0;row<3;++row){double zz=z+(row+.5)*h/3;auto&p=wood(b,id+".board."+std::to_string(side)+"."+std::to_string(row),{x-w*.5,side*d*.5,zz},{x+w*.5,side*d*.5,zz},{0,0,1},h/3-.001,th,"CrateSlat",seed,.65);p["item_id"]=id;}
 for(int side:{-1,1}){auto&p=wood(b,id+".end."+std::to_string(side),{x+side*(w*.5-th*.5),-d*.5,z+h*.5},{x+side*(w*.5-th*.5),d*.5,z+h*.5},{0,0,1},h,th,"CrateEnd",seed,.5,true);p["item_id"]=id;}auto&bottom=posed_box(b,id+".bottom",pose,{-w*.5,-d*.5,0},{w*.5,d*.5,.012},"wood",.001,"CrateBase");bottom["item_id"]=id;bottom["timber_frame"]={{"origin",V3{x-w*.5,-d*.5,z}},{"u",V3{1,0,0}},{"v",V3{0,1,0}},{"w",V3{0,0,1}}};
 // A dark recessed finger plate, bounded within the front slat.
 auto&handle=posed_box(b,id+".finger.plate",pose,{-w*.18,f-.007,h*.66},{w*.18,f-.005,h*.72},"iron",.006,"CrateFingerPlate");handle["item_id"]=id;
 }else{double split=h*.77;auto&body=posed_box(b,id+".body",pose,{-w*.5,-d*.5,0},{w*.5,d*.5,split},"iron",.007,"ArchiveBoxBody");body["item_id"]=id;auto&lid=posed_box(b,id+".lid",pose,{-w*.5,-d*.5,split+.001},{w*.5,d*.5,h},"iron",.004,"ArchiveBoxLid");lid["item_id"]=id;
 for(int side:{-1,1}){auto&band=posed_box(b,id+".strap."+std::to_string(side),pose,{side*w*.35-.010,f-.001,.01},{side*w*.35+.010,f+.001,h-.004},"iron",.0006,"BoxStrap");band["item_id"]=id;}
 auto&latch=posed_box(b,id+".latch",pose,{-w*.055,f-.004,split-.023},{w*.055,f-.001,split+.012},"iron",.001,"BoxLatch");latch["item_id"]=id;for(int side:{-1,1}){auto&p=b.cylinder(id+".handle.side."+std::to_string(side),"frame",pose.point({side*w*.065,f-.004,split-.030}),pose.point({side*w*.065,f-.010,split-.055}),.0024,"iron",8,0,"BoxHandle");p["item_id"]=id;}auto&p=b.cylinder(id+".handle.bar","frame",pose.point({-w*.065,f-.010,split-.055}),pose.point({w*.065,f-.010,split-.055}),.0024,"iron",8,0,"BoxHandle");p["item_id"]=id;
 }
 if(crate)pose.origin[1]=0;
 register_item(b,id,crate?"wood_crate":"metal_archive_box",pose,w,d+(crate?.020:.030),h,bay,support,"upright");
}
bool penetrates(const Json&a,const Json&b,double tolerance=.0001){auto ca=a["center"].get<V3>(),cb=b["center"].get<V3>(),ha=a["half"].get<V3>(),hb=b["half"].get<V3>();auto aa=a["axes"].get<std::array<V3,3>>(),ab=b["axes"].get<std::array<V3,3>>();std::vector<V3>axes;for(auto v:aa)axes.push_back(v);for(auto v:ab)axes.push_back(v);for(auto u:aa)for(auto v:ab){auto q=cross(u,v);if(dot(q,q)>1e-12)axes.push_back(norm(q));}for(auto axis:axes){double ra=0,rb=0;for(int k=0;k<3;++k){ra+=std::abs(dot(aa[k],axis))*ha[k];rb+=std::abs(dot(ab[k],axis))*hb[k];}if(std::abs(dot(sub(cb,ca),axis))>=ra+rb-tolerance)return false;}return true;}
}
Json default_bookshelf_recipe(){return {{"version",1},{"seed",417},{"width",1.35},{"height",2.10},{"depth",.39},{"compartments",4},{"shelf_thickness",.035},{"post_width",.065},{"wear",.72},{"fill",.82},{"book_scale",1.0}};}
Json build_bookshelf(const Json&input){
 auto r=default_bookshelf_recipe();r.merge_patch(input);int64_t seed=recipe_seed(r["seed"]);auto scalar=[&](const char*k,double lo,double hi){need(r.contains(k)&&r[k].is_number(),std::string(k)+" must be numeric");double q=r[k];need(std::isfinite(q)&&q>=lo&&q<=hi,std::string(k)+" outside supported range");return q;};
 double W=scalar("width",.90,2.1),H=scalar("height",1.45,2.7),D=scalar("depth",.30,.55),T=scalar("shelf_thickness",.025,.055),P=scalar("post_width",.05,.095),wear=scalar("wear",0,1),fill=scalar("fill",0,1),scale=scalar("book_scale",.7,1.15);double rows=scalar("compartments",3,6);need(rows==std::floor(rows),"compartments must be integral");int bays=int(rows);double base=.11,cap=H-.050,pitch=(cap-base)/bays,clear=pitch-T;need(clear>=.24,"compartments too short for books");
 Builder b(Json{{"seed",seed}});b.node("frame",{0,0,0},"Bookshelf",nullptr);b.scene["recipe"]=r;b.scene["metadata"]["family"]="bookshelf";b.scene["metadata"]["generator"]="JericoBookshelfCppLab C++17";b.scene["metadata"]["items"]=Json::array();b.scene["metadata"]["compartments"]=Json::array();b.scene["metadata"]["joints"]=Json::array();b.scene["metadata"]["deferred_accessories"]={"mug","lantern","bottle"};b.scene["metadata"]["physics"]="Separate stable item IDs and grab groups only; no engine physics or UI";
 double inner=W-2*P,xedge=inner*.5,postX=(W-P)*.5,postY=(D-P)*.5,shelfDepth=D-2*P+.012;
 for(int sx:{-1,1})for(int sy:{-1,1}){std::string id="frame.post."+std::to_string(sx)+"."+std::to_string(sy);auto&p=wood(b,id,{sx*postX,sy*postY,.030},{sx*postX,sy*postY,cap},{-1,0,0},P,P,"Stile",seed,wear);p["protected_support"]=true;auto&foot=wood(b,id+".foot",{sx*postX,sy*postY,0},{sx*postX,sy*postY,.10},{-1,0,0},P+.023,P+.023,"Foot",seed,wear*.7);foot["bearing_part_ids"]={id};}
 int backCount=int(std::ceil(inner/.115));double bw=inner/backCount;for(int k=0;k<backCount;++k){double x=-xedge+(k+.5)*bw;wood(b,"back.plank."+std::to_string(k),{x,D*.5-.011,base-T},{x,D*.5-.011,cap},{-1,0,0},bw-.0014,.022,"BackPlank",seed,wear);}
 for(int row=0;row<=bays;++row){double z=base+row*pitch;std::string shelf="shelf."+std::to_string(row);auto&p=wood(b,shelf,{-xedge-.008,0,z-T*.5},{xedge+.008,0,z-T*.5},{0,1,0},shelfDepth,T,"Shelf",seed,wear,true);p["bearing_part_ids"]={shelf+".cleat.L",shelf+".cleat.R"};
 for(int side:{-1,1}){auto id=shelf+".cleat."+(side<0?"L":"R");wood(b,id,{side*(xedge-.012),-postY,z-T-.015},{side*(xedge-.012),postY,z-T-.015},{1,0,0},.035,.030,"ShelfCleat",seed,wear*.45,true);b.scene["metadata"]["joints"].push_back({{"supported",shelf},{"support",id},{"type","supported_shelf_with_8mm_end_tenon"},{"intentional_embed_m",.008}});fastener(b,shelf+".nail."+std::to_string(side),side*postX,-postY-P*.5,z-T*.5);}
 if(row<bays)b.scene["metadata"]["compartments"].push_back({{"index",row},{"shelf_id",shelf},{"bottom",z},{"top",z+pitch-T},{"x_min",-xedge+.015},{"x_max",xedge-.015},{"y_min",-shelfDepth*.5+.003},{"y_max",shelfDepth*.5-.003}});
 }
 wood(b,"frame.cornice",{-W*.5-.028,0,H-.025},{W*.5+.028,0,H-.025},{0,1,0},D+.026,.050,"Cornice",seed,wear,true);
 wood(b,"frame.lower_apron",{-W*.5, -postY,.060},{W*.5,-postY,.060},{0,0,1},.055,.035,"LowerApron",seed,wear);
 for(int k=0;k<3;++k){double z=base+(cap-base)*(k+.35)/3;auto&p=wood(b,"back.crossbar."+std::to_string(k),{-W*.5+.018,D*.5+.010,z},{W*.5-.018,D*.5+.010,z},{0,0,1},.055,.022,"BackBrace",seed,wear*.6);p["joinery"]="Naillocked crossbar across all backing planks";for(int side:{-1,1})fastener(b,"back.nail."+std::to_string(k)+"."+std::to_string(side),side*postX,D*.5+.023,z);}
 auto&items=b.scene["metadata"]["items"];double L=-xedge+.027,R=xedge-.027,usable=R-L;double depth=std::min(.267,shelfDepth-.020),height=std::min({.365*scale,clear*.86*scale,clear-.016});
 auto upright=[&](int row,double left,double width,int count,bool binders,bool leaning){double z=base+row*pitch;std::string sup="shelf."+std::to_string(row),prev;double cursor=left,prevH=0;count=std::max(0,int(std::round(count*fill/.82)));double budget=width-(leaning&&count?height*.25:0);double unit=budget/std::max(1,count);for(int i=0;i<count;++i){auto id="item.r"+std::to_string(row)+".upright."+std::to_string(i);double t=std::min(.064*scale,unit*.88)*(.78+.22*random01(seed,id,0)),h=height*(.82+.18*random01(seed,id,1)),d=depth*(.82+.16*random01(seed,id,2));if(i==count-1&&leaning)h=height;Pose p;p.origin={cursor+t*.5,-.006,z};book(b,id,p,t,d,h,row,sup,"upright",seed,binders&&i%3==1);cursor+=t+.003;prev=id;prevH=h;}
 if(leaning&&count){auto id="item.r"+std::to_string(row)+".lean";double theta=-.20,t=.040*scale,h=prevH*.86,d=depth*.88;Pose p;p.x={std::cos(theta),0,-std::sin(theta)};p.z={std::sin(theta),0,std::cos(theta)};double touch=cursor-.003;p.origin={touch-std::sin(theta)*h+std::cos(theta)*t*.5,-.006,z+std::abs(std::sin(theta))*t*.5};if(item_max(obb(p,t,d,h))[0]<left+width)book(b,id,p,t,d,h,row,sup,"lean",seed,false,prev);}
 };
 auto stack=[&](int row,double x,double width,int count,double basez,const std::string&support,const std::string&name){double z=basez;std::string prev=support;count=std::max(0,int(std::round(count*fill/.82)));for(int i=0;i<count;++i){auto id="item.r"+std::to_string(row)+"."+name+"."+std::to_string(i);double h=std::min(.30*scale,width)*std::pow(.945,i),d=depth*.93*std::pow(.966,i),t=(.030+.011*random01(seed,id,0))*scale;Pose p;p.x={0,0,1};p.y={0,1,0};p.z={-1,0,0};p.origin={x+h*.5,-.006,z+t*.5};book(b,id,p,t,d,h,row,prev,"stack",seed);prev=id;z+=t;}}
 ;
 if(fill>0)for(int row=0;row<bays;++row){double z=base+row*pitch;std::string shelf="shelf."+std::to_string(row);if(row==0){double cw=usable*.30,ch=std::min(.255,clear*.65);archive_box(b,"item.r0.crate",L+cw*.5,z,cw,depth*.93,ch,row,shelf,seed,true);stack(row,L+usable*.46,usable*.25,3,z,shelf,"stack");upright(row,L+usable*.64,usable*.36,4,false,false);}
 else if(row%3==1){upright(row,L,usable*.66,7,true,true);double w=usable*.27,x=R-w*.5;archive_box(b,"item.r"+std::to_string(row)+".box",x,z,w,depth*.90,clear*.36,row,shelf,seed,false);stack(row,x,w*.92,1,z+clear*.36,"item.r"+std::to_string(row)+".box","boxstack");}
 else if(row%3==2){stack(row,L+usable*.18,usable*.33,4,z,shelf,"stack");upright(row,L+usable*.43,usable*.57,6,false,true);}
 else{upright(row,L,usable*.70,8,false,true);double w=usable*.24;archive_box(b,"item.r"+std::to_string(row)+".box",R-w*.5,z,w,depth*.88,clear*.40,row,shelf,seed,false);}}
 need(items.size()<=96,"item budget exceeded");b.scene["metadata"]["item_count"]=items.size();b.scene["metadata"]["shelf_top_contact_policy"]="Level shelf surface; books and stacks use closed OBB support, 1 mm geometric bevel tolerance. Lean OBB contacts a taller neighbour.";return b.scene;
}
Json validate_bookshelf(const Json&s){Json errors=Json::array();std::set<std::string>parts;for(auto&p:s.at("parts"))parts.insert(p.at("id"));std::map<std::string,Json>items;for(auto&i:s.at("metadata").at("items"))items[i["id"]]=i;for(auto&pair:items){const auto&i=pair.second;auto lo=item_min(i["obb"]),hi=item_max(i["obb"]);auto&bay=s["metadata"]["compartments"][i["compartment"].get<size_t>()];double bottom=bay["bottom"],top=bay["top"];if(lo[0]<bay["x_min"].get<double>()-1e-6||hi[0]>bay["x_max"].get<double>()+1e-6||lo[1]<bay["y_min"].get<double>()-.012||hi[1]>bay["y_max"].get<double>()+.001||hi[2]>top-.005||lo[2]<bottom-1e-6)errors.push_back(pair.first+": outside compartment");std::string support=i["support_id"];if(items.count(support)){auto&sup=items[support];auto sl=item_min(sup["obb"]),sh=item_max(sup["obb"]);if(std::abs(lo[2]-sh[2])>1e-7||lo[0]<sl[0]-1e-7||hi[0]>sh[0]+1e-7||lo[1]<sl[1]-1e-7||hi[1]>sh[1]+1e-7)errors.push_back(pair.first+": stack unsupported");}else if(!parts.count(support)||std::abs(lo[2]-bottom)>1e-7)errors.push_back(pair.first+": missing floor support");if(i.contains("lean_support_id")){std::string other=i["lean_support_id"];if(!items.count(other))errors.push_back(pair.first+": missing leaning neighbour");else{auto oh=item_max(items[other]["obb"]);if(std::abs(lo[0]-oh[0])>.0001||hi[2]>oh[2])errors.push_back(pair.first+": lean not touching taller neighbour");}}
 for(auto&other:items)if(other.first>pair.first&&penetrates(i["obb"],other.second["obb"]))errors.push_back(pair.first+" penetrates "+other.first);
 }return errors;}
}
