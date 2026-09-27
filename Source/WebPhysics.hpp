#pragma once

#include "WebLod.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace Playground::Web
{
struct PhysicalNetwork
{
    std::vector<Vec3d> positions;
    std::vector<std::pair<unsigned,unsigned>> edges;
    std::vector<unsigned> anchors;
    double stepCpuMilliseconds{};
};

class WebPhysics
{
  public:
    WebPhysics();
    ~WebPhysics();
    WebPhysics(const WebPhysics&)=delete;
    WebPhysics& operator=(const WebPhysics&)=delete;
    void reset(const Settings &settings,const Result &web);
    void step();
    const PhysicalNetwork &network() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
MeshResult makePhysicalMesh(const PhysicalNetwork &network,Vec3d normal);
}
