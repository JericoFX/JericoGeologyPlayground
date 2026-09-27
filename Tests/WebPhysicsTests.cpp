#include "WebPhysics.hpp"

#include <cmath>
#include <stdexcept>

using namespace Playground;

int main()
{
    Web::Settings settings;
    settings.physicalEdgeBudget=32;
    const auto web=Web::build(settings);
    Web::WebPhysics simulation;
    simulation.reset(settings,web);
    const auto initial=simulation.network();
    if (initial.edges.size()<20 || initial.edges.size()>32 ||
        initial.anchors.size()!=4)
        throw std::runtime_error("Web network did not respect anchor/edge budget");
    for (unsigned step=0;step<120;++step) simulation.step();
    const auto &after=simulation.network();
    for (auto anchor:after.anchors)
    {
        const auto a=initial.positions[anchor],b=after.positions[anchor];
        if (std::hypot(a.x-b.x,a.y-b.y,a.z-b.z)>1e-4)
            throw std::runtime_error("Web physics moved an authored anchor");
    }
    if (!(after.positions.front().y<initial.positions.front().y-.01))
        throw std::runtime_error("Web physical hub did not sag under gravity");
    settings.physicalEdgeBudget=0;
    simulation.reset(settings,web);
    if (!simulation.network().edges.empty())
        throw std::runtime_error("Disabled physical network retained edges");
}
