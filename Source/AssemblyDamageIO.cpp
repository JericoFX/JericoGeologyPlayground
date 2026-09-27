#include "AssemblyDamageIO.hpp"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <unordered_set>

namespace Playground::Assembly
{
namespace
{
using Json=nlohmann::json;
constexpr std::size_t maximumDamageFileBytes=1024*1024;
void finite(double value)
{if (!std::isfinite(value)) throw std::runtime_error("Damage recipe contains a non-finite number");}
void unit(double value)
{finite(value);if (value<0 || value>1) throw std::runtime_error("Damage probability or amount is outside [0,1]");}
void checkId(const std::string &id)
{if (id.size()>256) throw std::runtime_error("Damage selector ID is too long");}
Json vectorJson(Vec3d value)
{return Json::array({value.x,value.y,value.z});}
Vec3d readVector(const Json &value)
{
    if (!value.is_array() || value.size()!=3)
        throw std::runtime_error("Damage vector must contain three numbers");
    Vec3d result{value.at(0).get<double>(),value.at(1).get<double>(),value.at(2).get<double>()};
    finite(result.x);finite(result.y);finite(result.z);
    return result;
}
Json writeArea(const AreaDamageRule &rule)
{
    const auto &area=rule.area;
    Json result={{"id",area.id},{"enabled",area.enabled},
        {"semanticPartId",area.semanticPartId},{"semanticRole",area.semanticRole},
        {"amount",area.amount},{"state",int(rule.state)},
        {"projector",{{"center",vectorJson(area.projector.center)},
            {"tangent",vectorJson(area.projector.tangent)},
            {"bitangent",vectorJson(area.projector.bitangent)},
            {"normal",vectorJson(area.projector.normal)},
            {"scale",vectorJson(area.projector.scale)},
            {"rotationRadians",area.projector.rotationRadians}}},
        {"footprint",{{"radius",vectorJson(area.footprint.radius)},
            {"innerRadius",area.footprint.innerRadius},
            {"outerRadius",area.footprint.outerRadius},
            {"depth",area.footprint.depth},
            {"profilePower",area.footprint.profilePower},
            {"profileShape",area.footprint.profileShape},
            {"lipInner",area.footprint.lipInner},
            {"lipOuter",area.footprint.lipOuter},
            {"lipHeight",area.footprint.lipHeight}}}};
    if (area.indexRange)
        result["indexRange"]=Json::array({area.indexRange->first,area.indexRange->second});
    if (area.pathRangeMeters)
        result["pathRangeMeters"]=Json::array({area.pathRangeMeters->first,area.pathRangeMeters->second});
    return result;
}
AreaDamageRule readArea(const Json &value)
{
    AreaDamageRule rule;
    auto &area=rule.area;
    area.id=value.at("id").get<std::string>();
    area.enabled=value.at("enabled").get<bool>();
    area.semanticPartId=value.at("semanticPartId").get<std::string>();
    area.semanticRole=value.at("semanticRole").get<std::string>();
    area.amount=value.at("amount").get<double>();
    rule.state=State(value.at("state").get<int>());
    if (value.contains("indexRange"))
    {
        const auto &range=value.at("indexRange");
        if (!range.is_array() || range.size()!=2)
            throw std::runtime_error("Invalid damage index range");
        area.indexRange={{range.at(0).get<std::uint32_t>(),range.at(1).get<std::uint32_t>()}};
    }
    if (value.contains("pathRangeMeters"))
    {
        const auto &range=value.at("pathRangeMeters");
        if (!range.is_array() || range.size()!=2)
            throw std::runtime_error("Invalid damage path range");
        area.pathRangeMeters={{range.at(0).get<double>(),range.at(1).get<double>()}};
    }
    const auto &projector=value.at("projector");
    area.projector.center=readVector(projector.at("center"));
    area.projector.tangent=readVector(projector.at("tangent"));
    area.projector.bitangent=readVector(projector.at("bitangent"));
    area.projector.normal=readVector(projector.at("normal"));
    area.projector.scale=readVector(projector.at("scale"));
    area.projector.rotationRadians=projector.at("rotationRadians").get<double>();
    const auto &footprint=value.at("footprint");
    area.footprint.radius=readVector(footprint.at("radius"));
    area.footprint.innerRadius=footprint.at("innerRadius").get<double>();
    area.footprint.outerRadius=footprint.at("outerRadius").get<double>();
    area.footprint.depth=footprint.at("depth").get<double>();
    area.footprint.profilePower=footprint.at("profilePower").get<double>();
    area.footprint.profileShape=footprint.at("profileShape").get<double>();
    area.footprint.lipInner=footprint.at("lipInner").get<double>();
    area.footprint.lipOuter=footprint.at("lipOuter").get<double>();
    area.footprint.lipHeight=footprint.at("lipHeight").get<double>();
    return rule;
}
void validate(const DamageSettings &settings)
{
    if (settings.areaRules.size()>maximumDamageAreas || settings.rules.size()>4096 ||
        settings.manual.size()>4096 || settings.protectedIds.size()>4096)
        throw std::runtime_error("Damage selection budget exceeded");
    unit(settings.missingProbability);unit(settings.brokenProbability);
    unit(settings.looseProbability);unit(settings.bentProbability);
    unit(settings.clusterStrength);
    if (settings.missingProbability+settings.brokenProbability+
        settings.looseProbability+settings.bentProbability>1+1e-12)
        throw std::runtime_error("Damage probabilities exceed one");
    finite(settings.clusterCenterMeters);finite(settings.clusterRadiusMeters);
    if (settings.clusterRadiusMeters<0)
        throw std::runtime_error("Negative damage cluster radius");
    for (const auto &rule:settings.rules)
    {
        if (int(rule.mode)<0 || int(rule.mode)>int(SelectorMode::DistanceRange) ||
            int(rule.role)<0 || int(rule.role)>int(Role::Anchor) ||
            int(rule.state)<0 || int(rule.state)>int(State::Collapsed))
            throw std::runtime_error("Invalid damage rule enum");
        finite(rule.startMeters);finite(rule.endMeters);
        if (rule.firstIndex>rule.lastIndex && rule.mode==SelectorMode::IndexRange)
            throw std::runtime_error("Invalid damage rule index range");
        if (rule.startMeters>rule.endMeters && rule.mode==SelectorMode::DistanceRange)
            throw std::runtime_error("Invalid damage rule distance range");
    }
    std::unordered_set<std::string> areaIds;
    for (const auto &rule:settings.areaRules)
    {
        const auto &area=rule.area;
        checkId(area.id);checkId(area.semanticPartId);checkId(area.semanticRole);
        if (area.id.empty() || int(rule.state)<0 || int(rule.state)>int(State::Collapsed))
            throw std::runtime_error("Invalid damage area identity or state");
        if (!areaIds.insert(area.id).second)
            throw std::runtime_error("Duplicate damage area ID");
        unit(area.amount);
        for (const auto value:{area.projector.center,area.projector.tangent,
            area.projector.bitangent,area.projector.normal,area.projector.scale,
            area.footprint.radius})
        {finite(value.x);finite(value.y);finite(value.z);}
        finite(area.projector.rotationRadians);
        if (area.projector.scale.x<=0 || area.projector.scale.y<=0 ||
            area.projector.scale.z<=0 || area.footprint.radius.x<=0 ||
            area.footprint.radius.y<=0 || area.footprint.radius.z<=0)
            throw std::runtime_error("Invalid damage area scale or radius");
        for (double value:{area.footprint.innerRadius,area.footprint.outerRadius,
            area.footprint.depth,area.footprint.profilePower,area.footprint.profileShape,
            area.footprint.lipInner,area.footprint.lipOuter,area.footprint.lipHeight})
            finite(value);
        if (area.footprint.innerRadius<0 ||
            area.footprint.outerRadius<=area.footprint.innerRadius ||
            area.footprint.depth<0 || area.footprint.profilePower<=0 ||
            area.footprint.profileShape<=0 || area.footprint.lipInner<0 ||
            area.footprint.lipOuter<area.footprint.lipInner)
            throw std::runtime_error("Invalid damage area footprint");
        if (area.indexRange && area.indexRange->first>area.indexRange->second)
            throw std::runtime_error("Invalid damage area index range");
        if (area.pathRangeMeters)
        {
            finite(area.pathRangeMeters->first);finite(area.pathRangeMeters->second);
            if (area.pathRangeMeters->first>area.pathRangeMeters->second)
                throw std::runtime_error("Invalid damage area path range");
        }
        // Ask the shared primitives to validate their own geometry contract.
        projectorCoordinates(area.projector,area.projector.center);
        sampleDamageDecal({},area.footprint);
    }
    for (const auto &[id,state]:settings.manual)
    {checkId(id);if (int(state)<0 || int(state)>int(State::Collapsed))
        throw std::runtime_error("Invalid manual damage state");}
    for (const auto &id:settings.protectedIds) checkId(id);
}
}

std::string serializeAssemblyDamage(const DamageSettings &settings)
{
    validate(settings);
    Json result={{"version",1},{"proceduralEnabled",settings.proceduralEnabled},
        {"seed",settings.seed},{"missingProbability",settings.missingProbability},
        {"brokenProbability",settings.brokenProbability},
        {"looseProbability",settings.looseProbability},
        {"bentProbability",settings.bentProbability},
        {"clusterCenterMeters",settings.clusterCenterMeters},
        {"clusterRadiusMeters",settings.clusterRadiusMeters},
        {"clusterStrength",settings.clusterStrength}};
    result["rules"]=Json::array();
    for (const auto &rule:settings.rules)
        result["rules"].push_back({{"mode",int(rule.mode)},{"role",int(rule.role)},
            {"firstIndex",rule.firstIndex},{"lastIndex",rule.lastIndex},
            {"startMeters",rule.startMeters},{"endMeters",rule.endMeters},
            {"state",int(rule.state)}});
    result["areas"]=Json::array();
    for (const auto &rule:settings.areaRules) result["areas"].push_back(writeArea(rule));
    result["manual"]=Json::object();
    for (const auto &[id,state]:settings.manual) result["manual"][id]=int(state);
    std::vector<std::string> protectedIds(settings.protectedIds.begin(),settings.protectedIds.end());
    std::sort(protectedIds.begin(),protectedIds.end());
    result["protectedIds"]=protectedIds;
    auto text=result.dump(2);
    if (text.size()>maximumDamageFileBytes)
        throw std::runtime_error("Damage recipe file too large");
    return text;
}
DamageSettings deserializeAssemblyDamage(const std::string &text)
{
    if (text.size()>maximumDamageFileBytes)
        throw std::runtime_error("Damage recipe file too large");
    const auto root=Json::parse(text);
    if (root.at("version").get<int>()!=1)
        throw std::runtime_error("Unsupported damage recipe version");
    DamageSettings result;
    result.proceduralEnabled=root.at("proceduralEnabled").get<bool>();
    result.seed=root.at("seed").get<std::uint32_t>();
    result.missingProbability=root.at("missingProbability").get<double>();
    result.brokenProbability=root.at("brokenProbability").get<double>();
    result.looseProbability=root.at("looseProbability").get<double>();
    result.bentProbability=root.at("bentProbability").get<double>();
    result.clusterCenterMeters=root.at("clusterCenterMeters").get<double>();
    result.clusterRadiusMeters=root.at("clusterRadiusMeters").get<double>();
    result.clusterStrength=root.at("clusterStrength").get<double>();
    const auto &rules=root.at("rules");
    const auto &areas=root.at("areas");
    const auto &manual=root.at("manual");
    const auto &protectedIds=root.at("protectedIds");
    if (!rules.is_array() || !areas.is_array() || !manual.is_object() ||
        !protectedIds.is_array() || rules.size()>4096 || areas.size()>maximumDamageAreas ||
        manual.size()>4096 || protectedIds.size()>4096)
        throw std::runtime_error("Invalid damage selection collection");
    for (const auto &entry:rules)
        result.rules.push_back({SelectorMode(entry.at("mode").get<int>()),
            Role(entry.at("role").get<int>()),entry.at("firstIndex").get<std::uint32_t>(),
            entry.at("lastIndex").get<std::uint32_t>(),
            entry.at("startMeters").get<double>(),entry.at("endMeters").get<double>(),
            State(entry.at("state").get<int>())});
    for (const auto &entry:areas) result.areaRules.push_back(readArea(entry));
    for (auto it=manual.begin();it!=manual.end();++it)
        result.manual[it.key()]=State(it.value().get<int>());
    for (const auto &entry:protectedIds)
        result.protectedIds.insert(entry.get<std::string>());
    validate(result);
    return result;
}
void saveAssemblyDamage(const DamageSettings &settings,const std::filesystem::path &path)
{
    auto text=serializeAssemblyDamage(settings);
    auto temporary=path;
    temporary+=L".tmp";
    {
        std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
        if (!out || !(out<<text<<'\n'))
            throw std::runtime_error("Could not write damage recipe");
        out.flush();
        if (!out) throw std::runtime_error("Could not flush damage recipe");
    }
    if (!MoveFileExW(temporary.c_str(),path.c_str(),
                     MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Could not replace damage recipe file");
}
DamageSettings loadAssemblyDamage(const std::filesystem::path &path)
{
    if (std::filesystem::file_size(path)>maximumDamageFileBytes)
        throw std::runtime_error("Damage recipe file too large");
    std::ifstream in(path,std::ios::binary);
    if (!in) throw std::runtime_error("Could not open damage recipe");
    return deserializeAssemblyDamage(std::string(std::istreambuf_iterator<char>(in),{}));
}
}
