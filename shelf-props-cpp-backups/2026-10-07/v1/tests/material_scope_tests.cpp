#include "jerico/materials.hpp"
#include "jerico/fine_wear.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace jerico;
static void require(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
static double delta(const MaterialSample&a,const MaterialSample&b){double d=std::max(std::abs(a.roughness-b.roughness),std::abs(a.metallic-b.metallic));for(int i=0;i<3;i++)d=std::max(d,std::abs(a.base_color[i]-b.base_color[i]));return d;}
static Json sheet(const Json&scene){for(const auto&p:scene["parts"])if(p["id"]=="door.sheet")return p;throw std::runtime_error("real industrial part missing");}
int main(){try{
 Json recipe={{"preset","industrial_solid"},{"seed",29},{"angle",0}};
 Json op={{"type","edge_wear"},{"part_id","door.sheet"},{"side","front"},{"region",{{"shape","box"},{"min",{-2,-1,-1}},{"max",{2,1,3}}}},{"width_m",2},{"amount",1},{"override_protection",true},{"fine_breakup",true}};
 auto base=build_industrial(recipe);auto industrial=apply_operations(base,Json::array({op}));auto p=sheet(industrial);p["_surface_side"]="front";auto b=p["panel_bounds"].get<std::array<double,6>>();V3 point={b[0]+.001,b[2],(b[4]+b[5])*.5};auto absent=p;absent["edge_wear"][0].erase("fine_breakup");
 for(const auto*key:{"painted_steel","dark_steel","iron","rust"})for(const Json&flag:Json::array({true,false,"ignored in R2",1,nullptr})){
  auto r2=p;r2["edge_wear"][0]["fine_breakup"]=flag;require(delta(sample_material(r2,key,point),sample_material(absent,key,point))<1e-12,"R2 must ignore free fine_breakup metadata");
 }
 require(!industrial["metadata"].contains("native_material_profile"),"R2 incorrectly tagged FineWear");
 auto fine=apply_fine_wear_operations(base,Json::array({op}));require(fine["metadata"].value("native_material_profile","")=="finewear_1921","FineWear apply missing explicit source profile");
 auto fineRecipe=recipe;fineRecipe["operations"]=Json::array({op});auto built=build_fine_wear(fineRecipe);require(built["metadata"].value("native_material_profile","")=="finewear_1921","FineWear build missing explicit source profile");
 auto fp=sheet(fine);fp["_surface_side"]="front";fp["_material_family"]="fine";require(delta(sample_material(fp,"painted_steel",point),sample_material(absent,"painted_steel",point))>.01,"Explicit FineWear must retain source breakup");
 auto plainFine=fp;plainFine["edge_wear"][0]["fine_breakup"]=false;require(delta(sample_material(plainFine,"painted_steel",point),sample_material(absent,"painted_steel",point))<1e-12,"FineWear false must retain R2 base");
 // Bake the real native panel only, keeping its six faces and metric UVs. Verify
 // the profile crosses builder -> scene -> baker -> sampler, not just raw API.
 for(auto pair:std::array<std::pair<Json,std::string>,2>{{{industrial,"steel"},{fine,"fine"}}}){
  auto scene=pair.first;scene["parts"]=Json::array({sheet(scene)});const auto family=pair.second;
  auto report=bake_material_textures(scene,"/tmp/jerico-material-scope-"+family,32);bool front=false;
  for(const auto&probe:report["probes"]){require(probe["family"]==family,"Bake oracle provenance does not reflect explicit profile");auto expectedPart=probe["part"];expectedPart["_material_family"]=family;auto s=sample_material(expectedPart,probe["material"],probe["position"].get<V3>());for(int i=0;i<3;i++)require(std::abs(s.base_color[i]-probe["base_color"][i].get<double>())<1e-12,"Atlas sampler lost material source profile");if(expectedPart.value("_surface_side","")=="front"){front=true;expectedPart["_material_family"]="steel";auto baseline=sample_material(expectedPart,probe["material"],probe["position"].get<V3>());require(family=="fine"?delta(s,baseline)>.001:delta(s,baseline)<1e-12,"Baked front has wrong R2/FineWear behavior");}}
  require(front,"Scope regression did not sample a front face");
 }
 std::cout<<"material_scope_tests: native R2/FineWear profiles isolated for raw samples, wrappers and atlas\n";return 0;
}catch(const std::exception&e){std::cerr<<"material_scope_tests: FAIL: "<<e.what()<<'\n';return 1;}}
