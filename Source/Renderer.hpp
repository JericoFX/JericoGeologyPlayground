#pragma once
#include "Model.hpp"
#include "WebLod.hpp"
#include <memory>
#include <optional>

namespace Playground
{
struct NormalBake;
namespace Web {struct Result;}
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
using WebAppearance=Web::Optics;
class Renderer
{
  public:
    Renderer(void *window, unsigned width, unsigned height);
    ~Renderer();
    void begin(unsigned width, unsigned height);
    void upload(const MeshResult &mesh, const NormalBake *bake = nullptr,
                const std::vector<std::uint8_t> *diagnosticMap = nullptr,
                const std::vector<std::vector<std::uint8_t>> *diagnosticMips = nullptr,
                bool useNormalMoments = false,
                const std::vector<std::array<float, 2>> *cornerUV = nullptr);
    void uploadMaterial(const NormalBake *bake, const std::filesystem::path &diffusePath,
                        bool diffuseMips = true, int appearance = 0);
    void uploadWeb(const Web::Result &web,bool averagedCoverageMips=false,
                   const Web::Result *structural=nullptr,
                   const MeshResult *backdrop=nullptr);
    void clearMesh();
    void setWebAppearance(const WebAppearance &appearance);
    void setWebDisplacement(Vec3d displacement);
    void *draw(unsigned width, unsigned height, const Camera &camera, int mode);
    double measureDrawGpu(unsigned width,unsigned height,const Camera &camera,int mode);
    void present();
    void capture(const std::filesystem::path &path);
    std::optional<std::uint32_t> pickTriangle(unsigned width, unsigned height,
        const Camera &camera, unsigned pixelX, unsigned pixelY);
    void setNormalMipBias(float bias);
    GpuComparison compareFbm(const MeshResult &mesh, const Layer &layer);
    std::size_t bufferBytes() const;
    std::string adapter() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace Playground
