#pragma once
#include "SplineConstruction.hpp"
#include "SolidGeometry.hpp"
#include "SplineDamageIO.hpp"
#include "SplineDamageVolumeTests.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <set>
#include <stdexcept>

namespace SplineSurfaceFitTests
{
using namespace Playground;
using namespace Playground::Construction;
inline void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
inline PA::ProceduralSpline path(std::initializer_list<Vec3d> points)
{
    PA::ProceduralSpline result;result.splineGuid=Solid::region(9301,1);result.name="Surface fit fixture";
    for(auto p:points){PA::SplineControlPoint point;point.pointGuid=Solid::region(9301,result.points.size()+2);point.position=p;result.points.push_back(point);}
    return result;
}
inline double gap(Vec3d point,const std::vector<Vec3d>&profile)
{
    double nearest=1e30;
    for(std::size_t i=0;i<profile.size();++i)
    {
        const auto a=profile[i],b=profile[(i+1)%profile.size()];
        const auto x=b.x-a.x,y=b.y-a.y;
        const auto t=std::clamp(((point.x-a.x)*x+(point.y-a.y)*y)/(x*x+y*y),0.,1.);
        nearest=std::min(nearest,std::hypot(point.x-a.x-t*x,point.y-a.y-t*y));
    }
    return nearest;
}
inline void run()
{
    const auto host=path({{0,0,0},{0,0,3}});ConstructionSettings settings;settings.radiusMeters=.2;
    const auto source=buildConstruction(host,settings);const auto original=meshHash(source.mesh.mesh);
    std::set<std::pair<double,double>> xy;
    for(auto p:source.mesh.mesh.positions)if(std::abs(p.z)<1e-12&&std::hypot(p.x,p.y)>.19)xy.emplace(p.x,p.y);
    std::vector<Vec3d> profile;for(auto [x,y]:xy)profile.push_back({x,y,0});
    std::sort(profile.begin(),profile.end(),[](auto a,auto b){return std::atan2(a.y,a.x)<std::atan2(b.y,b.x);});
    // The sweep may duplicate the seam; remove the same geometric endpoint.
    if(std::hypot(profile.front().x-profile.back().x,profile.front().y-profile.back().y)<1e-12)profile.pop_back();
    PathDamageRule rule;rule.id="surface-follow";rule.operation=DamageVolumeOperation::Union;
    rule.followSurface=true;rule.surfaceDistanceMeters=.05;rule.cutter.radius=.012;rule.cutter.depth=.01;
    rule.cutter.projector.tangent={1,0,0};rule.cutter.projector.bitangent={0,1,0};rule.cutter.projector.normal={0,0,1};
    rule.cutter.path={{-.1,0,0},{.1,0,0}};rule.spatialPath=path({});
    for(unsigned i=0;i<9;++i){const double angle=-std::numbers::pi/3+2*std::numbers::pi*i/24;
        PA::SplineControlPoint point;point.pointGuid=Solid::region(9302,i+1);point.position={.2*std::cos(angle),.2*std::sin(angle),.4+2.2*i/8};rule.spatialPath->points.push_back(point);}
    const auto samples=sampleSplineDamage(rule,&source.mesh.mesh);
    double maximum=0,maximumChord=0;
    for(std::size_t i=0;i<samples.sections.size();++i)
    {
        const auto p=samples.sections[i].center;maximum=std::max(maximum,gap(p,profile));
        if(i)for(const double t:{.25,.5,.75}){const auto a=samples.sections[i-1].center;
            maximumChord=std::max(maximumChord,gap({std::lerp(a.x,p.x,t),std::lerp(a.y,p.y,t),std::lerp(a.z,p.z,t)},profile));}
    }
    std::cout<<"surface fit sections="<<samples.sections.size()<<" centerGap="<<maximum<<" chordGap="<<maximumChord<<'\n';
    require(maximum<1e-9,"follow surface must put the actual sweep centers on the receiver");
    require(maximumChord<=.0005+1e-9,"conformed chord samples must stay within the surface error bound");
    require(meshHash(source.mesh.mesh)==original,"surface fitting must not modify its receiver");
    settings.damagePaths.push_back(rule);const auto raised=buildConstruction(host,settings);
    require(SplineDamageVolumeTests::volume(raised.mesh.mesh)>SplineDamageVolumeTests::volume(source.mesh.mesh),"fitted decal must add real geometry");
    require(SplineDamageVolumeTests::connected(raised.mesh.mesh),"fitted decal must remain connected to the receiver");
    const auto encoded=serializeSplineDamage(settings.damagePaths);auto replay=settings;
    replay.damagePaths=deserializeSplineDamage(encoded);
    require(replay.damagePaths[0].followSurface&&replay.damagePaths[0].surfaceDistanceMeters==.05,"surface fitting settings must persist");
    auto inactive=rule;inactive.followSurface=false;inactive.surfaceDistanceMeters=.075;
    const auto inactiveReplay=deserializeSplineDamage(serializeSplineDamage({inactive}));
    require(!inactiveReplay[0].followSurface&&inactiveReplay[0].surfaceDistanceMeters==.075,"inactive fitting must retain the authored distance setting");
    require(meshHash(buildConstruction(host,replay).mesh.mesh)==meshHash(raised.mesh.mesh),"surface fitting must replay exact geometry");
    replay.damagePaths[0].cutter.enabled=false;
    require(meshHash(buildConstruction(host,replay).mesh.mesh)==original,"disabling fitted damage restores the exact receiver");
    auto reject=[](auto action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}require(failed,"invalid surface fitting must reject");};
    auto missing=rule;missing.cutter.projector.center.x=10;
    reject([&]{sampleSplineDamage(missing,&source.mesh.mesh);});
    reject([&]{sampleSplineDamage(rule);});
    auto invalid=rule;invalid.surfaceDistanceMeters=0;reject([&]{validateSplineDamage(invalid);});
    // Reuse the same guide after a nonuniform projector/receiver transform.
    auto transformed=source.mesh.mesh;auto scaled=rule;
    scaled.cutter.projector.center={2,3,-1};scaled.cutter.projector.scale={1.4,.7,1.2};
    for(auto &p:transformed.positions)p={2+1.4*p.x,3+.7*p.y,-1+1.2*p.z};
    const auto scaledSamples=sampleSplineDamage(scaled,&transformed);
    for(const auto &section:scaledSamples.sections)
        require(gap({(section.center.x-2)/1.4,(section.center.y-3)/.7,0},profile)<1e-9,"nonuniform scaling must retain surface contact");
    auto oriented=scaled;oriented.cutter.projector.bitangent={0,0,1};oriented.cutter.projector.normal={0,-1,0};
    oriented.cutter.projector.rotationRadians=.35;const auto cs=std::cos(.35),sn=std::sin(.35);
    transformed=source.mesh.mesh;
    for(auto &p:transformed.positions)p={2+1.4*(cs*p.x+sn*p.y),3-1.2*p.z,-1+.7*(-sn*p.x+cs*p.y)};
    for(const auto &section:sampleSplineDamage(oriented,&transformed).sections)
    {const auto x=(section.center.x-2)/1.4,y=(section.center.z+1)/.7;require(gap({cs*x-sn*y,sn*x+cs*y,0},profile)<1e-9,"rotated projectors must retain surface contact");}
    auto corner=rule;corner.surfaceDistanceMeters=.25;corner.spatialPath=path({{-.7,1,.3},{.7,1,.3},{1,.7,.3},{1,0,.8},{1,-.7,-.3}});
    const auto cube=SplineDamageVolumeTests::cube();const auto cornerSamples=sampleSplineDamage(corner,&cube);
    for(const auto &section:cornerSamples.sections)
    {const auto p=section.center;require(std::min({std::abs(std::abs(p.x)-1),std::abs(std::abs(p.y)-1),std::abs(std::abs(p.z)-1)})<1e-9,"surface fitting must follow a sharp cube corner");}
    require(cornerSamples.maximumSurfaceChordErrorMeters<=.0005+1e-9,"sharp-corner chord probes must respect the fitting error");
    auto bezier=rule;bezier.spatialPath=path({{-.7,1.01,.3},{.7,1.01,.3}});bezier.spatialPath->interpolation=PA::SplineInterpolation::CubicBezier;
    bezier.spatialPath->points[0].outgoingHandle={.4,.02,.1};bezier.spatialPath->points[1].incomingHandle={-.4,-.02,-.1};
    bezier.cutter.projector.center={3,2,1};auto translatedCube=cube;for(auto&p:translatedCube.positions)p={p.x+3,p.y+2,p.z+1};
    for(const auto &section:sampleSplineDamage(bezier,&translatedCube).sections)
        require(std::abs(section.center.y-3)<1e-9,"Bezier handles must transform as vectors when following a translated surface");
    std::cout<<"PASS fitted decal geometry, nonuniform scale, cube corner, persistence, off and distance limits\n";
}
}
