#include "jerico/dock.hpp"
#include "jerico/evaluate.hpp"
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace jerico;
void require(bool c,const std::string&m){if(!c)throw std::runtime_error(m);}
double length(V3 v){return std::sqrt(dot(v,v));}
int main(){try{
 DockSpline straight({{0,0,1.5},{4,0,1.5},{8,0,1.5}});
 require(std::abs(straight.length()-8)<1e-6,"straight length");
 require(length(sub(straight.sample(3).position,{3,0,1.5}))<1e-5,"arc distance sample");
 DockSpline curve({{0,0,1.5},{2,.1,1.5},{4,.65,1.5},{6,1,1.5},{8,1.1,1.5}});
 for(double d=0;d<=curve.length();d+=.13){auto q=curve.sample(d);require(std::abs(dot(q.tangent,q.right))<1e-6,"orthogonal spline frame");require(std::abs(length(q.right)-1)<1e-6,"unit frame");}
 auto recipe=default_dock_recipe();auto a=build_dock(recipe),b=build_dock(recipe);
 require(a.dump()==b.dump(),"deterministic same inputs");
 recipe["seed"]=42;auto c=build_dock(recipe);require(a["parts"]!=c["parts"],"seed varies geometry");
 require(validate_scene(a).empty(),"closed outward native solids");require(validate_dock(a).empty(),"dock structure contract");
 std::set<std::string> roles;std::set<std::string> signatures;size_t posts=0,stairs=0,damaged=0;
 for(auto&p:a["parts"]){roles.insert(p["role"]);if(p["role"]=="DeckBoard")signatures.insert(p["timber_profile"].dump());if(p["role"]=="Pile")++posts;if(p["role"]=="StairTread")++stairs;if(p.value("fractured",false))++damaged;}
 require(signatures.size()>20,"real individual board silhouette variation");require(posts>=10,"bilateral individual piles");require(stairs>=7,"explicit staircase");require(damaged>=2,"regional broken silhouettes");
 int shapedPiles=0;for(auto&p:a["parts"])if(p["role"]=="Pile"){auto o=p["timber_frame"]["origin"].get<V3>();double lo=1e9,hi=0;for(size_t i=0;i+2<p["vertices"].size();++i){auto v=p["vertices"][i].get<V3>();double rad=std::hypot(v[0]-o[0],v[1]-o[1]);lo=std::min(lo,rad);hi=std::max(hi,rad);}if(hi-lo>.045)++shapedPiles;}require(shapedPiles>=8,"piles need visibly distinct irregular shaft/head profiles");
 require(roles.count("Brace")&&roles.count("LandingBoard")&&roles.count("BranchBoard"),"landing branch bracing present");
 auto attachment=a["metadata"]["branch_attachment"];auto start=attachment["start"].get<V3>(),end=attachment["end"].get<V3>();auto along=norm(sub(end,start)),across=cross(along,V3{0,0,1});
 for(auto&p:a["parts"])if(p["role"]=="Pile"&&p["id"].get<std::string>().find("main.pile.")==0){auto local=sub(p["timber_frame"]["origin"].get<V3>(),start);if(dot(local,along)>-.55&&dot(local,along)<.10&&std::abs(dot(local,across))<.85)require(p["head_height_m"].get<double>()<=start[2]-.075,"main pile obstructs branch entrance");}
 auto intactRecipe=default_dock_recipe();intactRecipe["damage"]["enabled"]=false;auto intact=build_dock(intactRecipe);std::map<std::string,Json> base;for(auto&p:intact["parts"])base[p["id"]]=p;
 for(auto&p:a["parts"])if(p["role"]=="Pile"||p["role"]=="Beam"||p["role"]=="StairTread")require(base.at(p["id"])==p,"protected supports/stairs unchanged by deck damage");
 for(auto&socket:a["sockets"])if(!socket.value("enabled",true)){bool found=false;for(auto&original:intact["sockets"])if(original["id"]==socket["id"]&&original.value("enabled",false))found=true;require(found,"invalidated socket must retain original intact ID");}
 auto zeroRecipe=default_dock_recipe();zeroRecipe["damage"]["severity"]=0;auto zero=build_dock(zeroRecipe);for(auto&p:zero["parts"])require(!p.value("fractured",false),"severity zero must preserve intact geometry");
 auto slopeRecipe=default_dock_recipe();for(size_t i=0;i<slopeRecipe["main"].size();++i)slopeRecipe["main"][i][2]=double(i)*.1;auto slope=build_dock(slopeRecipe);for(auto&p:slope["parts"])if(p["role"]=="StairTread")require(std::abs(p["timber_frame"]["u"][2].get<double>())<1e-10&&std::abs(p["timber_frame"]["v"][2].get<double>())<1e-10,"stair treads remain horizontal on elevated spline");
 auto evaluated=evaluate_scene(a);require(validate_scene(evaluated).empty(),"evaluated finite normals indices UV manifold");
 std::cout<<"PASS dock spline, determinism, wood variation, support protection, staircase, regional damage, mesh validation\n";
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
