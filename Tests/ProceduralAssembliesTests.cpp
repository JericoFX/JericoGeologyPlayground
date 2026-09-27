#include "ProceduralAssemblies.hpp"
#include "SolidGeometry.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace Playground;
using namespace Playground::Assembly;

namespace
{
void require(bool condition,const char *message)
{if (!condition) throw std::runtime_error(message);}
PA::ProceduralSpline curvedPath()
{
    PA::ProceduralSpline spline;
    spline.splineGuid=Solid::region(2201,1);
    spline.name="Assembly path fixture";
    spline.interpolation=PA::SplineInterpolation::CatmullRom;
    const Vec3d positions[]={{0,0,0},{2,0,.1},{4,0,-.1},{6,0,0}};
    for (unsigned i=0;i<4;++i)
    {
        PA::SplineControlPoint point;
        point.pointGuid=Solid::region(2201,i+2);
        point.position=positions[i];
        spline.points.push_back(point);
    }
    return spline;
}
double length(Vec3d a)
{return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);}
Vec3d sub(Vec3d a,Vec3d b)
{return {a.x-b.x,a.y-b.y,a.z-b.z};}
double dot(Vec3d a,Vec3d b)
{return a.x*b.x+a.y*b.y+a.z*b.z;}
}
int main()
{
    try
    {
        const auto fit=fitCountSpacing(3.0,.25,.05);
        require(fit.count==10 && std::abs(fit.actualWidthMeters-.255)<1e-12,
                "board fit must use the documented count/width equation");
        const auto path=Construction::samplePath(curvedPath(),{});
        const auto left=offsetPath(path,0,-.7175),right=offsetPath(path,0,.7175);
        require(left.frames.size()==path.frames.size() && right.frames.size()==path.frames.size(),
                "offset paths must preserve station correspondence");
        for (std::size_t i=0;i<path.frames.size();++i)
        {
            const auto separation=sub(right.frames[i].position,left.frames[i].position);
            require(std::abs(length(separation)-1.435)<1e-8 &&
                    std::abs(dot(separation,path.frames[i].tangent))<1e-8,
                    "rail gauge must stay perpendicular to centerline tangent");
        }
        const auto id=stablePartId(Assembly::Kind::WoodenPier,Role::DeckPlank,18);
        require(id==stablePartId(Assembly::Kind::WoodenPier,Role::DeckPlank,18) &&
                id!=stablePartId(Assembly::Kind::WoodenPier,Role::DeckPlank,19),
                "semantic IDs must be stable and distinguish logical parts");
        Part plank;
        plank.id=id;plank.role=Role::DeckPlank;plank.index=18;plank.stationMeters=5;
        DamageSettings damage;
        damage.proceduralEnabled=true;damage.seed=123;
        damage.missingProbability=.3;damage.brokenProbability=.2;
        const auto seeded=resolveDamage(damage,plank);
        require(seeded==resolveDamage(damage,plank),"seeded damage must be deterministic");
        damage.manual[id]=State::Broken;
        require(resolveDamage(damage,plank)==State::Broken,
                "manual state must override procedural damage");
        damage.protectedIds.insert(id);
        require(resolveDamage(damage,plank)==State::Broken,
                "protected manual state must survive regeneration");
        damage.manual.erase(id);
        require(resolveDamage(damage,plank)==State::Intact,
                "protected unmodified part must ignore procedural damage");
        damage.protectedIds.clear();damage.proceduralEnabled=false;
        damage.rules.push_back({SelectorMode::IndexRange,Role::DeckPlank,12,18,0,0,State::Missing});
        require(resolveDamage(damage,plank)==State::Missing,
                "index range selector must include endpoint board");
        damage.rules.clear();
        damage.rules.push_back({SelectorMode::DistanceRange,Role::DeckPlank,0,0,4.9,5.1,State::Loose});
        require(resolveDamage(damage,plank)==State::Loose,
                "distance selector must use part station");
        require(std::abs(parabolicSag(0,12,1.2))<1e-12 &&
                std::abs(parabolicSag(6,12,1.2)-1.2)<1e-12 &&
                std::abs(parabolicSag(3,12,1.2)-parabolicSag(9,12,1.2))<1e-12,
                "suspension parabola endpoints, sag and symmetry must hold");
        const auto a=solveCatenaryA(12,1.2);
        require(std::isfinite(a) && a>0 &&
                std::abs(catenaryHeight(6,12,1.2,a)-1.2)<1e-7 &&
                std::abs(catenaryHeight(0,12,1.2,a))<1e-9,
                "bounded catenary solve must match anchors and requested sag");
        bool rejected=false;
        try {solveCatenaryA(12,0);} catch (const std::exception &) {rejected=true;}
        require(rejected,"invalid zero sag must be rejected");
        Settings pipe;
        pipe.kind=Assembly::Kind::PipeCouplings;
        pipe.couplingSpacingMeters=1;
        const auto coupled=build(curvedPath(),pipe);
        std::vector<Part> collars;
        for (const auto &part:coupled.parts)
            if (part.role==Role::Coupling) collars.push_back(part);
        require(collars.size()>=5,"pipe run needs independently selectable couplings");
        for (std::size_t i=1;i<collars.size();++i)
            require(std::abs(collars[i].stationMeters-collars[i-1].stationMeters-1)<1e-9,
                    "couplings must use one-metre arc-length stationing");
        Settings railway;
        railway.kind=Assembly::Kind::Railway;
        railway.sleeperSpacingMeters=.6;
        const auto track=build(curvedPath(),railway);
        std::size_t sleepers=0,rails=0;
        for (const auto &part:track.parts)
        {
            if (part.role==Role::Sleeper) ++sleepers;
            if (part.role==Role::Rail) ++rails;
        }
        require(sleepers>=9 && rails>=2,"railway must have selectable sleepers and rails");
        railway.damage.manual[stablePartId(railway.kind,Role::Sleeper,2)]=State::Missing;
        const auto brokenTrack=build(curvedPath(),railway);
        bool missingSleeper=false;
        for (const auto &part:brokenTrack.parts)
            if (part.role==Role::Sleeper && part.index==2)
                missingSleeper=part.state==State::Missing && part.triangleCount==0;
        require(missingSleeper,"missing sleeper must leave an actual geometry gap");
        Settings pier;
        pier.kind=Assembly::Kind::WoodenPier;
        const auto intactPier=build(curvedPath(),pier);
        const auto expected=fitCountSpacing(intactPier.path.lengthMeters,pier.boardWidthMeters,pier.boardGapMeters);
        std::size_t boards=0;
        for (const auto &part:intactPier.parts)
            if (part.role==Role::DeckPlank) ++boards;
        require(boards==expected.count,"pier board fit must use true path length");
        const auto boardId=stablePartId(pier.kind,Role::DeckPlank,3);
        pier.damage.manual[boardId]=State::Broken;
        auto damagedPier=build(curvedPath(),pier);
        pier.lod=2;
        auto distantPier=build(curvedPath(),pier);
        bool brokenNear=false,brokenFar=false;
        for (const auto &part:damagedPier.parts)
            if (part.id==boardId) brokenNear=part.state==State::Broken && part.triangleCount>0;
        for (const auto &part:distantPier.parts)
            if (part.id==boardId) brokenFar=part.state==State::Broken && part.triangleCount>0;
        require(brokenNear && brokenFar,"manual broken board must survive LOD changes");
        for (int family=0;family<=int(Assembly::Kind::RopePlankBridge);++family)
        {
            Settings caseSettings;
            caseSettings.kind=Assembly::Kind(family);
            const auto near=build(curvedPath(),caseSettings);
            caseSettings.lod=2;
            const auto far=build(curvedPath(),caseSettings);
            require(!near.parts.empty() && near.parts.size()==far.parts.size(),
                    "every assembly family needs stable semantic part count across LOD");
            require(near.mesh.mesh.indices.size()==near.cornerUV.size() &&
                    near.mesh.mesh.indices.size()==near.mesh.cornerNormals.size() &&
                    near.mesh.mesh.triangleRegions.size()==near.mesh.mesh.indices.size()/3,
                    "every assembly triangle needs a normal, UV and semantic region");
            for (std::size_t i=0;i<near.parts.size();++i)
                require(near.parts[i].id==far.parts[i].id &&
                        near.parts[i].guid==far.parts[i].guid &&
                        near.parts[i].state==far.parts[i].state,
                        "LOD must preserve stable part identity and damage");
            for (auto position:near.mesh.mesh.positions)
                require(std::isfinite(position.x) && std::isfinite(position.y) &&
                        std::isfinite(position.z),"assembly position must remain finite");
            for (auto normal:near.mesh.cornerNormals)
                require(std::isfinite(normal.x) && std::isfinite(normal.y) &&
                        std::isfinite(normal.z),"assembly normal must remain finite");
            for (std::size_t triangle=0;triangle<near.mesh.mesh.indices.size();triangle+=3)
            {
                const auto a=near.mesh.mesh.positions[near.mesh.mesh.indices[triangle]];
                const auto b=near.mesh.mesh.positions[near.mesh.mesh.indices[triangle+1]];
                const auto c=near.mesh.mesh.positions[near.mesh.mesh.indices[triangle+2]];
                const auto ab=sub(b,a),ac=sub(c,a);
                const auto squared=dot(ab,ab)*dot(ac,ac)-dot(ab,ac)*dot(ab,ac);
                require(squared>1e-18,"assembly must not contain degenerate triangles");
            }
        }
        Settings railing;
        railing.kind=Assembly::Kind::Railing;
        const auto railId=stablePartId(railing.kind,Role::TopRail,1,-1);
        railing.damage.manual[railId]=State::Missing;
        const auto gap=build(curvedPath(),railing);
        bool actualGap=false;
        for (const auto &part:gap.parts)
            if (part.id==railId) actualGap=part.triangleCount==0;
        require(actualGap,"missing railing bay must remove only that member's geometry");
        Settings suspension;
        suspension.kind=Assembly::Kind::SuspensionBridge;
        const auto suspended=build(curvedPath(),suspension);
        std::size_t hangers=0;
        for (const auto &part:suspended.parts)
        {
            if (part.role!=Role::Hanger) continue;
            ++hangers;
            const auto frame=Construction::frameAtDistance(suspended.path,part.stationMeters);
            double minimum=1e20,maximum=-1e20;
            for (std::size_t triangle=part.firstTriangle;
                 triangle<part.firstTriangle+part.triangleCount;++triangle)
                for (unsigned corner=0;corner<3;++corner)
                {
                    const auto vertex=suspended.mesh.mesh.positions[
                        suspended.mesh.mesh.indices[triangle*3+corner]];
                    const auto height=dot(sub(vertex,frame.position),frame.normal);
                    minimum=std::min(minimum,height);maximum=std::max(maximum,height);
                }
            const auto cable=suspension.towerHeightMeters-
                parabolicSag(part.stationMeters,suspended.path.lengthMeters,
                             suspension.sagMeters);
            require(std::abs(minimum)<.06 && std::abs(maximum-cable)<.06,
                    "hanger geometry must terminate at deck and parabolic cable");
        }
        require(hangers>=20,"suspension bridge must expose individual hanger stations");
        const auto cableId=stablePartId(suspension.kind,Role::Cable,0,-1);
        suspension.damage.manual[cableId]=State::Broken;
        const auto cutCable=build(curvedPath(),suspension);
        std::size_t intactCableTriangles=0,cutCableTriangles=0;
        for (const auto &part:suspended.parts)
            if (part.id==cableId) intactCableTriangles=part.triangleCount;
        for (const auto &part:cutCable.parts)
            if (part.id==cableId) cutCableTriangles=part.triangleCount;
        require(cutCableTriangles>0 && cutCableTriangles<intactCableTriangles,
                "broken suspension cable must have a real geometry gap");
        Settings rope;
        rope.kind=Assembly::Kind::RopePlankBridge;
        rope.damage.manual[stablePartId(rope.kind,Role::DeckPlank,5)]=State::Missing;
        const auto ropeGap=build(curvedPath(),rope);
        bool missingRopeBoard=false;
        for (const auto &part:ropeGap.parts)
            if (part.role==Role::DeckPlank && part.index==5)
                missingRopeBoard=part.triangleCount==0;
        require(missingRopeBoard,"missing rope bridge plank must produce a real gap");
        bool invalid=false;
        try {auto bad=railway;bad.gaugeMeters=bad.sleeperLengthMeters;
             (void)build(curvedPath(),bad);} catch (const std::exception &) {invalid=true;}
        require(invalid,"impossible railway gauge must be rejected");
        invalid=false;
        try {auto bad=pipe;bad.couplingSpacingMeters=0;
             (void)build(curvedPath(),bad);} catch (const std::exception &) {invalid=true;}
        require(invalid,"zero coupling spacing must be rejected");
        invalid=false;
        try {auto bad=rope;bad.sagMeters=0;
             (void)build(curvedPath(),bad);} catch (const std::exception &) {invalid=true;}
        require(invalid,"zero catenary sag must be rejected");
        pipe.couplingVariationMeters=.005;
        const auto healthy=build(curvedPath(),pipe);
        pipe.damage.seed=999;
        const auto unrelatedSeed=build(curvedPath(),pipe);
        require(meshHash(healthy.mesh.mesh)==meshHash(unrelatedSeed.mesh.mesh),
                "damage seed must not alter healthy coupling variation");
        pipe.damage.manual[stablePartId(pipe.kind,Role::PipeBody,0)]=State::Broken;
        const auto cutPipe=build(curvedPath(),pipe);
        std::size_t intactPipeTriangles=0,brokenPipeTriangles=0;
        for (const auto &part:healthy.parts)
            if (part.role==Role::PipeBody) intactPipeTriangles=part.triangleCount;
        for (const auto &part:cutPipe.parts)
            if (part.role==Role::PipeBody) brokenPipeTriangles=part.triangleCount;
        require(brokenPipeTriangles>0 && brokenPipeTriangles<intactPipeTriangles,
                "broken pipe body must have a visible gap");
        require(!stateAllowed(Role::Coupling,State::Bent) &&
                stateAllowed(Role::Rail,State::Bent),
                "state menu must expose only represented member damage");
        auto oversized=path;
        oversized.frames.resize(4097);
        invalid=false;
        try {(void)offsetPath(oversized,0,1);} catch (const std::exception &) {invalid=true;}
        require(invalid,"offset paths beyond validated intersection budget must reject");
        std::cout<<"PASS assembly stationing, offset gauge, damage identity and bridge curves\n";
        return 0;
    }
    catch (const std::exception &error)
    {std::cerr<<error.what()<<'\n';return 1;}
}
