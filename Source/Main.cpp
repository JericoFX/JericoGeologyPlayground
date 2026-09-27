#include "Model.hpp"
#include "MeshReduction.hpp"
#include "NormalBake.hpp"
#include "Renderer.hpp"
#include "SolidGeometry.hpp"
#include "SurfaceGeometry.hpp"
#include "SurfaceFeatures.hpp"
#include "ViewportProjection.hpp"
#include "SplineLabUI.hpp"
#include "AssemblyLabUI.hpp"
#include "AssemblyLabEvidence.hpp"
#include "WebLabUI.hpp"
#include "WebLabEvidence.hpp"
#include "SplineLabEvidence.hpp"
#include <Jerico/Core/Guid.hpp>
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
#include <numbers>
#include <optional>
#include <random>
#include <set>
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
int materialAppearance(const Recipe &recipe, int choice)
{
    if (choice > 0)
        return choice - 1;
    if (recipe.base == Base::Plane || recipe.base == Base::BspWall)
        return 1;
    if (std::any_of(recipe.layers.begin(), recipe.layers.end(), [](const Layer &layer)
                    { return layer.kind == Kind::Layers || layer.kind == Kind::StrataRelief; }))
        return 2;
    if (std::any_of(recipe.layers.begin(), recipe.layers.end(), [](const Layer &layer)
                    { return layer.kind == Kind::Planes || layer.kind == Kind::FractureQuadrants; }))
        return 3;
    return 0;
}
std::filesystem::path materialDiffuse(int appearance)
{
    return std::filesystem::path(PLAYGROUND_ROOT) / "Evidence/cc0-references" /
        (appearance == 1 || appearance == 2 ? "cliff_side-diff-1k.jpg" : "rock_06-diff-1k.jpg");
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
        change |= ImGui::Combo("Loop edge", &l.profile,
                               "Classic\0Soft\0RBrush push\0RBrush push + eroded rim\0"
                               "RBrush push + signed rim\0");
        if (l.profile >= 2)
            change |= ImGui::Combo("Brush alpha", &l.alphaSource,
                                    "Analytic only\0Poly Haven cliff_side CC0\0");
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
        change |= ImGui::Checkbox("Shared feature edges", &l.useSharedFeatureEdges);
        if (l.useSharedFeatureEdges)
            ImGui::TextWrapped("Common crease distance and curvature; bounded by local triangle scale.");
        else
        {
            ImGui::TextWrapped("Legacy stratum side distance for saved recipes. Enable shared feature edges to migrate this layer.");
            change |= ImGui::SliderFloat3("Layer axis", l.direction.data(), -1, 1);
        }
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
                change |= ImGui::Combo("Horizon contour", &l.profile,
                                       "Shared polygon\0Shared polygon (legacy 1)\0Independent fractured horizons\0");
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
                         "Convexity\0Concavity\0Ambient occlusion\0Exposure\0Boundary proximity\0Thickness\0Cavity\0"))
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
                l.mask == Mask::Thickness || l.mask == Mask::Cavity)
                change |= slider("Feature radius", l.featureRadius, 0.01f, 2);
            if (l.mask == Mask::EdgeProximity)
                change |= slider("Crease angle (degrees)", l.featureAngle, 1, 120);
            if (l.mask == Mask::Contact || l.mask == Mask::AmbientOcclusion ||
                l.mask == Mask::Cavity)
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
void captureExtremeDeformation(Renderer &renderer, const std::filesystem::path &evidence)
{
    nlohmann::json proof = {{"scope", "Extreme field, solidify and cliff incision displacement"},
                            {"adapter", renderer.adapter()}};
    Camera camera;
    camera.yaw = 0.5f;
    for (int family : {0, 11})
    {
        auto recipe = preset(family);
        if (family == 0)
            recipe.layers.clear();
        else
            recipe.layers.back().enabled = false;
        auto noise = defaultLayer(Kind::NormalNoise);
        noise.amplitude = 5;
        noise.frequency = 100;
        recipe.layers.push_back(noise);
        auto result = evaluate(recipe, recipe.finalResolution);
        if (!PA::validateClosedMesh(result.mesh).hasValue())
            throw std::runtime_error("Extreme deformation candidate is not closed");
        const auto label = family == 0 ? "sphere" : "layered";
        save(recipe, evidence / (std::string(label) + "-extreme-recipe.json"));
        camera.distance = family == 0 ? 5.8f : 6.8f;
        renderer.upload(result);
        renderer.draw(1280, 960, camera, 0);
        renderer.capture(evidence / (std::string(label) + "-extreme-neutral.png"));
        proof["cases"].push_back({{"name", label},
                                   {"meshHash", PA::hashTriangleMesh(result.mesh)},
                                   {"vertices", result.mesh.positions.size()},
                                   {"triangles", result.mesh.indices.size() / 3},
                                   {"cpuMs", result.milliseconds}});
    }
    for (const auto [amplitude, seed] : {std::pair{0.16f, 1U},
                                         std::pair{0.28f, 1U},
                                         std::pair{0.22f, 12U},
                                         std::pair{0.30f, 1U}})
    {
        auto recipe = preset(11);
        recipe.layers[0].perimeterAmplitude = amplitude;
        recipe.layers[0].seed = seed;
        auto result = evaluate(recipe, recipe.finalResolution);
        if (!PA::validateClosedMesh(result.mesh).hasValue())
            throw std::runtime_error("Extreme layer perimeter candidate is not closed");
        const auto label = "perimeter-" + std::to_string(int(std::lround(amplitude * 100))) +
                           "-seed-" + std::to_string(seed);
        save(recipe, evidence / (label + "-recipe.json"));
        camera.distance = 4.8f;
        renderer.upload(result);
        for (int view = 0; view < 2; ++view)
        {
            camera.yaw = 0.5f + view * 1.57079632679f;
            renderer.draw(1280, 960, camera, 0);
            renderer.capture(evidence / (label + "-yaw-" + std::to_string(view) + ".png"));
        }
        proof["cases"].push_back({{"name", label},
                                   {"meshHash", PA::hashTriangleMesh(result.mesh)},
                                   {"vertices", result.mesh.positions.size()},
                                   {"triangles", result.mesh.indices.size() / 3},
                                   {"cpuMs", result.milliseconds}});
    }
    camera.yaw = 0.5f;
    Recipe wall;
    wall.base = Base::Plane;
    wall.planeSize = {6, 4};
    wall.finalResolution = 16;
    Layer relief;
    relief.kind = Kind::StrataRelief;
    relief.amplitude = 0.3f;
    relief.frequency = 1.5f;
    Layer solid;
    solid.kind = Kind::Solidify;
    solid.thickness = 3;
    solid.solidAlongNormals = true;
    wall.layers = {relief, solid};
    auto thickWall = evaluate(wall, wall.finalResolution);
    if (!PA::validateClosedMesh(thickWall.mesh).hasValue())
        throw std::runtime_error("Extreme normal solidify candidate is not closed");
    save(wall, evidence / "solidify-extreme-recipe.json");
    camera.distance = 8;
    renderer.upload(thickWall);
    renderer.draw(1280, 960, camera, 0);
    renderer.capture(evidence / "solidify-extreme-neutral.png");
    const auto a = thickWall.mesh.positions[0];
    const auto b = thickWall.mesh.positions[thickWall.mesh.positions.size() / 2];
    proof["cases"].push_back({{"name", "solidify"},
                                {"requestedThickness", solid.thickness},
                                {"effectiveThickness", std::hypot(a.x - b.x, a.y - b.y, a.z - b.z)},
                                {"meshHash", PA::hashTriangleMesh(thickWall.mesh)},
                                {"vertices", thickWall.mesh.positions.size()},
                                {"triangles", thickWall.mesh.indices.size() / 3},
                                {"cpuMs", thickWall.milliseconds}});
    auto cliff = preset(8);
    cliff.version = 13;
    cliff.layers[4].profile = 0;
    auto incision = defaultLayer(Kind::CliffDrainage);
    incision.amplitude = 0.16f;
    incision.width = 0.30f;
    incision.translation[0] = -1.5f;
    incision.lateral = 0.22f;
    incision.frequency = 1;
    incision.thicknessFrequency = 2.2f;
    incision.phase = 0.25f;
    cliff.layers.insert(cliff.layers.begin() + 4, incision);
    for (int variant = 0; variant < 2; ++variant)
    {
        auto recipe = cliff;
        if (variant == 1)
        {
            recipe.layers[4].amplitude = 0.5f;
            recipe.layers[4].width = 0.10f;
        }
        auto result = evaluate(recipe, 32);
        if (!PA::validateClosedMesh(result.mesh).hasValue())
            throw std::runtime_error("Extreme cliff incision candidate is not closed");
        const auto label = variant == 0 ? "cliff-incision-control" : "cliff-incision-extreme";
        save(recipe, evidence / (std::string(label) + "-recipe.json"));
        renderer.upload(result);
        for (int view = 0; view < 2; ++view)
        {
            Camera cliffCamera;
            cliffCamera.yaw = view == 0 ? 0.0f : 0.55f;
            cliffCamera.pitch = view == 0 ? 0.0f : -0.2f;
            cliffCamera.distance = 10;
            renderer.draw(1280, 960, cliffCamera, 0);
            renderer.capture(evidence / (std::string(label) + "-view-" +
                                         std::to_string(view) + ".png"));
        }
        proof["cases"].push_back({{"name", label},
                                   {"meshHash", PA::hashTriangleMesh(result.mesh)},
                                   {"vertices", result.mesh.positions.size()},
                                   {"triangles", result.mesh.indices.size() / 3},
                                   {"cpuMs", result.milliseconds}});
    }
    std::ofstream(evidence / "extreme-deformation-proof.json") << proof.dump(2) << '\n';
    std::cout << proof.dump(2) << '\n';
}
void captureCliffSoftCreation(Renderer &renderer, const std::filesystem::path &evidence)
{
    auto classic = preset(8);
    classic.version = 10;
    classic.layers[4].profile = 0;
    const auto beforeCut = evaluate(classic, 32, 3);
    nlohmann::json proof = {{"scope", "Cliff cut creation before high-to-low"},
                            {"adapter", renderer.adapter()}, {"resolution", 32},
                            {"imageSize", {1280, 960}}};
    for (int variant = 0; variant < 7; ++variant)
    {
        auto recipe = classic;
        const char *label = "classic";
        if (variant)
        {
            const bool metric = variant >= 3 && variant != 6;
            recipe.version = metric ? 18 : 13;
            recipe.layers[4].profile = metric ? 5 : 2;
            recipe.layers[4].width = variant == 1 || variant == 3 ? 0.35f :
                                     variant == 5 || variant == 6 ? 1.30f : 0.70f;
            label = variant == 1 ? "push-035" : variant == 2 ? "push-070" :
                    variant == 3 ? "metric-035" : variant == 4 ? "metric-070" :
                    variant == 5 ? "metric-130" : "push-130";
        }
        const auto open = evaluate(recipe, 32, 4);
        const auto closed = evaluate(recipe, 32);
        if (variant == 6 && meshHash(closed.mesh) != meshHash(evaluate(preset(8), 32).mesh))
            throw std::runtime_error("Default cliff differs from the soft creation candidate");
        if (!PA::validateClosedMesh(closed.mesh).hasValue() ||
            meshHash(closed.mesh) != meshHash(evaluate(deserialize(serialize(recipe)), 32).mesh))
            throw std::runtime_error("Cliff soft creation must close and replay");
        double maximumRetreat = 0, maximumNeighborDelta = 0;
        if (variant)
        {
            if (open.mesh.indices != beforeCut.mesh.indices ||
                open.mesh.positions.size() != beforeCut.mesh.positions.size())
                throw std::runtime_error("Soft cliff cut changed the source grid");
            for (std::size_t i = 0; i < open.mesh.positions.size(); ++i)
                maximumRetreat = std::max(maximumRetreat,
                    beforeCut.mesh.positions[i].z - open.mesh.positions[i].z);
            for (std::size_t t = 0; t < open.mesh.indices.size(); t += 3)
                for (int c = 0; c < 3; ++c)
                {
                    const auto a = open.mesh.indices[t + c];
                    const auto b = open.mesh.indices[t + (c + 1) % 3];
                    const auto retreatA = beforeCut.mesh.positions[a].z - open.mesh.positions[a].z;
                    const auto retreatB = beforeCut.mesh.positions[b].z - open.mesh.positions[b].z;
                    maximumNeighborDelta = std::max(maximumNeighborDelta,
                                                    std::abs(retreatA - retreatB));
                }
        }
        save(recipe, evidence / (std::string(label) + "-recipe.json"));
        renderer.upload(closed);
        for (int view = 0; view < 2; ++view)
        {
            Camera camera;
            camera.yaw = view == 0 ? 0.0f : 0.55f;
            camera.pitch = view == 0 ? 0.0f : -0.2f;
            camera.distance = 10;
            renderer.draw(1280, 960, camera, 0);
            renderer.capture(evidence / (std::string(label) + "-view-" +
                                         std::to_string(view) + ".png"));
        }
        proof["cases"].push_back({{"name", label}, {"meshHash", PA::hashTriangleMesh(closed.mesh)},
                                  {"vertices", closed.mesh.positions.size()},
                                  {"triangles", closed.mesh.indices.size() / 3},
                                  {"maximumRetreat", maximumRetreat},
                                  {"maximumNeighborRetreatDifference", maximumNeighborDelta},
                                  {"cpuMs", closed.milliseconds}});
    }
    for (int variant = 0; variant < 2; ++variant)
    {
        const auto recipe = variant == 0 ? classic : preset(8);
        const auto result = evaluate(recipe, recipe.finalResolution);
        if (!PA::validateClosedMesh(result.mesh).hasValue())
            throw std::runtime_error("Final-resolution cliff creation is not closed");
        const auto label = variant == 0 ? "classic-64" : "soft-default-64";
        save(recipe, evidence / (std::string(label) + "-recipe.json"));
        renderer.upload(result);
        for (int view = 0; view < 2; ++view)
        {
            Camera camera;
            camera.yaw = view == 0 ? 0.0f : 0.55f;
            camera.pitch = view == 0 ? 0.0f : -0.2f;
            camera.distance = 10;
            renderer.draw(1280, 960, camera, 0);
            renderer.capture(evidence / (std::string(label) + "-view-" +
                                         std::to_string(view) + ".png"));
        }
        proof["cases"].push_back({{"name", label},
                                  {"meshHash", PA::hashTriangleMesh(result.mesh)},
                                  {"vertices", result.mesh.positions.size()},
                                  {"triangles", result.mesh.indices.size() / 3},
                                  {"cpuMs", result.milliseconds}});
    }
    std::ofstream(evidence / "cliff-soft-creation-proof.json") << proof.dump(2) << '\n';
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
                             double curvatureRelaxation = 0,
                             std::optional<double> contactRadius = std::nullopt,
                             std::optional<double> ambientRadius = std::nullopt,
                             bool normalMomentMips = false)
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
                                .contactRadius = contactRadius,
                                .ambientRadius = ambientRadius,
                                .edgeRadius = edgeRadius,
                                .featureSamples = 16,
                                .curvatureScale = 0.25,
                                .curvatureRelaxation = curvatureRelaxation,
                                .thicknessRange = 4,
                                .chartLayout = layout ? layout->charts : std::vector<NormalBakeChart>{},
                                .automaticUv = automaticUv,
                                .normalMomentMips = normalMomentMips},
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
                         double curvatureRelaxation = 0, bool angularFracture = false,
                         bool angularFourBlocks = false, bool cliffDrainage = false,
                         bool cliffEdgeDamage = false, bool cliffRBrushAlpha = false,
                         bool layeredOcclusion = false,
                         bool layeredMaterial = false, bool layeredMaterialNoMips = false,
                         bool cliffNonharmonic = false, bool cliffMacroWarp = false,
                         bool cliffErodedRim = false, bool layeredHorizonFracture = false,
                         bool layeredSlateSource = false, bool cliffSignedRim = false,
                         bool cliffMetricRelief = false)
{
    nlohmann::json report;
    report["adapter"] = renderer.adapter();
    report["scope"] = "C++ high source projected independently to three low-mesh LODs per rock family";
    report["automaticUv"] = automaticUv;
    report["curvatureRelaxation"] = curvatureRelaxation;
    report["angularFracture"] = angularFracture;
    report["angularFourBlocks"] = angularFourBlocks;
    report["cliffDrainage"] = cliffDrainage;
    report["cliffEdgeDamage"] = cliffEdgeDamage;
    report["cliffRBrushAlpha"] = cliffRBrushAlpha;
    report["layeredOcclusion"] = layeredOcclusion;
    report["layeredMaterial"] = layeredMaterial;
    report["layeredMaterialNoMips"] = layeredMaterialNoMips;
    report["cliffNonharmonic"] = cliffNonharmonic;
    report["cliffMacroWarp"] = cliffMacroWarp;
    report["cliffErodedRim"] = cliffErodedRim;
    report["cliffSignedRim"] = cliffSignedRim;
    report["cliffMetricRelief"] = cliffMetricRelief;
    report["layeredHorizonFracture"] = layeredHorizonFracture;
    report["layeredSlateSource"] = layeredSlateSource;
    report["imageSize"] = {1280, 960};
    report["textureResolution"] = 2048;
    Camera camera;
    for (int family : {5, 8, 9, 10, 11, 12})
    {
        if (familyFilter >= 0 && family != familyFilter)
            continue;
        auto recipe = layeredSlateSource && family == 11 ? preset(13) : preset(family);
        for (auto &layer : recipe.layers)
            layer.seed = 123;
        if (angularFracture && family == 10)
        {
            recipe.version = 11;
            auto shear = defaultLayer(Kind::FractureShear);
            shear.seed = 123;
            recipe.layers.insert(recipe.layers.end() - 1, shear);
        }
        if (angularFourBlocks && family == 10)
        {
            recipe.version = 12;
            auto blocks = defaultLayer(Kind::FractureQuadrants);
            blocks.seed = 123;
            recipe.layers.insert(recipe.layers.end() - 1, blocks);
        }
        if (layeredHorizonFracture && !layeredSlateSource && family == 11)
        {
            recipe.version = 15;
            for (auto &layer : recipe.layers)
                if (layer.kind == Kind::Layers)
                    layer.profile = 2;
        }
        if ((cliffDrainage || cliffEdgeDamage || cliffRBrushAlpha || cliffErodedRim ||
             cliffSignedRim || cliffMetricRelief) && family == 8)
        {
            recipe.version = cliffMetricRelief ? 18 : cliffSignedRim ? 17 : cliffErodedRim ? 14 : 13;
            if (cliffEdgeDamage || cliffRBrushAlpha || cliffErodedRim || cliffSignedRim || cliffMetricRelief)
            {
                auto damage = defaultLayer(Kind::DamageDecal);
                damage.amplitude = 0.15f;
                damage.direction = {0, 0, 1};
                damage.translation = {0.45f, 0.15f, -0.15f};
                damage.projectorRadius = {1.8f, 1.4f};
                damage.featureRadius = 1;
                damage.threshold = 0.55f;
                damage.width = 0.45f;
                damage.damageLip = 0;
                damage.solidAlongNormals = false;
                recipe.layers.insert(recipe.layers.begin() + 4, damage);
                for (auto &layer : recipe.layers)
                    if (layer.kind == Kind::CliffCutLoop)
                    {
                        layer.profile = cliffMetricRelief ? 5 : cliffSignedRim ? 4 : cliffErodedRim ? 3 :
                                        cliffRBrushAlpha ? 2 : 1;
                        layer.alphaSource = cliffRBrushAlpha || cliffErodedRim || cliffSignedRim ||
                                            cliffMetricRelief ? 1 : 0;
                        layer.width = 0.35f;
                    }
            }
            auto drainage = defaultLayer(Kind::CliffDrainage);
            drainage.seed = 123;
            if (cliffEdgeDamage || cliffRBrushAlpha || cliffErodedRim || cliffSignedRim || cliffMetricRelief)
            {
                drainage.amplitude = 0.30f;
                drainage.width = 0.22f;
            }
            recipe.layers.insert(recipe.layers.begin() +
                                     (cliffEdgeDamage || cliffRBrushAlpha || cliffErodedRim ||
                                      cliffSignedRim || cliffMetricRelief ? 5 : 4),
                                 drainage);
        }
        if (cliffNonharmonic && family == 8)
            for (auto &layer : recipe.layers)
                if (layer.kind == Kind::CliffCutLoop)
                    layer.lacunarity = 1.87f;
        if ((cliffMacroWarp || cliffErodedRim || cliffSignedRim || cliffMetricRelief) && family == 8)
            for (auto &layer : recipe.layers)
                if (layer.kind == Kind::VectorNoise)
                {
                    layer.amplitude = 0.35f;
                    layer.frequency = 0.6f;
                    layer.octaves = 3;
                    layer.coordinateScale = {0.7f, 0.55f, 1};
                    layer.outputScale = {0.8f, 0.6f, 1};
                }
        double baseError = family == 10 ? 0.005 : family == 11 ||
                           (family == 8 && (cliffRBrushAlpha || cliffErodedRim || cliffSignedRim ||
                                            cliffMetricRelief)) ? 0.02 :
                           family == 5 ? 0.08 : 0.04;
        auto projectionDistance = family == 5 || family == 11 ? 0.2 : 0.15;
        std::size_t previousTriangles = std::numeric_limits<std::size_t>::max();
        nlohmann::json familyReport = {{"family", family}, {"lods", nlohmann::json::array()},
                                       {"defaultMaterialAppearance", materialAppearance(recipe, 0)},
                                       {"defaultMaterialDiffuse", materialDiffuse(materialAppearance(recipe, 0)).filename().string()}};
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
                                                curvatureRelaxation,
                                                layeredOcclusion ? std::optional<double>{0.08} : std::nullopt,
                                                layeredOcclusion ? std::optional<double>{0.8} : std::nullopt);
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
                if (layeredMaterial || cliffRBrushAlpha || layeredHorizonFracture)
                {
                    const int appearance = layeredHorizonFracture ? materialAppearance(recipe, 0) :
                                           cliffRBrushAlpha ? 1 : 0;
                    const auto diffuse = layeredHorizonFracture ? materialDiffuse(appearance) :
                        std::filesystem::path(PLAYGROUND_ROOT) /
                            (cliffRBrushAlpha ? "Evidence/cc0-references/cliff_side-diff-1k.jpg" :
                                               "Evidence/cc0-references/rock_06-diff-1k.jpg");
                    renderer.upload(preview.high);
                    renderer.uploadMaterial(nullptr, diffuse, !layeredMaterialNoMips, appearance);
                    if (layeredSlateSource && lod == 2)
                    {
                        auto farCamera = camera;
                        farCamera.distance *= 2;
                        renderer.draw(1280, 960, farCamera, 5);
                        renderer.capture(directory / "high-material-far.png");
                    }
                    renderer.draw(1280, 960, camera, 5);
                    renderer.capture(directory / "high-material.png");
                    if (layeredSlateSource && lod == 0)
                        for (auto [name, yaw] : {std::pair{"high-material-side.png", 1.65f},
                                                 std::pair{"high-material-back.png", 2.8f}})
                        {
                            auto alternateCamera = camera;
                            alternateCamera.yaw = yaw;
                            renderer.draw(1280, 960, alternateCamera, 5);
                            renderer.capture(directory / name);
                        }
                    renderer.upload(preview.bake.low, &preview.bake);
                    renderer.uploadMaterial(&preview.bake, diffuse, !layeredMaterialNoMips, appearance);
                    renderer.draw(1280, 960, camera, 5);
                    renderer.capture(directory / "low-material.png");
                    if (lod == 2)
                    {
                        auto farCamera = camera;
                        farCamera.distance *= 2;
                        renderer.draw(1280, 960, farCamera, 5);
                        renderer.capture(directory / "low-material-far.png");
                    }
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
void captureSlateTemporal(Renderer &renderer, const std::filesystem::path &evidence)
{
    auto recipe = preset(13);
    for (auto &layer : recipe.layers)
        layer.seed = 123;
    auto preview = buildBakePreview(recipe, 0.04f, 2048, 0.2, nullptr,
                                    NormalProjection::ClosestSurface, 14, nullptr, 0.02, true);
    if (preview.bake.misses || preview.bake.backfaceHits)
        throw std::runtime_error("Slate temporal reference has incomplete high-to-low bake");
    exportNormalBake(preview.bake, evidence / "bake");
    const auto appearance = materialAppearance(recipe, 0);
    const auto diffuse = materialDiffuse(appearance);
    Camera camera;
    camera.distance = 11.6f;
    constexpr std::array offsets{-0.001f, -0.0005f, 0.0f, 0.0005f, 0.001f};
    auto captureSequence = [&](const std::string &name)
    {
        for (std::size_t i = 0; i < offsets.size(); ++i)
        {
            auto view = camera;
            view.yaw += offsets[i];
            renderer.draw(1280, 960, view, 5);
            renderer.capture(evidence / (name + "-material-" + std::to_string(i) + ".png"));
            renderer.draw(1280, 960, view, 1);
            renderer.capture(evidence / (name + "-normal-" + std::to_string(i) + ".png"));
        }
    };
    renderer.upload(preview.high);
    renderer.uploadMaterial(nullptr, diffuse, true, appearance);
    captureSequence("high");
    renderer.upload(preview.bake.low, &preview.bake);
    renderer.uploadMaterial(&preview.bake, diffuse, true, appearance);
    const auto mippedBufferBytes = renderer.bufferBytes();
    captureSequence("low-mips");
    auto savedMips = std::move(preview.bake.normalMips);
    renderer.upload(preview.bake.low, &preview.bake);
    renderer.uploadMaterial(&preview.bake, diffuse, true, appearance);
    const auto unmippedBufferBytes = renderer.bufferBytes();
    captureSequence("low-no-normal-mips");
    preview.bake.normalMips = std::move(savedMips);
    nlohmann::json report = {{"imageSize", {1280, 960}}, {"textureResolution", 2048},
                             {"normalMipLevels", preview.bake.normalMips.size() + 1},
                             {"yawOffsets", offsets}, {"cameraDistance", camera.distance},
                             {"highHash", preview.bake.highHash}, {"lowHash", preview.bake.lowHash},
                             {"samples", preview.bake.samples}, {"misses", preview.bake.misses},
                             {"backfaces", preview.bake.backfaceHits},
                             {"bakeCpuMs", preview.bake.milliseconds},
                             {"mippedBufferBytes", mippedBufferBytes},
                             {"unmippedBufferBytes", unmippedBufferBytes}};
    std::ofstream(evidence / "slate-temporal-proof.json") << report.dump(2) << '\n';
    std::cout << report.dump(2) << '\n';
}
void captureRockTemporal(Renderer &renderer, const std::filesystem::path &evidence, int family,
                         bool biasSweep = false, bool momentSweep = false)
{
    if (family != 8 && family != 11)
        throw std::runtime_error("Rock temporal capture requires signed cliff or fractured strata");
    const auto reference = std::filesystem::path(PLAYGROUND_ROOT) / "Evidence" /
        (family == 8 ? "2026-09-25-cliff-signed-rim-candidate/rock-8/lod-2" :
                       "2026-09-25-layered-fractured-horizons-gentle/rock-11/lod-2");
    const auto recipe = load(reference / "high-recipe.json");
    nlohmann::json control;
    std::ifstream controlFile(reference / "bake.json");
    if (!controlFile)
        throw std::runtime_error("Could not read retained temporal bake reference");
    controlFile >> control;
    const int lowResolution = family == 11 ? 14 : 0;
    auto preview = buildBakePreview(recipe, 0.04f, 2048, family == 11 ? 0.2 : 0.15,
                                    nullptr, NormalProjection::ClosestSurface, lowResolution,
                                    nullptr, family == 8 ? 0.1 : 0.02, true,
                                    0, std::nullopt, std::nullopt, momentSweep);
    if (preview.bake.misses || preview.bake.backfaceHits ||
        preview.bake.highHash != control.at("highHash").get<std::uint64_t>() ||
        preview.bake.lowHash != control.at("lowHash").get<std::uint64_t>())
        throw std::runtime_error("Temporal high-to-low bake differs from retained reference");
    exportNormalBake(preview.bake, evidence / "bake");
    const auto appearance = materialAppearance(recipe, 0);
    const auto diffuse = materialDiffuse(appearance);
    Camera camera;
    camera.distance = family == 8 ? 20.0f : 11.6f;
    const auto farDistance = camera.distance;
    constexpr std::array offsets{-0.001f, -0.0005f, 0.0f, 0.0005f, 0.001f};
    auto captureSequence = [&](const std::string &name)
    {
        for (std::size_t i = 0; i < offsets.size(); ++i)
        {
            auto view = camera;
            view.yaw += offsets[i];
            renderer.draw(1280, 960, view, 5);
            renderer.capture(evidence / (name + "-material-" + std::to_string(i) + ".png"));
            renderer.draw(1280, 960, view, 1);
            renderer.capture(evidence / (name + "-normal-" + std::to_string(i) + ".png"));
            renderer.draw(1280, 960, view, 6);
            renderer.capture(evidence / (name + "-depth-" + std::to_string(i) + ".png"));
            renderer.draw(1280, 960, view, 7);
            renderer.capture(evidence / (name + "-primitive-" + std::to_string(i) + ".png"));
        }
    };
    renderer.upload(preview.high);
    renderer.uploadMaterial(nullptr, diffuse, true, appearance);
    captureSequence("high");
    renderer.upload(preview.bake.low, &preview.bake);
    renderer.uploadMaterial(&preview.bake, diffuse, true, appearance);
    const auto mippedBufferBytes = renderer.bufferBytes();
    captureSequence("low-mips");
    auto savedMips = std::move(preview.bake.normalMips);
    renderer.upload(preview.bake.low, &preview.bake);
    renderer.uploadMaterial(&preview.bake, diffuse, true, appearance);
    const auto unmippedBufferBytes = renderer.bufferBytes();
    captureSequence("low-no-normal-mips");
    preview.bake.normalMips = std::move(savedMips);
    if (momentSweep)
    {
        renderer.upload(preview.bake.low, &preview.bake, nullptr, nullptr, true);
        renderer.uploadMaterial(&preview.bake, diffuse, true, appearance);
        captureSequence("low-moments");
    }
    if (biasSweep)
    {
        renderer.upload(preview.bake.low, &preview.bake);
        renderer.uploadMaterial(&preview.bake, diffuse, true, appearance);
        renderer.setNormalMipBias(-0.5f);
        captureSequence("low-bias-half");
        renderer.setNormalMipBias(-1.0f);
        captureSequence("low-bias-one");
        renderer.setNormalMipBias(0.0f);
    }
    if (biasSweep || momentSweep)
    {
        camera.distance = farDistance * 0.5f;
        renderer.upload(preview.high);
        renderer.uploadMaterial(nullptr, diffuse, true, appearance);
        captureSequence("near-high");
        renderer.upload(preview.bake.low, &preview.bake);
        renderer.uploadMaterial(&preview.bake, diffuse, true, appearance);
        captureSequence("near-low-mips");
        auto nearMips = std::move(preview.bake.normalMips);
        renderer.upload(preview.bake.low, &preview.bake);
        renderer.uploadMaterial(&preview.bake, diffuse, true, appearance);
        captureSequence("near-low-no-normal-mips");
        preview.bake.normalMips = std::move(nearMips);
        renderer.upload(preview.bake.low, &preview.bake);
        renderer.uploadMaterial(&preview.bake, diffuse, true, appearance);
        if (biasSweep)
        {
            renderer.setNormalMipBias(-0.5f);
            captureSequence("near-low-bias-half");
            renderer.setNormalMipBias(-1.0f);
            captureSequence("near-low-bias-one");
            renderer.setNormalMipBias(0.0f);
        }
        if (momentSweep)
        {
            renderer.upload(preview.bake.low, &preview.bake, nullptr, nullptr, true);
            renderer.uploadMaterial(&preview.bake, diffuse, true, appearance);
            captureSequence("near-low-moments");
        }
    }
    nlohmann::json report = {{"family", family}, {"adapter", renderer.adapter()},
                             {"imageSize", {1280, 960}}, {"textureResolution", 2048},
                             {"reprojectionVersion", 1},
                             {"depthEncoding", "linear-view-24-bit-rgb"},
                             {"depthEncodingMaxMeters", 100.0},
                             {"primitiveEncoding", "triangle-index-plus-one-24-bit-rgb"},
                             {"projectionScale", 2.4},
                             {"cameraYaw", camera.yaw}, {"cameraPitch", camera.pitch},
                             {"cameraTarget", camera.target},
                             {"normalMipLevels", preview.bake.normalMips.size() + 1},
                             {"yawOffsets", offsets}, {"cameraDistance", farDistance},
                             {"highHash", preview.bake.highHash},
                             {"lowHash", preview.bake.lowHash},
                             {"samples", preview.bake.samples},
                             {"misses", preview.bake.misses},
                             {"backfaces", preview.bake.backfaceHits},
                             {"bakeCpuMs", preview.bake.milliseconds},
                             {"mippedBufferBytes", mippedBufferBytes},
                             {"unmippedBufferBytes", unmippedBufferBytes}};
    if (biasSweep || momentSweep)
    {
        if (biasSweep)
            report["normalMipBiasVariants"] = {{"low-bias-half", -0.5}, {"low-bias-one", -1.0}};
        if (momentSweep)
        {
            report["normalMomentVariants"] = {"low-moments"};
            report["normalMomentMipLevels"] = preview.bake.normalMomentMips.size() + 1;
        }
        report["nearCameraDistance"] = camera.distance;
    }
    std::ofstream(evidence / "rock-temporal-proof.json") << report.dump(2) << '\n';
    std::cout << report.dump(2) << '\n';
}
std::vector<Jerico::Guid> caveReliefFaceSelection(const MeshResult &source, CaveForm form)
{
    std::set<Jerico::Guid> selected;
    if (form != CaveForm::SquareTunnel)
        selected.insert(source.mesh.triangleRegions.begin(), source.mesh.triangleRegions.end());
    else
    {
        const auto cutterId = Solid::region(0xca7eU, 3);
        std::set<Jerico::Guid> cutterFaces;
        for (std::uint64_t face = 0; face < 6; ++face)
            cutterFaces.insert(PA::deriveFaceRegionGuid(cutterId, face));
        for (std::size_t t = 0; t < source.mesh.indices.size(); t += 3)
        {
            const auto id = source.mesh.triangleRegions[t / 3];
            if (!cutterFaces.contains(id))
                continue;
            const auto a = source.mesh.positions[source.mesh.indices[t]],
                       b = source.mesh.positions[source.mesh.indices[t + 1]],
                       c = source.mesh.positions[source.mesh.indices[t + 2]];
            const double x = (a.x + b.x + c.x) / 3.0,
                         y = (a.y + b.y + c.y) / 3.0;
            if (std::abs(x) > 0.85 || y > 0.45)
                selected.insert(id);
        }
    }
    if (selected.empty() ||
        (form == CaveForm::SquareTunnel && selected.size() != 3))
        throw std::runtime_error("Cave relief selection has missing roof or wall regions");
    return {selected.begin(), selected.end()};
}
void captureCaveForms(Renderer &renderer, const std::filesystem::path &evidence,
                      bool geologicalMacro, bool axialFeatureReduced, bool blindChamber = false,
                      bool squareWallRelief = false, bool selectedRelief = false)
{
    nlohmann::json report = {{"adapter", renderer.adapter()},
                             {"scope", blindChamber ?
                                 "BSP blind arch chamber and square through-tunnel with high-to-low bakes" :
                                 "BSP subtractive arch and square through-tunnels with high-to-low bakes"},
                             {"geologicalMacro", geologicalMacro},
                             {"axialFeatureReduced", axialFeatureReduced},
                             {"blindChamber", blindChamber},
                             {"squareWallRelief", squareWallRelief},
                             {"squareInteriorRelief", geologicalMacro || squareWallRelief},
                             {"selectedRelief", selectedRelief},
                             {"textureResolution", 1024}, {"imageSize", {1280, 960}},
                             {"forms", nlohmann::json::array()}};
    Camera camera;
    camera.yaw = 0.12f;
    camera.pitch = -0.14f;
    camera.distance = 10.5f;
    if (blindChamber)
        camera.yaw = float(std::numbers::pi);
    Camera entrance = camera;
    entrance.yaw = blindChamber ? float(std::numbers::pi) : 0;
    entrance.pitch = -0.02f;
    entrance.distance = 5.0f;
    entrance.target = {0, -0.5f, 0};
    Camera interior = entrance;
    interior.distance = 1.8f;
    interior.target = {0, -0.5f, -0.4f};
    Camera squareInterior = interior;
    squareInterior.distance = 1.2f;
    report["entranceCamera"] = {{"yaw", entrance.yaw}, {"pitch", entrance.pitch},
                                {"distance", entrance.distance}, {"target", entrance.target}};
    for (auto [form, name] : {std::pair{blindChamber ? CaveForm::BlindArch : CaveForm::Arch,
                                      "arch"},
                              std::pair{CaveForm::SquareTunnel, "square"}})
    {
        const bool sculptSquare = form == CaveForm::SquareTunnel &&
                                  (geologicalMacro || squareWallRelief);
        std::vector<Jerico::Guid> selectedFaces;
        MeshResult macroHigh;
        if (selectedRelief)
        {
            auto basis = form == CaveForm::SquareTunnel ? caveSource(form, 8) :
                         axialCaveSource(form, 8);
            selectedFaces = caveReliefFaceSelection(basis, form);
            macroHigh = selectedGeologicalRelief(std::move(basis), selectedFaces,
                                                form == CaveForm::SquareTunnel ? 0.24 : 0.18, 123);
        }
        else
            macroHigh = squareWallRelief && form == CaveForm::SquareTunnel ?
                        squareTunnelReliefSource(8) :
                        axialFeatureReduced ? axialCaveSource(form, 8) :
                        geologicalMacro ? geologicalCaveSource(form, 8) : caveSource(form, 8);
        auto high = macroHigh;
        Recipe detail;
        auto noise = defaultLayer(Kind::NormalNoise);
        noise.seed = 123;
        noise.amplitude = form == CaveForm::SquareTunnel ? 0.025f : 0.12f;
        noise.frequency = 2.5f;
        noise.lacunarity = 1.87f;
        detail.layers = {noise};
        if (!(selectedRelief && form == CaveForm::SquareTunnel))
            high = evaluateSource(detail, high, 8);
        auto formDirectory = evidence / name;
        nlohmann::json formReport = {{"form", name}, {"highTriangles", high.mesh.indices.size() / 3},
                                     {"highHash", PA::hashTriangleMesh(high.mesh)},
                                     {"selectedFaceCount", selectedFaces.size()},
                                     {"lods", nlohmann::json::array()}};
        formReport["selectedFaceGuids"] = nlohmann::json::array();
        for (auto id : selectedFaces)
            formReport["selectedFaceGuids"].push_back(Jerico::formatGuid(id));
        std::size_t previousTriangles = high.mesh.indices.size() / 3;
        for (int lod = 0; lod < 3; ++lod)
        {
            const bool reducedArch = axialFeatureReduced && form != CaveForm::SquareTunnel;
            const int subdivisions = reducedArch ? 0 :
                                     sculptSquare ?
                                         (lod == 0 ? 7 : lod == 1 ? 6 : 5) :
                                     lod == 0 ? 6 : lod == 1 ? 4 : 2;
            const double reductionBudget = lod == 0 ? 0.01 : lod == 1 ? 0.05 : 0.10;
            double macroReductionError = 0;
            MeshResult low;
            if (reducedArch)
            {
                low.mesh = reduceClosedMesh(macroHigh.mesh, reductionBudget,
                                            macroReductionError, nullptr);
                low.regions = macroHigh.regions;
                low.sourceSelection = macroHigh.sourceSelection;
                low.sourceRevision = macroHigh.sourceRevision;
                low.field.resize(low.mesh.positions.size());
                low.trianglesBeforeReduction = macroHigh.mesh.indices.size() / 3;
                normals(low);
            }
            else
                if (selectedRelief && form == CaveForm::SquareTunnel)
                    low = selectedGeologicalRelief(caveSource(form, subdivisions),
                                                   selectedFaces, 0.24, 123);
                else
                    low = squareWallRelief && form == CaveForm::SquareTunnel ?
                                             squareTunnelReliefSource(subdivisions) :
                                             geologicalMacro ? geologicalCaveSource(form, subdivisions) :
                                             caveSource(form, subdivisions);
            low.sampledReductionError = sampledBidirectionalMeshError(high.mesh, low.mesh);
            if ((reducedArch || sculptSquare) &&
                low.sampledReductionError > 0.15)
                throw std::runtime_error(std::string(name) + " LOD" + std::to_string(lod) +
                                         " exceeds the high-to-low surface error gate: " +
                                         std::to_string(low.sampledReductionError));
            const auto triangles = low.mesh.indices.size() / 3;
            if (!low.closedSolid || triangles >= previousTriangles)
                throw std::runtime_error(std::string(name) + " LOD is not a reduced closed solid");
            previousTriangles = triangles;
            auto bake = bakeNormals(high, low, {.resolution = 1024,
                                               .projectionDistance = 0.25,
                                               .padding = 4,
                                               .matchRegions = true,
                                               .projection = NormalProjection::ClosestSurface,
                                               .specialistMaps = true,
                                               .featureRadius = 0.5,
                                               .ambientRadius = selectedRelief ?
                                                   std::optional<double>{3.0} : std::nullopt,
                                               .edgeRadius = 0.1,
                                               .featureSamples = selectedRelief ? 48u : 16u,
                                               .curvatureScale = 0.25,
                                               .thicknessRange = 4,
                                               .automaticUv = true});
            bake.lowStrategy = selectedRelief && form == CaveForm::SquareTunnel ?
                               "bsp-selected-roof-walls" :
                               selectedRelief ? "native-feature-reduction-selected-all" :
                               sculptSquare ?
                               "bsp-regenerated-interior-relief" :
                               reducedArch ? "native-feature-reduction" :
                               geologicalMacro && form == CaveForm::Arch ?
                                   "bsp-regenerated-vector-warp" : "bsp-regenerated";
            auto directory = formDirectory / ("lod-" + std::to_string(lod));
            exportNormalBake(bake, directory);
            const auto diffuse = materialDiffuse(0);
            auto capturePbrAoPair = [&](NormalBake &surfaceBake,
                                        const std::filesystem::path &output,
                                        const char *prefix)
            {
                auto ambient = surfaceBake.ambientRgba;
                auto ambientMips = surfaceBake.specialistMips.ambient;
                auto clearAmbient = [](std::vector<std::uint8_t> &pixels)
                {
                    for (std::size_t pixel = 0; pixel < pixels.size(); pixel += 4)
                        pixels[pixel] = pixels[pixel + 1] = pixels[pixel + 2] = 0;
                };
                clearAmbient(surfaceBake.ambientRgba);
                for (auto &mip : surfaceBake.specialistMips.ambient)
                    clearAmbient(mip);
                renderer.uploadMaterial(&surfaceBake, diffuse, true, 4);
                renderer.draw(1280, 960, entrance, 5);
                renderer.capture(output / (std::string(prefix) + "-pbr-no-ao.png"));
                surfaceBake.ambientRgba = std::move(ambient);
                surfaceBake.specialistMips.ambient = std::move(ambientMips);
                renderer.uploadMaterial(&surfaceBake, diffuse, true, 4);
                renderer.draw(1280, 960, entrance, 5);
                renderer.capture(output / (std::string(prefix) + "-pbr-ao.png"));
            };
            if (lod == 0)
            {
                renderer.upload(high);
                renderer.uploadMaterial(nullptr, diffuse, true, 4);
                renderer.draw(1280, 960, camera, 5);
                renderer.capture(formDirectory / "high-material.png");
                {
                    auto oblique = camera;
                    oblique.yaw = 0.65f;
                    oblique.pitch = -0.22f;
                    renderer.draw(1280, 960, oblique, 5);
                    renderer.capture(formDirectory / "high-oblique-material.png");
                }
                renderer.draw(1280, 960, camera, 0);
                renderer.capture(formDirectory / "high.png");
                renderer.draw(1280, 960, entrance, 5);
                renderer.capture(formDirectory / "high-entrance-material.png");
                if (selectedRelief && form != CaveForm::SquareTunnel)
                {
                    renderer.draw(1280, 960, entrance, 0);
                    renderer.capture(formDirectory / "high-entrance-geometry.png");
                }
                if (sculptSquare)
                {
                    renderer.draw(1280, 960, entrance, 0);
                    renderer.capture(formDirectory / "high-entrance-geometry.png");
                }
                if (blindChamber && form == CaveForm::BlindArch ||
                    sculptSquare)
                {
                    renderer.draw(1280, 960, form == CaveForm::SquareTunnel ? squareInterior : interior, 5);
                    renderer.capture(formDirectory / "high-interior-material.png");
                    if (sculptSquare)
                    {
                        renderer.draw(1280, 960, squareInterior, 0);
                        renderer.capture(formDirectory / "high-interior-geometry.png");
                    }
                    if (selectedRelief && form != CaveForm::SquareTunnel)
                    {
                        renderer.draw(1280, 960, interior, 0);
                        renderer.capture(formDirectory / "high-interior-geometry.png");
                    }
                }
                if (selectedRelief)
                {
                    auto highBake = bakeNormals(high, high, bake.settings);
                    if (highBake.misses || highBake.backfaceHits)
                        throw std::runtime_error("High PBR self-bake misses the cave surface");
                    exportNormalBake(highBake, formDirectory / "high-pbr-bake");
                    formReport["highPbrBakeSamples"] = highBake.samples;
                    formReport["highPbrBakeMisses"] = highBake.misses;
                    renderer.upload(highBake.low, &highBake);
                    capturePbrAoPair(highBake, formDirectory, "high");
                }
            }
            renderer.upload(bake.low, &bake);
            if (selectedRelief && lod == 0)
                capturePbrAoPair(bake, directory, "low");
            else
                renderer.uploadMaterial(&bake, diffuse, true, 4);
            renderer.draw(1280, 960, camera, 5);
            renderer.capture(directory / "low-material.png");
            if (lod == 2)
            {
                auto oblique = camera;
                oblique.yaw = 0.65f;
                oblique.pitch = -0.22f;
                renderer.draw(1280, 960, oblique, 5);
                renderer.capture(directory / "low-oblique-material.png");
                renderer.draw(1280, 960, entrance, 5);
                renderer.capture(directory / "low-entrance-material.png");
                if (sculptSquare)
                {
                    renderer.draw(1280, 960, entrance, 0);
                    renderer.capture(directory / "low-entrance-geometry.png");
                }
                if (selectedRelief && form != CaveForm::SquareTunnel)
                {
                    renderer.draw(1280, 960, entrance, 0);
                    renderer.capture(directory / "low-entrance-geometry.png");
                }
                if (blindChamber && form == CaveForm::BlindArch ||
                    sculptSquare)
                {
                    renderer.draw(1280, 960, form == CaveForm::SquareTunnel ? squareInterior : interior, 5);
                    renderer.capture(directory / "low-interior-material.png");
                }
            }
            renderer.draw(1280, 960, camera, 0);
            renderer.capture(directory / "low-normal.png");
            formReport["lods"].push_back({{"lod", lod}, {"subdivisions", subdivisions},
                                           {"triangles", triangles}, {"sampledSurfaceError", low.sampledReductionError},
                                           {"samples", bake.samples}, {"misses", bake.misses},
                                           {"backfaces", bake.backfaceHits}, {"bakeCpuMs", bake.milliseconds},
                                           {"uvIslands", bake.automaticUvIslandCount},
                                           {"uvActiveAreaFraction", bake.automaticUvActiveAreaFraction},
                                           {"lowHash", bake.lowHash}});
            if (reducedArch)
            {
                formReport["lods"].back()["reductionBudget"] = reductionBudget;
                formReport["lods"].back()["macroReductionError"] = macroReductionError;
            }
            report["forms"].push_back(formReport);
            std::ofstream(evidence / "cave-forms-proof.json") << report.dump(2) << '\n';
            report["forms"].erase(report["forms"].end() - 1);
        }
        report["forms"].push_back(std::move(formReport));
        std::ofstream(evidence / "cave-forms-proof.json") << report.dump(2) << '\n';
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
void captureCliffStrataCreation(Renderer &renderer, const std::filesystem::path &evidence)
{
    auto baseline = preset(8);
    baseline.version = 19;
    baseline.layers[4].profile = 0;
    auto candidate = baseline;
    for (auto &layer : candidate.layers)
        if (layer.kind == Kind::StrataRelief)
        {
            layer.profile = 3;
            layer.amplitude = 0.45f;
            layer.frequency = 1.6f;
            layer.width = 0.12f;
            layer.mask = Mask::None;
        }
    auto before = evaluate(baseline, baseline.finalResolution);
    auto after = evaluate(candidate, candidate.finalResolution);
    auto beforeRelief = evaluate(baseline, baseline.finalResolution, 1);
    auto afterRelief = evaluate(candidate, candidate.finalResolution, 1);
    if (beforeRelief.mesh.indices != afterRelief.mesh.indices ||
        beforeRelief.mesh.positions.size() != afterRelief.mesh.positions.size())
        throw std::runtime_error("Cliff strata relief changed the open grid");
    if (before.mesh.positions.size() != after.mesh.positions.size() ||
        before.mesh.indices.size() != after.mesh.indices.size() ||
        before.mesh.triangleRegions.size() != after.mesh.triangleRegions.size())
        throw std::runtime_error("Cliff strata changed grid size");
    std::size_t changedIndices = 0, changedTriangleRegions = 0;
    for (std::size_t i = 0; i < before.mesh.indices.size(); ++i)
        changedIndices += before.mesh.indices[i] != after.mesh.indices[i];
    for (std::size_t i = 0; i < before.mesh.triangleRegions.size(); ++i)
        changedTriangleRegions += before.mesh.triangleRegions[i] != after.mesh.triangleRegions[i];
    std::set<Jerico::Guid> beforeCuts, afterCuts;
    for (const auto &region : before.regions)
        if (region.role == SurfaceRole::CutLoop)
            beforeCuts.insert(region.id);
    for (const auto &region : after.regions)
        if (region.role == SurfaceRole::CutLoop)
            afterCuts.insert(region.id);
    if (beforeCuts.empty() || beforeCuts != afterCuts)
        throw std::runtime_error("Cliff strata changed cut selection identity");
    if (!PA::validateClosedMesh(after.mesh).hasValue())
        throw std::runtime_error("Cliff strata opened the mesh");
    if (meshHash(after.mesh) != meshHash(evaluate(deserialize(serialize(candidate)),
                                            candidate.finalResolution).mesh))
        throw std::runtime_error("Cliff strata replay mismatch");
    auto wallOnly = candidate;
    wallOnly.layers.erase(std::remove_if(wallOnly.layers.begin(), wallOnly.layers.end(),
                                         [](const Layer &layer)
                                         { return layer.kind == Kind::CliffCutLoop; }),
                          wallOnly.layers.end());
    auto wallMesh = evaluate(wallOnly, wallOnly.finalResolution);
    if (!PA::validateClosedMesh(wallMesh.mesh).hasValue() ||
        meshHash(wallMesh.mesh) != meshHash(evaluate(deserialize(serialize(wallOnly)),
                                                  wallOnly.finalResolution).mesh))
        throw std::runtime_error("Uncut cliff wall failed closed replay");
    auto variedWall = wallOnly;
    variedWall.version = 20;
    for (auto &layer : variedWall.layers)
        if (layer.kind == Kind::StrataRelief)
        {
            layer.profile = 4;
            layer.jitter = 0.7f;
            layer.profileSharpness = 1;
        }
    auto variedMesh = evaluate(variedWall, variedWall.finalResolution);
    if (!PA::validateClosedMesh(variedMesh.mesh).hasValue() ||
        meshHash(variedMesh.mesh) != meshHash(evaluate(deserialize(serialize(variedWall)),
                                                      variedWall.finalResolution).mesh))
        throw std::runtime_error("Varied cliff wall failed closed replay");
    auto outlinedWall = variedWall;
    auto perimeter = defaultLayer(Kind::VectorNoise);
    perimeter.seed = 911;
    perimeter.amplitude = 0.3f;
    perimeter.frequency = 0.7f;
    perimeter.octaves = 2;
    perimeter.outputScale = {0.8f, 0.8f, 0};
    perimeter.mask = Mask::BoundaryProximity;
    perimeter.maskLow = 0;
    perimeter.maskHigh = 1;
    perimeter.featureRadius = 0.75f;
    outlinedWall.layers.insert(outlinedWall.layers.begin() + 1, perimeter);
    auto outlinedMesh = evaluate(outlinedWall, outlinedWall.finalResolution);
    if (!PA::validateClosedMesh(outlinedMesh.mesh).hasValue() ||
        meshHash(outlinedMesh.mesh) != meshHash(evaluate(deserialize(serialize(outlinedWall)),
                                                          outlinedWall.finalResolution).mesh))
        throw std::runtime_error("Outlined cliff wall failed closed replay");
    auto beforeOutline = evaluate(variedWall, variedWall.finalResolution, 0);
    auto afterOutline = evaluate(outlinedWall, outlinedWall.finalResolution, 1);
    if (beforeOutline.mesh.indices != afterOutline.mesh.indices)
        throw std::runtime_error("Cliff perimeter changed the open grid");
    double maxOutlineXY = 0, maxCenterXY = 0;
    for (std::size_t i = 0; i < beforeOutline.mesh.positions.size(); ++i)
    {
        const auto a = beforeOutline.mesh.positions[i], b = afterOutline.mesh.positions[i];
        const auto xy = std::hypot(a.x - b.x, a.y - b.y);
        maxOutlineXY = std::max(maxOutlineXY, xy);
        if (std::abs(a.x) < 1.5 && std::abs(a.y) < 1.0)
            maxCenterXY = std::max(maxCenterXY, xy);
    }
    if (maxOutlineXY < 0.03 || maxCenterXY > 1e-9)
        throw std::runtime_error("Cliff perimeter failed localized displacement");
    auto drainedWall = outlinedWall;
    auto drainage = defaultLayer(Kind::CliffDrainage);
    drainage.amplitude = 0.28f;
    drainage.width = 0.35f;
    drainage.translation[0] = -0.8f;
    drainage.lateral = 0.25f;
    drainage.frequency = 0.75f;
    drainedWall.layers.insert(drainedWall.layers.end() - 1, drainage);
    auto drainedMesh = evaluate(drainedWall, drainedWall.finalResolution);
    if (!PA::validateClosedMesh(drainedMesh.mesh).hasValue() ||
        meshHash(drainedMesh.mesh) != meshHash(evaluate(deserialize(serialize(drainedWall)),
                                                         drainedWall.finalResolution).mesh))
        throw std::runtime_error("Drained cliff wall failed closed replay");
    auto finiteWall = drainedWall;
    finiteWall.version = 21;
    for (auto &layer : finiteWall.layers)
        if (layer.kind == Kind::CliffDrainage)
        {
            layer.profile = 1;
            layer.translation[1] = 0.1f;
            layer.projectorRadius[1] = 1.25f;
        }
    auto finiteMesh = evaluate(finiteWall, finiteWall.finalResolution);
    if (!PA::validateClosedMesh(finiteMesh.mesh).hasValue() ||
        meshHash(finiteMesh.mesh) != meshHash(evaluate(deserialize(serialize(finiteWall)),
                                                        finiteWall.finalResolution).mesh))
        throw std::runtime_error("Finite cliff wall failed closed replay");
    auto outlinedOpen = evaluate(outlinedWall, outlinedWall.finalResolution,
                                int(outlinedWall.layers.size()) - 2);
    auto finiteOpen = evaluate(finiteWall, finiteWall.finalResolution,
                              int(finiteWall.layers.size()) - 2);
    if (outlinedOpen.mesh.indices != finiteOpen.mesh.indices)
        throw std::runtime_error("Finite cliff incision changed open grid topology");
    double maxFiniteRetreat = 0, maxOutsideFiniteRetreat = 0;
    for (std::size_t i = 0; i < outlinedOpen.mesh.positions.size(); ++i)
    {
        const auto before = outlinedOpen.mesh.positions[i], after = finiteOpen.mesh.positions[i];
        const auto retreat = before.z - after.z;
        maxFiniteRetreat = std::max(maxFiniteRetreat, retreat);
        if (std::abs(before.y - 0.1) > 1.25)
            maxOutsideFiniteRetreat = std::max(maxOutsideFiniteRetreat, std::abs(retreat));
    }
    if (maxFiniteRetreat < 0.1 || maxOutsideFiniteRetreat > 1e-9)
        throw std::runtime_error("Finite cliff incision failed localized retreat");
    save(baseline, evidence / "baseline-recipe.json");
    save(candidate, evidence / "candidate-recipe.json");
    save(wallOnly, evidence / "wall-only-recipe.json");
    save(variedWall, evidence / "varied-wall-recipe.json");
    save(outlinedWall, evidence / "outlined-wall-recipe.json");
    save(drainedWall, evidence / "drained-wall-recipe.json");
    save(finiteWall, evidence / "finite-wall-recipe.json");
    double maximum = 0, sum = 0;
    for (std::size_t i = 0; i < beforeRelief.mesh.positions.size(); ++i)
    {
        const auto a = beforeRelief.mesh.positions[i], b = afterRelief.mesh.positions[i];
        const auto displacement = std::hypot(a.x - b.x, a.y - b.y, a.z - b.z);
        maximum = std::max(maximum, displacement);
        sum += displacement;
    }
    const int appearance = materialAppearance(candidate, 0);
    nlohmann::json proof = {{"scope", "High geometry creation only; no low mesh or bake"},
                            {"adapter", renderer.adapter()},
                            {"imageSize", {1280, 960}},
                            {"baselineHash", meshHash(before.mesh)},
                            {"candidateHash", meshHash(after.mesh)},
                            {"wallOnlyHash", meshHash(wallMesh.mesh)},
                            {"variedWallHash", meshHash(variedMesh.mesh)},
                            {"outlinedWallHash", meshHash(outlinedMesh.mesh)},
                            {"drainedWallHash", meshHash(drainedMesh.mesh)},
                            {"finiteWallHash", meshHash(finiteMesh.mesh)},
                            {"triangles", after.mesh.indices.size() / 3},
                            {"wallOnlyTriangles", wallMesh.mesh.indices.size() / 3},
                            {"variedWallTriangles", variedMesh.mesh.indices.size() / 3},
                            {"outlinedWallTriangles", outlinedMesh.mesh.indices.size() / 3},
                            {"drainedWallTriangles", drainedMesh.mesh.indices.size() / 3},
                            {"finiteWallTriangles", finiteMesh.mesh.indices.size() / 3},
                            {"maximumFiniteIncisionRetreatM", maxFiniteRetreat},
                            {"maximumOutsideFiniteIncisionRetreatM", maxOutsideFiniteRetreat},
                            {"maximumPerimeterXYDisplacementM", maxOutlineXY},
                            {"maximumCenterXYDisplacementM", maxCenterXY},
                            {"maximumReliefStageDisplacementM", maximum},
                            {"meanReliefStageDisplacementM", sum / beforeRelief.mesh.positions.size()},
                            {"baselineGenerationCpuMs", before.milliseconds},
                            {"candidateGenerationCpuMs", after.milliseconds},
                            {"variedWallGenerationCpuMs", variedMesh.milliseconds},
                            {"outlinedWallGenerationCpuMs", outlinedMesh.milliseconds},
                            {"drainedWallGenerationCpuMs", drainedMesh.milliseconds},
                            {"finiteWallGenerationCpuMs", finiteMesh.milliseconds},
                            {"gridSizeAndCutIdentityPreserved", true},
                            {"changedTriangleIndexEntries", changedIndices},
                            {"changedTriangleRegionEntries", changedTriangleRegions},
                            {"closedAndReplay", true}};
    for (const auto &stage : after.stages)
        proof["candidateStages"].push_back({{"kind", name(stage.kind)},
                                            {"cpuMs", stage.milliseconds}});
    for (int view = 0; view < 2; ++view)
    {
        Camera camera;
        camera.yaw = view == 0 ? 0.0f : 0.55f;
        camera.pitch = view == 0 ? 0.0f : -0.2f;
        camera.distance = 10.0f;
        proof["views"].push_back({{"name", view == 0 ? "front" : "oblique"},
                                  {"yaw", camera.yaw}, {"pitch", camera.pitch},
                                  {"distance", camera.distance}});
        for (int variant = 0; variant < 7; ++variant)
        {
            renderer.upload(variant == 0 ? before : variant == 1 ? after :
                            variant == 2 ? wallMesh : variant == 3 ? variedMesh :
                            variant == 4 ? outlinedMesh : variant == 5 ? drainedMesh : finiteMesh);
            renderer.uploadMaterial(nullptr, materialDiffuse(appearance), true, appearance);
            const auto stem = std::string(variant == 0 ? "baseline" :
                                          variant == 1 ? "candidate" :
                                          variant == 2 ? "wall-only" :
                                          variant == 3 ? "varied-wall" :
                                          variant == 4 ? "outlined-wall" :
                                          variant == 5 ? "drained-wall" : "finite-wall") +
                              (view == 0 ? "-front" : "-oblique");
            for (int mode : {0, 5})
            {
                renderer.draw(1280, 960, camera, mode);
                renderer.capture(evidence / (stem + (mode == 5 ? "-material.png" : "-neutral.png")));
            }
        }
    }
    proof["renderBufferBytes"] = renderer.bufferBytes();
    std::ofstream(evidence / "cliff-strata-creation-proof.json") << proof.dump(2) << '\n';
    std::cout << proof.dump(2) << '\n';
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
    bool splineEvidence=false,assemblyEvidence=false,webEvidence=false;
    bool smoke = false, materialGallerySmoke = false, featureSmoke = false,
         angularProfileSmoke = false, extremeDeformationSmoke = false,
         cliffSoftCreationSmoke = false, rockLodSmoke = false,
         autoUvRockLodSmoke = false, curvatureRelaxationSmoke = false,
         angularFractureSmoke = false, angularFourBlocksSmoke = false,
         cliffDrainageSmoke = false,
         cliffEdgeDamageSmoke = false, cliffRBrushAlphaSmoke = false,
         cliffNonharmonicSmoke = false, cliffMacroWarpSmoke = false,
         cliffErodedRimSmoke = false, cliffSignedRimSmoke = false,
         cliffMetricReliefSmoke = false, cliffStrataCreationSmoke = false,
         layeredOcclusionSmoke = false,
         layeredMaterialBaselineSmoke = false, layeredMaterialCandidateSmoke = false,
         layeredMaterialNoMipsSmoke = false, layeredHorizonFractureSmoke = false,
         layeredSlateSourceSmoke = false,
         edgeRadiusSmoke = false, regeneratedLodSmoke = false, angularLod2Smoke = false,
         bakeSmoke = false, seededBakeSmoke = false,
         orbitBakeSmoke = false, scaleBakeSmoke = false, lowSweepSmoke = false,
         low10BakeSmoke = false, low10OrbitSmoke = false, adaptiveBakeSmoke = false,
         adaptiveOrbitSmoke = false, automaticUvBakeSmoke = false, caveFormsSmoke = false,
         caveGeologySmoke = false, caveAxialSmoke = false, caveBlindSmoke = false,
         caveSquareReliefSmoke = false, caveSelectedReliefSmoke = false,
         slateTemporalSmoke = false, cliffTemporalSmoke = false, strataTemporalSmoke = false,
         cliffMipBiasSmoke = false, strataMipBiasSmoke = false,
         cliffMomentSmoke = false, strataMomentSmoke = false;
    unsigned scaleTextureResolution = 4096;
    std::filesystem::path evidence = std::filesystem::path(PLAYGROUND_ROOT) / "Evidence";
    std::filesystem::path adaptiveLayoutPath;
    unsigned adaptiveSeed = 123;
    int automaticUvFamily = -1;
    int automaticUvRockLodFamily = -1;
    for (int i=1;i<argc;++i)
        if (std::string(argv[i])=="--spline-construction-test")
        {
            splineEvidence=true;
            if (i+1<argc) evidence=argv[i+1];
        }
    for (int i=1;i<argc;++i)
        if (std::string(argv[i])=="--procedural-assemblies-test")
        {
            assemblyEvidence=true;
            if (i+1<argc) evidence=argv[i+1];
        }
    for (int i=1;i<argc;++i)
        if (std::string(argv[i])=="--web-surface-test")
        {
            webEvidence=true;
            if (i+1<argc) evidence=argv[i+1];
        }
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--smoke-test" ||
            std::string(argv[i]) == "--material-gallery-test" ||
            std::string(argv[i]) == "--surface-features-test" ||
            std::string(argv[i]) == "--angular-profiles-test" ||
            std::string(argv[i]) == "--extreme-deformation-test" ||
            std::string(argv[i]) == "--cliff-soft-creation-test" ||
            std::string(argv[i]) == "--rock-lod-bakes-test" ||
            std::string(argv[i]) == "--auto-uv-rock-lod-bakes-test" ||
            std::string(argv[i]) == "--curvature-relaxation-bakes-test" ||
            std::string(argv[i]) == "--angular-fracture-bakes-test" ||
            std::string(argv[i]) == "--angular-four-block-bakes-test" ||
            std::string(argv[i]) == "--cliff-drainage-bakes-test" ||
            std::string(argv[i]) == "--cliff-edge-damage-bakes-test" ||
            std::string(argv[i]) == "--cliff-rbrush-alpha-bakes-test" ||
            std::string(argv[i]) == "--cliff-nonharmonic-bakes-test" ||
            std::string(argv[i]) == "--cliff-macro-warp-bakes-test" ||
            std::string(argv[i]) == "--cliff-eroded-rim-bakes-test" ||
            std::string(argv[i]) == "--cliff-signed-rim-bakes-test" ||
            std::string(argv[i]) == "--cliff-metric-relief-bakes-test" ||
            std::string(argv[i]) == "--cliff-strata-creation-test" ||
            std::string(argv[i]) == "--layered-occlusion-bakes-test" ||
            std::string(argv[i]) == "--layered-material-baseline-test" ||
            std::string(argv[i]) == "--layered-material-candidate-test" ||
            std::string(argv[i]) == "--layered-material-no-mips-test" ||
            std::string(argv[i]) == "--layered-horizon-fracture-bakes-test" ||
            std::string(argv[i]) == "--layered-slate-source-bakes-test" ||
            std::string(argv[i]) == "--slate-temporal-bake-test" ||
            std::string(argv[i]) == "--cliff-temporal-bake-test" ||
            std::string(argv[i]) == "--strata-temporal-bake-test" ||
            std::string(argv[i]) == "--cliff-mip-bias-test" ||
            std::string(argv[i]) == "--strata-mip-bias-test" ||
            std::string(argv[i]) == "--cliff-normal-moments-test" ||
            std::string(argv[i]) == "--strata-normal-moments-test" ||
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
            std::string(argv[i]) == "--auto-uv-bake-test" ||
            std::string(argv[i]) == "--cave-forms-bakes-test" ||
            std::string(argv[i]) == "--cave-geology-bakes-test" ||
            std::string(argv[i]) == "--cave-axial-lods-bakes-test" ||
            std::string(argv[i]) == "--cave-blind-chamber-bakes-test" ||
            std::string(argv[i]) == "--cave-square-relief-bakes-test" ||
            std::string(argv[i]) == "--cave-selected-relief-bakes-test")
        {
            materialGallerySmoke = std::string(argv[i]) == "--material-gallery-test";
            featureSmoke = std::string(argv[i]) == "--surface-features-test";
            angularProfileSmoke = std::string(argv[i]) == "--angular-profiles-test";
            extremeDeformationSmoke = std::string(argv[i]) == "--extreme-deformation-test";
            cliffSoftCreationSmoke = std::string(argv[i]) == "--cliff-soft-creation-test";
            rockLodSmoke = std::string(argv[i]) == "--rock-lod-bakes-test";
            autoUvRockLodSmoke = std::string(argv[i]) == "--auto-uv-rock-lod-bakes-test";
            curvatureRelaxationSmoke = std::string(argv[i]) == "--curvature-relaxation-bakes-test";
            angularFractureSmoke = std::string(argv[i]) == "--angular-fracture-bakes-test";
            angularFourBlocksSmoke = std::string(argv[i]) == "--angular-four-block-bakes-test";
            cliffDrainageSmoke = std::string(argv[i]) == "--cliff-drainage-bakes-test";
            cliffEdgeDamageSmoke = std::string(argv[i]) == "--cliff-edge-damage-bakes-test";
            cliffRBrushAlphaSmoke = std::string(argv[i]) == "--cliff-rbrush-alpha-bakes-test";
            cliffNonharmonicSmoke = std::string(argv[i]) == "--cliff-nonharmonic-bakes-test";
            cliffMacroWarpSmoke = std::string(argv[i]) == "--cliff-macro-warp-bakes-test";
            cliffErodedRimSmoke = std::string(argv[i]) == "--cliff-eroded-rim-bakes-test";
            cliffSignedRimSmoke = std::string(argv[i]) == "--cliff-signed-rim-bakes-test";
            cliffMetricReliefSmoke = std::string(argv[i]) == "--cliff-metric-relief-bakes-test";
            cliffStrataCreationSmoke = std::string(argv[i]) == "--cliff-strata-creation-test";
            layeredOcclusionSmoke = std::string(argv[i]) == "--layered-occlusion-bakes-test";
            layeredMaterialBaselineSmoke = std::string(argv[i]) == "--layered-material-baseline-test";
            layeredMaterialCandidateSmoke = std::string(argv[i]) == "--layered-material-candidate-test";
            layeredMaterialNoMipsSmoke = std::string(argv[i]) == "--layered-material-no-mips-test";
            layeredHorizonFractureSmoke = std::string(argv[i]) == "--layered-horizon-fracture-bakes-test";
            layeredSlateSourceSmoke = std::string(argv[i]) == "--layered-slate-source-bakes-test";
            slateTemporalSmoke = std::string(argv[i]) == "--slate-temporal-bake-test";
            cliffTemporalSmoke = std::string(argv[i]) == "--cliff-temporal-bake-test";
            strataTemporalSmoke = std::string(argv[i]) == "--strata-temporal-bake-test";
            cliffMipBiasSmoke = std::string(argv[i]) == "--cliff-mip-bias-test";
            strataMipBiasSmoke = std::string(argv[i]) == "--strata-mip-bias-test";
            cliffMomentSmoke = std::string(argv[i]) == "--cliff-normal-moments-test";
            strataMomentSmoke = std::string(argv[i]) == "--strata-normal-moments-test";
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
            caveFormsSmoke = std::string(argv[i]) == "--cave-forms-bakes-test" ||
                             std::string(argv[i]) == "--cave-geology-bakes-test" ||
                             std::string(argv[i]) == "--cave-axial-lods-bakes-test" ||
                             std::string(argv[i]) == "--cave-blind-chamber-bakes-test" ||
                             std::string(argv[i]) == "--cave-square-relief-bakes-test" ||
                             std::string(argv[i]) == "--cave-selected-relief-bakes-test";
            caveGeologySmoke = std::string(argv[i]) == "--cave-geology-bakes-test" ||
                               std::string(argv[i]) == "--cave-axial-lods-bakes-test" ||
                               std::string(argv[i]) == "--cave-blind-chamber-bakes-test" ||
                               std::string(argv[i]) == "--cave-square-relief-bakes-test" ||
                               std::string(argv[i]) == "--cave-selected-relief-bakes-test";
            caveAxialSmoke = std::string(argv[i]) == "--cave-axial-lods-bakes-test" ||
                             std::string(argv[i]) == "--cave-blind-chamber-bakes-test" ||
                             std::string(argv[i]) == "--cave-square-relief-bakes-test" ||
                             std::string(argv[i]) == "--cave-selected-relief-bakes-test";
            caveBlindSmoke = std::string(argv[i]) == "--cave-blind-chamber-bakes-test" ||
                             std::string(argv[i]) == "--cave-square-relief-bakes-test" ||
                             std::string(argv[i]) == "--cave-selected-relief-bakes-test";
            caveSquareReliefSmoke = std::string(argv[i]) == "--cave-square-relief-bakes-test";
            caveSelectedReliefSmoke = std::string(argv[i]) == "--cave-selected-relief-bakes-test";
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
    if (smoke || splineEvidence || assemblyEvidence || webEvidence)
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
    if (splineEvidence)
    {
        captureSplineConstruction(renderer,evidence);
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(previousProc));
        return 0;
    }
    if (assemblyEvidence)
    {
        captureProceduralAssemblies(renderer,evidence);
        SetWindowLongPtrW(hwnd,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(previousProc));
        return 0;
    }
    if (webEvidence)
    {
        captureWebSurface(renderer,evidence);
        SetWindowLongPtrW(hwnd,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(previousProc));
        return 0;
    }
    Recipe recipe = preset(0);
    MeshResult displayed = evaluate(recipe, recipe.previewResolution);
    renderer.upload(displayed);
    renderer.uploadMaterial(nullptr, materialDiffuse(materialAppearance(recipe, 0)));
    Camera camera;
    SplineLabUI splineLab;
    AssemblyLabUI assemblyLab;
    WebLabUI webLab;
    int workspace = 0;
    int selected = 0, presetIndex = 0, mode = 4, inspection = 0, addKind = 1;
    int materialChoice = 0;
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
    bool movingDamageArea=false;
    auto bindMaterial = [&](const NormalBake *bake)
    {
        const int appearance = materialAppearance(recipe, materialChoice);
        renderer.uploadMaterial(bake, materialDiffuse(appearance), true, appearance);
    };
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
            bindMaterial(nullptr);
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
                    bindMaterial(nullptr);
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
                    bindMaterial(&bakedPreview->bake);
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
        ImGui::TextUnformatted("Workspace:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Geology", workspace==0) && workspace!=0)
        {
            workspace = 0;
            renderer.upload(displayed);
            bindMaterial(nullptr);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Spline Construction", workspace==1) && workspace!=1)
        {
            workspace = 1;
            splineLab.activate(renderer);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Procedural Assemblies", workspace==2) && workspace!=2)
        {
            workspace = 2;
            assemblyLab.activate(renderer);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Spider Web Surface",workspace==3) && workspace!=3)
        {
            workspace=3;
            webLab.activate();
        }
        ImGui::End();
        if (workspace==1)
        {
            splineLab.frame(renderer,w,h);
            renderer.present();
            continue;
        }
        if (workspace==2)
        {
            assemblyLab.frame(renderer,w,h);
            renderer.present();
            continue;
        }
        if (workspace==3)
        {
            webLab.frame(renderer,w,h);
            renderer.present();
            continue;
        }
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
                "Angular wear v4\0Layered rock v9\0Crater rock v5\0Slate source experiment\0"))
        {
            replace(preset(presetIndex));
            lowError = presetIndex == 10 ? 0.005f : presetIndex == 11 || presetIndex == 13 ? 0.02f :
                       presetIndex == 5 ? 0.08f : 0.04f;
            projectionDistance = presetIndex == 5 || presetIndex == 11 || presetIndex == 13 ? 0.2f : 0.15f;
            lowStrategy = presetIndex == 5 ? 1 : 0;
        }
        int base = int(recipe.base);
        if (ImGui::Combo("Base", &base,
                         "Cube\0Sphere\0Cylinder\0Cone\0Plane / Wall\0BSP wall (engine fixture)\0"))
        {
            recipe.base = static_cast<Base>(base);
            if (recipe.base == Base::Cube && std::any_of(recipe.layers.begin(), recipe.layers.end(),
                [](const Layer &layer) { return layer.kind == Kind::Layers && layer.profile == 2; }))
                recipe.version = std::max(recipe.version, 16);
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
            if (editLayer(recipe.layers[selected]))
            {
                if (recipe.layers[selected].kind == Kind::CliffCutLoop &&
                    recipe.layers[selected].profile > 0)
                    recipe.version = std::max(recipe.version,
                                              recipe.layers[selected].profile == 4 ? 17 :
                                              recipe.layers[selected].profile == 3 ? 14 : 13);
                if (recipe.layers[selected].kind == Kind::Layers &&
                    recipe.layers[selected].profile == 2)
                    recipe.version = std::max(recipe.version, recipe.base == Base::Cube ? 16 : 15);
                anyChange = true;
            }
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
                                                                &token, projection, lowResolution,
                                                                nullptr, 0.02, true);
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
            {
                renderer.upload(bakeView == 0 ? bakedPreview->high : b.low, bakeView == 2 ? &b : nullptr);
                bindMaterial(bakeView == 2 ? &b : nullptr);
            }
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
            ImGui::TextWrapped("Experimental automatic UV charts. Preserve UVs and split normals; no "
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
        ImGui::Combo("View", &mode, "Lit\0Normals\0Wireframe\0Field\0Material\0");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(125);
        if (ImGui::Combo("Material", &materialChoice,
                         "Auto\0Rock\0Cliff\0Slate\0Basalt\0"))
            bindMaterial(bakedPreview && bakeView == 2 ? &bakedPreview->bake : nullptr);
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
        const bool selectedDamageArea=selected>=0 && selected<int(recipe.layers.size()) &&
            recipe.layers[selected].kind==Kind::DamageDecal;
        ImGui::TextUnformatted(selectedDamageArea ?
            "Shift+drag: move selected damage area | Drag: orbit | Wheel: zoom" :
            "Drag left: orbit | Middle/right: pan | Wheel: zoom");
        auto size = ImGui::GetContentRegionAvail();
        size.x = std::max(size.x, 32.0f);
        size.y = std::max(size.y, 32.0f);
        auto texture = renderer.draw(unsigned(size.x), unsigned(size.y), camera, mode == 4 ? 5 : mode);
        ImGui::Image(reinterpret_cast<ImTextureID>(texture), size);
        if (selectedDamageArea)
        {
            const auto &center=recipe.layers[selected].translation;
            if (const auto point=projectToViewport({center[0],center[1],center[2]},
                                                   camera,unsigned(size.x),unsigned(size.y)))
                if (point->x>=0 && point->x<=size.x && point->y>=0 && point->y<=size.y)
                {
                    const auto origin=ImGui::GetItemRectMin();
                    const ImVec2 at{origin.x+float(point->x),origin.y+float(point->y)};
                    auto *draw=ImGui::GetWindowDrawList();
                    draw->AddCircle(at,10,IM_COL32(255,195,55,255),24,2);
                    draw->AddLine({at.x-14,at.y},{at.x+14,at.y},
                                  IM_COL32(255,195,55,255),1);
                    draw->AddLine({at.x,at.y-14},{at.x,at.y+14},
                                  IM_COL32(255,195,55,255),1);
                }
        }
        if (ImGui::IsItemHovered())
        {
            auto &io = ImGui::GetIO();
            const bool moveArea=selectedDamageArea && io.KeyShift &&
                ImGui::IsMouseDown(ImGuiMouseButton_Left);
            if (moveArea)
            {
                auto &layer=recipe.layers[selected];
                const Vec3d center{layer.translation[0],layer.translation[1],
                                   layer.translation[2]};
                const Vec3d normal{layer.direction[0],layer.direction[1],
                                   layer.direction[2]};
                const auto origin=ImGui::GetItemRectMin();
                if (const auto hit=pointOnPlaneAtPixel(camera,unsigned(size.x),unsigned(size.y),
                        io.MousePos.x-origin.x,io.MousePos.y-origin.y,center,normal))
                {
                    if (!movingDamageArea) remember(recipe);
                    movingDamageArea=true;
                    layer.translation={float(hit->x),float(hit->y),float(hit->z)};
                    changed();
                }
            }
            if (!moveArea && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
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
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) movingDamageArea=false;
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
            if (materialGallerySmoke)
            {
                nlohmann::json report = {{"adapter", renderer.adapter()},
                                         {"imageSize", {1280, 960}},
                                         {"bakedMaps", false},
                                         {"presets", nlohmann::json::array()}};
                for (int index = 0; index < 14; ++index)
                {
                    auto sample = preset(index);
                    auto mesh = evaluate(sample, sample.finalResolution);
                    const int appearance = materialAppearance(sample, 0);
                    renderer.upload(mesh);
                    renderer.uploadMaterial(nullptr, materialDiffuse(appearance), true, appearance);
                    auto captureCamera = camera;
                    captureCamera.distance = sample.base == Base::Plane ||
                        sample.base == Base::BspWall ? 10.0f : 5.8f;
                    renderer.draw(1280, 960, captureCamera, 5);
                    renderer.capture(evidence / ("preset-" + std::to_string(index) + "-material.png"));
                    report["presets"].push_back({{"preset", index}, {"appearance", appearance},
                                                  {"diffuse", materialDiffuse(appearance).filename().string()},
                                                  {"meshHash", meshHash(mesh.mesh)},
                                                  {"generationCpuMs", mesh.milliseconds}});
                }
                auto sample = preset(8);
                auto mesh = evaluate(sample, sample.finalResolution);
                renderer.upload(mesh);
                for (int appearance = 0; appearance < 4; ++appearance)
                {
                    renderer.uploadMaterial(nullptr, materialDiffuse(appearance), true, appearance);
                    auto captureCamera = camera;
                    captureCamera.distance = 10;
                    renderer.draw(1280, 960, captureCamera, 5);
                    renderer.capture(evidence / ("cliff-material-" + std::to_string(appearance) + ".png"));
                }
                std::ofstream(evidence / "material-gallery-proof.json") << report.dump(2) << '\n';
                window.requestClose();
                continue;
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
            if (extremeDeformationSmoke)
            {
                captureExtremeDeformation(renderer, evidence);
                window.requestClose();
                continue;
            }
            if (cliffSoftCreationSmoke)
            {
                captureCliffSoftCreation(renderer, evidence);
                window.requestClose();
                continue;
            }
            if (cliffStrataCreationSmoke)
            {
                captureCliffStrataCreation(renderer, evidence);
                window.requestClose();
                continue;
            }
            if (slateTemporalSmoke)
            {
                captureSlateTemporal(renderer, evidence);
                window.requestClose();
                continue;
            }
            if (cliffTemporalSmoke || strataTemporalSmoke || cliffMipBiasSmoke || strataMipBiasSmoke ||
                cliffMomentSmoke || strataMomentSmoke)
            {
                captureRockTemporal(renderer, evidence,
                                    cliffTemporalSmoke || cliffMipBiasSmoke || cliffMomentSmoke ? 8 : 11,
                                    cliffMipBiasSmoke || strataMipBiasSmoke,
                                    cliffMomentSmoke || strataMomentSmoke);
                window.requestClose();
                continue;
            }
            if (caveFormsSmoke)
            {
                captureCaveForms(renderer, evidence, caveGeologySmoke, caveAxialSmoke,
                                 caveBlindSmoke, caveSquareReliefSmoke, caveSelectedReliefSmoke);
                window.requestClose();
                continue;
            }
            if (rockLodSmoke || autoUvRockLodSmoke || curvatureRelaxationSmoke ||
                angularFractureSmoke || angularFourBlocksSmoke || cliffDrainageSmoke ||
                cliffEdgeDamageSmoke || cliffRBrushAlphaSmoke || cliffNonharmonicSmoke || cliffMacroWarpSmoke ||
                cliffErodedRimSmoke || cliffSignedRimSmoke || cliffMetricReliefSmoke || layeredOcclusionSmoke ||
                layeredMaterialBaselineSmoke || layeredMaterialCandidateSmoke ||
                layeredMaterialNoMipsSmoke || layeredHorizonFractureSmoke || layeredSlateSourceSmoke)
            {
                captureRockLodBakes(renderer, evidence,
                                    autoUvRockLodSmoke || curvatureRelaxationSmoke ||
                                        angularFractureSmoke || angularFourBlocksSmoke ||
                                        cliffDrainageSmoke || cliffEdgeDamageSmoke || cliffRBrushAlphaSmoke || cliffNonharmonicSmoke ||
                                        cliffMacroWarpSmoke || cliffErodedRimSmoke || cliffSignedRimSmoke ||
                                        cliffMetricReliefSmoke ||
                                        layeredOcclusionSmoke ||
                                        layeredMaterialBaselineSmoke || layeredMaterialCandidateSmoke ||
                                        layeredMaterialNoMipsSmoke || layeredHorizonFractureSmoke ||
                                        layeredSlateSourceSmoke,
                                    cliffDrainageSmoke || cliffEdgeDamageSmoke || cliffRBrushAlphaSmoke || cliffNonharmonicSmoke ||
                                    cliffMacroWarpSmoke || cliffErodedRimSmoke || cliffSignedRimSmoke ||
                                    cliffMetricReliefSmoke ? 8 :
                                    layeredOcclusionSmoke || layeredMaterialBaselineSmoke ||
                                        layeredMaterialCandidateSmoke || layeredMaterialNoMipsSmoke ||
                                        layeredHorizonFractureSmoke || layeredSlateSourceSmoke ? 11 :
                                    angularFractureSmoke || angularFourBlocksSmoke ? 10 :
                                        automaticUvRockLodFamily,
                                    curvatureRelaxationSmoke ? 0.5 : 0, angularFractureSmoke,
                                    angularFourBlocksSmoke, cliffDrainageSmoke,
                                    cliffEdgeDamageSmoke,
                                    cliffRBrushAlphaSmoke || cliffNonharmonicSmoke || cliffMacroWarpSmoke ||
                                    cliffErodedRimSmoke || cliffSignedRimSmoke || cliffMetricReliefSmoke,
                                    layeredOcclusionSmoke || layeredMaterialCandidateSmoke,
                                    layeredMaterialBaselineSmoke || layeredMaterialCandidateSmoke ||
                                    layeredMaterialNoMipsSmoke || layeredHorizonFractureSmoke ||
                                    layeredSlateSourceSmoke,
                                    layeredMaterialNoMipsSmoke, cliffNonharmonicSmoke,
                                    cliffMacroWarpSmoke, cliffErodedRimSmoke,
                                    layeredHorizonFractureSmoke || layeredSlateSourceSmoke,
                                    layeredSlateSourceSmoke, cliffSignedRimSmoke,
                                    cliffMetricReliefSmoke);
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
