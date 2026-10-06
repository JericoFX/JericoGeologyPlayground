#include "jerico/interfaces.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace jerico;
namespace fs=std::filesystem;
namespace {
size_t checks=0;
void require(bool ok,const std::string&why){++checks;if(!ok)throw std::runtime_error(why);}
Json load(const fs::path&path){std::ifstream in(path);require(bool(in),"Cannot read "+path.string());Json value;in>>value;return value;}
}
int main(int argc,char**argv){try{
 fs::path root=argc>1?argv[1]:"fixtures";
 for(const auto&record:load(root/"interfaces/compatibility.json"))require(can_connect(record["socket"],record["component"])==record["expected"].get<bool>(),"source compatibility mismatch: "+record["name"].get<std::string>());
 double maximum=0;for(const auto&record:load(root/"interfaces/distances.json")){double error=std::abs(grip_surface_distance(record["scene"],record["grip"])-record["expected"].get<double>());maximum=std::max(maximum,error);require(error<=2e-12,"point-triangle distance mismatch");}
 size_t scenes=0,grips=0;
 for(const std::string family:{"furniture","wood","damp"})for(const auto&record:load(root/family/"manifest.json")){
  auto scene=load(root/family/record["file"].get<std::string>());auto errors=validate_interfaces(scene);require(errors.empty(),"fixture interaction validation: "+record["file"].get<std::string>()+": "+errors.dump());
  for(const auto&grip:scene["grips"])if(grip["enabled"].get<bool>()){require(grip_surface_distance(scene,grip)<=1e-8,"enabled grasp lacks surface contact");++grips;}
  ++scenes;
 }
 auto scene=build_wood(Json::object());require(validate_interfaces(scene).empty(),"native scene interaction validation");Json socket;for(const auto&s:scene["sockets"])if(s["role"]=="HandleFront")socket=s;
 for(const std::string style:{"lever","pull"}){auto component=handle_interface(style);require(component["style"]==style&&component["footprint"]==Json({.05,.22}),"handle shape mismatch");require(can_connect(socket,component),"valid handle rejected");}
 bool rejected=false;try{handle_interface("knob");}catch(const std::invalid_argument&){rejected=true;}require(rejected,"unknown handle style accepted");
 for(double value:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-1.}){
  auto component=handle_interface("lever");component["projection"]=value;require(!can_connect(socket,component),"nonfinite/negative component projection accepted");
  auto bad_socket=socket;bad_socket["coverage"]["max_projection"]=value;require(!can_connect(bad_socket,handle_interface("lever")),"nonfinite/negative mounting coverage accepted");
 }
 auto bad=scene;bad["sockets"][0]["translation"][0]=std::numeric_limits<double>::quiet_NaN();require(!validate_interfaces(bad).empty(),"nonfinite socket transform accepted");
 bad=scene;bad["nodes"][0]["rotation_z"]=std::numeric_limits<double>::infinity();require(!validate_interfaces(bad).empty(),"nonfinite node transform accepted");
 bad=scene;bad["sockets"][0]["basis"]["x"]={-1,0,0};require(!validate_interfaces(bad).empty(),"left-handed socket basis accepted");
 bad=scene;bad["grips"][0]["approach"]={0,-1,0};require(!validate_interfaces(bad).empty(),"outward grasp approach accepted");
 bad=scene;bad["grips"][0]["position"][0]=20;require(!validate_interfaces(bad).empty(),"unsupported grasp accepted");
 bad=scene;for(auto&s:bad["sockets"])if(s["id"]==bad["grips"][0]["socket_id"])s["enabled"]=false;require(!validate_interfaces(bad).empty(),"enabled grip on disabled socket accepted");
 bad=scene;bad["metadata"]["grip_supports"].erase(bad["grips"][0]["id"].get<std::string>());require(std::isinf(grip_surface_distance(bad,bad["grips"][0])),"missing support distance must be infinite");
 for(const std::string kind:{"door","window"})for(const std::string style:{"lever","pull"}){
  auto broken=build_wood({{"kind",kind},{"handle",style},{"damage","broken"}});require(validate_interfaces(broken).empty(),"damage state invalid interaction");bool disabled=false;for(const auto&g:broken["grips"])if(!g["enabled"].get<bool>()){disabled=true;require(std::isinf(grip_surface_distance(broken,g)),"removed handle unexpectedly retains support");}require(disabled,"broken support did not disable any grip");
 }
 std::cout<<"PASS interfaces: "<<checks<<" checks, 47 source compatibility cases, 12 source distance cases (max delta "<<maximum<<"), "<<scenes<<" complete scene contracts and "<<grips<<" supported enabled grabs; invalid data, reflected bases and damaged support rejected\n";
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
