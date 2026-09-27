#pragma once

#include "Renderer.hpp"
#include "WebLod.hpp"
#include "WebPhysics.hpp"

#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <string>

namespace Playground
{
class WebLabUI
{
  public:
    WebLabUI();
    void activate();
    void frame(Renderer &renderer,unsigned width,unsigned height);

  private:
    Web::Settings settings;
    Web::Result built,debug;
    Web::WebPhysics physics;
    Camera camera;
    std::future<Web::Result> pending;
    std::shared_ptr<std::atomic_bool> pendingCancellation;
    std::chrono::steady_clock::time_point edited{};
    std::optional<std::chrono::steady_clock::time_point> breakStarted;
    std::string error,selectedEdge;
    unsigned revision{},builtRevision{},debugRevision{~0u},pendingRevision{},
             physicalRevision{~0u};
    int view{},uploadedView{-1},selectedPoint{};
    bool dirty{true},pendingDebug{},fitCamera{true},placePoint{},dragPoint{};
    bool averagedMips{},autoLod{true},simulatePhysics{true};
    std::chrono::steady_clock::time_point physicsTick{};
    double physicsAccumulator{};
    double breakLatencyMs{},drawCpuMs{};
    void changed();
    void process();
};
}
