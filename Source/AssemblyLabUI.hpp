#pragma once

#include "ProceduralAssemblies.hpp"
#include "Renderer.hpp"

#include <string>

namespace Playground
{
class AssemblyLabUI
{
  public:
    AssemblyLabUI();
    void activate(Renderer &renderer);
    void frame(Renderer &renderer,unsigned width,unsigned height);

  private:
    PA::ProceduralSpline spline;
    Assembly::Settings settings;
    Assembly::Result built;
    Camera camera;
    std::string selectedId,error;
    int selectedPoint{},view{},uploadedView{-1};
    bool dirty{true},fitCamera{true};
    unsigned nextPointId{30};
    void rebuild();
    void choosePreset(int preset);
    const Assembly::Part *selectedPart() const;
};
}
