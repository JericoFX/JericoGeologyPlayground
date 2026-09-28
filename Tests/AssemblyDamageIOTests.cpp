#include "AssemblyDamageIO.hpp"
#include "ProceduralAssemblies.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using namespace Playground;
using namespace Playground::Assembly;

namespace
{
void require(bool condition,const char *message)
{if (!condition) throw std::runtime_error(message);}
template<class F> void rejects(F function,const char *message)
{
    try {function();}
    catch (const std::exception &) {return;}
    throw std::runtime_error(message);
}
}
int main()
{
    try
    {
    DamageSettings source;
    source.proceduralEnabled=true;source.seed=873;
    source.missingProbability=.1;source.brokenProbability=.2;
    source.looseProbability=.03;source.bentProbability=.04;
    source.clusterCenterMeters=1.4;source.clusterRadiusMeters=.8;
    source.clusterStrength=.35;
    source.rules.push_back({SelectorMode::DistanceRange,Role::DeckPlank,0,0,1.2,2.8,State::Loose});
    source.manual["WoodenPier/DeckPlank_007"]=State::Broken;
    source.protectedIds.insert("WoodenPier/DeckPlank_008");
    AreaDamageRule rule;
    rule.area.id="area-22";rule.area.semanticRole="DeckPlank";
    rule.area.indexRange={{5,9}};rule.area.pathRangeMeters={{1.1,3.5}};
    rule.area.projector.center={1,2,3};
    rule.area.projector.tangent={0,1,0};
    rule.area.projector.bitangent={0,0,1};
    rule.area.projector.normal={1,0,0};
    rule.area.projector.scale={2,3,.4};
    rule.area.projector.rotationRadians=.3;
    rule.area.footprint.radius={.6,.8,1};
    rule.area.footprint.innerRadius=.4;
    rule.area.footprint.outerRadius=1;
    rule.area.boundary.irregularity=.1;
    rule.area.boundary.frequency=3;
    rule.area.boundary.rimFraction=.2;
    rule.area.boundary.seed=12;
    rule.area.amount=.8;rule.state=State::Broken;
    source.areaRules.push_back(rule);
    PathDamageRule crack;
    crack.id="beam-crack-1";
    crack.semanticPartId="WoodenPier/DeckPlank_006";
    crack.cutter.projector.center={1,2,3};
    crack.cutter.path={{-.3,0,0},{0,.1,0},{.3,0,0}};
    crack.cutter.radius=.025;
    crack.cutter.depth=.04;
    source.pathRules.push_back(crack);
    auto restored=deserializeAssemblyDamage(serializeAssemblyDamage(source));
    require(serializeAssemblyDamage(restored)==serializeAssemblyDamage(source),
            "damage recipe must round trip exactly and deterministically");
    require(restored.pathRules.size()==1 && restored.pathRules.front().cutter.path.size()==3,
            "selected crack path must survive damage recipe roundtrip");
    auto shaped=source;
    shaped.pathRules.front().cutter.path={{-.3,0,0},{.3,0,0}};
    shaped.pathRules.front().cutter.irregularity=.7;
    shaped.pathRules.front().cutter.separationMeters=.1;
    shaped.pathRules.front().cutter.seed=827;
    const auto shapedText=serializeAssemblyDamage(shaped);
    require(nlohmann::json::parse(shapedText).at("version")==3 &&
            serializeAssemblyDamage(deserializeAssemblyDamage(shapedText))==shapedText,
            "irregular separated fracture must roundtrip as schema v3");
    auto falselyOld=nlohmann::json::parse(shapedText);
    falselyOld["version"]=2;
    rejects([&]{deserializeAssemblyDamage(falselyOld.dump());},
            "older schema must reject new fracture semantics");
    auto geometric=source;
    geometric.areaRules.front().operation=AreaOperation::Cavity;
    geometric.areaRules.front().state=State::Intact;
    const auto geometricText=serializeAssemblyDamage(geometric);
    require(nlohmann::json::parse(geometricText).at("version")==4 &&
            deserializeAssemblyDamage(geometricText).areaRules.front().operation==
                AreaOperation::Cavity &&
            serializeAssemblyDamage(deserializeAssemblyDamage(geometricText))==geometricText,
            "selected geometric cavity must persist as damage recipe v4");
    auto geometricOld=nlohmann::json::parse(geometricText);
    geometricOld["version"]=3;
    rejects([&]{deserializeAssemblyDamage(geometricOld.dump());},
            "older assembly recipes must reject cavity semantics");
    auto cavityOnly=geometric;
    cavityOnly.pathRules.clear();
    require(deserializeAssemblyDamage(serializeAssemblyDamage(cavityOnly)).pathRules.empty(),
            "v4 geometric cavity needs no crack path");
    for (const char *operation:{"Dent","ThroughHole"})
    {
        auto selected=nlohmann::json::parse(serializeAssemblyDamage(cavityOnly));
        selected["areas"][0]["operation"]=operation;
        const auto parsed=deserializeAssemblyDamage(selected.dump());
        require(nlohmann::json::parse(serializeAssemblyDamage(parsed))["areas"][0]
                    ["operation"]==operation,
                "selected geometric damage operation must roundtrip");
    }
    auto legacy=source;
    legacy.pathRules.clear();
    require(nlohmann::json::parse(serializeAssemblyDamage(legacy)).at("version")==1 &&
            deserializeAssemblyDamage(serializeAssemblyDamage(legacy)).pathRules.empty(),
            "legacy area-only damage recipe must stay version 1");
    auto oldJson=nlohmann::json::parse(serializeAssemblyDamage(source));
    oldJson.at("areas").at(0).erase("boundary");
    require(deserializeAssemblyDamage(oldJson.dump()).areaRules.front().area.boundary.irregularity==0,
            "old assembly damage files must retain the original circular boundary");
    const auto file=std::filesystem::current_path()/"assembly-damage-io-test.json";
    saveAssemblyDamage(source,file);
    require(serializeAssemblyDamage(loadAssemblyDamage(file))==serializeAssemblyDamage(source),
            "saved damage recipe must load without losing selection state");
    std::filesystem::remove(file);
    restored.proceduralEnabled=false;
    restored.rules.clear();
    Part selected;
    selected.id="WoodenPier/DeckPlank_006";
    selected.role=Role::DeckPlank;selected.index=6;
    selected.stationMeters=2;selected.position={1,2,3};
    require(resolveDamage(restored,selected)==State::Broken,
            "restored projector must select the same semantic part");
    restored.areaRules.front().area.projector.center={100,2,3};
    require(resolveDamage(restored,selected)!=State::Broken,
            "moving restored projector must change damage without modifying source");
    selected.id="WoodenPier/DeckPlank_008";selected.index=8;
    restored.areaRules.front().area.projector.center={1,2,3};
    require(resolveDamage(restored,selected)==State::Intact,
            "restored protected selection must override area damage");
    rejects([]{deserializeAssemblyDamage("{\"version\":5}");},"unknown schema version accepted");
    rejects([]{deserializeAssemblyDamage("{\"version\":1,\"areas\":[{}]}");},
            "incomplete area accepted");
    auto bad=source;bad.areaRules.front().area.projector.scale.x=0;
    rejects([&]{serializeAssemblyDamage(bad);},"zero projector scale accepted");
    bad=source;bad.areaRules.front().area.projector.center.x=std::nan("");
    rejects([&]{serializeAssemblyDamage(bad);},"NaN projector center accepted");
    bad=source;bad.areaRules.front().area.projector.tangent={1,1,0};
    rejects([&]{serializeAssemblyDamage(bad);},"non-orthonormal projector basis accepted");
    bad=source;bad.areaRules.front().area.footprint.lipInner=1;
    rejects([&]{serializeAssemblyDamage(bad);},"invalid lip range accepted");
    bad=source;bad.areaRules.front().area.boundary.irregularity=.3;
    rejects([&]{serializeAssemblyDamage(bad);},"invalid boundary irregularity accepted");
    bad=source;bad.areaRules.push_back(bad.areaRules.front());
    rejects([&]{serializeAssemblyDamage(bad);},"duplicate operation ID accepted");
    bad=source;bad.areaRules.resize(maximumDamageAreas+1);
    rejects([&]{serializeAssemblyDamage(bad);},"excessive area count accepted");
    bad=source;bad.pathRules.front().cutter.path[1].z=.1;
    rejects([&]{serializeAssemblyDamage(bad);},"path point outside projector plane accepted");
    bad=source;bad.pathRules.push_back(bad.pathRules.front());
    rejects([&]{serializeAssemblyDamage(bad);},"duplicate selected crack accepted");
    }
    catch (const std::exception &error)
    {std::cerr<<error.what()<<'\n';return 1;}
}
