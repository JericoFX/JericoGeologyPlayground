#include "jerico/scene.hpp"
#include "jerico/damage.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

namespace jerico {
namespace {
double amount(const Json&value,const std::string&name){
 if(!value.is_number())throw std::invalid_argument(name+" must be finite in 0..1");
 double x=value.get<double>();if(!std::isfinite(x)||x<0||x>1)throw std::invalid_argument(name+" must be finite in 0..1");return x;
}
std::array<double,6> bounds_of(const Json&part){
 std::array<double,6> bounds={INFINITY,-INFINITY,INFINITY,-INFINITY,INFINITY,-INFINITY};
 for(const auto&v:part.at("vertices"))for(size_t axis=0;axis<3;++axis){bounds[2*axis]=std::min(bounds[2*axis],v[axis].get<double>());bounds[2*axis+1]=std::max(bounds[2*axis+1],v[axis].get<double>());}
 return bounds;
}
Json wood_member(const Json&part,const Json&recipe,double age,int index){
 auto bounds=bounds_of(part);double x0=bounds[0],x1=bounds[1],y0=bounds[2],y1=bounds[3],z0=bounds[4],z1=bounds[5];bool horizontal=x1-x0>z1-z0;
 std::string name=part.at("id");Builder b(recipe);Json current=b.box(name,part.at("node"),x0,x1,y0,y1,z0,z1,"damp_wood",.0006,part.at("role"));
 double initial=(x1-x0)*(y1-y0)*(z1-z0);current["panel_bounds"]=bounds;int64_t seed=recipe.at("seed");PythonRandom rng(seed,uint64_t(9071+index*997));
 double load=name=="frame.left"?1.:name=="frame.right"?.52:name=="frame.head"?.40:1.15;
 double depth=.0012+(age>.35?.030*load:.010)*age;
 double extent=(.010+(age>.35?.080*load:.030)*age)*(1+rng.uniform(-.17,.17));Json profile;
 if(horizontal){
  double edge=name=="frame.sill"?z0:z1,sign=name=="frame.sill"?1:-1;
  double along=x0+(x1-x0)*(name=="frame.sill"&&age>.35?.23:.22+.09*index);
  profile={{along-extent,edge-sign*.004},{along+extent*.6,edge-sign*.004},{along+extent*.41,edge+sign*depth*.55},{along+extent*.07,edge+sign*depth},{along-extent*.36,edge+sign*depth*.43}};
 }else{
  double edge=name=="frame.left"?x0:x1,sign=name=="frame.left"?1:-1;
  double along=z0+(name=="frame.left"&&age>.35?std::min(.095,(z1-z0)*.12):(z1-z0)*(.27+.16*index));
  profile={{edge-sign*.004,along-extent},{edge+sign*depth*.44,along-extent*.63},{edge+sign*depth,along+extent*.14},{edge+sign*depth*.35,along+extent*.72},{edge-sign*.004,along+extent}};
 }
 current=carve_panel(current,profile,"front",age>.35?Json(nullptr):Json(.0022));
 if(age>.35||name=="frame.right"){
  V2 center={(x0+x1)/2,(z0+z1)/2};
  if(!horizontal){center[0]+=(x1-x0)*(index==0?.13:-.13);center[1]=z0+(z1-z0)*.61;}
  else{center[0]=x0+(x1-x0)*.62;center[1]=(z0+z1)/2;}
  double length=(horizontal?x1-x0:z1-z0)*(.045+.33*age);
  auto profiles=crack_profiles(center,length,.0012+.0023*age,.0012+.007*age,seed,horizontal?V2{1,0}:V2{0,1},0,3,uint64_t(index*31));
  current=carve_panel(current,profiles[0]["polygon_xz"],"front",profiles[0]["depth"]);
  current["wood_check"]={{"grain_axis",horizontal?"X":"Z"},{"centerline_xz",profiles[0]["centerline_xz"]},{"length_m",length},{"depth_m",profiles[0]["depth"]}};
 }
 if(age>.35&&name=="frame.sill"){
  auto second=crack_profiles({x0+(x1-x0)*.33,z0+(z1-z0)*.63},(x1-x0)*.15,.0014,.0038,seed,{1,0},0,3,771)[0];
  current=carve_panel(current,second["polygon_xz"],"front",second["depth"]);
 }
 for(auto&material:current["face_materials"])if(material=="cut_metal")material="wood_exposed";
 current["wood_damage"]={{"age",age},{"initial_volume_m3",initial},{"removed_volume_m3",initial-current["cut_math"]["remaining_volume"].get<double>()},{"policy","Localized outer-edge loss and shallow checks parallel to grain; no structural fracture or strength model"}};
 return current;
}
} // namespace

Json build_damp_wood(const Json&recipe){
 if(!recipe.is_object())throw std::invalid_argument("Recipe must be an object");
 Json local=recipe;auto family_json=local.value("family",Json("old"));local.erase("family");
 if(family_json!="old"&&family_json!="new")throw std::invalid_argument("family must be old or new");
 std::string family=family_json;double age=amount(local.value("age",Json(family=="old"?.86:.08)),"age"),wetness=amount(local.value("wetness",Json(.82)),"wetness");local.erase("age");local.erase("wetness");
 const Json defaults={{"seed",29},{"width",1.12},{"height",1.28},{"thickness",.06},{"glazing","intact"}};
 for(const auto&field:defaults.items())if(!local.contains(field.key()))local[field.key()]=field.value();
 local["preset"]="single_pane_fixed";Json scene=build_singlepane(local),r=scene["recipe"];std::string glazing=r["glazing"];
 auto glass=build_fracture({{"preset","industrial_window"},{"kind","window"},{"width",r["width"]},{"height",r["height"]},{"thickness",r["thickness"]},{"glazing",glazing},{"seed",r["seed"]},{"energy",.66},{"impact",{.15,.56}},{"ray_count",18}});
 Json parts=Json::array();for(const auto&p:scene["parts"])if(p["role"]!="GlassPane")parts.push_back(p);for(const auto&p:glass["parts"])if(p["role"]=="GlassPane")parts.push_back(p);scene["parts"]=std::move(parts);
 for(const std::string key:{"glass_hole","glass_fracture","retained_glass_fraction"})if(glass["metadata"].contains(key))scene["metadata"][key]=glass["metadata"][key];
 Json samples=Json::array();int member_index=0;
 for(auto&p:scene["parts"]){
  if(p["role"]=="Frame"){
   p=wood_member(p,r,age,member_index++);auto bounds=p["panel_bounds"].get<std::array<double,6>>();double x0=bounds[0],x1=bounds[1],y0=bounds[2],z0=bounds[4],z1=bounds[5];bool horizontal=x1-x0>z1-z0;
   for(double f:{.20,.50,.80}){
    V3 point=horizontal?V3{x0+(x1-x0)*f,y0+.022,p["id"]=="frame.sill"?z1-.004:z0+.004}:V3{p["id"]=="frame.left"?x1-.004:x0+.004,y0+.022,z0+(z1-z0)*f};
    if(!point_in_panel(p,point))throw std::invalid_argument("Wood damage compromised protected supporting core");
    samples.push_back({{"part_id",p["id"]},{"point",point}});
   }
  }
  if(p["material"]=="wood"||p["material"]=="wood_rough"||p["material"]=="damp_wood"){
   p["material"]="damp_wood";if(p.contains("face_materials"))for(auto&material:p["face_materials"])if(material=="wood"||material=="wood_rough")material="damp_wood";
   auto bounds=bounds_of(p);p["wood_finish"]={{"family",family},{"age",age},{"wetness",wetness},{"grain_axis",bounds[1]-bounds[0]>bounds[5]-bounds[4]?"X":"Z"},{"height_m",r["height"]},{"seed",r["seed"]}};
  }
  if(p["role"]=="DirtSeam"&&age<.35){p["material"]="damp_wood";p["wood_finish"]={{"family",family},{"age",age},{"wetness",wetness},{"grain_axis","Z"},{"height_m",r["height"]},{"seed",r["seed"]}};}
 }
 scene["recipe"].update(Json{{"family",family},{"age",age},{"wetness",wetness},{"preset","damp_wood_single_pane"}});
 scene["metadata"].update(Json{
  {"generator","JericoProceduralCppLab/damp_wood.cpp"},{"preset","damp_wood_single_pane"},{"wood_finish",{{"family",family},{"age",age},{"wetness",wetness}}},{"wood_support_samples",samples},
  {"art_direction","Common wet timber, newer with restrained handling wear; old timber has desaturated grain, localized edge splinters and real lengthwise checks."},
  {"damage_policy","Age controls real local edge loss and shallow grain-aligned checks independently of wetness. All six states reuse the same supporting frame per family."},
  {"physics_policy","Authoring geometry and support references only. Engine owns joints, strength, collision and all interaction mechanics."},
  {"authoring_only",true},{"simulation",false},{"approved_glass_snapshot","source-snapshot-3c6da6d852c6"},
  {"finish_portability","Wet color darkening and roughness variation bake into PBR maps. No runtime-only moisture shader, droplets, or animation is required."}});
 return scene;
}
} // namespace jerico
