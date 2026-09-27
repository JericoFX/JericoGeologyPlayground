#include "ProceduralAssemblies.hpp"
#include "DamageField.hpp"
#include "SolidGeometry.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
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
double partVolume(const Result &result,const Part &part)
{
    double sum=0;
    const auto &mesh=result.mesh.mesh;
    for (std::size_t face=part.firstTriangle;face<part.firstTriangle+part.triangleCount;++face)
    {
        const auto a=mesh.positions[mesh.indices[3*face]];
        const auto b=mesh.positions[mesh.indices[3*face+1]];
        const auto c=mesh.positions[mesh.indices[3*face+2]];
        sum+=a.x*(b.y*c.z-b.z*c.y)+a.y*(b.z*c.x-b.x*c.z)+
             a.z*(b.x*c.y-b.y*c.x);
    }
    return sum/6;
}
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
        const auto sharedSample=damageRandom01(damage.seed,plank.id,"state");
        damage.missingProbability=sharedSample-1e-12;
        damage.brokenProbability=0;
        require(resolveDamage(damage,plank)==State::Intact,
                "assembly state must use the shared seeded damage stream");
        damage.missingProbability=sharedSample+1e-12;
        require(resolveDamage(damage,plank)==State::Missing,
                "shared seeded damage threshold must select the semantic part");
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
        damage.rules.clear();
        DamageArea selectedArea;
        selectedArea.id="plank-break";
        selectedArea.footprint.radius={.3,.3,1};
        selectedArea.semanticPartId=id;
        plank.position={0,0,0};
        damage.areaRules.push_back({selectedArea,State::Broken});
        require(resolveDamage(damage,plank)==State::Broken,
                "shared damage area must select a semantic plank");
        damage.areaRules.front().area.projector.center={2,0,0};
        require(resolveDamage(damage,plank)==State::Intact,
                "moving the damage area must restore the previously selected plank");
        plank.position={2,0,0};
        require(resolveDamage(damage,plank)==State::Broken,
                "moving the damage area must affect the new position");
        damage.protectedIds.insert(id);
        require(resolveDamage(damage,plank)==State::Intact,
                "protected plank must ignore movable procedural area");
        damage.areaRules.clear();damage.protectedIds.clear();
        Settings oversizedAreas;
        oversizedAreas.damage.areaRules.resize(65);
        bool areaBudgetRejected=false;
        try {build(curvedPath(),oversizedAreas);}
        catch (const std::exception &) {areaBudgetRejected=true;}
        require(areaBudgetRejected,"damage area budget must be bounded before generation");
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
        const auto pipeBody=std::find_if(coupled.parts.begin(),coupled.parts.end(),
            [](const Part &part){return part.role==Role::PipeBody;});
        require(pipeBody!=coupled.parts.end() &&
                    length(sub(pipeBody->position,
                        Construction::frameAtDistance(coupled.path,coupled.path.lengthMeters*.5).position))<1e-8,
                "pipe body selection center must follow the middle of its spline");
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
        const auto railPart=std::find_if(track.parts.begin(),track.parts.end(),
            [](const Part &part){return part.role==Role::Rail && part.side==1;});
        require(railPart!=track.parts.end(),"rail fixture needs a right rail");
        const auto railFrame=Construction::frameAtDistance(track.path,railPart->stationMeters);
        const Vec3d railCenter{
            railFrame.position.x+railFrame.binormal.x*railway.gaugeMeters*.5+
                railFrame.normal.x*railway.railHeightMeters*.5,
            railFrame.position.y+railFrame.binormal.y*railway.gaugeMeters*.5+
                railFrame.normal.y*railway.railHeightMeters*.5,
            railFrame.position.z+railFrame.binormal.z*railway.gaugeMeters*.5+
                railFrame.normal.z*railway.railHeightMeters*.5};
        require(length(sub(railPart->position,railCenter))<1e-8,
                "rail area selection point must follow the actual side rail");
        Settings railingSettings;
        railingSettings.kind=Assembly::Kind::Railing;
        const auto railingResult=build(curvedPath(),railingSettings);
        const auto post=std::find_if(railingResult.parts.begin(),railingResult.parts.end(),
            [](const Part &part){return part.role==Role::Post && part.side==1;});
        require(post!=railingResult.parts.end(),"railing fixture needs a side post");
        const auto postFrame=Construction::frameAtDistance(railingResult.path,post->stationMeters);
        const Vec3d postCenter{
            postFrame.position.x+postFrame.binormal.x*railingSettings.widthMeters*.5+
                postFrame.normal.x*railingSettings.heightMeters*.5,
            postFrame.position.y+postFrame.binormal.y*railingSettings.widthMeters*.5+
                postFrame.normal.y*railingSettings.heightMeters*.5,
            postFrame.position.z+postFrame.binormal.z*railingSettings.widthMeters*.5+
                postFrame.normal.z*railingSettings.heightMeters*.5};
        require(length(sub(post->position,postCenter))<1e-8,
                "railing damage area must follow the side post rather than the path center");
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
        const auto intactBoard=std::find_if(intactPier.parts.begin(),intactPier.parts.end(),
            [](const Part &part){return part.role==Role::DeckPlank;});
        require(intactBoard!=intactPier.parts.end() && intactBoard->triangleCount>12,
                "near deck planks need bevelled edges rather than plain boxes");
        Settings crackedPier=pier;
        PathDamageRule plankCrack;
        plankCrack.id="plank-crack-0";
        plankCrack.semanticPartId=intactBoard->id;
        const auto boardFrame=Construction::frameAtDistance(intactPier.path,
            intactBoard->stationMeters);
        plankCrack.cutter.projector.center={
            intactBoard->position.x+boardFrame.normal.x*pier.boardThicknessMeters*.5,
            intactBoard->position.y+boardFrame.normal.y*pier.boardThicknessMeters*.5,
            intactBoard->position.z+boardFrame.normal.z*pier.boardThicknessMeters*.5};
        plankCrack.cutter.projector.tangent=boardFrame.tangent;
        plankCrack.cutter.projector.normal=boardFrame.normal;
        plankCrack.cutter.projector.bitangent={
            boardFrame.normal.y*boardFrame.tangent.z-boardFrame.normal.z*boardFrame.tangent.y,
            boardFrame.normal.z*boardFrame.tangent.x-boardFrame.normal.x*boardFrame.tangent.z,
            boardFrame.normal.x*boardFrame.tangent.y-boardFrame.normal.y*boardFrame.tangent.x};
        plankCrack.cutter.path={{0,-.35,0},{0,.35,0}};
        plankCrack.cutter.radius=.025;
        plankCrack.cutter.depth=.04;
        crackedPier.damage.pathRules.push_back(plankCrack);
        const auto cutPier=build(curvedPath(),crackedPier);
        require(meshHash(cutPier.mesh.mesh)!=meshHash(intactPier.mesh.mesh) &&
                cutPier.parts.size()==intactPier.parts.size() &&
                cutPier.cornerUV.size()==cutPier.mesh.mesh.indices.size() &&
                cutPier.mesh.cornerNormals.size()==cutPier.mesh.mesh.indices.size(),
                "selected plank crack must change geometry with complete UV and normals");
        require(std::any_of(cutPier.mesh.regions.begin(),cutPier.mesh.regions.end(),
                [](const RegionInfo &region){return region.role==SurfaceRole::DamageInterior;}),
                "selected plank crack must record fresh interior lineage");
        require(partVolume(cutPier,cutPier.parts.front())>0 &&
                partVolume(cutPier,cutPier.parts.front())<
                    partVolume(intactPier,intactPier.parts.front()) &&
                cutPier.parts.front().freshInteriorRegions.size()==1,
                "selected plank crack must remove material and retain selectable fresh faces");
        for (std::size_t i=0;i<cutPier.parts.size();++i)
        {
            require(cutPier.parts[i].id==intactPier.parts[i].id &&
                    cutPier.parts[i].guid==intactPier.parts[i].guid,
                    "plank crack must preserve every stable semantic part identity");
            if (i)
            {
                const auto before=intactPier.parts[i].firstTriangle*3;
                const auto after=cutPier.parts[i].firstTriangle*3;
                require(cutPier.parts[i].triangleCount==intactPier.parts[i].triangleCount &&
                        std::equal(intactPier.cornerUV.begin()+std::ptrdiff_t(before),
                                   intactPier.cornerUV.begin()+std::ptrdiff_t(
                                       before+3*intactPier.parts[i].triangleCount),
                                   cutPier.cornerUV.begin()+std::ptrdiff_t(after)),
                        "cracking one plank must preserve UVs on other parts");
            }
        }
        require(meshHash(cutPier.mesh.mesh)==meshHash(build(curvedPath(),crackedPier).mesh.mesh),
                "selected plank crack must regenerate deterministically");
        crackedPier.damage.pathRules.front().cutter.projector.center.x+=100;
        require(meshHash(build(curvedPath(),crackedPier).mesh.mesh)==
                meshHash(intactPier.mesh.mesh),
                "moving plank crack off part must restore original geometry");
        crackedPier.damage.pathRules.front().cutter=plankCrack.cutter;
        crackedPier.damage.protectedIds.insert(intactBoard->id);
        require(meshHash(build(curvedPath(),crackedPier).mesh.mesh)==
                meshHash(intactPier.mesh.mesh),
                "locked plank must reject selected crack without changing source");
        crackedPier.damage.protectedIds.clear();
        const auto secondBoard=std::find_if(intactPier.parts.begin(),intactPier.parts.end(),
            [](const Part &part){return part.role==Role::DeckPlank && part.index==1;});
        require(secondBoard!=intactPier.parts.end(),"crack reassignment needs another plank");
        const auto secondFrame=Construction::frameAtDistance(intactPier.path,
            secondBoard->stationMeters);
        auto &reassigned=crackedPier.damage.pathRules.front();
        reassigned.semanticPartId=secondBoard->id;
        reassigned.cutter.projector.center={
            secondBoard->position.x+secondFrame.normal.x*pier.boardThicknessMeters*.5,
            secondBoard->position.y+secondFrame.normal.y*pier.boardThicknessMeters*.5,
            secondBoard->position.z+secondFrame.normal.z*pier.boardThicknessMeters*.5};
        reassigned.cutter.projector.tangent=secondFrame.tangent;
        reassigned.cutter.projector.normal=secondFrame.normal;
        reassigned.cutter.projector.bitangent={
            secondFrame.normal.y*secondFrame.tangent.z-secondFrame.normal.z*secondFrame.tangent.y,
            secondFrame.normal.z*secondFrame.tangent.x-secondFrame.normal.x*secondFrame.tangent.z,
            secondFrame.normal.x*secondFrame.tangent.y-secondFrame.normal.y*secondFrame.tangent.x};
        const auto movedPier=build(curvedPath(),crackedPier);
        require(std::abs(partVolume(movedPier,movedPier.parts.front())-
                         partVolume(intactPier,intactPier.parts.front()))<1e-8 &&
                partVolume(movedPier,movedPier.parts[1])<
                    partVolume(intactPier,intactPier.parts[1]),
                "reassigning a crack must restore the old plank and cut the selected plank");
        const auto beamPart=std::find_if(intactPier.parts.begin(),intactPier.parts.end(),
            [](const Part &part){return part.role==Role::Beam;});
        require(beamPart!=intactPier.parts.end(),"shared wood beam fixture missing");
        auto beamCrack=plankCrack;
        beamCrack.id="beam-crack-0";
        beamCrack.semanticPartId=beamPart->id;
        const auto beamFrame=Construction::frameAtDistance(intactPier.path,
            beamPart->stationMeters);
        beamCrack.cutter.projector.center={
            beamPart->position.x+beamFrame.normal.x*pier.beamHeightMeters*.5,
            beamPart->position.y+beamFrame.normal.y*pier.beamHeightMeters*.5,
            beamPart->position.z+beamFrame.normal.z*pier.beamHeightMeters*.5};
        beamCrack.cutter.projector.tangent=beamFrame.tangent;
        beamCrack.cutter.projector.normal=beamFrame.normal;
        beamCrack.cutter.projector.bitangent={
            beamFrame.normal.y*beamFrame.tangent.z-beamFrame.normal.z*beamFrame.tangent.y,
            beamFrame.normal.z*beamFrame.tangent.x-beamFrame.normal.x*beamFrame.tangent.z,
            beamFrame.normal.x*beamFrame.tangent.y-beamFrame.normal.y*beamFrame.tangent.x};
        beamCrack.cutter.path={{-.3,0,0},{.3,0,0}};
        beamCrack.cutter.depth=.12;
        Settings crackedBeam=pier;
        crackedBeam.damage.pathRules.push_back(beamCrack);
        const auto cutBeam=build(curvedPath(),crackedBeam);
        const auto cutBeamPart=std::find_if(cutBeam.parts.begin(),cutBeam.parts.end(),
            [&](const Part &part){return part.id==beamPart->id;});
        require(cutBeamPart!=cutBeam.parts.end() &&
                partVolume(cutBeam,*cutBeamPart)<partVolume(intactPier,*beamPart) &&
                cutBeamPart->freshInteriorRegions.size()==1,
                "same shared path cutter must cut an actual wooden beam");
        Settings tinyBoard=pier;
        tinyBoard.widthMeters=6e-5;
        tinyBoard.boardWidthMeters=6e-5;
        tinyBoard.boardThicknessMeters=6e-5;
        tinyBoard.boardGapMeters=(intactPier.path.lengthMeters-
            10*tinyBoard.boardWidthMeters)/9;
        const auto tinyDeck=build(curvedPath(),tinyBoard);
        require(!tinyDeck.mesh.mesh.indices.empty(),
                "valid small planks must fall back from unsafe bevels");
        const auto boardId=stablePartId(pier.kind,Role::DeckPlank,3);
        pier.damage.manual[boardId]=State::Broken;
        auto damagedPier=build(curvedPath(),pier);
        const auto brokenBoardPart=std::find_if(damagedPier.parts.begin(),damagedPier.parts.end(),
            [&](const Part &part){return part.id==boardId;});
        require(brokenBoardPart!=damagedPier.parts.end(),
                "local fracture density fixture needs broken plank");
        const auto breakFrame=Construction::frameAtDistance(damagedPier.path,
            brokenBoardPart->stationMeters);
        const auto cutCenter=pier.widthMeters*.5*(damageRandom01(pier.damage.seed,
            boardId,"cut-position")-.5);
        std::set<std::uint32_t> boardVertices;
        for (std::size_t face=brokenBoardPart->firstTriangle;
             face<brokenBoardPart->firstTriangle+brokenBoardPart->triangleCount;++face)
            for (int corner=0;corner<3;++corner)
                boardVertices.insert(damagedPier.mesh.mesh.indices[3*face+corner]);
        std::size_t nearBreak=0,farEnd=0;
        for (const auto vertex:boardVertices)
        {
            const auto local=dot(sub(damagedPier.mesh.mesh.positions[vertex],
                                     breakFrame.position),breakFrame.binormal);
            nearBreak+=std::abs(local-cutCenter)<.25;
            farEnd+=std::abs(std::abs(local)-pier.widthMeters*.5)<.08;
        }
        require(nearBreak>=80 && farEnd<=16 && brokenBoardPart->triangleCount<500,
                "broken plank needs bounded tessellation concentrated near fracture");
        TriangleMesh brokenOnly;
        std::map<std::uint32_t,std::uint32_t> localVertices;
        for (std::size_t face=brokenBoardPart->firstTriangle;
             face<brokenBoardPart->firstTriangle+brokenBoardPart->triangleCount;++face)
            for (int corner=0;corner<3;++corner)
            {
                const auto source=damagedPier.mesh.mesh.indices[3*face+corner];
                const auto [it,inserted]=localVertices.emplace(
                    source,std::uint32_t(brokenOnly.positions.size()));
                if (inserted)
                    brokenOnly.positions.push_back(damagedPier.mesh.mesh.positions[source]);
                brokenOnly.indices.push_back(it->second);
            }
        brokenOnly.triangleRegions.assign(
            damagedPier.mesh.mesh.triangleRegions.begin()+
                std::ptrdiff_t(brokenBoardPart->firstTriangle),
            damagedPier.mesh.mesh.triangleRegions.begin()+
                std::ptrdiff_t(brokenBoardPart->firstTriangle+
                    brokenBoardPart->triangleCount));
        const auto closedBroken=PA::validateClosedMesh(brokenOnly);
        require(closedBroken.hasValue(),
                "locally tessellated fractured plank must remain a closed solid");
        pier.lod=2;
        auto distantPier=build(curvedPath(),pier);
        bool brokenNear=false,brokenFar=false;
        for (const auto &part:damagedPier.parts)
            if (part.id==boardId) brokenNear=part.state==State::Broken && part.triangleCount>0;
        for (const auto &part:distantPier.parts)
            if (part.id==boardId) brokenFar=part.state==State::Broken && part.triangleCount>0;
        require(brokenNear && brokenFar,"manual broken board must survive LOD changes");
        Settings varied=pier;
        varied.damage.manual.clear();
        for (unsigned i=2;i<18;++i)
            varied.damage.manual[stablePartId(varied.kind,Role::DeckPlank,i)]=State::Broken;
        const auto variedBoards=build(curvedPath(),varied);
        bool leftBreak=false,rightBreak=false;
        for (const auto &part:variedBoards.parts)
        {
            if (part.role!=Role::DeckPlank || part.state!=State::Broken) continue;
            const auto frame=Construction::frameAtDistance(variedBoards.path,part.stationMeters);
            double sum=0;std::size_t count=0;
            for (std::size_t triangle=part.firstTriangle;
                 triangle<part.firstTriangle+part.triangleCount;++triangle)
                for (unsigned corner=0;corner<3;++corner)
                {
                    const auto position=variedBoards.mesh.mesh.positions[
                        variedBoards.mesh.mesh.indices[triangle*3+corner]];
                    const auto lateral=dot(sub(position,frame.position),frame.binormal);
                    if (std::abs(lateral)<varied.widthMeters*.3)
                    {sum+=lateral;++count;}
                }
            require(count>0,"broken board must retain a local cut boundary");
            leftBreak|=sum/count<-.1*varied.widthMeters;
            rightBreak|=sum/count>.1*varied.widthMeters;
        }
        require(leftBreak && rightBreak,
                "broken boards must vary the cut side by stable piece identity");
        auto reseeded=varied;
        ++reseeded.damage.seed;
        require(meshHash(variedBoards.mesh.mesh)!=meshHash(build(curvedPath(),reseeded).mesh.mesh),
                "damage seed must change broken-board geometry without changing IDs");
        Settings placedCut=pier;
        const auto targetId=stablePartId(placedCut.kind,Role::DeckPlank,3);
        const auto targetPart=std::find_if(intactPier.parts.begin(),intactPier.parts.end(),
            [&](const Part &part){return part.id==targetId;});
        require(targetPart!=intactPier.parts.end(),"movable break fixture needs target plank");
        const auto targetFrame=Construction::frameAtDistance(intactPier.path,
            targetPart->stationMeters);
        AreaDamageRule placedArea;
        placedArea.area.id="placed-cut";
        placedArea.area.semanticPartId=targetId;
        placedArea.area.projector.center=targetPart->position;
        placedArea.area.projector.tangent=targetFrame.tangent;
        placedArea.area.projector.bitangent=targetFrame.binormal;
        placedArea.area.projector.normal=targetFrame.normal;
        placedArea.area.footprint.radius={2,2,1};
        placedCut.damage.areaRules.push_back(placedArea);
        placedCut.damage.areaRules.front().area.projector.center=
            sub(targetPart->position,{targetFrame.binormal.x*.3,
                targetFrame.binormal.y*.3,targetFrame.binormal.z*.3});
        const auto cutLeft=build(curvedPath(),placedCut);
        placedCut.damage.areaRules.front().area.projector.center={
            targetPart->position.x+targetFrame.binormal.x*.3,
            targetPart->position.y+targetFrame.binormal.y*.3,
            targetPart->position.z+targetFrame.binormal.z*.3};
        const auto cutRight=build(curvedPath(),placedCut);
        require(meshHash(cutLeft.mesh.mesh)!=meshHash(cutRight.mesh.mesh) &&
                cutLeft.parts.size()==cutRight.parts.size(),
                "moving an area within a broken plank must move the cut, not replace the source");
        for (std::size_t i=0;i<cutLeft.parts.size();++i)
            require(cutLeft.parts[i].id==cutRight.parts[i].id &&
                    cutLeft.parts[i].state==cutRight.parts[i].state,
                    "moving an area within a plank must preserve semantic identity and state");
        Settings ropeOrientation;
        ropeOrientation.kind=Assembly::Kind::RopePlankBridge;
        ropeOrientation.spanMeters=12;ropeOrientation.sagMeters=2;
        const auto ropeDeck=build(curvedPath(),ropeOrientation);
        require(std::count_if(ropeDeck.parts.begin(),ropeDeck.parts.end(),
                [](const Part &part){return part.role==Role::Hanger;})>=12,
                "rope bridge hand ropes need distributed side ties");
        const auto ropeA=solveCatenaryA(ropeDeck.path.lengthMeters,ropeOrientation.sagMeters);
        const auto ropeMiddle=std::find_if(ropeDeck.parts.begin(),ropeDeck.parts.end(),
            [&](const Part &part){return part.role==Role::DeckPlank &&
                std::abs(part.stationMeters-ropeDeck.path.lengthMeters*.5)<ropeOrientation.boardWidthMeters;});
        require(ropeMiddle!=ropeDeck.parts.end(),"rope fixture needs a central plank");
        const auto ropeFrame=Construction::frameAtDistance(ropeDeck.path,ropeMiddle->stationMeters);
        const auto ropeCenter=sub(ropeFrame.position,
            {ropeFrame.normal.x*catenaryHeight(ropeMiddle->stationMeters,ropeDeck.path.lengthMeters,ropeOrientation.sagMeters,ropeA),
             ropeFrame.normal.y*catenaryHeight(ropeMiddle->stationMeters,ropeDeck.path.lengthMeters,ropeOrientation.sagMeters,ropeA),
             ropeFrame.normal.z*catenaryHeight(ropeMiddle->stationMeters,ropeDeck.path.lengthMeters,ropeOrientation.sagMeters,ropeA)});
        require(length(sub(ropeMiddle->position,ropeCenter))<1e-8,
                "rope plank selection position must follow the sagged deck");
        bool checkedRopeNormal=false;
        for (const auto &part:ropeDeck.parts)
        {
            if (part.role!=Role::DeckPlank || part.index>2) continue;
            const auto station=part.stationMeters;
            const auto step=.01;
            const auto aFrame=Construction::frameAtDistance(ropeDeck.path,station-step);
            const auto bFrame=Construction::frameAtDistance(ropeDeck.path,station+step);
            const auto aPoint=sub(aFrame.position,{0,catenaryHeight(station-step,
                ropeDeck.path.lengthMeters,ropeOrientation.sagMeters,ropeA),0});
            const auto bPoint=sub(bFrame.position,{0,catenaryHeight(station+step,
                ropeDeck.path.lengthMeters,ropeOrientation.sagMeters,ropeA),0});
            const auto tangent=sub(bPoint,aPoint);
            const auto desiredNormal=Construction::frameAtDistance(ropeDeck.path,station).normal;
            double best=-1;
            for (std::size_t triangle=part.firstTriangle;
                 triangle<part.firstTriangle+part.triangleCount;++triangle)
            {
                const auto actual=ropeDeck.mesh.cornerNormals[triangle*3];
                const auto span=length(tangent);
                const auto tilted=sub(desiredNormal,
                    {tangent.x*dot(desiredNormal,tangent)/(span*span),
                     tangent.y*dot(desiredNormal,tangent)/(span*span),
                     tangent.z*dot(desiredNormal,tangent)/(span*span)});
                best=std::max(best,dot(actual,tilted)/length(tilted));
            }
            require(best>.995,"rope bridge plank top must follow sagged deck normal");
            checkedRopeNormal=true;
        }
        require(checkedRopeNormal,"rope bridge orientation fixture must contain planks");
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
        const auto intactRailing=build(curvedPath(),railing);
        const auto metalMember=std::find_if(intactRailing.parts.begin(),intactRailing.parts.end(),
            [](const Part &part){return part.role==Role::TopRail && part.index==1 && part.side==-1;});
        require(metalMember!=intactRailing.parts.end(),
                "metal railing fixture needs a stable semantic member");
        AreaDamageRule metalArea;
        metalArea.area.id="shared-projector-metal";
        metalArea.area.semanticPartId=metalMember->id;
        metalArea.area.projector.center=metalMember->position;
        metalArea.area.footprint.radius={1,1,1};
        metalArea.state=State::Broken;
        railing.damage.areaRules.push_back(metalArea);
        const auto brokenMetal=build(curvedPath(),railing);
        const auto brokenMember=std::find_if(brokenMetal.parts.begin(),brokenMetal.parts.end(),
            [&](const Part &part){return part.id==metalMember->id;});
        require(brokenMember!=brokenMetal.parts.end() && brokenMember->state==State::Broken &&
                meshHash(brokenMetal.mesh.mesh)!=meshHash(intactRailing.mesh.mesh),
                "same movable shared projector must choose a broken metal variant");
        railing.damage.areaRules.front().area.projector.center.x+=10;
        const auto restoredMetal=build(curvedPath(),railing);
        require(meshHash(restoredMetal.mesh.mesh)==meshHash(intactRailing.mesh.mesh),
                "moving metal damage area away must restore the untouched source variant");
        railing.damage.areaRules.clear();
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
        require(std::none_of(suspended.parts.begin(),suspended.parts.end(),
                [](const Part &part){return part.role==Role::Pile;}),
                "suspension deck must not inherit pier ground piles");
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
            const Vec3d hangerCenter{
                frame.position.x+frame.binormal.x*part.side*suspension.widthMeters*.52+
                    frame.normal.x*cable*.5,
                frame.position.y+frame.binormal.y*part.side*suspension.widthMeters*.52+
                    frame.normal.y*cable*.5,
                frame.position.z+frame.binormal.z*part.side*suspension.widthMeters*.52+
                    frame.normal.z*cable*.5};
            require(length(sub(part.position,hangerCenter))<1e-8,
                    "suspension hanger damage selection must use its actual center");
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
