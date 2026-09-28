#pragma once

#include "ProceduralAssemblies.hpp"

#include <array>
#include <memory>
#include <string>

namespace Playground::Assembly
{
struct FragmentPose
{
    std::string id;
    Vec3d position{};
    std::array<double,4> rotation{0,0,0,1};
    double stepCpuMilliseconds{};
};
class FragmentPhysics
{
  public:
    FragmentPhysics();
    ~FragmentPhysics();
    FragmentPhysics(const FragmentPhysics &)=delete;
    FragmentPhysics &operator=(const FragmentPhysics &)=delete;
    void activate(const Result &source,const Fragment &fragment);
    void reset();
    void step();
    bool active() const;
    FragmentPose pose() const;
    MeshResult displacedMesh(const Result &source,const MeshResult &mesh) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
