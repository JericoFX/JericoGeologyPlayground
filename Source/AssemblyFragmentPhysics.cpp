#include "AssemblyFragmentPhysics.hpp"
#include "PhysicsRuntime.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <stdexcept>

namespace Playground::Assembly
{
namespace
{
constexpr JPH::ObjectLayer staticLayer=0,dynamicLayer=1;
class Layers final:public JPH::BroadPhaseLayerInterface,
                   public JPH::ObjectVsBroadPhaseLayerFilter,
                   public JPH::ObjectLayerPairFilter
{
  public:
    JPH::uint GetNumBroadPhaseLayers() const override {return 2;}
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {return JPH::BroadPhaseLayer(layer);}
    bool ShouldCollide(JPH::ObjectLayer layer,JPH::BroadPhaseLayer other) const override
    {return layer==dynamicLayer || other==JPH::BroadPhaseLayer(dynamicLayer);}
    bool ShouldCollide(JPH::ObjectLayer first,JPH::ObjectLayer second) const override
    {return first==dynamicLayer || second==dynamicLayer;}
};
Vec3d cross(Vec3d a,Vec3d b)
{return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double dot(Vec3d a,Vec3d b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3d rotate(Vec3d point,const std::array<double,4> &q)
{
    const auto axis=Vec3d{q[0],q[1],q[2]};
    const auto twice=cross(axis,point);
    const auto t=Vec3d{twice.x*2,twice.y*2,twice.z*2};
    const auto turn=cross(axis,t);
    return {point.x+q[3]*t.x+turn.x,point.y+q[3]*t.y+turn.y,
            point.z+q[3]*t.z+turn.z};
}
double volume(const TriangleMesh &mesh,Vec3d origin)
{
    double signedVolume=0;
    for (std::size_t i=0;i<mesh.indices.size();i+=3)
    {
        const auto a=mesh.positions[mesh.indices[i]],
                   b=mesh.positions[mesh.indices[i+1]],
                   c=mesh.positions[mesh.indices[i+2]];
        signedVolume+=dot({a.x-origin.x,a.y-origin.y,a.z-origin.z},
            cross({b.x-origin.x,b.y-origin.y,b.z-origin.z},
                  {c.x-origin.x,c.y-origin.y,c.z-origin.z}))/6;
    }
    return std::abs(signedVolume);
}
}
struct FragmentPhysics::Impl
{
    Physics::RuntimeLease runtime;
    Layers layers;
    JPH::PhysicsSystem system;
    JPH::TempAllocatorImpl allocator{8*1024*1024};
    JPH::JobSystemSingleThreaded jobs{JPH::cMaxPhysicsJobs};
    JPH::BodyID fragmentBody,hostBody,floorBody;
    Fragment fragment;
    double stepMilliseconds{};
    Impl() {system.Init(16,0,256,256,layers,layers,layers);}
    ~Impl() {clear();}
    void clear()
    {
        auto &bodies=system.GetBodyInterface();
        for (auto *id:{&fragmentBody,&hostBody,&floorBody})
            if (!id->IsInvalid())
            {bodies.RemoveBody(*id);bodies.DestroyBody(*id);*id=JPH::BodyID();}
        fragment={};stepMilliseconds=0;
    }
};
FragmentPhysics::FragmentPhysics():impl(std::make_unique<Impl>()) {}
FragmentPhysics::~FragmentPhysics()=default;
void FragmentPhysics::reset() {impl=std::make_unique<Impl>();}
bool FragmentPhysics::active() const {return !impl->fragmentBody.IsInvalid();}
void FragmentPhysics::activate(const Result &source,const Fragment &fragment)
{
    reset();
    auto &p=*impl;
    try
    {
        const auto piece=fragmentMesh(source,fragment);
        if (piece.positions.size()<4 || piece.positions.size()>4096 ||
            source.mesh.mesh.indices.size()/3>100000)
            throw std::runtime_error("Fragment physics exceeds convex or static mesh budget");
        JPH::Array<JPH::Vec3> points;
        points.reserve(piece.positions.size());
        for (const auto vertex:piece.positions)
            points.emplace_back(float(vertex.x-fragment.center.x),
                float(vertex.y-fragment.center.y),float(vertex.z-fragment.center.z));
        JPH::ConvexHullShapeSettings hullSettings(points,0.0f);
        const auto hull=hullSettings.Create();
        if (hull.HasError())
            throw std::runtime_error(std::string("Jolt convex fragment proxy: ")+
                                     hull.GetError().c_str());
        const auto sourceVolume=volume(piece,fragment.center);
        if (!(sourceVolume>1e-8) ||
            hull.Get()->GetVolume()>sourceVolume*1.35)
            throw std::runtime_error("Fragment convex proxy exceeds 35% volume error");
        JPH::VertexList vertices;
        vertices.reserve(source.mesh.mesh.positions.size());
        double lowest=fragment.center.y;
        for (const auto vertex:source.mesh.mesh.positions)
        {
            vertices.emplace_back(float(vertex.x),float(vertex.y),float(vertex.z));
            lowest=std::min(lowest,vertex.y);
        }
        JPH::IndexedTriangleList triangles;
        for (std::size_t face=0;face<source.mesh.mesh.indices.size()/3;++face)
            if (!std::binary_search(fragment.triangles.begin(),fragment.triangles.end(),face))
                triangles.emplace_back(source.mesh.mesh.indices[face*3],
                    source.mesh.mesh.indices[face*3+1],
                    source.mesh.mesh.indices[face*3+2],0);
        auto &bodies=p.system.GetBodyInterface();
        if (!triangles.empty())
        {
            JPH::MeshShapeSettings hostSettings(std::move(vertices),
                                                std::move(triangles));
            const auto host=hostSettings.Create();
            if (host.HasError())
                throw std::runtime_error(std::string("Jolt assembly collision: ")+
                                         host.GetError().c_str());
            JPH::BodyCreationSettings hostBody(host.Get().GetPtr(),
                JPH::RVec3::sZero(),JPH::Quat::sIdentity(),
                JPH::EMotionType::Static,staticLayer);
            p.hostBody=bodies.CreateAndAddBody(hostBody,JPH::EActivation::DontActivate);
            if (p.hostBody.IsInvalid()) throw std::runtime_error("Jolt rejected assembly collision");
        }
        JPH::BoxShapeSettings floorSettings(JPH::Vec3(100,.1f,100));
        const auto floor=floorSettings.Create();
        if (floor.HasError())
            throw std::runtime_error(std::string("Jolt floor: ")+
                                     floor.GetError().c_str());
        JPH::BodyCreationSettings floorBody(floor.Get().GetPtr(),
            JPH::RVec3(0,lowest-.3,0),JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,staticLayer);
        p.floorBody=bodies.CreateAndAddBody(floorBody,JPH::EActivation::DontActivate);
        if (p.floorBody.IsInvalid()) throw std::runtime_error("Jolt rejected floor collision");
        JPH::BodyCreationSettings moving(hull.Get().GetPtr(),
            JPH::RVec3(fragment.center.x,fragment.center.y,fragment.center.z),
            JPH::Quat::sIdentity(),JPH::EMotionType::Dynamic,dynamicLayer);
        moving.mFriction=.7f;
        moving.mRestitution=.05f;
        p.fragmentBody=bodies.CreateAndAddBody(moving,JPH::EActivation::Activate);
        if (p.fragmentBody.IsInvalid()) throw std::runtime_error("Jolt rejected fragment body");
        p.fragment=fragment;
    }
    catch (...)
    {p.clear();throw;}
}
void FragmentPhysics::step()
{
    if (!active()) return;
    const auto start=std::chrono::steady_clock::now();
    if (impl->system.Update(1.0f/60,1,&impl->allocator,&impl->jobs)!=
        JPH::EPhysicsUpdateError::None)
        throw std::runtime_error("Jolt fragment fixed step failed");
    impl->stepMilliseconds=std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now()-start).count();
}
FragmentPose FragmentPhysics::pose() const
{
    FragmentPose result;
    if (!active()) return result;
    JPH::RVec3 position;
    JPH::Quat rotation;
    impl->system.GetBodyInterface().GetPositionAndRotation(
        impl->fragmentBody,position,rotation);
    result.id=impl->fragment.id;
    result.position={position.GetX(),position.GetY(),position.GetZ()};
    result.rotation={rotation.GetX(),rotation.GetY(),rotation.GetZ(),rotation.GetW()};
    result.stepCpuMilliseconds=impl->stepMilliseconds;
    return result;
}
MeshResult FragmentPhysics::displacedMesh(const Result &source,
                                         const MeshResult &mesh) const
{
    if (!active()) return mesh;
    const auto state=pose();
    MeshResult moved=mesh;
    const auto &fragment=impl->fragment;
    std::vector<bool> vertices(moved.mesh.positions.size());
    for (const auto face:fragment.triangles)
    {
        if (face>=source.mesh.mesh.indices.size()/3)
            throw std::runtime_error("Moved fragment face is stale");
        for (int corner=0;corner<3;++corner)
        {
            const auto index=source.mesh.mesh.indices[face*3+corner];
            if (index>=vertices.size())
                throw std::runtime_error("Moved fragment vertex is stale");
            vertices[index]=true;
            moved.cornerNormals[face*3+corner]=rotate(mesh.cornerNormals[face*3+corner],
                                                     state.rotation);
        }
    }
    for (std::size_t i=0;i<vertices.size();++i)
        if (vertices[i])
        {
            const auto original=mesh.mesh.positions[i];
            const auto local=rotate({original.x-fragment.center.x,
                original.y-fragment.center.y,original.z-fragment.center.z},
                state.rotation);
            moved.mesh.positions[i]={state.position.x+local.x,
                state.position.y+local.y,state.position.z+local.z};
            moved.normals[i]=rotate(mesh.normals[i],state.rotation);
        }
    return moved;
}
}
