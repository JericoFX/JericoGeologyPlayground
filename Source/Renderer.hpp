#pragma once
#include "Model.hpp"
#include <memory>

namespace Playground
{
struct NormalBake;
struct Camera
{
    float yaw{0.5f}, pitch{-0.28f}, distance{6};
    std::array<float, 3> target{};
};
struct GpuComparison
{
    double gpuMilliseconds{-1}, maxError{}, cpuMilliseconds{};
    std::size_t samples{};
};
class Renderer
{
  public:
    Renderer(void *window, unsigned width, unsigned height);
    ~Renderer();
    void begin(unsigned width, unsigned height);
    void upload(const MeshResult &mesh, const NormalBake *bake = nullptr,
                const std::vector<std::uint8_t> *diagnosticMap = nullptr,
                const std::vector<std::vector<std::uint8_t>> *diagnosticMips = nullptr,
                bool useNormalMoments = false);
    void uploadMaterial(const NormalBake *bake, const std::filesystem::path &diffusePath,
                        bool diffuseMips = true, int appearance = 0);
    void *draw(unsigned width, unsigned height, const Camera &camera, int mode);
    void present();
    void capture(const std::filesystem::path &path);
    void setNormalMipBias(float bias);
    GpuComparison compareFbm(const MeshResult &mesh, const Layer &layer);
    std::size_t bufferBytes() const;
    std::string adapter() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace Playground
