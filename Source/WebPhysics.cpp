#include "WebPhysics.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/PhysicsMaterialSimple.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace Playground::Web
{
namespace
{
Vec3d add(Vec3d a,Vec3d b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3d sub(Vec3d a,Vec3d b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3d mul(Vec3d a,double s) {return {a.x*s,a.y*s,a.z*s};}
double dot(Vec3d a,Vec3d b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3d cross(Vec3d a,Vec3d b)
{return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3d unit(Vec3d p)
{const auto length=std::sqrt(dot(p,p));return length>1e-9?mul(p,1/length):Vec3d{};}
class Layers final:public JPH::BroadPhaseLayerInterface,
                   public JPH::ObjectVsBroadPhaseLayerFilter,
                   public JPH::ObjectLayerPairFilter
{
  public:
    JPH::uint GetNumBroadPhaseLayers() const override {return 1;}
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer) const override
    {return JPH::BroadPhaseLayer(0);}
    bool ShouldCollide(JPH::ObjectLayer,JPH::BroadPhaseLayer) const override
    {return false;}
    bool ShouldCollide(JPH::ObjectLayer,JPH::ObjectLayer) const override
    {return false;}
};
struct JoltRuntime
{
    JoltRuntime()
    {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance=new JPH::Factory();
        JPH::RegisterTypes();
    }
    ~JoltRuntime()
    {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance=nullptr;
    }
};
}
struct WebPhysics::Impl
{
    JoltRuntime runtime;
    Layers layers;
    JPH::PhysicsSystem system;
    JPH::TempAllocatorImpl allocator{4*1024*1024};
    JPH::JobSystemSingleThreaded jobs{JPH::cMaxPhysicsJobs};
    JPH::BodyID body;
    PhysicalNetwork state;
    Impl()
    {
        system.Init(128,0,128,128,layers,layers,layers);
    }
    ~Impl()
    {
        clear();
    }
    void clear()
    {
        if (!body.IsInvalid())
        {
            auto &bodies=system.GetBodyInterface();
            bodies.RemoveBody(body);
            bodies.DestroyBody(body);
            body=JPH::BodyID();
        }
        state={};
    }
    void read()
    {
        JPH::BodyLockRead lock(system.GetBodyLockInterface(),body);
        if (!lock.Succeeded()) throw std::runtime_error("Web soft body lock failed");
        const auto &b=lock.GetBody();
        const auto *motion=static_cast<const JPH::SoftBodyMotionProperties *>(
            b.GetMotionProperties());
        const auto transform=b.GetCenterOfMassTransform();
        for (unsigned i=0;i<state.positions.size();++i)
        {
            const auto p=transform*motion->GetVertex(i).mPosition;
            state.positions[i]={p.GetX(),p.GetY(),p.GetZ()};
        }
    }
};
WebPhysics::WebPhysics():impl(std::make_unique<Impl>()) {}
WebPhysics::~WebPhysics()=default;
void WebPhysics::reset(const Settings &settings,const Result &web)
{
    auto &p=*impl;
    p.clear();
    if (!settings.physicalEdgeBudget || !web.activeEdgeCount) return;
    const auto &authored=settings.authoring==Authoring::Anchors?settings.anchors:
        settings.authoring==Authoring::Area?settings.area:
        std::vector<Vec3d>{
            add(settings.center,add(mul(settings.axisU,-settings.widthMeters*.5),
                                    mul(settings.axisV,-settings.heightMeters*.5))),
            add(settings.center,add(mul(settings.axisU,settings.widthMeters*.5),
                                    mul(settings.axisV,-settings.heightMeters*.5))),
            add(settings.center,add(mul(settings.axisU,settings.widthMeters*.5),
                                    mul(settings.axisV,settings.heightMeters*.5))),
            add(settings.center,add(mul(settings.axisU,-settings.widthMeters*.5),
                                    mul(settings.axisV,settings.heightMeters*.5)))};
    const auto anchorCount=std::min<std::size_t>(authored.size(),
        settings.physicalEdgeBudget/2);
    if (anchorCount<2) return;
    auto shared=JPH::Ref<JPH::SoftBodySharedSettings>(new JPH::SoftBodySharedSettings);
    auto appendVertex=[&](Vec3d position,bool pinned)
    {
        JPH::SoftBodySharedSettings::Vertex vertex;
        vertex.mPosition=JPH::Float3(float(position.x),float(position.y),
                                     float(position.z));
        vertex.mInvMass=pinned?0.0f:1.0f;
        shared->mVertices.push_back(vertex);
        p.state.positions.push_back(position);
        return unsigned(p.state.positions.size()-1);
    };
    auto appendEdge=[&](unsigned a,unsigned b)
    {
        shared->mEdgeConstraints.emplace_back(a,b,0.0002f);
        p.state.edges.emplace_back(a,b);
    };
    const auto hub=appendVertex(web.graph.nodes.front(),false);
    const auto segmentsPerArm=std::clamp<unsigned>(
        (settings.physicalEdgeBudget-unsigned(anchorCount))/unsigned(anchorCount),
        2,12);
    std::vector<unsigned> midpoints;
    for (unsigned i=0;i<anchorCount;++i)
    {
        const unsigned sample=unsigned(i*authored.size()/anchorCount);
        const auto anchor=appendVertex(authored[sample],true);
        p.state.anchors.push_back(anchor);
        auto previous=anchor;
        for (unsigned segment=1;segment<segmentsPerArm;++segment)
        {
            const auto t=double(segment)/segmentsPerArm;
            const auto middle=appendVertex(add(mul(authored[sample],1-t),
                                               mul(web.graph.nodes.front(),t)),false);
            appendEdge(previous,middle);
            if (segment==segmentsPerArm/2) midpoints.push_back(middle);
            previous=middle;
        }
        appendEdge(previous,hub);
    }
    for (unsigned i=0;i<midpoints.size() &&
         p.state.edges.size()<settings.physicalEdgeBudget;++i)
        appendEdge(midpoints[i],midpoints[(i+1)%midpoints.size()]);
    shared->mFaces.emplace_back(hub,p.state.anchors[0],p.state.anchors[1]);
    shared->mMaterials.push_back(new JPH::PhysicsMaterialSimple(
        "Playground web",JPH::Color::sWhite));
    if (p.state.edges.size()>50)
        throw std::runtime_error("Web physical network exceeds 50 edges");
    shared->CalculateEdgeLengths();
    shared->Optimize();
    JPH::SoftBodyCreationSettings creation(shared,JPH::RVec3::sZero(),
                                           JPH::Quat::sIdentity(),0);
    creation.mNumIterations=5;
    creation.mGravityFactor=float(settings.gravityInfluence);
    creation.mUpdatePosition=false;
    creation.mAllowSleeping=false;
    p.body=p.system.GetBodyInterface().CreateAndAddSoftBody(
        creation,JPH::EActivation::Activate);
    if (p.body.IsInvalid()) throw std::runtime_error("Jolt rejected the web network");
    p.read();
}
void WebPhysics::step()
{
    auto &p=*impl;
    if (p.body.IsInvalid()) return;
    const auto start=std::chrono::steady_clock::now();
    const auto error=p.system.Update(1.0f/60,1,&p.allocator,&p.jobs);
    if (error!=JPH::EPhysicsUpdateError::None)
        throw std::runtime_error("Jolt web fixed step failed");
    p.read();
    p.state.stepCpuMilliseconds=std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now()-start).count();
}
const PhysicalNetwork &WebPhysics::network() const {return impl->state;}
MeshResult makePhysicalMesh(const PhysicalNetwork &network,Vec3d normal)
{
    MeshResult mesh;
    normal=unit(normal);
    for (const auto [a,b]:network.edges)
    {
        const auto start=network.positions[a],end=network.positions[b];
        auto across=unit(cross(normal,sub(end,start)));
        if (dot(across,across)<.5) across=unit(cross({0,1,0},sub(end,start)));
        if (dot(across,across)<.5) continue;
        const auto base=std::uint32_t(mesh.mesh.positions.size());
        constexpr double radius=.012;
        mesh.mesh.positions.insert(mesh.mesh.positions.end(),{
            sub(start,mul(across,radius)),sub(end,mul(across,radius)),
            add(end,mul(across,radius)),add(start,mul(across,radius))});
        mesh.normals.insert(mesh.normals.end(),4,normal);
        mesh.field.insert(mesh.field.end(),4,0);
        for (unsigned index:{0u,1u,2u,0u,2u,3u})
        {mesh.mesh.indices.push_back(base+index);mesh.cornerNormals.push_back(normal);}
    }
    return mesh;
}
}
