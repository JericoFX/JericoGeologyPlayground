#include "jerico/interfaces.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
namespace jerico {
namespace {
bool number(const Json&value,bool positive=false){return value.is_number()&&std::isfinite(value.get<double>())&&(positive?value.get<double>()>0:value.get<double>()>=0);}
bool pair(const Json&value,bool positive=true){return value.is_array()&&value.size()==2&&number(value[0],positive)&&number(value[1],positive);}
bool vector3(const Json&value){if(!value.is_array()||value.size()!=3)return false;for(const auto&v:value)if(!v.is_number()||!std::isfinite(v.get<double>()))return false;return true;}
bool contains(const Json&array,const Json&value){return array.is_array()&&std::find(array.begin(),array.end(),value)!=array.end();}
double point_segment_distance(V3 p,V3 a,V3 b){V3 ab=sub(b,a);double length=dot(ab,ab);if(length==0)return std::sqrt(dot(sub(p,a),sub(p,a)));double t=std::clamp(dot(sub(p,a),ab)/length,0.,1.);V3 d=sub(p,add(a,mul(ab,t)));return std::sqrt(dot(d,d));}
double point_triangle_distance(V3 p,V3 a,V3 b,V3 c){
 V3 ab=sub(b,a),ac=sub(c,a),ap=sub(p,a);double d1=dot(ab,ap),d2=dot(ac,ap);
 if(d1<=0&&d2<=0)return std::sqrt(dot(ap,ap));
 V3 bp=sub(p,b);double d3=dot(ab,bp),d4=dot(ac,bp);
 if(d3>=0&&d4<=d3)return std::sqrt(dot(bp,bp));
 double vc=d1*d4-d3*d2;
 if(vc<=0&&d1>=0&&d3<=0&&d1!=d3){V3 q=add(a,mul(ab,d1/(d1-d3)));return std::sqrt(dot(sub(p,q),sub(p,q)));}
 V3 cp=sub(p,c);double d5=dot(ab,cp),d6=dot(ac,cp);
 if(d6>=0&&d5<=d6)return std::sqrt(dot(cp,cp));
 double vb=d5*d2-d1*d6;
 if(vb<=0&&d2>=0&&d6<=0&&d2!=d6){V3 q=add(a,mul(ac,d2/(d2-d6)));return std::sqrt(dot(sub(p,q),sub(p,q)));}
 double va=d3*d6-d5*d4;
 if(va<=0&&d4-d3>=0&&d5-d6>=0&&(d4-d3)+(d5-d6)!=0){V3 q=add(b,mul(sub(c,b),(d4-d3)/((d4-d3)+(d5-d6))));return std::sqrt(dot(sub(p,q),sub(p,q)));}
 double sum=va+vb+vc;
 if(sum==0)return std::min({point_segment_distance(p,a,b),point_segment_distance(p,b,c),point_segment_distance(p,c,a)});
 double denominator=1/sum;V3 q=add(a,add(mul(ab,vb*denominator),mul(ac,vc*denominator)));return std::sqrt(std::max(0.,dot(sub(p,q),sub(p,q))));
}
bool valid_basis(const Json&basis){
 if(!basis.is_object()||!basis.contains("x")||!basis.contains("y")||!basis.contains("z")||!vector3(basis["x"])||!vector3(basis["y"])||!vector3(basis["z"]))return false;
 V3 x=basis["x"],y=basis["y"],z=basis["z"];
 return std::abs(dot(x,x)-1)<1e-8&&std::abs(dot(y,y)-1)<1e-8&&std::abs(dot(z,z)-1)<1e-8&&std::abs(dot(x,y))<1e-8&&std::abs(dot(x,z))<1e-8&&std::abs(dot(y,z))<1e-8&&std::abs(dot(cross(x,y),z)-1)<1e-8;
}
bool aligned(V3 a,V3 b){for(size_t i=0;i<3;++i)if(std::abs(a[i]-b[i])>1e-8)return false;return true;}
} // namespace

Json handle_interface(const std::string&style,const std::string&role){
 if(style!="lever"&&style!="pull")throw std::invalid_argument("Unknown handle style");
 return {{"interface","mount.handle.v1"},{"role",role},{"style",style},{"footprint",{.05,.22}},{"projection",.10},{"required_capabilities",{"hand_grasp","two_bolt_mount"}},{"thickness_range",{.028,.12}}};
}
bool can_connect(const Json&socket,const Json&component){
 try{
  if(!socket.is_object()||!component.is_object())return false;
  const auto&coverage=socket.at("coverage");if(!coverage.is_object()||!socket.at("enabled").is_boolean()||!socket.at("enabled").get<bool>()||socket.at("interface")!=component.at("interface")||socket.at("role")!=component.at("role"))return false;
  const auto&dimensions=component.at("footprint");Json available;
  if(coverage.contains("footprint"))available=coverage["footprint"];
  else{const auto&opening=coverage.at("opening");if(!opening.is_array()||opening.size()<2)return false;available={opening[0],opening[1]};}
  if(!pair(dimensions)||!pair(available))return false;
  for(size_t i=0;i<2;++i)if(dimensions[i].get<double>()>available[i].get<double>()+1e-9)return false;
  auto required=component.value("required_capabilities",Json::array());auto capabilities=coverage.value("capabilities",Json::array());if(!required.is_array()||!capabilities.is_array())return false;for(const auto&capability:required)if(!capability.is_string()||!contains(capabilities,capability))return false;
  auto projection=component.value("projection",Json(0));if(!number(projection))return false;
  if(coverage.contains("max_projection")&&(!number(coverage["max_projection"])||projection.get<double>()>coverage["max_projection"].get<double>()))return false;
  if(component.contains("style")&&!contains(coverage.value("styles",Json::array()),component["style"]))return false;
  if(component.contains("thickness_range")){
   const auto&limits=component["thickness_range"];auto thickness=coverage.value("support_thickness",Json(0));
   if(!pair(limits)||limits[0].get<double>()>limits[1].get<double>()||!number(thickness,true)||thickness.get<double>()<limits[0].get<double>()||thickness.get<double>()>limits[1].get<double>())return false;
  }
  const std::string interface=socket.at("interface");
  if(interface=="pane.rect.v1"){
   auto thickness=component.value("thickness",Json());const auto&maximum=coverage.at("max_thickness");if(!number(thickness,true)||!number(maximum,true)||thickness.get<double>()>maximum.get<double>())return false;
  }
  if(interface=="hinge.pin.v1"){
   auto diameter=component.value("pin_diameter",Json());const auto&socket_diameter=coverage.at("pin_diameter");auto tolerance=coverage.value("pin_tolerance",Json(.0005));
   if(!number(diameter,true)||!number(socket_diameter,true)||!number(tolerance)||std::abs(diameter.get<double>()-socket_diameter.get<double>())>tolerance.get<double>())return false;
  }
  if(component.contains("range_degrees")){
   const auto&span=component["range_degrees"];auto allowed=coverage.value("range_degrees",Json());
   if(!pair(span,false)||!pair(allowed,false)||span[0].get<double>()>span[1].get<double>()||span[0].get<double>()<allowed[0].get<double>()||span[1].get<double>()>allowed[1].get<double>())return false;
  }
  if(interface=="opening.rect.v1"&&component.contains("thickness")){
   const auto&thickness=component["thickness"];const auto&maximum=coverage.at("opening").at(2);if(!number(thickness,true)||!number(maximum,true)||thickness.get<double>()>maximum.get<double>())return false;
  }
  return true;
 }catch(const Json::exception&){return false;}
}

double grip_surface_distance(const Json&scene,const Json&grip){
 try{
  const auto&support_ids=scene.at("metadata").at("grip_supports");auto found=support_ids.find(grip.at("id").get<std::string>());if(found==support_ids.end()||!vector3(grip.at("position")))return INFINITY;
  const Json*part=nullptr;for(const auto&p:scene.at("parts"))if(p.at("id")==*found){part=&p;break;}
  if(!part||part->at("node")!=grip.at("node"))return INFINITY;
  double minimum=INFINITY;V3 position=grip.at("position");const auto&vs=part->at("vertices");
  for(const auto&face:part->at("faces"))for(size_t i=1;i+1<face.size();++i){const auto&a=vs.at(face[0].get<size_t>());const auto&b=vs.at(face[i].get<size_t>());const auto&c=vs.at(face[i+1].get<size_t>());if(!vector3(a)||!vector3(b)||!vector3(c))return INFINITY;minimum=std::min(minimum,point_triangle_distance(position,a.get<V3>(),b.get<V3>(),c.get<V3>()));}
  return minimum;
 }catch(const Json::exception&){return INFINITY;}
}

Json validate_interfaces(const Json&scene){
 Json errors=Json::array();
 try{
  std::set<std::string> nodes,parts;
  for(const auto&n:scene.at("nodes")){
   nodes.insert(n.at("id"));
   if(!vector3(n.at("translation"))||!n.at("rotation_z").is_number()||!std::isfinite(n.at("rotation_z").get<double>()))errors.push_back(n.at("id").get<std::string>()+": nonfinite node transform");
  }
  for(const auto&p:scene.at("parts"))parts.insert(p.at("id"));
  for(const auto&socket:scene.at("sockets")){
   std::string prefix=socket.value("id",std::string("<socket>"))+": ";
   try{
    if(!nodes.count(socket.at("node")))errors.push_back(prefix+"missing node");
    if(!vector3(socket.at("translation"))||!socket.at("rotation_z").is_number()||!std::isfinite(socket.at("rotation_z").get<double>()))errors.push_back(prefix+"nonfinite socket transform");
    if(!valid_basis(socket.at("basis")))errors.push_back(prefix+"invalid right-handed orthonormal basis");
    else if(!vector3(socket.at("normal"))||!aligned(socket.at("normal").get<V3>(),socket.at("basis").at("z").get<V3>()))errors.push_back(prefix+"normal differs from basis.z");
    if(!socket.at("enabled").is_boolean())errors.push_back(prefix+"enabled must be boolean");
    else if(socket["enabled"].get<bool>())for(const auto&support:socket.at("coverage").value("support_part_ids",Json::array()))if(!support.is_string()||!parts.count(support.get<std::string>()))errors.push_back(prefix+"missing structural support");
   }catch(const Json::exception&){errors.push_back(prefix+"malformed socket contract");}
  }
  for(const auto&grip:scene.at("grips")){
   std::string prefix=grip.value("id",std::string("<grip>"))+": ";
   try{
    const Json*socket=nullptr;for(const auto&s:scene.at("sockets"))if(s.at("id")==grip.at("socket_id")){socket=&s;break;}
    if(!socket)errors.push_back(prefix+"missing socket");
    if(!nodes.count(grip.at("node")))errors.push_back(prefix+"missing node");
    bool basis_ok=valid_basis(grip.at("basis"));if(!basis_ok)errors.push_back(prefix+"invalid right-handed orthonormal basis");
    if(!vector3(grip.at("position"))||!vector3(grip.at("approach"))||!number(grip.at("reach_radius"),true))errors.push_back(prefix+"invalid position, approach or reach radius");
    else if(basis_ok&&!aligned(grip.at("approach").get<V3>(),mul(grip.at("basis").at("z").get<V3>(),-1)))errors.push_back(prefix+"approach differs from inward grasp normal");
    if(socket&&basis_ok&&vector3(socket->at("normal"))&&!aligned(grip.at("basis").at("z").get<V3>(),socket->at("normal").get<V3>()))errors.push_back(prefix+"grasp normal differs from socket normal");
    if(!grip.at("enabled").is_boolean())errors.push_back(prefix+"enabled must be boolean");
    else if(grip.at("enabled").get<bool>()){
     if(socket&&(!socket->at("enabled").is_boolean()||!socket->at("enabled").get<bool>()))errors.push_back(prefix+"enabled on disabled socket");
     if(grip_surface_distance(scene,grip)>1e-8)errors.push_back(prefix+"unsupported grasp");
    }
   }catch(const Json::exception&){errors.push_back(prefix+"malformed grasp contract");}
  }
 }catch(const Json::exception&){errors.push_back("Malformed scene interaction collections");}
 return errors;
}
} // namespace jerico
