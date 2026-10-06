#pragma once
#include "scene.hpp"
namespace jerico {
struct DockSample { V3 position,tangent,right,up; double distance=0; };
class DockSpline {
 public:
  explicit DockSpline(const std::vector<V3>& controls);
  double length()const;
  DockSample sample(double distance)const;
 private:
  std::vector<DockSample> samples_;
};
Json default_dock_recipe();
Json build_dock(const Json& recipe);
Json validate_dock(const Json& scene);
}
