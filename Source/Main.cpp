#include "Model.hpp"
#include "MeshReduction.hpp"
#include "NormalBake.hpp"
#include "Renderer.hpp"
#include "SurfaceFeatures.hpp"
#include <Jerico/Platform/Window.hpp>
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <Windows.h>
#include <algorithm>
#include <backends/imgui_impl_win32.h>
#include <chrono>
#include <cmath>
#include <commdlg.h>
#include <fstream>
#include <future>
#include <imgui.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <random>
#include <thread>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
namespace Playground
{
namespace
{
WNDPROC previousProc{};
LRESULT CALLBACK inputProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_GETMINMAXINFO)
    {
        auto *limits = reinterpret_cast<MINMAXINFO *>(lparam);
        limits->ptMinTrackSize = {1200, 750};
        return 0;
    }
    if (ImGui::GetCurrentContext())
        ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam);
    return CallWindowProcW(previousProc, window, message, wparam, lparam);
}
std::filesystem::path chooseFile(HWND owner, bool saveFile)
{
    wchar_t filename[MAX_PATH] = L"GeologyRecipe.json";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = L"Playground recipe (*.json)\0*.json\0\0";
    dialog.lpstrFile = filename;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrDefExt = L"json";
    dialog.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (saveFile ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    bool accepted = saveFile ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
    ImGui::GetIO().ClearInputMouse();
    if (accepted)
        return filename;
    return {};
}
bool slider(const char *name, float &value, float low, float high)
{
    return ImGui::SliderFloat(name, &value, low, high, "%.3f");
}
bool editLayer(Layer &l)
{
    bool change = false;
    change |= ImGui::Checkbox("Enabled", &l.enabled);
    change |= ImGui::InputScalar("Seed", ImGuiDataType_U32, &l.seed);
    if (ImGui::Button("Previous seed"))
    {
        --l.seed;
        change = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Next seed"))
    {
        ++l.seed;
        change = true;
    }
    if (ImGui::Button("Random seed"))
    {
        l.seed = std::random_device{}();
        change = true;
    }
    if (l.kind == Kind::Transform)
    {
        change |= ImGui::SliderFloat3("Scale", l.scale.data(), 0.05f, 5);
        change |= ImGui::SliderFloat3("Rotation", l.rotation.data(), -180, 180);
        change |= ImGui::SliderFloat3("Translation", l.translation.data(), -5, 5);
    }
    else if (l.kind == Kind::StrataRelief || l.kind == Kind::RidgedNoise)
    {
        change |= slider("Amplitude", l.amplitude, -1, 1);
        change |= slider("Frequency", l.frequency, 0.05f, 12);
        change |= slider("Profile sharpness", l.profileSharpness, 0.25f, 8);
        if (l.kind == Kind::StrataRelief)
        {
            change |= ImGui::SliderFloat3("Strata axis", l.direction.data(), -1, 1);
            change |= ImGui::Combo("Profile", &l.profile, "Sine\0Ledge\0Triangle\0");
            change |= slider("Phase", l.phase, -4, 4);
            change |= slider("Distortion", l.distortion, 0, 2);
            change |= slider("Ledge rise", l.width, 0.02f, 0.7f);
        }
        else
        {
            change |= ImGui::SliderFloat3("Coordinate XYZ", l.coordinateScale.data(), 0.05f, 5);
            change |= ImGui::SliderInt("Octaves", &l.octaves, 1, 8);
            change |= slider("Lacunarity", l.lacunarity, 1, 4);
            change |= slider("Roughness", l.roughness, 0, 1);
        }
    }
    else if (l.kind == Kind::SurfaceDamage)
    {
        ImGui::TextWrapped("Inward surface wear, restricted by the mask. Local sampling and layer thickness "
                           "bound the depth. Does not create separate fracture chunks.");
        change |= slider("Removal depth", l.amplitude, 0, 0.3f);
        change |= slider("Damage frequency", l.frequency, 0.05f, 20);
        change |= slider("Damage threshold", l.threshold, 0, 1);
        change |= slider("Damage transition", l.width, 0.01f, 1);
        change |= ImGui::SliderInt("Octaves", &l.octaves, 1, 8);
        change |= ImGui::SliderFloat3("Coordinate XYZ", l.coordinateScale.data(), 0.05f, 5);
    }
    else if (l.kind == Kind::DamageDecal)
    {
        ImGui::TextWrapped("Native oriented projector: soft damage mask, smooth indentation and bounded lip. "
                           "This tier changes surface geometry without a Boolean through-hole.");
        change |= ImGui::SliderFloat3("Projector center", l.translation.data(), -5, 5);
        change |= ImGui::SliderFloat3("Projector normal", l.direction.data(), -1, 1);
        change |= ImGui::SliderFloat2("Ellipse radii", l.projectorRadius.data(), 0.02f, 5);
        change |= slider("Projector rotation", l.projectorRotation, -180, 180);
        change |= slider("Surface depth band", l.featureRadius, 0.001f, 2);
        change |= slider("Indentation depth", l.amplitude, 0, 1);
        change |= slider("Full mask radius", l.threshold, 0, 0.99f);
        change |= slider("Mask transition", l.width, 0.001f, 1);
        change |= slider("Crater profile power", l.profileSharpness, 0.25f, 8);
        change |= slider("Lip height", l.damageLip, 0, 0.3f);
    }
    else if (l.kind == Kind::CliffCutLoop)
    {
        ImGui::TextWrapped("Grid-conforming local cliff loop. Its interior is a selectable region "
                           "and a separate automatic UV chart boundary.");
        change |= ImGui::SliderFloat2("Loop center XY", l.translation.data(), -3, 3);
        change |= ImGui::SliderFloat2("Loop radii XY", l.projectorRadius.data(), 0.1f, 3);
        change |= slider("Loop rotation", l.projectorRotation, -180, 180);
        change |= slider("Contour irregularity", l.distortion, 0, 0.3f);
        change |= slider("Inset depth", l.amplitude, 0.01f, 1.5f);
        change |= slider("Interior tilt", l.lateral, 0, 0.5f);
    }
    else if (l.kind == Kind::Reduce)
    {
        ImGui::TextWrapped("Native closed-mesh simplification. Preserves region identities and checks "
                           "sampled bidirectional surface error. Solidify an open wall first.");
        change |= slider("Maximum sampled error", l.reductionError, 0.0001f, 0.15f);
    }
    else if (l.kind == Kind::Solidify)
    {
        ImGui::TextWrapped("Creates a closed back and rim from an open surface. Source stays editable.");
        change |= slider("Thickness", l.thickness, 0.02f, 3);
        change |= slider("Offset: back / centered / front", l.solidOffset, -1, 1);
        change |= ImGui::Checkbox("Along surface normals", &l.solidAlongNormals);
        if (!l.solidAlongNormals)
            change |= ImGui::SliderFloat3("Direction", l.direction.data(), -1, 1);
    }
    else if (l.kind == Kind::EdgeBreakup)
    {
        ImGui::TextWrapped("Distance to lateral faces of each stratum. Align direction with the layer axis. "
                           "Strength is capped to 20% of authored layer thickness.");
        change |= ImGui::SliderFloat3("Layer axis", l.direction.data(), -1, 1);
        change |= slider("Edge width", l.width, 0.01f, 1);
        change |= slider("Edge strength", l.amplitude, 0, 0.15f);
        change |= slider("Edge frequency", l.frequency, 0.1f, 20);
    }
    else if (l.kind == Kind::Envelope)
    {
        ImGui::TextWrapped(
            "Closed intersection with support planes. Curved shapes are faceted approximations.");
        change |= ImGui::Combo("Envelope shape", &l.envelopeShape, "Ellipsoid\0Box\0Rounded box\0Cylinder\0");
        change |= ImGui::SliderFloat3("Half extents", l.scale.data(), 0.05f, 5);
        change |= ImGui::SliderFloat3("Center", l.translation.data(), -5, 5);
        change |= ImGui::SliderFloat3("Rotation", l.rotation.data(), -180, 180);
        if (l.envelopeShape == 2)
            change |= slider("Roundness", l.envelopeRoundness, 0, 1);
        change |= ImGui::SliderInt("Cut tessellation", &l.cutSubdivisions, 1, 4);
    }
    else if (l.kind == Kind::Planes)
    {
        ImGui::TextWrapped(
            "Closed solid cuts using the actual engine API. Preserves original and new plane regions.");
        change |= ImGui::SliderInt("Plane count", &l.count, 3, 12);
        change |= slider("Cut depth", l.amplitude, 0.01f, 0.45f);
        change |= ImGui::SliderInt("Cut tessellation", &l.cutSubdivisions, 1, 4);
    }
    else if (l.kind == Kind::Layers)
    {
        change |= ImGui::Checkbox("Coherent thickness (v2)", &l.coherentLayers);
        ImGui::TextWrapped("Structural geometry: oriented copies of the incoming shape, compressed into "
                           "slabs. No Boolean union.");
        change |= ImGui::SliderFloat3("Direction", l.direction.data(), -1, 1);
        change |= ImGui::SliderInt("Layer count", &l.count, 1, 12);
        change |=
            slider("Thickness", l.thickness, l.coherentLayers ? std::max(0.02f, l.minimumThickness) : 0.02f,
                   l.coherentLayers ? l.maximumThickness : 1.5f);
        change |= slider("Spacing", l.spacing, 0, 0.5f);
        change |= slider("Scale per layer", l.layerScale, -0.15f, 0.15f);
        change |= slider("Thickness jitter", l.jitter, 0, 0.8f);
        change |= slider("Lateral offset", l.lateral, 0, 1.5f);
        if (l.coherentLayers)
        {
            change |= slider("Thickness frequency", l.thicknessFrequency, 0.05f, 4);
            change |= slider("Thickness correlation", l.thicknessCorrelation, 0, 1);
            change |= slider("Minimum thickness", l.minimumThickness, 0.01f, l.thickness);
            change |= slider("Maximum thickness", l.maximumThickness, l.thickness, 3);
            change |= slider("Drift correlation", l.driftCorrelation, 0, 1);
            change |= slider("Rotation drift (degrees)", l.rotationDrift, 0, 45);
            change |= slider("Scale drift", l.scaleDrift, 0, 0.3f);
            change |= slider("Stratum contour + side relief", l.perimeterAmplitude, 0, 0.3f);
            if (l.perimeterAmplitude <= 0)
                l.shapeOuterCaps = false;
            if (l.perimeterAmplitude > 0)
            {
                change |= slider("Perimeter frequency", l.frequency, 0.1f, 12);
                change |= ImGui::Checkbox("Shape outer top/base", &l.shapeOuterCaps);
            }
            change |= ImGui::SliderFloat3("Drift bias", l.driftBias.data(), -1, 1);
        }
    }
    else if (l.kind == Kind::LayerSettle)
    {
        ImGui::TextWrapped("Move each upper stratum to first geometric support along this up axis.");
        change |= ImGui::SliderFloat3("Up axis", l.direction.data(), -1, 1);
    }
    else if (l.kind == Kind::Relax)
    {
        ImGui::TextWrapped("Mesh neighbor relaxation. Iterative position averaging; may shrink volume.");
        change |= ImGui::SliderInt("Iterations", &l.iterations, 0, 30);
        change |= slider("Strength", l.relaxStrength, 0, 0.5f);
    }
    else
    {
        if (l.kind == Kind::Warp)
            ImGui::TextWrapped(
                "Warps sampling coordinates for later fields. This layer alone does not move the mesh.");
        if (l.kind == Kind::SharedWarp)
            ImGui::TextWrapped("Moves the whole formation with one shared low-frequency field. Place after "
                               "Layers to bend them together.");
        if (l.kind == Kind::Fracture)
            ImGui::TextWrapped("SURFACE / FIELD FRACTURE: a displacement experiment, not physical fracture.");
        change |= slider("Amplitude", l.amplitude, -1, 1);
        if (l.kind == Kind::VectorNoise || l.kind == Kind::Warp || l.kind == Kind::SharedWarp)
        {
            ImGui::TextWrapped("Amplitude is per XYZ component.");
            change |= ImGui::SliderFloat3("Output XYZ", l.outputScale.data(), -2, 2);
        }
        change |= slider("Frequency", l.frequency, 0.05f, 12);
        change |= ImGui::SliderFloat3("Coordinate XYZ", l.coordinateScale.data(), 0.05f, 5);
        if (l.kind == Kind::Voronoi || l.kind == Kind::Fracture)
        {
            change |= ImGui::Combo("Feature", &l.voronoiMode, "F1\0F2\0Distance to edge\0");
            change |= slider("Randomness", l.randomness, 0, 1);
            change |= slider("Threshold", l.threshold, 0, 0.7f);
            change |= slider("Width", l.width, 0.001f, 0.7f);
        }
        else
        {
            change |= ImGui::SliderInt("Octaves", &l.octaves, 1, 8);
            change |= slider("Lacunarity", l.lacunarity, 1, 4);
            change |= slider("Roughness", l.roughness, 0, 1);
        }
    }
    if (l.kind != Kind::Layers && l.kind != Kind::Transform && l.kind != Kind::Planes &&
        l.kind != Kind::Envelope && l.kind != Kind::Solidify && l.kind != Kind::Reduce &&
        l.kind != Kind::CliffCutLoop)
    {
        ImGui::Separator();
        int mask = int(l.mask);
        if (ImGui::Combo("Mask", &mask,
                         "None\0Height\0Slope\0Noise\0Direction\0Radial\0Gradient\0"
                         "Signed curvature\0Crease proximity\0Contact occlusion\0"
                         "Convexity\0Concavity\0Ambient occlusion\0Exposure\0Boundary proximity\0Thickness\0"))
        {
            l.mask = static_cast<Mask>(mask);
            if (mask >= int(Mask::Curvature))
            {
                l.maskLow = 0;
                l.maskHigh = 1;
            }
            change = true;
        }
        if (l.mask != Mask::None)
        {
            const bool curvature = l.mask == Mask::Curvature || l.mask == Mask::Convexity ||
                                   l.mask == Mask::Concavity;
            const float range = curvature ? 20.0f : l.mask == Mask::Thickness ? 10.0f : 2.0f;
            change |= slider("Mask low", l.maskLow, -range, range);
            change |= slider("Mask high", l.maskHigh, -range, range);
            if (l.maskHigh <= l.maskLow)
                l.maskHigh = l.maskLow + 0.001f;
            change |= ImGui::Checkbox("Invert mask", &l.maskInvert);
            if (l.mask == Mask::Direction || l.mask == Mask::Gradient)
            {
                ImGui::TextWrapped("Zero mask axis inherits the operation direction.");
                change |= ImGui::SliderFloat3("Mask axis", l.maskAxis.data(), -1, 1);
            }
            if (l.mask == Mask::Exposure)
                change |= ImGui::SliderFloat3("Exposure direction", l.direction.data(), -1, 1);
            if (curvature)
                ImGui::TextWrapped("Signed inverse-length curvature: positive convex, negative concave. "
                                   "Use a negative range and invert for recessed areas.");
            if (l.mask == Mask::EdgeProximity || l.mask == Mask::Contact ||
                l.mask == Mask::AmbientOcclusion || l.mask == Mask::BoundaryProximity ||
                l.mask == Mask::Thickness)
                change |= slider("Feature radius", l.featureRadius, 0.01f, 2);
            if (l.mask == Mask::EdgeProximity)
                change |= slider("Crease angle (degrees)", l.featureAngle, 1, 120);
            if (l.mask == Mask::Contact || l.mask == Mask::AmbientOcclusion)
                change |= ImGui::SliderInt("Feature rays", &l.contactSamples, 4, 64);
        }
    }
    return change;
}
void captureSurfaceFeatures(Renderer &renderer, const std::filesystem::path &evidence)
{
    nlohmann::json proof;
    proof["adapter"] = renderer.adapter();
    proof["resolution"] = {1280, 960};
    proof["scope"] = "Standalone CPU surface features and wear; Vulkan preview, no engine integration";
    Camera camera;
    camera.distance = 5.8f;
    proof["camera"] = {{"yaw", camera.yaw}, {"pitch", camera.pitch}, {"distance", camera.distance}};
    for (int family : {5, 10, 11, 12})
        for (unsigned seed : {17U, 123U, 911U})
        {
            auto recipe = preset(family);
            for (auto &layer : recipe.layers)
                layer.seed = seed;
            auto baseline = recipe;
            for (auto &layer : baseline.layers)
            {
                if (layer.kind == Kind::SurfaceDamage || layer.kind == Kind::DamageDecal)
                    layer.enabled = false;
                if ((family == 5 || family == 11) && layer.kind == Kind::Layers)
                {
                    layer.perimeterAmplitude = 0;
                    layer.shapeOuterCaps = false;
                }
            }
            auto before = evaluate(baseline, recipe.finalResolution);
            auto after = evaluate(recipe, recipe.finalResolution);
            if (before.mesh.indices != after.mesh.indices ||
                before.mesh.triangleRegions != after.mesh.triangleRegions ||
                !PA::validateClosedMesh(after.mesh).hasValue())
                throw std::runtime_error("Surface feature proof changed topology/regions or opened mesh");
            auto label = std::string(family == 5 ? "sedimentary" : family == 10 ? "angular" : family == 11 ? "layered" : "crater") +
                         "-seed-" + std::to_string(seed);
            save(baseline, evidence / (label + "-before.json"));
            save(recipe, evidence / (label + "-after.json"));
            auto replay = evaluate(load(evidence / (label + "-after.json")), recipe.finalResolution);
            if (PA::hashTriangleMesh(after.mesh) != PA::hashTriangleMesh(replay.mesh))
                throw std::runtime_error("Surface feature proof replay mismatch");
            for (bool enabled : {false, true})
            {
                renderer.upload(enabled ? after : before);
                for (int mode : {0, 1})
                {
                    renderer.draw(1280, 960, camera, mode);
                    renderer.capture(evidence / (label + (enabled ? "-after" : "-before") +
                                                 (mode == 1 ? "-normals.png" : ".png")));
                }
            }
            double maximum = 0, sum = 0;
            std::size_t moved = 0;
            for (std::size_t i = 0; i < before.mesh.positions.size(); ++i)
            {
                auto p = before.mesh.positions[i], q = after.mesh.positions[i];
                double distance = std::hypot(p.x - q.x, p.y - q.y, p.z - q.z);
                maximum = std::max(maximum, distance);
                sum += distance;
                if (distance > 1e-10)
                    ++moved;
            }
            if (moved == 0)
                throw std::runtime_error("Surface feature proof has no visible geometry change");
            nlohmann::json entry = {{"family", family},
                                    {"seed", seed},
                                    {"meshResolution", recipe.finalResolution},
                                    {"beforeHash", PA::hashTriangleMesh(before.mesh)},
                                    {"afterHash", PA::hashTriangleMesh(after.mesh)},
                                    {"vertices", after.mesh.positions.size()},
                                    {"triangles", after.mesh.indices.size() / 3},
                                    {"movedVertices", moved},
                                    {"maxDisplacement", maximum},
                                    {"meanDisplacement", sum / before.mesh.positions.size()},
                                    {"beforeCpuMs", before.milliseconds},
                                    {"afterCpuMs", after.milliseconds},
                                    {"replay", true},
                                    {"renderBufferBytes", renderer.bufferBytes()}};
            for (auto &stage : after.stages)
                entry["stages"].push_back({{"kind", name(stage.kind)}, {"cpuMs", stage.milliseconds}});
            proof["cases"].push_back(entry);
            if (seed == 123)
            {
                auto features =
                    surfaceFeatures(before.mesh, {.contact = true,
                                                  .radius = family == 10 ? 0.3 : (family == 5 || family == 11) ? 0.2 : 0.45,
                                                  .creaseAngleDegrees = family == 10 ? 22.0 : 25.0});
                nlohmann::json fields = {{"domain", "vertex"},
                                         {"sourceHash", PA::hashTriangleMesh(before.mesh)},
                                         {"signedDihedralCurvature", features.signedDihedralCurvature},
                                         {"edgeDistance", features.edgeDistance},
                                         {"contactOcclusion", features.contactOcclusion},
                                         {"boundary", features.boundary},
                                         {"nullEdgeDistanceMeans", "no reachable crease"}};
                std::ofstream(evidence / (label + "-fields.json")) << fields.dump(2) << '\n';
                for (int kind = 0; kind < 3; ++kind)
                {
                    auto diagnostic = before;
                    for (std::size_t i = 0; i < diagnostic.field.size(); ++i)
                        diagnostic.field[i] = float(
                            kind == 0 ? 0.5 + 0.5 * std::tanh(features.signedDihedralCurvature[i] * 0.15)
                            : kind == 1
                                ? std::max(0.0, 1 - features.edgeDistance[i] /
                                                           (family == 10 ? 0.3 : (family == 5 || family == 11) ? 0.2 : 0.45))
                                : features.contactOcclusion[i]);
                    const char *fieldName = kind == 0 ? "curvature" : kind == 1 ? "edges" : "contact";
                    renderer.upload(diagnostic);
                    renderer.draw(1280, 960, camera, 3);
                    renderer.capture(evidence / (label + "-" + fieldName + ".png"));
                }
            }
        }
    std::ofstream(evidence / "surface-features-proof.json") << proof.dump(2) << '\n';
    std::cout << proof.dump(2) << '\n';
}
void captureAngularProfiles(Renderer &renderer, const std::filesystem::path &evidence)
{
    nlohmann::json proof;
    proof["scope"] = "Angular rock macroshape candidate comparison; no fracture pieces or bake acceptance";
    proof["adapter"] = renderer.adapter();
    Camera camera;
    camera.distance = 5.8f;
    for (int variant = 0; variant < 3; ++variant)
    {
        auto recipe = preset(10);
        const char *label = variant == 0 ? "baseline" : variant == 1 ? "more-cuts" : "cross-cuts";
        if (variant == 1)
        {
            recipe.layers[1].count = 12;
            recipe.layers[1].amplitude = 0.40f;
        }
        if (variant == 2)
        {
            recipe.layers[1].cutSubdivisions = 1;
            auto cross = defaultLayer(Kind::Planes);
            cross.count = 6;
            cross.amplitude = 0.34f;
            cross.seed = recipe.layers[1].seed + 937U;
            cross.cutSubdivisions = 3;
            recipe.layers.insert(recipe.layers.begin() + 2, cross);
        }
        auto result = evaluate(recipe, recipe.finalResolution);
        if (!PA::validateClosedMesh(result.mesh).hasValue())
            throw std::runtime_error("Angular profile candidate is not closed");
        save(recipe, evidence / (std::string(label) + "-recipe.json"));
        renderer.upload(result);
        for (int view = 0; view < 2; ++view)
        {
            camera.yaw = 0.5f + view * 1.57079632679f;
            renderer.draw(1280, 960, camera, 0);
            renderer.capture(evidence / (std::string(label) + "-yaw-" + std::to_string(view) + ".png"));
        }
        proof["cases"].push_back({{"name", label},
                                  {"meshHash", PA::hashTriangleMesh(result.mesh)},
                                  {"vertices", result.mesh.positions.size()},
                                  {"triangles", result.mesh.indices.size() / 3},
                                  {"cpuMs", result.milliseconds}});
    }
    std::ofstream(evidence / "angular-profiles-proof.json") << proof.dump(2) << '\n';
    std::cout << proof.dump(2) << '\n';
}
struct BakePreview
{
    Recipe recipe;
    MeshResult high;
    NormalBake bake;
};
struct AdaptiveBakeLayout
{
    std::vector<NormalBakeChart> charts;
    std::uint64_t highHash{}, lowHash{};
};
AdaptiveBakeLayout loadAdaptiveBakeLayout(const std::filesystem::path &path)
{
    std::ifstream source(path);
    if (!source)
        throw std::runtime_error("Could not open adaptive atlas layout");
    nlohmann::json manifest;
    source >> manifest;
    if (manifest.at("resolution").get<unsigned>() != 2048 ||
        manifest.at("padding").get<unsigned>() != 4 ||
        manifest.at("triangles").get<std::size_t>() != 18432)
        throw std::runtime_error("Adaptive atlas manifest has incompatible dimensions");
    AdaptiveBakeLayout result;
    result.highHash = manifest.at("sourceHighHash").get<std::uint64_t>();
    result.lowHash = manifest.at("sourceLowHash").get<std::uint64_t>();
    for (const auto &entry : manifest.at("rectanglesByTriangle"))
    {
        if (!entry.is_array() || entry.size() != 3)
            throw std::runtime_error("Adaptive atlas chart rectangle is malformed");
        result.charts.push_back({entry[0].get<unsigned>(), entry[1].get<unsigned>(),
                                 entry[2].get<unsigned>()});
    }
    return result;
}
BakePreview buildBakePreview(const Recipe &recipe, float lowError, unsigned textureResolution,
                             double projectionDistance, const Jerico::CancellationToken *cancel = nullptr,
                             NormalProjection projection = NormalProjection::ClosestSurface,
                             int lowResolution = 0, const AdaptiveBakeLayout *layout = nullptr,
                             double edgeRadius = 0.02, bool automaticUv = false,
                             double curvatureRelaxation = 0)
{
    BakePreview preview;
    preview.recipe = recipe;
    preview.high = evaluate(recipe, recipe.finalResolution, -1, -1, cancel);
    if (!preview.high.closedSolid)
        throw std::runtime_error("Low reduction requires a closed source; Solidify first");
    MeshResult low;
    if (lowResolution > 0)
    {
        if (lowResolution >= recipe.finalResolution)
            throw std::runtime_error("Regenerated low must use a lower recipe resolution");
        low = evaluate(recipe, lowResolution, -1, -1, cancel);
        low.sampledReductionError =
            sampledBidirectionalMeshError(preview.high.mesh, low.mesh, cancel);
        if (low.sampledReductionError > lowError)
            throw std::runtime_error("Regenerated low exceeds the selected sampled surface error: " +
                                     std::to_string(low.sampledReductionError));
    }
    else
    {
        Recipe reduction;
        auto reduce = defaultLayer(Kind::Reduce);
        reduce.reductionError = lowError;
        reduction.layers = {reduce};
        low = evaluateSource(reduction, preview.high, recipe.finalResolution, -1, -1, cancel);
    }
    if (low.mesh.indices.size() >= preview.high.mesh.indices.size())
        throw std::runtime_error(
            "No lower mesh within the selected error budget; adjust low error explicitly");
    if (layout && (PA::hashTriangleMesh(preview.high.mesh) != layout->highHash ||
                   PA::hashTriangleMesh(low.mesh) != layout->lowHash))
        throw std::runtime_error("Adaptive atlas layout does not match high/low mesh identity");
    preview.bake = bakeNormals(preview.high, low,
                               {.resolution = textureResolution,
                                .projectionDistance = projectionDistance,
                                .padding = 4,
                                .matchRegions = true,
                                .projection = projection,
                                .specialistMaps = true,
                                .featureRadius = 0.5,
                                .edgeRadius = edgeRadius,
                                .featureSamples = 16,
                                .curvatureScale = 0.25,
                                .curvatureRelaxation = curvatureRelaxation,
                                .thicknessRange = 4,
                                .chartLayout = layout ? layout->charts : std::vector<NormalBakeChart>{},
                                .automaticUv = automaticUv},
                               cancel);
    preview.bake.lowStrategy = lowResolution > 0 ? "recipe-regenerated" : "native-simplify";
    return preview;
}
void auditSedimentaryLowSweep(const std::filesystem::path &evidence)
{
    nlohmann::json report;
    report["scope"] = "preset 5 high resolution 32 versus regenerated low; sampled surface error and atlas capacity only, no bake";
    report["maximumSampledError"] = 0.08;
    for (unsigned seed : {7U, 17U, 123U, 911U})
    {
        auto recipe = preset(5);
        for (auto &layer : recipe.layers)
            layer.seed = seed;
        auto high = evaluate(recipe, recipe.finalResolution);
        nlohmann::json sampled;
        sampled["seed"] = seed;
        sampled["highTriangles"] = high.mesh.indices.size() / 3;
        for (int resolution : {8, 10, 12, 14, 16})
        {
            nlohmann::json entry;
            entry["lowResolution"] = resolution;
            try
            {
                auto low = evaluate(recipe, resolution);
                const auto triangles = low.mesh.indices.size() / 3;
                const auto columns = unsigned(std::ceil(std::sqrt(double(triangles))));
                const auto tile = 2048U / columns;
                entry["lowTriangles"] = triangles;
                entry["closed"] = Jerico::Tools::ProceduralAuthoring::validateClosedMesh(low.mesh).hasValue();
                entry["sampledBidirectionalError"] =
                    sampledBidirectionalMeshError(high.mesh, low.mesh);
                entry["insideErrorBudget"] = entry["sampledBidirectionalError"].get<double>() <= 0.08;
                entry["potential2048TileInteriorTexels"] = int(tile) - 8;
            }
            catch (const std::exception &error)
            {
                entry["error"] = error.what();
            }
            sampled["candidates"].push_back(std::move(entry));
        }
        report["seeds"].push_back(std::move(sampled));
    }
    std::ofstream(evidence / "sedimentary-low-resolution-sweep.json") << report.dump(2) << '\n';
    std::cout << report.dump(2) << '\n';
}
void captureNormalBakes(Renderer &renderer, const std::filesystem::path &evidence,
                        bool sedimentarySeeds = false, unsigned textureResolution = 2048,
                        int sedimentaryLowResolution = 16,
                        const AdaptiveBakeLayout *layout = nullptr, unsigned adaptiveSeed = 123,
                        bool automaticUv = false, int familyFilter = -1)
{
    nlohmann::json report;
    report["adapter"] = renderer.adapter();
    Camera camera;
    camera.distance = 5.8f;
    std::vector<std::pair<int, unsigned>> cases{{5, 123}, {8, 123}, {9, 123},
                                                 {10, 123}, {11, 123}, {12, 123}};
    if (layout)
        cases = {{5, adaptiveSeed}};
    else if (sedimentaryLowResolution == 10)
        cases = {{5, 7}, {5, 17}, {5, 123}, {5, 911}};
    else if (sedimentarySeeds)
        cases = {{5, 7}, {5, 17}, {5, 911}};
    else if (textureResolution > 2048)
        cases = {{5, 123}};
    if (familyFilter >= 0)
    {
        cases.erase(std::remove_if(cases.begin(), cases.end(), [familyFilter](const auto &entry)
                                   { return entry.first != familyFilter; }), cases.end());
        if (cases.empty())
            throw std::runtime_error("Requested automatic UV bake family is outside this proof set");
    }
    for (auto [family, seed] : cases)
    {
        auto recipe = preset(family);
        for (auto &layer : recipe.layers)
            layer.seed = seed;
        auto preview = buildBakePreview(recipe, family == 10 ? 0.005f : family == 11 ? 0.02f :
                                               family == 5 ? 0.08f : 0.04f, textureResolution,
                                        family == 5 || family == 11 ? 0.2 : 0.15, nullptr,
                                        NormalProjection::ClosestSurface,
                                        family == 5 ? sedimentaryLowResolution : 0, layout, 0.02,
                                        automaticUv);
        auto directory = evidence / ("normal-bake-" + std::to_string(family) +
                                     (sedimentarySeeds || sedimentaryLowResolution == 10 ?
                                          "-seed-" + std::to_string(seed) : ""));
        exportNormalBake(preview.bake, directory);
        save(recipe, directory / "high-recipe.json");
        camera.distance = family == 8 || family == 9 ? 10.0f : 5.8f;
        for (int variant = 0; variant < 3; ++variant)
        {
            renderer.upload(variant == 0 ? preview.high : preview.bake.low,
                            variant == 2 ? &preview.bake : nullptr);
            for (int mode : {0, 1})
            {
                renderer.draw(1280, 960, camera, mode);
                const char *name = variant == 0 ? "high" : variant == 1 ? "low" : "low-normal";
                renderer.capture(directory / (std::string(name) + (mode == 1 ? "-normals.png" : ".png")));
            }
        }
        report["cases"].push_back({{"family", family},
                                   {"seed", seed},
                                   {"lowStrategy", family == 5 ? "recipe-regenerated" : "native-simplify"},
                                   {"lowResolution", family == 5 ? 16 : recipe.finalResolution},
                                   {"highTriangles", preview.high.mesh.indices.size() / 3},
                                   {"lowTriangles", preview.bake.low.mesh.indices.size() / 3},
                                   {"automaticUv", automaticUv},
                                   {"uvIslands", preview.bake.automaticUvIslandCount},
                                   {"uvActiveAreaFraction", preview.bake.automaticUvActiveAreaFraction},
                                   {"uvSubpixelTriangles", preview.bake.automaticUvSubpixelTriangles},
                                   {"reductionSampledError", preview.bake.low.sampledReductionError},
                                   {"sampledSurfaceError", preview.bake.low.sampledReductionError},
                                   {"samples", preview.bake.samples},
                                   {"misses", preview.bake.misses},
                                   {"backfaces", preview.bake.backfaceHits},
                                   {"maxProjectionDistance", preview.bake.maximumDistance},
                                   {"bakeCpuMs", preview.bake.milliseconds}});
    }
    std::ofstream(evidence / "normal-bake-proof.json") << report.dump(2) << '\n';
    std::cout << report.dump(2) << '\n';
}
void captureRockLodBakes(Renderer &renderer, const std::filesystem::path &evidence,
                         bool automaticUv = false, int familyFilter = -1,
                         double curvatureRelaxation = 0)
{
    nlohmann::json report;
    report["adapter"] = renderer.adapter();
    report["scope"] = "C++ high source projected independently to three low-mesh LODs per rock family";
    report["automaticUv"] = automaticUv;
    report["curvatureRelaxation"] = curvatureRelaxation;
    report["imageSize"] = {1280, 960};
    report["textureResolution"] = 2048;
    Camera camera;
    for (int family : {5, 8, 9, 10, 11, 12})
    {
        if (familyFilter >= 0 && family != familyFilter)
            continue;
        auto recipe = preset(family);
        for (auto &layer : recipe.layers)
            layer.seed = 123;
        double baseError = family == 10 ? 0.005 : family == 11 ? 0.02 :
                           family == 5 ? 0.08 : 0.04;
        auto projectionDistance = family == 5 || family == 11 ? 0.2 : 0.15;
        std::size_t previousTriangles = std::numeric_limits<std::size_t>::max();
        nlohmann::json familyReport = {{"family", family}, {"lods", nlohmann::json::array()}};
        auto familyDirectory = evidence / ("rock-" + std::to_string(family));
        camera.distance = family == 8 || family == 9 ? 10.0f : 5.8f;
        for (int lod = 0; lod < 3; ++lod)
        {
            try
            {
                auto lowResolution = family == 5 ? (lod == 0 ? 16 : lod == 1 ? 12 : 10) :
                                     family == 11 ? (lod == 0 ? 20 : lod == 1 ? 16 : 14) :
                                     family == 12 && lod == 2 ? 7 : 0;
                auto error = family == 10 && lod == 2 ? 0.008f :
                             family == 9 && lod == 1 ? 0.05f :
                             family == 5 ? baseError : baseError * (1 + 0.5 * lod);
                auto preview = buildBakePreview(recipe, float(error), 2048, projectionDistance,
                                                nullptr, NormalProjection::ClosestSurface,
                                                lowResolution, nullptr,
                                                family == 8 ? 0.1 : 0.02, automaticUv,
                                                curvatureRelaxation);
                auto triangles = preview.bake.low.mesh.indices.size() / 3;
                if (triangles >= previousTriangles)
                    throw std::runtime_error("LOD does not reduce triangle count");
                previousTriangles = triangles;
                auto directory = familyDirectory / ("lod-" + std::to_string(lod));
                exportNormalBake(preview.bake, directory);
                save(recipe, directory / "high-recipe.json");
                for (int variant = 0; variant < 3; ++variant)
                {
                    renderer.upload(variant == 0 ? preview.high : preview.bake.low,
                                    variant == 2 ? &preview.bake : nullptr);
                    renderer.draw(1280, 960, camera, 0);
                    renderer.capture(directory / (variant == 0 ? "high.png" :
                                                  variant == 1 ? "low.png" : "low-normal.png"));
                }
                struct MapView
                {
                    const char *name;
                    const std::vector<std::uint8_t> *image;
                    const std::vector<std::vector<std::uint8_t>> *mips;
                };
                const MapView maps[] = {
                    {"low-ao.png", &preview.bake.ambientRgba, &preview.bake.specialistMips.ambient},
                    {"low-contact.png", &preview.bake.contactRgba, &preview.bake.specialistMips.contact},
                    {"low-cavity.png", &preview.bake.cavityRgba, &preview.bake.specialistMips.cavity},
                    {"low-convex-edge.png", &preview.bake.edgeRgba, &preview.bake.specialistMips.edge}};
                nlohmann::json diagnosticMipMaps = nlohmann::json::object();
                if (lod == 2 || (automaticUv && family == 9 && lod == 1))
                {
                    auto farCamera = camera;
                    farCamera.distance *= 2;
                    renderer.upload(preview.high);
                    renderer.draw(1280, 960, farCamera, 0);
                    renderer.capture(directory / "high-far.png");
                    renderer.upload(preview.bake.low, &preview.bake);
                    renderer.draw(1280, 960, farCamera, 0);
                    renderer.capture(directory / "low-normal-far.png");
                    if (automaticUv)
                        for (const auto &map : maps)
                        {
                            auto stem = std::string(map.name);
                            stem.resize(stem.size() - 4);
                            renderer.upload(preview.bake.low, &preview.bake, map.image);
                            auto baseBytes = renderer.bufferBytes();
                            renderer.draw(1280, 960, farCamera, 4);
                            renderer.capture(directory / (stem + "-far-base.png"));
                            renderer.upload(preview.bake.low, &preview.bake, map.image, map.mips);
                            auto mipBytes = renderer.bufferBytes();
                            if (mipBytes <= baseBytes)
                                throw std::runtime_error("Specialist mip preview did not allocate mip levels");
                            diagnosticMipMaps[stem.substr(4)] = {
                                {"levels", map.mips->size() + 1},
                                {"bytesAddedEstimate", mipBytes - baseBytes}};
                            renderer.draw(1280, 960, farCamera, 4);
                            renderer.capture(directory / (stem + "-far-mips.png"));
                        }
                }
                for (const auto &map : maps)
                {
                    renderer.upload(preview.bake.low, &preview.bake, map.image,
                                    automaticUv ? map.mips : nullptr);
                    renderer.draw(1280, 960, camera, 4);
                    renderer.capture(directory / map.name);
                }
                familyReport["lods"].push_back({{"lod", lod},
                    {"status", preview.bake.misses ? "BAKED_WITH_MISSES" : "BAKED"},
                    {"highTriangles", preview.high.mesh.indices.size() / 3},
                    {"lowTriangles", triangles}, {"highHash", preview.bake.highHash},
                    {"lowHash", preview.bake.lowHash},
                    {"diagnosticMipMaps", diagnosticMipMaps},
                    {"edgeRadius", preview.bake.settings.edgeRadius},
                    {"sampledSurfaceError", preview.bake.low.sampledReductionError},
                    {"errorBudget", error}, {"samples", preview.bake.samples},
                    {"misses", preview.bake.misses}, {"backfaces", preview.bake.backfaceHits},
                    {"bakeCpuMs", preview.bake.milliseconds}});
            }
            catch (const std::exception &error)
            {
                familyReport["lods"].push_back({{"lod", lod}, {"status", "REJECTED"},
                                                {"reason", error.what()}});
            }
        }
        report["cases"].push_back(std::move(familyReport));
        std::ofstream(evidence / "rock-lod-bakes-proof.json") << report.dump(2) << '\n';
    }
    std::cout << report.dump(2) << '\n';
}
void captureEdgeRadiusSweep(Renderer &renderer, const std::filesystem::path &evidence)
{
    nlohmann::json report;
    report["scope"] = "C++ unquantized high curvature edge-radius diagnostic; same high and LOD0 low per family";
    report["adapter"] = renderer.adapter();
    Camera camera;
    for (int family : {5, 8, 10, 12})
    {
        auto recipe = preset(family);
        for (auto &layer : recipe.layers)
            layer.seed = 123;
        auto baseline = buildBakePreview(recipe, family == 10 ? 0.005f : family == 5 ? 0.08f : 0.04f,
                                         2048, family == 5 ? 0.2 : 0.15, nullptr,
                                         NormalProjection::ClosestSurface, family == 5 ? 16 : 0);
        camera.distance = family == 8 ? 10.0f : 5.8f;
        nlohmann::json familyReport = {{"family", family},
                                       {"highHash", baseline.bake.highHash},
                                       {"lowHash", baseline.bake.lowHash},
                                       {"radii", nlohmann::json::array()}};
        for (double radius : {0.02, 0.1, 0.2})
        {
            auto bake = baseline.bake;
            if (radius != 0.02)
            {
                auto settings = baseline.bake.settings;
                settings.edgeRadius = radius;
                bake = bakeNormals(baseline.high, baseline.bake.low, settings);
                bake.lowStrategy = baseline.bake.lowStrategy;
            }
            auto directory = evidence / ("rock-" + std::to_string(family)) /
                             ("edge-radius-" + std::to_string(int(std::lround(radius * 100))));
            exportNormalBake(bake, directory);
            save(recipe, directory / "high-recipe.json");
            renderer.upload(bake.low, &bake, &bake.edgeRgba);
            renderer.draw(1280, 960, camera, 4);
            renderer.capture(directory / "low-convex-edge.png");
            familyReport["radii"].push_back({{"metres", radius},
                                             {"samples", bake.samples},
                                             {"misses", bake.misses},
                                             {"bakeCpuMs", bake.milliseconds}});
        }
        report["cases"].push_back(std::move(familyReport));
        std::ofstream(evidence / "edge-radius-sweep-proof.json") << report.dump(2) << '\n';
    }
    std::cout << report.dump(2) << '\n';
}
void captureRegeneratedLodSweep(Renderer &renderer, const std::filesystem::path &evidence)
{
    nlohmann::json report;
    report["scope"] = "Same-recipe regenerated low candidates versus current native LOD0; independent high-to-low bake";
    report["adapter"] = renderer.adapter();
    for (int family : {9, 10, 12})
    {
        auto recipe = preset(family);
        for (auto &layer : recipe.layers)
            layer.seed = 123;
        const auto baseError = family == 10 ? 0.005f : 0.04f;
        const auto maximumError = family == 10 ? 0.01 : 0.08;
        auto baseline = buildBakePreview(recipe, baseError, 2048, 0.15);
        nlohmann::json familyReport = {{"family", family},
                                       {"highHash", baseline.bake.highHash},
                                       {"currentLod0Hash", baseline.bake.lowHash},
                                       {"currentLod0Triangles", baseline.bake.low.mesh.indices.size() / 3},
                                       {"maximumSampledError", maximumError},
                                       {"candidates", nlohmann::json::array()}};
        std::vector<int> resolutions = family == 9 ? std::vector<int>{32, 24, 20} :
                                       family == 10 ? std::vector<int>{16, 12, 8, 6, 4} :
                                                      std::vector<int>{8, 7, 6, 5, 4};
        Camera camera;
        camera.distance = family == 9 ? 10.0f : 5.8f;
        for (auto resolution : resolutions)
        {
            nlohmann::json entry = {{"lowResolution", resolution}};
            try
            {
                auto low = evaluate(recipe, resolution);
                entry["lowCpuMs"] = low.milliseconds;
                entry["lowTriangles"] = low.mesh.indices.size() / 3;
                entry["lowHash"] = PA::hashTriangleMesh(low.mesh);
                entry["closed"] = PA::validateClosedMesh(low.mesh).hasValue();
                if (!entry["closed"].get<bool>())
                    throw std::runtime_error("Regenerated candidate is not a closed solid");
                auto errorStart = std::chrono::steady_clock::now();
                low.sampledReductionError = sampledBidirectionalMeshError(baseline.high.mesh, low.mesh);
                entry["sampledSurfaceError"] = low.sampledReductionError;
                entry["errorCpuMs"] = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - errorStart).count();
                if (low.mesh.indices.size() >= baseline.bake.low.mesh.indices.size())
                    throw std::runtime_error("Candidate is not lower than current LOD0");
                if (low.sampledReductionError > maximumError)
                    throw std::runtime_error("Candidate exceeds current LOD2 surface-error budget");
                auto bake = bakeNormals(baseline.high, low, baseline.bake.settings);
                bake.lowStrategy = "recipe-regenerated";
                auto directory = evidence / ("rock-" + std::to_string(family)) /
                                 ("low-resolution-" + std::to_string(resolution));
                exportNormalBake(bake, directory);
                save(recipe, directory / "high-recipe.json");
                renderer.upload(bake.low, &bake);
                renderer.draw(1280, 960, camera, 0);
                renderer.capture(directory / "low-normal.png");
                for (const auto &[name, map] :
                     {std::pair<const char *, const std::vector<std::uint8_t> *>{"low-cavity.png", &bake.cavityRgba},
                      {"low-convex-edge.png", &bake.edgeRgba}})
                {
                    renderer.upload(bake.low, &bake, map);
                    renderer.draw(1280, 960, camera, 4);
                    renderer.capture(directory / name);
                }
                entry["status"] = bake.misses ? "BAKED_WITH_MISSES" : "BAKED";
                entry["samples"] = bake.samples;
                entry["misses"] = bake.misses;
                entry["missesOtherRegionFront"] = bake.missesOtherRegionFront;
                entry["missesNearbyBackface"] = bake.missesNearbyBackface;
                entry["missesBeyondRadius"] = bake.missesBeyondRadius;
                entry["bakeCpuMs"] = bake.milliseconds;
            }
            catch (const std::exception &error)
            {
                entry["status"] = "REJECTED";
                entry["reason"] = error.what();
            }
            familyReport["candidates"].push_back(std::move(entry));
        }
        report["cases"].push_back(std::move(familyReport));
        std::ofstream(evidence / "regenerated-lod-sweep-proof.json") << report.dump(2) << '\n';
    }
    std::cout << report.dump(2) << '\n';
}
void captureAngularLod2Sweep(Renderer &renderer, const std::filesystem::path &evidence)
{
    auto recipe = preset(10);
    for (auto &layer : recipe.layers)
        layer.seed = 123;
    auto lod1 = buildBakePreview(recipe, 0.0075f, 2048, 0.15);
    nlohmann::json report = { {"scope", "Angular LOD2 strict simplification sweep; unchanged high source and projection cage"},
                              {"adapter", renderer.adapter()},
                              {"highHash", lod1.bake.highHash},
                              {"lod1Triangles", lod1.bake.low.mesh.indices.size() / 3},
                              {"lod1Misses", lod1.bake.misses},
                              {"candidates", nlohmann::json::array()} };
    Camera camera;
    camera.distance = 5.8f;
    for (float error : {0.008f, 0.00825f, 0.0085f, 0.00875f, 0.009f, 0.00925f})
    {
        nlohmann::json entry = {{"errorBudget", error}};
        try
        {
            auto candidate = buildBakePreview(recipe, error, 2048, 0.15);
            auto triangles = candidate.bake.low.mesh.indices.size() / 3;
            entry["lowTriangles"] = triangles;
            entry["lowHash"] = candidate.bake.lowHash;
            entry["sampledSurfaceError"] = candidate.bake.low.sampledReductionError;
            entry["samples"] = candidate.bake.samples;
            entry["misses"] = candidate.bake.misses;
            entry["missesOtherRegionFront"] = candidate.bake.missesOtherRegionFront;
            entry["missesNearbyBackface"] = candidate.bake.missesNearbyBackface;
            entry["missesBeyondRadius"] = candidate.bake.missesBeyondRadius;
            entry["bakeCpuMs"] = candidate.bake.milliseconds;
            if (triangles >= lod1.bake.low.mesh.indices.size() / 3)
                throw std::runtime_error("Candidate does not reduce LOD1 triangle count");
            auto directory = evidence / ("error-" + std::to_string(int(std::round(error * 100000))));
            exportNormalBake(candidate.bake, directory);
            save(recipe, directory / "high-recipe.json");
            renderer.upload(candidate.bake.low, &candidate.bake);
            renderer.draw(1280, 960, camera, 0);
            renderer.capture(directory / "low-normal.png");
            for (const auto &[name, map] :
                 {std::pair<const char *, const std::vector<std::uint8_t> *>{"low-ao.png", &candidate.bake.ambientRgba},
                  {"low-contact.png", &candidate.bake.contactRgba},
                  {"low-cavity.png", &candidate.bake.cavityRgba},
                  {"low-convex-edge.png", &candidate.bake.edgeRgba}})
            {
                renderer.upload(candidate.bake.low, &candidate.bake, map);
                renderer.draw(1280, 960, camera, 4);
                renderer.capture(directory / name);
            }
            entry["status"] = candidate.bake.misses ? "BAKED_WITH_MISSES" : "BAKED";
        }
        catch (const std::exception &error)
        {
            entry["status"] = "REJECTED";
            entry["reason"] = error.what();
        }
        report["candidates"].push_back(std::move(entry));
        std::ofstream(evidence / "angular-lod2-sweep-proof.json") << report.dump(2) << '\n';
    }
    std::cout << report.dump(2) << '\n';
}
void captureNormalBakeOrbit(Renderer &renderer, const std::filesystem::path &evidence,
                            int lowResolution = 16, const AdaptiveBakeLayout *layout = nullptr,
                            unsigned seed = 123)
{
    auto recipe = preset(5);
    for (auto &layer : recipe.layers)
        layer.seed = seed;
    auto preview = buildBakePreview(recipe, 0.08f, 2048, 0.2, nullptr,
                                    NormalProjection::ClosestSurface, lowResolution, layout);
    nlohmann::json report;
    report["adapter"] = renderer.adapter();
    report["resolution"] = {1280, 960};
    report["seed"] = seed;
    report["lowResolution"] = lowResolution;
    report["highHash"] = preview.bake.highHash;
    report["lowHash"] = preview.bake.lowHash;
    report["samples"] = preview.bake.samples;
    report["misses"] = preview.bake.misses;
    for (int view = 0; view < 4; ++view)
    {
        Camera camera;
        camera.distance = 5.8f;
        camera.yaw = 0.5f + view * 1.57079632679f;
        const auto directory = evidence / ("normal-bake-5-yaw-" + std::to_string(view));
        std::filesystem::create_directories(directory);
        for (int variant = 0; variant < 3; ++variant)
        {
            renderer.upload(variant == 0 ? preview.high : preview.bake.low,
                            variant == 2 ? &preview.bake : nullptr);
            renderer.draw(1280, 960, camera, 0);
            const char *name = variant == 0 ? "high.png" :
                               variant == 1 ? "low.png" : "low-normal.png";
            renderer.capture(directory / name);
        }
        report["views"].push_back({{"index", view}, {"yaw", camera.yaw},
                                    {"pitch", camera.pitch}, {"distance", camera.distance}});
    }
    std::ofstream(evidence / "normal-bake-orbit-proof.json") << report.dump(2) << '\n';
    std::cout << report.dump(2) << '\n';
}
struct Build
{
    std::future<MeshResult> future;
    std::shared_ptr<Jerico::CancellationSource> cancel;
    unsigned generation{};
    int resolution{};
};
} // namespace
int run(int argc, char **argv)
{
    bool smoke = false, featureSmoke = false, angularProfileSmoke = false, rockLodSmoke = false,
         autoUvRockLodSmoke = false, curvatureRelaxationSmoke = false,
         edgeRadiusSmoke = false, regeneratedLodSmoke = false, angularLod2Smoke = false,
         bakeSmoke = false, seededBakeSmoke = false,
         orbitBakeSmoke = false, scaleBakeSmoke = false, lowSweepSmoke = false,
         low10BakeSmoke = false, low10OrbitSmoke = false, adaptiveBakeSmoke = false,
         adaptiveOrbitSmoke = false, automaticUvBakeSmoke = false;
    unsigned scaleTextureResolution = 4096;
    std::filesystem::path evidence = std::filesystem::path(PLAYGROUND_ROOT) / "Evidence";
    std::filesystem::path adaptiveLayoutPath;
    unsigned adaptiveSeed = 123;
    int automaticUvFamily = -1;
    int automaticUvRockLodFamily = -1;
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--smoke-test" || std::string(argv[i]) == "--surface-features-test" ||
            std::string(argv[i]) == "--angular-profiles-test" ||
            std::string(argv[i]) == "--rock-lod-bakes-test" ||
            std::string(argv[i]) == "--auto-uv-rock-lod-bakes-test" ||
            std::string(argv[i]) == "--curvature-relaxation-bakes-test" ||
            std::string(argv[i]) == "--edge-radius-sweep-test" ||
            std::string(argv[i]) == "--regenerated-lod-sweep-test" ||
            std::string(argv[i]) == "--angular-lod2-sweep-test" ||
            std::string(argv[i]) == "--normal-bake-test" ||
            std::string(argv[i]) == "--normal-bake-seeds-test" ||
            std::string(argv[i]) == "--normal-bake-orbit-test" ||
            std::string(argv[i]) == "--normal-bake-scale-test" ||
            std::string(argv[i]) == "--normal-bake-3072-test" ||
            std::string(argv[i]) == "--sedimentary-low-sweep-test" ||
            std::string(argv[i]) == "--normal-bake-low10-test" ||
            std::string(argv[i]) == "--normal-bake-low10-orbit-test" ||
            std::string(argv[i]) == "--normal-bake-adaptive-test" ||
            std::string(argv[i]) == "--normal-bake-adaptive-orbit-test" ||
            std::string(argv[i]) == "--auto-uv-bake-test")
        {
            featureSmoke = std::string(argv[i]) == "--surface-features-test";
            angularProfileSmoke = std::string(argv[i]) == "--angular-profiles-test";
            rockLodSmoke = std::string(argv[i]) == "--rock-lod-bakes-test";
            autoUvRockLodSmoke = std::string(argv[i]) == "--auto-uv-rock-lod-bakes-test";
            curvatureRelaxationSmoke = std::string(argv[i]) == "--curvature-relaxation-bakes-test";
            edgeRadiusSmoke = std::string(argv[i]) == "--edge-radius-sweep-test";
            regeneratedLodSmoke = std::string(argv[i]) == "--regenerated-lod-sweep-test";
            angularLod2Smoke = std::string(argv[i]) == "--angular-lod2-sweep-test";
            bakeSmoke = std::string(argv[i]) == "--normal-bake-test";
            seededBakeSmoke = std::string(argv[i]) == "--normal-bake-seeds-test";
            orbitBakeSmoke = std::string(argv[i]) == "--normal-bake-orbit-test";
            scaleBakeSmoke = std::string(argv[i]) == "--normal-bake-scale-test" ||
                             std::string(argv[i]) == "--normal-bake-3072-test";
            lowSweepSmoke = std::string(argv[i]) == "--sedimentary-low-sweep-test";
            low10BakeSmoke = std::string(argv[i]) == "--normal-bake-low10-test";
            low10OrbitSmoke = std::string(argv[i]) == "--normal-bake-low10-orbit-test";
            adaptiveBakeSmoke = std::string(argv[i]) == "--normal-bake-adaptive-test";
            adaptiveOrbitSmoke = std::string(argv[i]) == "--normal-bake-adaptive-orbit-test";
            automaticUvBakeSmoke = std::string(argv[i]) == "--auto-uv-bake-test";
            if (std::string(argv[i]) == "--normal-bake-3072-test")
                scaleTextureResolution = 3072;
            smoke = true;
            if (i + 1 < argc)
                evidence = argv[++i];
            if (automaticUvBakeSmoke && i + 1 < argc)
                automaticUvFamily = std::stoi(argv[++i]);
            if (autoUvRockLodSmoke && i + 1 < argc)
                automaticUvRockLodFamily = std::stoi(argv[++i]);
            if (adaptiveBakeSmoke || adaptiveOrbitSmoke)
            {
                if (i + 2 >= argc)
                    throw std::runtime_error("Adaptive bake needs layout manifest and seed");
                adaptiveLayoutPath = argv[++i];
                adaptiveSeed = unsigned(std::stoul(argv[++i]));
            }
        }
    if (smoke)
        std::filesystem::create_directories(evidence);
    if (lowSweepSmoke)
    {
        auditSedimentaryLowSweep(evidence);
        return 0;
    }
    auto created = Jerico::Platform::Window::create({"Jerico Geology Playground", 1440, 900, true});
    if (!created.hasValue())
        throw std::runtime_error(created.error().message);
    auto window = std::move(created.value());
    auto hwnd = reinterpret_cast<HWND>(window.nativeHandle().value);
    Renderer renderer(hwnd, window.clientWidth(), window.clientHeight());
    previousProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(inputProc)));
    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 3;
    style.FrameRounding = 3;
    style.FramePadding = {6, 5};
    style.ItemSpacing = {8, 7};
    Recipe recipe = preset(0);
    MeshResult displayed = evaluate(recipe, recipe.previewResolution);
    renderer.upload(displayed);
    Camera camera;
    int selected = 0, presetIndex = 0, mode = 0, inspection = 0, addKind = 1;
    std::string error, status = "Ready", path;
    bool dirty = true, wasActive = false;
    unsigned generation = 1, frames = 0;
    int builtResolution = recipe.previewResolution;
    Build build;
    std::future<BakePreview> bakeFuture;
    std::shared_ptr<Jerico::CancellationSource> bakeCancel;
    std::unique_ptr<BakePreview> bakedPreview;
    unsigned bakeGeneration{};
    float lowError = 0.04f, projectionDistance = 0.15f;
    int bakeView = 0, textureChoice = 1, projectionChoice = 1;
    int lowStrategy = 0, regeneratedResolution = 16;
    GpuComparison comparison;
    bool hasComparison = false;
    auto lastEdit = std::chrono::steady_clock::now();
    std::vector<Recipe> undo, redo;
    auto remember = [&](const Recipe &before)
    {
        if (undo.size() >= 64)
            undo.erase(undo.begin());
        undo.push_back(before);
        redo.clear();
    };
    auto changed = [&]
    {
        if (bakeCancel)
            bakeCancel->requestCancellation();
        if (bakedPreview)
        {
            bakedPreview.reset();
            bakeView = 0;
            renderer.upload(displayed);
        }
        hasComparison = false;
        ++generation;
        dirty = true;
        lastEdit = std::chrono::steady_clock::now();
        if (build.cancel)
            build.cancel->requestCancellation();
    };
    auto replace = [&](Recipe next)
    {
        validate(next);
        remember(recipe);
        recipe = std::move(next);
        selected = recipe.layers.empty() ? -1 : 0;
        error.clear();
        changed();
    };
    while (window.pumpEvents())
    {
        const auto w = window.clientWidth(), h = window.clientHeight();
        if (w < 100 || h < 100)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        // Drain the engine keyboard queue; ImGui's platform backend owns UI text input.
        while (true)
        {
            auto event = window.pollInputEvent();
            if (!event.hasValue() || !event.value())
                break;
        }
        if (build.future.valid() &&
            build.future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            try
            {
                auto next = build.future.get();
                if (build.generation == generation)
                {
                    displayed = std::move(next);
                    renderer.upload(displayed);
                    builtResolution = build.resolution;
                    status = "Ready";
                    error.clear();
                }
            }
            catch (const std::exception &e)
            {
                if (build.generation == generation)
                {
                    error = e.what();
                    status = "Last valid mesh retained";
                }
            }
            build.cancel.reset();
        }
        if (bakeFuture.valid() &&
            bakeFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            try
            {
                auto ready = bakeFuture.get();
                if (bakeGeneration == generation)
                {
                    bakedPreview = std::make_unique<BakePreview>(std::move(ready));
                    bakeView = 2;
                    renderer.upload(bakedPreview->bake.low, &bakedPreview->bake);
                    status = "Low + baked high normals";
                    error.clear();
                }
            }
            catch (const std::exception &e)
            {
                if (bakeGeneration == generation)
                    error = e.what();
            }
            bakeCancel.reset();
        }
        renderer.begin(w, h);
        float left = 285, right = 360, bottom = 78, top = 54;
        if (w < 1200)
        {
            left = 245;
            right = 300;
        }
        auto panel = [&](const char *title, float x, float y, float width, float height)
        {
            ImGui::SetNextWindowPos({x, y});
            ImGui::SetNextWindowSize({width, height});
            ImGui::Begin(title, nullptr,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        };
        panel("Jerico Geology Playground", 0, 0, float(w), top);
        ImGui::TextUnformatted(
            "Independent experiment | Shared Jerico window, mesh types and CPU FBM | No Blender");
        ImGui::End();
        bool anyChange = false;
        Recipe before = recipe;
        panel("Operations", 0, top, left, h - top - bottom);
        if (ImGui::Button("New"))
        {
            replace(Recipe{});
        }
        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            try
            {
                auto file = chooseFile(hwnd, true);
                if (!file.empty())
                {
                    save(recipe, file);
                    path = file.string();
                    status = "Recipe saved";
                }
            }
            catch (const std::exception &e)
            {
                error = e.what();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load"))
        {
            try
            {
                auto file = chooseFile(hwnd, false);
                if (!file.empty())
                {
                    replace(load(file));
                    path = file.string();
                    status = "Recipe loaded";
                }
            }
            catch (const std::exception &e)
            {
                error = e.what();
            }
        }
        if (ImGui::Button("Undo") && !undo.empty())
        {
            redo.push_back(recipe);
            recipe = undo.back();
            undo.pop_back();
            selected = std::min(selected, int(recipe.layers.size()) - 1);
            changed();
        }
        ImGui::SameLine();
        if (ImGui::Button("Redo") && !redo.empty())
        {
            undo.push_back(recipe);
            recipe = redo.back();
            redo.pop_back();
            selected = std::min(selected, int(recipe.layers.size()) - 1);
            changed();
        }
        if (ImGui::Combo(
                "Preset", &presetIndex,
                "Basic Rock\0Layered Rock\0Jagged Rock\0Sedimentary Experiment\0Cliff "
                "Experiment\0Sedimentary v2\0Jagged planes v2\0Plane cliff\0Solid cliff\0BSP geology\0"
                "Angular wear v4\0Layered rock v9\0Crater rock v5\0"))
        {
            replace(preset(presetIndex));
            lowError = presetIndex == 10 ? 0.005f : presetIndex == 11 ? 0.02f :
                       presetIndex == 5 ? 0.08f : 0.04f;
            projectionDistance = presetIndex == 5 || presetIndex == 11 ? 0.2f : 0.15f;
            lowStrategy = presetIndex == 5 ? 1 : 0;
        }
        int base = int(recipe.base);
        if (ImGui::Combo("Base", &base,
                         "Cube\0Sphere\0Cylinder\0Cone\0Plane / Wall\0BSP wall (engine fixture)\0"))
        {
            recipe.base = static_cast<Base>(base);
            anyChange = true;
        }
        if (recipe.base == Base::Plane)
            anyChange |= ImGui::SliderFloat2("Wall size", recipe.planeSize.data(), 0.1f, 20);
        ImGui::Separator();
        for (int i = 0; i < int(recipe.layers.size()); ++i)
        {
            ImGui::PushID(i);
            anyChange |= ImGui::Checkbox("##enabled", &recipe.layers[i].enabled);
            ImGui::SameLine();
            if (ImGui::Selectable(name(recipe.layers[i].kind), i == selected))
            {
                selected = i;
                if (inspection != 0)
                    changed();
            }
            ImGui::PopID();
        }
        ImGui::Separator();
        ImGui::Combo(
            "Operation", &addKind,
            "Transform\0Normal / FBM\0Vector / FBM\0Voronoi\0Layer formation\0Domain warp\0Surface "
            "fracture\0Relax\0Structural planes\0Shared macro warp\0Macroform envelope\0Layer edge "
            "breakup\0Strata relief\0Ridged noise\0Solidify surface\0Reduce mesh\0Surface damage / wear\0"
            "Oriented damage decal\0Layer settling\0Cliff cut loop\0");
        if (ImGui::Button("+ Add layer", {-1, 0}) && recipe.layers.size() < 24)
        {
            recipe.layers.push_back(defaultLayer(static_cast<Kind>(addKind)));
            selected = int(recipe.layers.size()) - 1;
            anyChange = true;
        }
        if (selected >= 0 && selected < int(recipe.layers.size()))
        {
            if (ImGui::Button("Up") && selected > 0)
            {
                std::swap(recipe.layers[selected], recipe.layers[selected - 1]);
                --selected;
                anyChange = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Down") && selected + 1 < int(recipe.layers.size()))
            {
                std::swap(recipe.layers[selected], recipe.layers[selected + 1]);
                ++selected;
                anyChange = true;
            }
            if (ImGui::Button("Duplicate") && recipe.layers.size() < 24)
            {
                auto copy = recipe.layers[selected];
                recipe.layers.insert(recipe.layers.begin() + selected + 1, copy);
                ++selected;
                anyChange = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete"))
            {
                recipe.layers.erase(recipe.layers.begin() + selected);
                selected = std::min(selected, int(recipe.layers.size()) - 1);
                anyChange = true;
            }
            if (selected >= 0 && ImGui::Button("Reset layer"))
            {
                recipe.layers[selected] = defaultLayer(recipe.layers[selected].kind);
                anyChange = true;
            }
        }
        if (ImGui::Button("Reset recipe"))
        {
            replace(Recipe{});
        }
        ImGui::End();
        panel("Parameters", float(w) - right, top, right, h - top - bottom);
        if (selected >= 0 && selected < int(recipe.layers.size()))
        {
            ImGui::TextWrapped("%s", name(recipe.layers[selected].kind));
            anyChange |= editLayer(recipe.layers[selected]);
        }
        else
            ImGui::TextWrapped("Choose a base and add an operation.");
        ImGui::Separator();
        anyChange |= ImGui::SliderInt("Preview res.", &recipe.previewResolution, 4, 40);
        anyChange |= ImGui::SliderInt("Final res.", &recipe.finalResolution, 8, 64);
        ImGui::TextWrapped(
            "Resolution is subdivisions per primitive face, not voxel count. Topology builds on CPU.");
        bool canCompare = selected >= 0 && recipe.layers[selected].kind == Kind::NormalNoise && !dirty &&
                          !build.future.valid() && !anyChange && error.empty();
        ImGui::BeginDisabled(!canCompare);
        if (ImGui::Button("Compare scalar FBM: GPU / engine CPU"))
        {
            try
            {
                Layer l = selected >= 0 ? recipe.layers[selected] : defaultLayer(Kind::NormalNoise);
                comparison = renderer.compareFbm(displayed, l);
                hasComparison = true;
            }
            catch (const std::exception &e)
            {
                error = e.what();
            }
        }
        ImGui::EndDisabled();
        if (!canCompare)
            ImGui::TextWrapped("Select a normal FBM layer and wait for generation to finish.");
        if (hasComparison)
        {
            if (comparison.gpuMilliseconds >= 0)
                ImGui::Text("GPU dispatch only: %.3f ms", comparison.gpuMilliseconds);
            else
                ImGui::TextUnformatted("GPU timing unavailable");
            ImGui::Text("CPU reference + check: %.3f ms", comparison.cpuMilliseconds);
            ImGui::Text("Max scalar error: %.3g", comparison.maxError);
            ImGui::TextWrapped("Scalar FBM sampled at displayed vertices. Excludes GPU upload/readback. "
                               "Different timing scopes: not a speedup benchmark or M8 parity test.");
        }
        ImGui::Separator();
        ImGui::TextUnformatted("High -> Low -> normal map");
        ImGui::Combo("Low mesh", &lowStrategy, "Simplify high\0Regenerate recipe\0");
        if (lowStrategy == 1)
        {
            regeneratedResolution = std::clamp(regeneratedResolution, 4,
                                                std::max(4, recipe.finalResolution - 1));
            ImGui::SliderInt("Low recipe resolution", &regeneratedResolution, 4,
                             std::max(4, recipe.finalResolution - 1));
        }
        ImGui::SliderFloat("Low surface error", &lowError, 0.001f, 0.15f, "%.3f");
        ImGui::SliderFloat("Projection distance", &projectionDistance, 0.01f, 0.5f, "%.3f");
        ImGui::Combo("Normal texture", &textureChoice, "1024\0 2048\0 4096\0");
        ImGui::Combo("Projection", &projectionChoice, "Normal rays\0Nearest same-region surface\0");
        ImGui::BeginDisabled(dirty || anyChange || build.future.valid() || bakeFuture.valid() ||
                             builtResolution != recipe.finalResolution ||
                             (lowStrategy == 1 && recipe.finalResolution <= 4));
        if (ImGui::Button("Build low + bake high normals"))
        {
            auto snapshot = recipe;
            bakeCancel = std::make_shared<Jerico::CancellationSource>();
            auto cancel = bakeCancel;
            bakeGeneration = generation;
            auto tolerance = lowError, distance = projectionDistance;
            unsigned resolution = 1024U << textureChoice;
            auto projection = static_cast<NormalProjection>(projectionChoice);
            auto lowResolution = lowStrategy == 1 ? regeneratedResolution : 0;
            bakeFuture = std::async(std::launch::async,
                                    [snapshot, cancel, tolerance, distance, resolution, projection,
                                     lowResolution]
                                    {
                                        auto token = cancel->token();
                                        return buildBakePreview(snapshot, tolerance, resolution, distance,
                                                                &token, projection, lowResolution);
                                    });
            status = "Building low and baking high normals";
        }
        ImGui::EndDisabled();
        if (bakeFuture.valid() && ImGui::Button("Cancel bake"))
            bakeCancel->requestCancellation();
        if (bakedPreview)
        {
            auto &b = bakedPreview->bake;
            if (ImGui::Combo("Geometry preview", &bakeView, "High\0Low\0Low + normal map\0"))
                renderer.upload(bakeView == 0 ? bakedPreview->high : b.low, bakeView == 2 ? &b : nullptr);
            ImGui::Text("Triangles: %zu high / %zu low", bakedPreview->high.mesh.indices.size() / 3,
                        b.low.mesh.indices.size() / 3);
            ImGui::Text("Coverage: %.3f%% | Misses: %zu",
                        100 * (1 - double(b.misses) / std::max(std::size_t(1), b.samples)), b.misses);
            ImGui::Text("Backfaces: %zu | Bake CPU: %.1f ms", b.backfaceHits, b.milliseconds);
            if (ImGui::Button("Export low OBJ + normal PNG"))
            {
                try
                {
                    auto directory = evidence / ("normal-bake-" + std::to_string(GetTickCount64()));
                    exportNormalBake(b, directory);
                    save(bakedPreview->recipe, directory / "high-recipe.json");
                    status = "Exported " + directory.string();
                }
                catch (const std::exception &e)
                {
                    error = e.what();
                }
            }
            ImGui::TextWrapped("Experimental triangle UV atlas. Preserve UVs and split normals; no "
                               "silhouette detail is stored in a normal map.");
            ImGui::TextWrapped("Export also includes native AO, signed curvature, inward thickness and "
                               "semantic region-ID maps from the same projection coverage.");
        }
        if (!error.empty())
            ImGui::TextWrapped("Error: %s", error.c_str());
        ImGui::End();
        if (anyChange)
        {
            remember(before);
            changed();
        }
        panel("Viewport", left, top, float(w) - left - right, h - top - bottom);
        ImGui::SetNextItemWidth(140);
        ImGui::Combo("View", &mode, "Lit\0Normals\0Wireframe\0Field\0");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(190);
        if (ImGui::Combo("Inspect", &inspection,
                         "Final result\0Up to selected layer\0Selected layer field\0"))
        {
            if (inspection == 2)
                mode = 3;
            changed();
        }
        if (mode == 3 && inspection != 2)
        {
            inspection = 2;
            changed();
        }
        ImGui::SameLine();
        if (ImGui::Button("Fit"))
        {
            camera = {};
            if (!displayed.mesh.positions.empty())
            {
                auto lo = displayed.mesh.positions.front(), hi = lo;
                for (auto p : displayed.mesh.positions)
                {
                    lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
                    hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
                }
                camera.target = {float((lo.x + hi.x) * 0.5), float((lo.y + hi.y) * 0.5),
                                 float((lo.z + hi.z) * 0.5)};
                float radius = float(std::sqrt((hi.x - lo.x) * (hi.x - lo.x) + (hi.y - lo.y) * (hi.y - lo.y) +
                                               (hi.z - lo.z) * (hi.z - lo.z)) *
                                     0.5);
                float aspect = std::max(0.2f, (float(w) - left - right) / (float(h) - top - bottom - 75));
                camera.distance =
                    std::clamp(radius * (1 + 2.4f / std::min(1.0f, aspect)) * 1.1f, 1.0f, 90.0f);
            }
        }
        ImGui::TextUnformatted("Drag left: orbit | Middle/right: pan | Wheel: zoom");
        auto size = ImGui::GetContentRegionAvail();
        size.x = std::max(size.x, 32.0f);
        size.y = std::max(size.y, 32.0f);
        auto texture = renderer.draw(unsigned(size.x), unsigned(size.y), camera, mode);
        ImGui::Image(reinterpret_cast<ImTextureID>(texture), size);
        if (ImGui::IsItemHovered())
        {
            auto &io = ImGui::GetIO();
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                camera.yaw -= io.MouseDelta.x * 0.008f;
                camera.pitch = std::clamp(camera.pitch + io.MouseDelta.y * 0.008f, -1.4f, 1.4f);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
                ImGui::IsMouseDragging(ImGuiMouseButton_Right))
            {
                float dx = -io.MouseDelta.x * camera.distance * 0.0018f;
                float dy = io.MouseDelta.y * camera.distance * 0.0018f;
                float cy = std::cos(camera.yaw), sy = std::sin(camera.yaw);
                float cp = std::cos(camera.pitch), sp = std::sin(camera.pitch);
                camera.target[0] += dx * cy - dy * sp * sy;
                camera.target[1] += dy * cp;
                camera.target[2] += -dx * sy - dy * sp * cy;
            }
            camera.distance = std::clamp(camera.distance * std::exp(-io.MouseWheel * 0.1f), 1.0f, 90.0f);
        }
        ImGui::End();
        panel("Metrics", 0, float(h) - bottom, float(w), bottom);
        ImGui::Text("%s | Mesh generation CPU: %.2f ms | Vertices: %zu | Triangles: %zu | Resolution: %d | "
                    "GPU buffers + targets estimate: %.2f MiB",
                    status.c_str(), displayed.milliseconds, displayed.mesh.positions.size(),
                    displayed.mesh.indices.size() / 3, builtResolution, renderer.bufferBytes() / 1048576.0);
        ImGui::Text("%s | %s | BSP selection: %zu | GPU stack: CPU reference", renderer.adapter().c_str(),
                    displayed.closedSolid ? "SOLID" : "SURFACE", displayed.sourceSelection.size());
        if (displayed.trianglesBeforeReduction)
            ImGui::Text("Reduction: %zu -> %zu triangles | sampled error %.6f",
                        displayed.trianglesBeforeReduction, displayed.mesh.indices.size() / 3,
                        displayed.sampledReductionError);
        ImGui::End();
        bool active = ImGui::IsAnyItemActive();
        if (wasActive && !active)
        {
            dirty = true;
        }
        wasActive = active;
        if (dirty && !build.future.valid() && !bakeFuture.valid())
        {
            bool preview =
                active || std::chrono::steady_clock::now() - lastEdit < std::chrono::milliseconds(160);
            int resolution = preview ? recipe.previewResolution : recipe.finalResolution;
            auto snapshot = recipe;
            int stop = inspection != 0 ? selected : -1;
            int field = inspection == 2 ? selected : -1;
            build.cancel = std::make_shared<Jerico::CancellationSource>();
            auto cancel = build.cancel;
            build.generation = generation;
            build.resolution = resolution;
            build.future = std::async(std::launch::async,
                                      [snapshot, resolution, stop, field, cancel]
                                      {
                                          auto token = cancel->token();
                                          return evaluate(snapshot, resolution, stop, field, &token);
                                      });
            dirty = false;
            status = preview ? "Building preview" : "Building final";
        }
        if (!active && !dirty && !build.future.valid() && builtResolution != recipe.finalResolution &&
            error.empty())
            dirty = true;
        renderer.present();
        if (smoke && ++frames == 10)
        {
            if (build.cancel)
                build.cancel->requestCancellation();
            if (build.future.valid())
            {
                try
                {
                    build.future.get();
                }
                catch (...)
                {
                }
            }
            if (bakeSmoke || seededBakeSmoke || scaleBakeSmoke || low10BakeSmoke ||
                automaticUvBakeSmoke ||
                adaptiveBakeSmoke)
            {
                auto adaptiveLayout = adaptiveBakeSmoke ?
                    std::make_optional(loadAdaptiveBakeLayout(adaptiveLayoutPath)) : std::nullopt;
                captureNormalBakes(renderer, evidence, seededBakeSmoke,
                                   scaleBakeSmoke ? scaleTextureResolution : 2048,
                                   low10BakeSmoke ? 10 : 16,
                                   adaptiveLayout ? &*adaptiveLayout : nullptr, adaptiveSeed,
                                   automaticUvBakeSmoke, automaticUvFamily);
                window.requestClose();
                continue;
            }
            if (orbitBakeSmoke || low10OrbitSmoke || adaptiveOrbitSmoke)
            {
                auto adaptiveLayout = adaptiveOrbitSmoke ?
                    std::make_optional(loadAdaptiveBakeLayout(adaptiveLayoutPath)) : std::nullopt;
                captureNormalBakeOrbit(renderer, evidence, low10OrbitSmoke ? 10 : 16,
                                       adaptiveLayout ? &*adaptiveLayout : nullptr, adaptiveSeed);
                window.requestClose();
                continue;
            }
            if (featureSmoke)
            {
                captureSurfaceFeatures(renderer, evidence);
                window.requestClose();
                continue;
            }
            if (angularProfileSmoke)
            {
                captureAngularProfiles(renderer, evidence);
                window.requestClose();
                continue;
            }
            if (rockLodSmoke || autoUvRockLodSmoke || curvatureRelaxationSmoke)
            {
                captureRockLodBakes(renderer, evidence, autoUvRockLodSmoke || curvatureRelaxationSmoke,
                                    automaticUvRockLodFamily, curvatureRelaxationSmoke ? 0.5 : 0);
                window.requestClose();
                continue;
            }
            if (edgeRadiusSmoke)
            {
                captureEdgeRadiusSweep(renderer, evidence);
                window.requestClose();
                continue;
            }
            if (regeneratedLodSmoke)
            {
                captureRegeneratedLodSweep(renderer, evidence);
                window.requestClose();
                continue;
            }
            if (angularLod2Smoke)
            {
                captureAngularLod2Sweep(renderer, evidence);
                window.requestClose();
                continue;
            }
            nlohmann::json proof;
            proof["adapter"] = renderer.adapter();
            proof["engineWindow"] = true;
            proof["engineTriangleMesh"] = true;
            proof["engineFbmSource"] = true;
            for (int i = 0; i < 10; ++i)
            {
                auto sample = preset(i);
                int captureResolution = i >= 7 ? 64 : 32;
                auto mesh = evaluate(sample, captureResolution);
                renderer.upload(mesh);
                auto captureCamera = camera;
                if (i >= 7)
                    captureCamera.distance = 10;
                renderer.draw(900, 720, captureCamera, 0);
                renderer.capture(evidence / ("preset-" + std::to_string(i) + ".png"));
                save(sample, evidence / ("preset-" + std::to_string(i) + ".json"));
                auto roundtrip =
                    evaluate(load(evidence / ("preset-" + std::to_string(i) + ".json")), captureResolution);
                if (meshHash(mesh.mesh) != meshHash(roundtrip.mesh))
                    throw std::runtime_error("Runtime recipe roundtrip mismatch");
                proof["presets"].push_back({{"preset", i},
                                            {"hash", meshHash(mesh.mesh)},
                                            {"geometryAndRegionHash", PA::hashTriangleMesh(mesh.mesh)},
                                            {"resolution", captureResolution},
                                            {"closedSolid", mesh.closedSolid},
                                            {"selectedBrushes", mesh.sourceSelection.size()},
                                            {"vertices", mesh.mesh.positions.size()},
                                            {"triangles", mesh.mesh.indices.size() / 3},
                                            {"cpuMs", mesh.milliseconds}});
                for (auto &stage : mesh.stages)
                    proof["presets"].back()["stages"].push_back({{"operation", stage.operation},
                                                                 {"kind", name(stage.kind)},
                                                                 {"cpuMs", stage.milliseconds},
                                                                 {"inputVertices", stage.inputVertices},
                                                                 {"outputVertices", stage.outputVertices},
                                                                 {"outputTriangles", stage.outputTriangles}});
            }
            auto sharedRecipe = preset(1);
            for (int enabled = 0; enabled < 2; ++enabled)
            {
                auto comparisonRecipe = sharedRecipe;
                auto shared = defaultLayer(Kind::SharedWarp);
                shared.enabled = enabled != 0;
                comparisonRecipe.layers.push_back(shared);
                auto sharedMesh = evaluate(comparisonRecipe, 32);
                renderer.upload(sharedMesh);
                renderer.draw(900, 720, camera, 0);
                std::string label = enabled ? "shared-warp-on" : "shared-warp-off";
                renderer.capture(evidence / (label + ".png"));
                save(comparisonRecipe, evidence / (label + ".json"));
            }
            auto sample = preset(2);
            for (int enabled = 0; enabled < 2; ++enabled)
            {
                Recipe thicknessRecipe;
                auto l = defaultLayer(Kind::Layers);
                l.coherentLayers = true;
                l.count = 5;
                l.thickness = 0.32f;
                l.jitter = enabled ? 0.7f : 0;
                l.lateral = 0;
                l.layerScale = 0;
                thicknessRecipe.layers = {l};
                auto thicknessMesh = evaluate(thicknessRecipe, 32, 0, 0);
                renderer.upload(thicknessMesh);
                renderer.draw(900, 720, camera, 0);
                std::string label = enabled ? "thickness-on" : "thickness-off";
                renderer.capture(evidence / (label + ".png"));
                save(thicknessRecipe, evidence / (label + ".json"));
                if (enabled)
                {
                    renderer.draw(900, 720, camera, 3);
                    renderer.capture(evidence / "thickness-field.png");
                }
            }
            for (int enabled = 0; enabled < 2; ++enabled)
            {
                Recipe driftRecipe;
                auto l = defaultLayer(Kind::Layers);
                l.coherentLayers = true;
                l.count = 7;
                l.thickness = 0.28f;
                l.jitter = 0;
                l.lateral = 0.6f;
                l.layerScale = 0;
                l.driftCorrelation = enabled ? 0.95f : 0;
                l.rotationDrift = 18;
                l.scaleDrift = 0.1f;
                driftRecipe.layers = {l};
                auto mesh = evaluate(driftRecipe, 32);
                renderer.upload(mesh);
                renderer.draw(900, 720, camera, 0);
                std::string label = enabled ? "drift-on" : "drift-off";
                renderer.capture(evidence / (label + ".png"));
                save(driftRecipe, evidence / (label + ".json"));
            }
            for (int shape = -1; shape < 4; ++shape)
            {
                Recipe envelopeRecipe;
                auto formation = defaultLayer(Kind::Layers);
                formation.coherentLayers = true;
                formation.count = 5;
                formation.jitter = 0;
                formation.lateral = 0;
                formation.layerScale = 0;
                auto envelope = defaultLayer(Kind::Envelope);
                envelope.enabled = shape >= 0;
                envelope.envelopeShape = std::max(0, shape);
                envelope.scale = {0.94f, 1.05f, 0.9f};
                envelope.envelopeRoundness = 0.65f;
                envelopeRecipe.layers = {formation, envelope};
                auto mesh = evaluate(envelopeRecipe, 24);
                renderer.upload(mesh);
                renderer.draw(900, 720, camera, 0);
                std::string label = shape < 0 ? "envelope-off" : "envelope-" + std::to_string(shape);
                renderer.capture(evidence / (label + ".png"));
                save(envelopeRecipe, evidence / (label + ".json"));
                proof["envelopes"].push_back({{"shape", shape},
                                              {"cpuMs", mesh.milliseconds},
                                              {"vertices", mesh.mesh.positions.size()},
                                              {"triangles", mesh.mesh.indices.size() / 3}});
            }
            for (int enabled = 0; enabled < 2; ++enabled)
            {
                Recipe edgeRecipe;
                auto formation = defaultLayer(Kind::Layers);
                formation.coherentLayers = true;
                formation.count = 5;
                formation.jitter = 0;
                formation.lateral = 0;
                formation.layerScale = 0;
                auto edge = defaultLayer(Kind::EdgeBreakup);
                edge.enabled = enabled != 0;
                edge.amplitude = 0.065f;
                edge.frequency = 7;
                edge.width = 0.24f;
                edgeRecipe.layers = {formation, edge};
                auto mesh = evaluate(edgeRecipe, 32, 1, 1);
                renderer.upload(mesh);
                renderer.draw(900, 720, camera, 0);
                std::string label = enabled ? "edge-on" : "edge-off";
                renderer.capture(evidence / (label + ".png"));
                save(edgeRecipe, evidence / (label + ".json"));
                if (enabled)
                {
                    renderer.draw(900, 720, camera, 3);
                    renderer.capture(evidence / "edge-field.png");
                }
            }
            for (int shape : {0, 8})
                for (int reduced = 0; reduced < (shape == 8 ? 3 : 2); ++reduced)
                {
                    auto recipe = shape == 0 ? Recipe{} : preset(8);
                    float limit = reduced == 2 ? 0.05f : 0.01f;
                    if (reduced)
                    {
                        auto operation = defaultLayer(Kind::Reduce);
                        operation.reductionError = limit;
                        recipe.layers.push_back(operation);
                    }
                    int resolution = shape == 0 ? 24 : 64;
                    auto mesh = evaluate(recipe, resolution);
                    auto captureCamera = camera;
                    if (shape == 8)
                        captureCamera.distance = 10;
                    renderer.upload(mesh);
                    std::string label =
                        std::string(shape == 0 ? "cube" : "cliff") + (reduced ? "-reduced" : "-source");
                    if (reduced == 2)
                        label += "-005";
                    for (int mode : {0, 2})
                    {
                        renderer.draw(900, 720, captureCamera, mode);
                        renderer.capture(evidence / (label + (mode == 2 ? "-wire" : "") + ".png"));
                    }
                    save(recipe, evidence / (label + ".json"));
                    proof["reduction"].push_back({{"case", label},
                                                  {"triangles", mesh.mesh.indices.size() / 3},
                                                  {"cpuMs", mesh.milliseconds},
                                                  {"sampledBidirectionalError", mesh.sampledReductionError},
                                                  {"errorLimit", limit}});
                }
            auto field = evaluate(sample, 32, 2, 2);
            renderer.upload(field);
            renderer.draw(900, 720, camera, 3);
            renderer.capture(evidence / "fracture-field.png");
            auto compare = renderer.compareFbm(field, defaultLayer(Kind::NormalNoise));
            proof["fbmGpuMs"] = compare.gpuMilliseconds;
            proof["fbmCpuMs"] = compare.cpuMilliseconds;
            proof["fbmMaxError"] = compare.maxError;
            proof["fbmSamples"] = compare.samples;
            if (compare.maxError > 0.002)
                throw std::runtime_error("GPU FBM exceeds CPU reference tolerance");
            std::ofstream(evidence / "runtime-proof.json") << proof.dump(2) << '\n';
            std::cout << proof.dump(2) << '\n';
            window.requestClose();
        }
    }
    if (bakeCancel)
        bakeCancel->requestCancellation();
    if (bakeFuture.valid())
    {
        try
        {
            bakeFuture.get();
        }
        catch (...)
        {
        }
    }
    if (build.cancel)
        build.cancel->requestCancellation();
    if (build.future.valid())
    {
        try
        {
            build.future.get();
        }
        catch (...)
        {
        }
    }
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(previousProc));
    return 0;
}
} // namespace Playground
int main(int argc, char **argv)
{
    try
    {
        return Playground::run(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Playground error: " << e.what() << '\n';
        return 1;
    }
}
