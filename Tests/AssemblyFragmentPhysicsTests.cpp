#include "AssemblyFragmentPhysics.hpp"
#include "WebPhysics.hpp"
#include "SolidGeometry.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>

using namespace Playground;
using namespace Playground::Assembly;

namespace
{
void require(bool condition,const char *message)
{if (!condition) throw std::runtime_error(message);}
double distance(Vec3d a,Vec3d b)
{return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);}
PA::ProceduralSpline path()
{
    PA::ProceduralSpline spline;
    spline.splineGuid=Solid::region(5200,1);
    spline.name="Fragment physics fixture";
    spline.interpolation=PA::SplineInterpolation::CatmullRom;
    for (unsigned i=0;i<4;++i)
    {
        PA::SplineControlPoint point;
        point.pointGuid=Solid::region(5200,i+2);
        point.position={double(i)*2-3,0,0};
        spline.points.push_back(point);
    }
    return spline;
}
}
int main() try
{
    Settings settings;
    settings.kind=Assembly::Kind::WoodenPier;
    const auto partId=stablePartId(settings.kind,Role::DeckPlank,7);
    settings.damage.manual[partId]=State::Broken;
    const auto source=build(path(),settings);
    const auto selected=std::find_if(source.fragments.begin(),source.fragments.end(),
        [&](const Fragment &fragment){return fragment.parentPartId==partId;});
    require(selected!=source.fragments.end(),"broken plank lacks a selectable fragment");
    FragmentPhysics physics;
    Web::WebPhysics web;
    Web::Settings webSettings;
    webSettings.physicalEdgeBudget=32;
    web.reset(webSettings,Web::build(webSettings));
    physics.activate(source,*selected);
    require(physics.active() && physics.pose().id==selected->id,
            "selected fragment did not receive a Jolt body");
    const auto atActivation=physics.displacedMesh(source,source.mesh);
    for (const auto face:selected->triangles)
        for (int corner=0;corner<3;++corner)
        {
            const auto vertex=source.mesh.mesh.indices[face*3+corner];
            require(distance(atActivation.mesh.positions[vertex],
                             source.mesh.mesh.positions[vertex])<1e-6,
                    "Jolt body starts displaced from authored fragment");
        }
    const auto initial=physics.pose();
    for (unsigned step=0;step<120;++step) physics.step();
    web.step();
    const auto fallen=physics.pose();
    require(distance(initial.position,fallen.position)>.01 &&
            fallen.position.y> -10 && fallen.stepCpuMilliseconds>=0,
            "Jolt fragment did not move and collide within scene bounds");
    const auto moved=physics.displacedMesh(source,source.mesh);
    require(moved.mesh.indices==source.mesh.mesh.indices &&
            moved.mesh.triangleRegions==source.mesh.mesh.triangleRegions,
            "fragment animation changed semantic topology");
    const auto activeVertex=source.mesh.mesh.indices[selected->triangles.front()*3];
    require(distance(moved.mesh.positions[activeVertex],
                     source.mesh.mesh.positions[activeVertex])>.01,
            "rendered fragment did not follow Jolt transform");
    const auto neighbor=std::find_if(source.fragments.begin(),source.fragments.end(),
        [&](const Fragment &fragment){return fragment.id!=selected->id;});
    require(neighbor!=source.fragments.end(),"physics fixture lacks neighboring fragment");
    const auto neighborVertex=source.mesh.mesh.indices[neighbor->triangles.front()*3];
    require(distance(moved.mesh.positions[neighborVertex],
                     source.mesh.mesh.positions[neighborVertex])<1e-9,
            "moving one fragment changed another fragment");
    for (std::size_t face=0;face<source.mesh.mesh.indices.size()/3;++face)
        if (!std::binary_search(selected->triangles.begin(),selected->triangles.end(),face))
            for (int corner=0;corner<3;++corner)
            {
                const auto vertex=source.mesh.mesh.indices[face*3+corner];
                require(distance(moved.mesh.positions[vertex],
                                 source.mesh.mesh.positions[vertex])<1e-9,
                        "moving fragment displaced adjacent assembly geometry");
            }
    for (unsigned step=0;step<120;++step) physics.step();
    const auto later=physics.pose();
    const auto lowest=std::min_element(source.mesh.mesh.positions.begin(),
        source.mesh.mesh.positions.end(),
        [](Vec3d a,Vec3d b){return a.y<b.y;})->y;
    const auto settledMesh=physics.displacedMesh(source,source.mesh);
    double fragmentBottom=1e9;
    for (const auto face:selected->triangles)
        for (int corner=0;corner<3;++corner)
            fragmentBottom=std::min(fragmentBottom,
                settledMesh.mesh.positions[source.mesh.mesh.indices[face*3+corner]].y);
    require(fragmentBottom>=lowest-.21,
            "dynamic fragment penetrated the Jolt floor");
    require(distance(later.position,fallen.position)<.01,
            "fragment did not settle after contact");
    physics.reset();
    require(!physics.active(),"reset retained a Jolt body");
    physics.activate(source,*selected);
    for (unsigned step=0;step<120;++step) physics.step();
    require(distance(physics.pose().position,fallen.position)<1e-4,
            "fixed-step Jolt fragment replay changed pose");
    std::printf("fragment moved %.4f m; floor top %.4f m; bottom %.4f m; "
        "settling drift %.5f m\n",distance(initial.position,fallen.position),
        lowest-.2,fragmentBottom,distance(later.position,fallen.position));
}
catch (const std::exception &e)
{
    std::fprintf(stderr,"fragment physics test: %s\n",e.what());
    return 1;
}
