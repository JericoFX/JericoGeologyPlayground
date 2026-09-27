#pragma once

#include "ProceduralAssemblies.hpp"
#include "Renderer.hpp"

#include <string>
#include <optional>
#include <vector>

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
    int selectedPoint{},selectedArea{-1},view{},uploadedView{-1};
    bool dirty{true},fitCamera{true},moveDamageAreaMode{},movingDamageArea{};
    Vec3d damageGrabOffset{};
    unsigned nextPointId{30},nextAreaId{1};
    std::vector<Assembly::DamageSettings> damageUndo,damageRedo;
    std::optional<Assembly::DamageSettings> pendingDamageEdit;
    void rebuild();
    void choosePreset(int preset);
    const Assembly::Part *selectedPart() const;
};
}
