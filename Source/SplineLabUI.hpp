#pragma once

#include "Renderer.hpp"
#include "SplineConstruction.hpp"

#include <string>

namespace Playground
{
Construction::ConstructionSettings splinePreset(int index);
class SplineLabUI
{
  public:
    SplineLabUI();
    void activate(Renderer &renderer);
    void frame(Renderer &renderer, unsigned width, unsigned height);

  private:
    PA::ProceduralSpline spline;
    Construction::ConstructionSettings settings;
    Construction::ConstructionResult built;
    Camera camera;
    std::string error;
    int selected{}, selectedDamage{-1}, preset{}, view{}, uploadedView{-1}, quality{1};
    bool dirty{true}, fitCamera{true}, moveDamageMode{}, movingDamage{};
    unsigned nextPointId{20}, nextDamageId{1};
    Vec3d damageGrabOffset{};
    void rebuild(Renderer &renderer);
    void choosePreset(int index);
};
}
