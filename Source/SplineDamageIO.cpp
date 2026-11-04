#include "SplineDamageIO.hpp"
#include "DamagePathIO.hpp"
#include <algorithm>
#include <set>
namespace Playground::Construction {
namespace {
void validatePaths(const std::vector<PathDamageRule>& paths){
 if(paths.size()>64)throw std::runtime_error("Spline damage path budget exceeded");
 std::set<std::string> ids;
 for(const auto& rule:paths){
  if(rule.id.empty()||rule.id.size()>64||!ids.insert(rule.id).second)throw std::runtime_error("Spline damage path ID is invalid or repeated");
  validateSplineDamage(rule);
 }
}
}
std::string serializeSplineDamage(const std::vector<PathDamageRule>& paths){
 validatePaths(paths);DamageIO::Json entries=DamageIO::Json::array();
 const bool volumeUnion=std::any_of(paths.begin(),paths.end(),[](const auto&rule){return rule.operation==DamageVolumeOperation::Union;});
 bool spatial=false;
 for(const auto& rule:paths){
  auto entry=DamageIO::writePathCutter(rule.cutter);entry["id"]=rule.id;
  if(volumeUnion)entry["operation"]=rule.operation==DamageVolumeOperation::Union?"VolumeUnion":"VolumeDifference";
  if(rule.spatialPath){
   spatial=true;const auto&s=*rule.spatialPath;DamageIO::Json points=DamageIO::Json::array();
   for(const auto&p:s.points)points.push_back({{"guid",Jerico::formatGuid(p.pointGuid)},
    {"position",DamageIO::vectorJson(p.position)},{"incoming",DamageIO::vectorJson(p.incomingHandle)},
    {"outgoing",DamageIO::vectorJson(p.outgoingHandle)},{"width",p.widthMeters},{"roll",p.rollRadians}});
   entry["spatialSpline"]={{"schemaVersion",s.schemaVersion},{"guid",Jerico::formatGuid(s.splineGuid)},
    {"name",s.name},{"interpolation",int(s.interpolation)},{"closed",s.closed},{"points",points}};
  }
  entries.push_back(std::move(entry));
 }
 const auto text=DamageIO::Json{{"format","SplineConstructionDamage"},{"version",volumeUnion?3:spatial?2:1},{"paths",entries}}.dump(2);
 if(text.size()>DamageIO::maximumFileBytes)throw std::runtime_error("Damage recipe file too large");
 return text;
}
std::vector<PathDamageRule> deserializeSplineDamage(const std::string& text){
 if(text.size()>DamageIO::maximumFileBytes)throw std::runtime_error("Damage recipe file too large");
 const auto root=DamageIO::Json::parse(text);
 if(!root.is_object()||root.at("format")!="SplineConstructionDamage"||!root.at("version").is_number_integer()||(root.at("version")!=1&&root.at("version")!=2&&root.at("version")!=3))
  throw std::runtime_error("Unsupported spline damage recipe");
 const auto& entries=root.at("paths");
 if(!entries.is_array()||entries.size()>64)throw std::runtime_error("Invalid spline damage path collection");
 std::vector<PathDamageRule> paths;
 for(const auto& entry:entries){
  PathDamageRule rule{entry.at("id").get<std::string>(),DamageIO::readPathCutter(entry)};
  if(root.at("version")==3){
   const auto operation=entry.at("operation").get<std::string>();
   if(operation=="VolumeUnion")rule.operation=DamageVolumeOperation::Union;
   else if(operation!="VolumeDifference")throw std::runtime_error("Unsupported spatial decal operation");
  }else if(entry.contains("operation"))throw std::runtime_error("Explicit spatial operations require damage schema 3");
  if(entry.contains("spatialSpline")){
   if(root.at("version")==1)throw std::runtime_error("Spatial splines require damage schema 2 or 3");
   const auto&v=entry.at("spatialSpline");
   if(!v.at("schemaVersion").is_number_integer()||v.at("schemaVersion")!=1||
      !v.at("interpolation").is_number_integer()||v.at("interpolation")<0||v.at("interpolation")>2||
      !v.at("points").is_array()||v.at("points").size()<2||v.at("points").size()>64)
    throw std::runtime_error("Invalid spatial spline schema, interpolation or point count");
   auto guid=[](const DamageIO::Json&value){auto result=Jerico::parseGuid(value.get<std::string>());if(!result.hasValue())throw std::runtime_error("Invalid spatial spline identity");return result.value();};
   PA::ProceduralSpline s;s.splineGuid=guid(v.at("guid"));s.name=v.at("name").get<std::string>();
   s.closed=v.at("closed").get<bool>();s.interpolation=PA::SplineInterpolation(v.at("interpolation").get<int>());
   for(const auto&p:v.at("points")){
    PA::SplineControlPoint point;point.pointGuid=guid(p.at("guid"));point.position=DamageIO::readVector(p.at("position"));
    point.incomingHandle=DamageIO::readVector(p.at("incoming"));point.outgoingHandle=DamageIO::readVector(p.at("outgoing"));
    point.widthMeters=p.at("width").get<double>();point.rollRadians=p.at("roll").get<double>();s.points.push_back(point);
   }
   rule.spatialPath=std::move(s);
  }
  paths.push_back(std::move(rule));
 }
 validatePaths(paths);return paths;
}
void saveSplineDamage(const std::vector<PathDamageRule>& paths,const std::filesystem::path& file){
 DamageIO::writeFile(serializeSplineDamage(paths),file);
}
std::vector<PathDamageRule> loadSplineDamage(const std::filesystem::path& file){
 return deserializeSplineDamage(DamageIO::readFile(file));
}
ConstructionResult loadSplineDamage(const PA::ProceduralSpline& spline,ConstructionSettings& settings,const std::filesystem::path& file){
 auto candidate=settings;candidate.damagePaths=loadSplineDamage(file);
 auto built=buildConstruction(spline,candidate);settings=std::move(candidate);return built;
}
std::string nextSplineDamageId(const std::vector<PathDamageRule>& paths){
 if(paths.size()>=64)throw std::runtime_error("Spline damage path budget exceeded");
 for(unsigned i=1;i<=65;++i){auto id="spline-damage-"+std::to_string(i);
  if(std::none_of(paths.begin(),paths.end(),[&](const auto& rule){return rule.id==id;}))return id;}
 throw std::runtime_error("No free spline damage path ID");
}
}
