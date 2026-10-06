#pragma once
#include <nlohmann/json.hpp>
#include <array>
#include <string>
#include <vector>
#include <cstdint>
namespace jerico {
using Json=nlohmann::ordered_json;
using V2=std::array<double,2>; using V3=std::array<double,3>;
class PythonRandom { std::array<uint32_t,624> mt{}; size_t index=624; public: explicit PythonRandom(int64_t seed); PythonRandom(int64_t seed,uint64_t positive_offset); uint32_t next(); double random(); double uniform(double a,double b){return a+(b-a)*random();} };
V3 add(V3,V3); V3 sub(V3,V3); V3 mul(V3,double); double dot(V3,V3); V3 cross(V3,V3); V3 norm(V3);
Json metric_uvs(const Json&vertices,const Json&faces);
std::vector<std::array<size_t,3>> triangulate(const std::vector<V2>&profile);
Json normalize_recipe(const Json&recipe);
int64_t recipe_seed(const Json&seed);
V3 world_point(const Json&scene,const std::string&node,V3 point);
struct Builder {
 Json r,scene; PythonRandom rng;
 explicit Builder(const Json&recipe);
 void node(std::string id,V3 translation,std::string role,Json parent="frame",double angle=0);
 Json& mesh(std::string id,std::string node,const Json&vertices,const Json&faces,std::string material="wood",double bevel=.0015,std::string role="Structure");
 Json& prism(std::string id,std::string node,const std::vector<V2>&profile,double y0,double y1,std::string material="wood",double bevel=.0015,std::string role="Structure");
 Json& box(std::string id,std::string node,double x0,double x1,double y0,double y1,double z0,double z1,std::string material="wood",double bevel=.0015,std::string role="Structure");
 Json& chipped_board(std::string id,std::string node,double x0,double x1,double y0,double y1,double z0,double z1,double severity=1,std::string role="Plank");
 Json& cylinder(std::string id,std::string node,V3 a,V3 b,double radius,std::string material="iron",int sides=12,double bevel=.0006,std::string role="Hardware");
 Json& socket(std::string id,std::string node,std::string role,std::string interface,V3 position,Json coverage,bool enabled=true);
 void rivet(std::string id,std::string node,double x,double y,double z,bool front=true,double radius=.009);
 void handle(std::string id,std::string node,double x,double y,double z,bool front,bool enabled=true,std::string style="",std::string support_id="",double support_width=.22,int lever_direction=-1);
};
void wood_frame(Builder&);
Json clearance_sweep(const Json&scene,double step_degrees=.5);
Json build_damp_wood(const Json&recipe);
Json build_industrial(const Json&recipe);
Json build_fracture(const Json&recipe);
Json build_singlepane(const Json&recipe);
Json build_wood(const Json&recipe);
Json build_furniture(const Json&recipe);
Json normalize_furniture_recipe(const Json&recipe);
Json sweep_furniture_clearance(const Json&scene);
Json validate_furniture(const Json&scene);
Json validate_scene(const Json&scene);
}
