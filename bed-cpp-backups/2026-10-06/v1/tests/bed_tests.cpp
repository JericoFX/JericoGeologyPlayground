#include "jerico/bed.hpp"
#include "jerico/export.hpp"
#include <iostream>
#include <stdexcept>
#include <set>
#include <cmath>
using namespace jerico;
void need(bool c,const std::string&m){if(!c)throw std::runtime_error(m);}
int main(){try{
 auto recipe=normalize_bed_recipe(Json::object());need(recipe["width"]==1.02,"single width");
 for(auto key:{"width","length","head_height","foot_height","tube_radius","bar_count"}){bool rejected=false;try{normalize_bed_recipe({{key,-1}});}catch(...){rejected=true;}need(rejected,std::string("reject ")+key);}
 for(auto bad:std::vector<Json>{{{"width",.7},{"bar_count",12},{"tube_radius",.03}},{{"foot_height",.4}},{{"preset","invalid"}},{{"seed",1.5}},{{"unknown",0}}}){bool rejected=false;try{normalize_bed_recipe(bad);}catch(...){rejected=true;}need(rejected,"reject invalid cross-field/range");}
 auto a=build_bed({{"preset","complete"}}),again=build_bed({{"preset","complete"}});need(a.dump()==again.dump(),"deterministic");need(a.dump()!=build_bed({{"seed",72}}).dump(),"seed changes geometry");
 need(validate_scene(a).empty(),"source scene valid: "+validate_scene(a).dump());need(validate_bed(a).empty(),"bed contacts/clearance: "+validate_bed(a).dump());
 auto bare=build_bed({{"preset","iron_only"}}),soft=build_bed({{"preset","bedding"}});need(bare["parts"].size()<soft["parts"].size()&&soft["parts"].size()<a["parts"].size(),"preset hierarchy");
 for(auto&p:bare["parts"]){bool found=false;for(auto&q:a["parts"])if(q["id"]==p["id"]){need(q==p,"structural geometry stable across presets");found=true;}need(found,"stable structure IDs");}

 // Independent analytical support oracle; would fail if pillow support is sampled before its XY skew.
 double mw=1.02-.080,ml=2.04-.11;
 auto support=[&](double x,double y){double u=x/(mw/2),v=y/(ml/2);double f=std::pow(std::max(0.,1-std::pow(u,16)),.38)*std::pow(std::max(0.,1-std::pow(v,16)),.38);return .548+.029*f+.0012*f*std::sin(x*21+y*13);};
 for(auto&p:a["parts"]){if(p["id"]=="pillow"){double contact=1;size_t half=p["vertices"].size()/2;for(size_t i=half;i<p["vertices"].size();i++){V3 q=p["vertices"][i];double gap=q[2]-support(q[0],q[1]);need(gap>=.00095,"pillow support clearance after XY skew");contact=std::min(contact,gap);}need(contact<.0011,"pillow physically supported");}
 if(p["id"]=="blanket")for(auto&v:p["vertices"]){V3 q=v;if(q[2]<.466&&q[2]>.36)for(int sign:{-1,1})for(double yy:{-.612,0.,.612})need(!(std::abs(q[0]-sign*.51)<.026&&std::abs(q[1]-yy)<.018),"blanket intersects retainer");}}

 for(auto&p:a["parts"])if(p["id"]=="mattress")for(int slat=0;slat<8;slat++){double yy=-1.02+.09+(2.04-.18)*slat/7,gap=1.;for(size_t i=p["vertices"].size()/2;i<p["vertices"].size();i++){V3 q=p["vertices"][i];if(std::abs(q[1]-yy)<.025&&std::abs(q[0])<.45)gap=std::min(gap,q[2]-.405);}need(gap<.003,"every slat supports mattress within 3mm");}
 for(auto&g:a["grips"])for(auto&s:a["sockets"])if(s["id"]==g["socket_id"])need(s["normal"]==g["basis"]["z"],"rail socket and grip outward axes agree");
 auto e=evaluate_bed(a);need(validate_scene(e).empty(),"evaluated valid: "+validate_scene(e).dump());need(validate_bed(e).empty(),"evaluated bed valid");
 for(auto&p:e["parts"])for(auto&f:p["normals"])for(auto&n:f){V3 v=n;need(std::abs(dot(v,v)-1)<1e-7,"unit normals");}
 for(auto r:std::vector<Json>{{{"width",.76},{"length",1.75},{"head_height",.9},{"foot_height",.7},{"tube_radius",.016},{"bar_count",3}},{{"width",1.4},{"length",2.3},{"head_height",1.4},{"foot_height",1.0},{"tube_radius",.026},{"bar_count",8}}}){auto s=build_bed(r);need(validate_scene(s).empty(),"range mesh valid");need(validate_bed(s).empty(),"range contacts valid");}
 std::cout<<"Bed geometry, recipe, deterministic presets, normals and contact tests passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
