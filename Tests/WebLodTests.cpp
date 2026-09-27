#include "WebLod.hpp"
#include "SolidGeometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>
#include <stdexcept>

using namespace Playground;
using namespace Playground::Web;

namespace
{
void require(bool condition,const char *message)
{if (!condition) throw std::runtime_error(message);}
}
int main()
{
    try
    {
        Settings settings;
        settings.resolution=128;
        settings.radiusMeters=.024;
        const auto graph=makeGraph(settings);
        require(graph.nodes.size()==graph.nodeIds.size() &&
                graph.edges.size()<=4096,"web graph IDs and edge budget must be bounded");
        Settings extreme;
        extreme.preset=Preset::Corner;
        extreme.style=Style::Ghost;
        extreme.spokes=64;extreme.rings=12;
        extreme.density=1;extreme.secondaryStrands=1;
        extreme.microStrandDensity=1;
        extreme.resolution=512;extreme.supersamples=8;
        const auto crowded=makeGraph(extreme);
        const auto structural=std::count_if(crowded.edges.begin(),crowded.edges.end(),
            [](const Edge &edge)
            {return edge.id.find("/Spoke_")!=std::string::npos ||
                    edge.id.find("/Ring_")!=std::string::npos;});
        require(crowded.edges.size()==4096 && structural==3072,
                "maximum Corner settings must retain all primary edges within budget");
        const auto crowdedBake=build(extreme);
        require(crowdedBake.cardAccepted && crowdedBake.bakeBudgetReduced &&
                crowdedBake.bakeResolution==64 &&
                crowdedBake.bakeSupersamples==4 &&
                crowdedBake.mips.front().width==128,
                "dense web must adapt texture size before an excessive CPU bake");
        extreme.resolution=64;
        const auto crowdedDebug=build(extreme,true);
        require(crowdedDebug.sourceTruncated &&
                crowdedDebug.sourceRenderedEdges==256,
                "dense 3D debug must cap the number of generated strand sweeps");
        std::atomic_bool cancelled{true};
        bool stopped=false;
        try {build(extreme,false,&cancelled);}
        catch (const std::runtime_error &) {stopped=true;}
        require(stopped,"obsolete web bake must honor cancellation");
        for (auto point:graph.nodes)
            require(std::isfinite(point.x)&&std::isfinite(point.y)&&
                    std::isfinite(point.z),"web node must be finite");
        require(std::abs(segmentPhi({.5,0},{0,0},{1,0},.1)+.1)<1e-12 &&
                std::abs(segmentPhi({.5,.1},{0,0},{1,0},.1))<1e-12 &&
                segmentPhi({.5,.5},{0,0},{1,0},.1)>0,
                "web signed field must be negative inside, zero at boundary and positive outside");
        const auto plane=fitPlane(graph,0,settings.cardPaddingMeters);
        require(plane.maximumPlaneErrorMeters<1e-10 &&
                plane.rmsPlaneErrorMeters<1e-10,
                "exact planar web must have zero fit error");
        const auto intact=build(settings,true);
        if (!intact.cardAccepted)
            std::cerr<<"Card rejection: "<<intact.fallbackReason<<'\n';
        require(intact.cardAccepted && intact.patches.size()==1 &&
                intact.card.mesh.indices.size()==6,
                "planar orb must bake to exactly one two-triangle card");
        require(intact.source.mesh.indices.size()>intact.ribbons.mesh.indices.size() &&
                intact.ribbons.mesh.indices.size()>intact.card.mesh.indices.size(),
                "source, ribbons and card must reduce geometry successively");
        const auto ordinary=build(settings);
        require(ordinary.cardAccepted && ordinary.source.mesh.indices.empty() &&
                ordinary.ribbons.mesh.indices.empty() &&
                ordinary.card.mesh.indices.size()==6,
                "normal web generation must bake a card without constructing strand meshes");
        require(!intact.mips.empty() && intact.mips.front().width==settings.resolution,
                "card must retain explicit texture levels");
        bool open=false,covered=false;
        for (std::size_t i=0;i<intact.mips.front().coverageRgba.size()/4;++i)
        {
            const auto &level=intact.mips.front();
            open|=level.coverageRgba[i*4+3]==0;
            covered|=level.coverageRgba[i*4+3]>0;
        }
        require(open && covered,"holes and nonzero thin strands must coexist");
        for (const auto &level:intact.mips)
            require(level.coverageRgba.size()==std::size_t(level.width)*level.height*4 &&
                    level.normalRgba.size()==level.coverageRgba.size() &&
                    level.distanceRgba.size()==level.coverageRgba.size(),
                    "every mip must have coverage, normal and distance maps");
        const auto replay=build(settings,true);
        require(meshHash(intact.source.mesh)==meshHash(replay.source.mesh) &&
                textureHash(intact.mips.front())==textureHash(replay.mips.front()) &&
                graph.nodeIds==replay.graph.nodeIds,
                "web geometry and texture must replay deterministically");
        for (std::size_t i=0;i<graph.edges.size();++i)
            require(graph.edges[i].id==replay.graph.edges[i].id,
                    "web edge identities must replay independently of damage");
        settings.brokenEdges.insert(graph.edges.front().id);
        const auto damaged=build(settings,true);
        require(damaged.cardAccepted &&
                damaged.activeEdgeCount+1==intact.activeEdgeCount &&
                damaged.graph.edges.front().state==EdgeState::Broken &&
                damaged.source.mesh.indices.size()<intact.source.mesh.indices.size() &&
                damaged.ribbons.mesh.indices.size()<intact.ribbons.mesh.indices.size() &&
                textureHash(damaged.mips.front())!=textureHash(intact.mips.front()),
                "broken edge must disappear from all representations");
        for (std::size_t i=1;i<intact.graph.edges.size();++i)
            require(damaged.graph.edges[i].id==intact.graph.edges[i].id &&
                    damaged.graph.edges[i].start==intact.graph.edges[i].start &&
                    damaged.graph.edges[i].end==intact.graph.edges[i].end &&
                    damaged.graph.edges[i].state==intact.graph.edges[i].state,
                    "breaking one EdgeID must preserve unrelated semantic edges");
        settings.brokenEdges.clear();
        const auto restored=build(settings);
        require(textureHash(restored.mips.front())==textureHash(intact.mips.front()),
                "restoring one edge must restore the original baked field");
        require(restored.graph.edges.front().state==EdgeState::Intact,
                "restored edge must return to its intact semantic state");
        for (const auto &edge:graph.edges)
            settings.brokenEdges.insert(edge.id);
        const auto empty=build(settings);
        require(empty.activeEdgeCount==0 && !empty.cardAccepted &&
                empty.ribbons.mesh.indices.empty() &&
                empty.fallbackReason=="Web has no visible source edges",
                "breaking every EdgeID must produce an empty web, not a stale card");
        settings.brokenEdges.clear();
        settings.warpMeters=.004;
        const auto mild=build(settings);
        require(mild.cardAccepted && mild.patches.size()==1,
                "mildly warped web within configured tolerance needs one card");
        settings.preset=Preset::Corner;
        settings.warpMeters=0;
        const auto corner=build(settings);
        require(corner.cardAccepted && corner.patches.size()==2 &&
                corner.card.mesh.indices.size()==12,
                "corner web must use two locally planar cards, not one flattened card");
        settings.maximumCards=1;
        const auto fallback=build(settings);
        require(!fallback.cardAccepted && !fallback.fallbackReason.empty(),
                "insufficient card budget must fall back to ribbons with diagnostic");
        settings.preset=Preset::Orb;
        settings.maximumCards=4;
        settings.radiusMeters=.00001;
        const auto thin=build(settings);
        require(!thin.cardAccepted && !thin.fallbackReason.empty(),
                "unrepresentable strand radius must reject card instead of disappearing");
        Settings authored;
        authored.resolution=64;
        authored.radiusMeters=.024;
        authored.authoring=Authoring::Anchors;
        authored.anchors={{-1,-1,0},{1,-1,0},{1,1,0},{-1,1,0}};
        const auto anchored=build(authored);
        require(anchored.cardAccepted && anchored.card.mesh.indices.size()==6,
                "four edited anchors must create a planar two-triangle web");
        require(anchored.physicalEdgeIds.size()<=authored.physicalEdgeBudget &&
                anchored.coarsePhysical.mesh.indices.empty(),
                "ordinary card generation may choose coarse IDs but must not build debug geometry");
        authored.authoring=Authoring::Area;
        authored.area={{-1,-1,0},{1,-1,0},{0,1,0}};
        const auto area=build(authored);
        require(area.cardAccepted && area.card.mesh.indices.size()==6,
                "polygon authoring must bake a card inside the region");
        authored.preset=Preset::Corner;
        const auto cornerArea=build(authored);
        require(cornerArea.cardAccepted && cornerArea.patches.size()==2,
                "corner preset with area authoring must retain both local cards");
        bool secondPatchCoverage=false;
        for (unsigned y=0;y<cornerArea.mips.front().height;++y)
            for (unsigned x=authored.resolution;x<cornerArea.mips.front().width;++x)
                secondPatchCoverage|=cornerArea.mips.front().coverageRgba[
                    4*(std::size_t(y)*cornerArea.mips.front().width+x)+3]>0;
        require(secondPatchCoverage,"area mask must transfer to the second corner card");
        authored.authoring=Authoring::Plane;
        authored.preset=Preset::Old;
        const auto old=makeGraph(authored);
        require(std::any_of(old.edges.begin(),old.edges.end(),[](const Edge &edge)
                {return edge.state==EdgeState::Missing;}),
                "old web preset must have stable missing strands");
        authored.preset=Preset::Orb;
        authored.radiusMeters=.04;
        authored.supersamples=1;
        const auto one=build(authored);
        authored.supersamples=8;
        const auto eight=build(authored);
        require(one.cardAccepted && eight.cardAccepted &&
                textureHash(one.mips.front())!=textureHash(eight.mips.front()),
                "1x and 8x coverage must be measured as distinct sampling policies");
        const auto &coarse=eight.mips[1],&fine=eight.mips[0];
        bool sourceDiffersFromAverage=false,coarseHasStrands=false;
        for (unsigned y=0;y<coarse.height;++y)
            for (unsigned x=0;x<coarse.width;++x)
            {
                const auto index=4*(std::size_t(y)*coarse.width+x);
                unsigned sum{};
                for (unsigned dy=0;dy<2;++dy)
                    for (unsigned dx=0;dx<2;++dx)
                        sum+=fine.coverageRgba[4*(std::size_t(2*y+dy)*fine.width+2*x+dx)+3];
                sourceDiffersFromAverage|=coarse.coverageRgba[index+3]!=sum/4;
                coarseHasStrands|=coarse.coverageRgba[index+3]>0;
            }
        require(sourceDiffersFromAverage && coarseHasStrands,
                "source-rasterized mip must retain strands and differ from averaged base");
        for (const auto &mip:eight.mips)
        {
            bool anyCoverage=false;
            for (std::size_t pixel=0;pixel<std::size_t(mip.width)*mip.height;++pixel)
                anyCoverage|=mip.coverageRgba[pixel*4+3]>0;
            if (mip.minimumRepresentedWidthPixels>=.6)
                require(anyCoverage,"representable strands must survive the source-generated mip");
        }
        auto fails=[](Settings invalid)
        {
            try {build(invalid);return false;}
            catch (const std::runtime_error &) {return true;}
        };
        authored.radiusMeters=0;
        require(fails(authored),"zero strand radius must fail");
        authored.radiusMeters=.024;authored.widthMeters=0;
        require(fails(authored),"zero authored area must fail");
        authored.widthMeters=3;authored.resolution=2048;
        require(fails(authored),"excessive resolution must fail");
        authored.resolution=64;authored.sagMeters=std::numeric_limits<double>::infinity();
        require(fails(authored),"infinite sag must fail");
        authored.sagMeters=0;authored.authoring=Authoring::Anchors;
        authored.anchors.resize(33);
        require(fails(authored),"excessive anchor count must fail");
        authored.anchors.resize(4);authored.physicalEdgeBudget=51;
        require(fails(authored),"excessive coarse physical budget must fail");
        authored.physicalEdgeBudget=32;
        authored.radiusMeters=std::numeric_limits<double>::quiet_NaN();
        require(fails(authored),"NaN radius must fail");
        std::cout<<"PASS web graph, plane fit, mesh/ribbon/card, crossing, damage and mips\n";
        return 0;
    }
    catch (const std::exception &error)
    {std::cerr<<error.what()<<'\n';return 1;}
}
