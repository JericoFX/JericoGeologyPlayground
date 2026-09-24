#include "NormalBake.hpp"
#include "SolidGeometry.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace Playground;
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
template <class F> void rejects(F f, const char *message)
{
    bool rejected = false;
    try
    {
        f();
    }
    catch (const std::exception &)
    {
        rejected = true;
    }
    require(rejected, message);
}
int main()
{
    try
    {
        Recipe recipe;
        recipe.base = Base::Plane;
        recipe.planeSize = {2, 2};
        auto low = evaluate(recipe, 4), high = evaluate(recipe, 16);
        NormalBakeSettings settings{.resolution = 128, .projectionDistance = 0.2, .padding = 2};
        auto flat = bakeNormals(high, low, settings);
        require(flat.samples > 1000 && flat.misses == 0, "flat high projects onto every covered low texel");
        require(flat.rgba.size() == 128 * 128 * 4 && flat.cornerUV.size() == low.mesh.indices.size(),
                "bake returns texture and target corner UVs");
        auto automaticSettings = settings;
        automaticSettings.automaticUv = true;
        auto automaticFlat = bakeNormals(high, low, automaticSettings);
        require(automaticFlat.normalMips.size() == 3 &&
                    automaticFlat.normalMips[0].size() == 64 * 64 * 4 &&
                    automaticFlat.normalMips[1].size() == 32 * 32 * 4 &&
                    automaticFlat.normalMips[2].size() == 16 * 16 * 4,
                "automatic UV normal bake produces the three requested filtered mip levels");
        require(automaticFlat.cornerUV != flat.cornerUV &&
                    automaticFlat.samples > flat.samples && automaticFlat.misses == 0 &&
                    automaticFlat.automaticUvIslandCount == 1 &&
                    automaticFlat.automaticUvMarginPixels == 8 &&
                    automaticFlat.uvTriangleIsland.size() == low.mesh.indices.size() / 3,
                "automatic UV charts drive the high-to-low normal bake coverage");
        for (std::size_t pixel = 0; pixel < automaticFlat.rgba.size(); pixel += 4)
            if (automaticFlat.rgba[pixel + 3])
                require(automaticFlat.rgba[pixel] == 128 &&
                            automaticFlat.rgba[pixel + 1] == 128 &&
                            automaticFlat.rgba[pixel + 2] == 255,
                        "automatic UV tangent basis encodes a flat high source as neutral");
        MeshResult softFold;
        softFold.mesh.positions = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0.2}};
        softFold.mesh.indices = {0, 1, 2, 1, 3, 2};
        softFold.mesh.triangleRegions.assign(2, low.mesh.triangleRegions[0]);
        normals(softFold);
        auto folded = bakeNormals(softFold, softFold, automaticSettings);
        auto sameNormal = [](Vec3d a, Vec3d b)
        { return a.x * b.x + a.y * b.y + a.z * b.z > 0.99999; };
        require(folded.automaticUvIslandCount == 1 &&
                    sameNormal(folded.low.cornerNormals[1], folded.low.cornerNormals[3]) &&
                    sameNormal(folded.low.cornerNormals[2], folded.low.cornerNormals[5]) &&
                    sameNormal(folded.cornerTangents[1], folded.cornerTangents[3]) &&
                    sameNormal(folded.cornerTangents[2], folded.cornerTangents[5]),
                "shared vertices on one smooth UV chart use continuous low normal basis");
        require(folded.misses == 0, "soft fold projects without holes");
        for (std::size_t pixel = 0; pixel < folded.rgba.size(); pixel += 4)
            if (folded.rgba[pixel + 3])
                require(std::abs(int(folded.rgba[pixel]) - 128) <= 1 &&
                            std::abs(int(folded.rgba[pixel + 1]) - 128) <= 1 &&
                            folded.rgba[pixel + 2] == 255,
                        "shared-chart fold encodes a smooth identical high as neutral tangent normal");
        automaticSettings.specialistMaps = true;
        auto automaticSpecialist = bakeNormals(high, low, automaticSettings);
        require(automaticSpecialist.misses == 0 &&
                    automaticSpecialist.ambientRgba.size() == automaticSpecialist.rgba.size() &&
                    automaticSpecialist.contactRgba.size() == automaticSpecialist.rgba.size() &&
                    automaticSpecialist.curvatureRgba.size() == automaticSpecialist.rgba.size() &&
                    automaticSpecialist.cavityRgba.size() == automaticSpecialist.rgba.size() &&
                    automaticSpecialist.edgeRgba.size() == automaticSpecialist.rgba.size() &&
                    automaticSpecialist.wireRgba.size() == automaticSpecialist.rgba.size() &&
                    automaticSpecialist.thicknessRgba.size() == automaticSpecialist.rgba.size() &&
                    automaticSpecialist.regionRgba.size() == automaticSpecialist.rgba.size() &&
                    automaticSpecialist.heightRgba.size() == automaticSpecialist.rgba.size(),
                "Ucupaint-guided specialist maps use the automatic UV coverage");
        auto roughPlane = high;
        for (auto &position : roughPlane.mesh.positions)
            if (std::abs(position.x) < 1e-12 && std::abs(position.y) < 1e-12)
                position.z = 0.08;
        normals(roughPlane);
        auto rawSettings = automaticSettings;
        rawSettings.projection = NormalProjection::ClosestSurface;
        auto rawBake = bakeNormals(roughPlane, roughPlane, rawSettings);
        auto relaxedSettings = rawSettings;
        relaxedSettings.curvatureRelaxation = 0.5;
        auto relaxedBake = bakeNormals(roughPlane, roughPlane, relaxedSettings);
        require(rawBake.curvatureRgba != relaxedBake.curvatureRgba &&
                    rawBake.rgba == relaxedBake.rgba &&
                    rawBake.ambientRgba == relaxedBake.ambientRgba &&
                    rawBake.contactRgba == relaxedBake.contactRgba &&
                    rawBake.regionRgba == relaxedBake.regionRgba,
                "optional source curvature relaxation changes only curvature-derived bake maps");
        for (std::size_t pixel = 0; pixel < rawBake.rgba.size(); pixel += 4)
            if (rawBake.rgba[pixel + 3])
                require((rawBake.curvatureRgba[pixel] <= 129 ||
                         relaxedBake.curvatureRgba[pixel] >= 127) &&
                            (rawBake.curvatureRgba[pixel] >= 127 ||
                             relaxedBake.curvatureRgba[pixel] <= 129),
                        "relaxed bake cannot swap convex and concave source polarity");
        relaxedSettings.curvatureRelaxation = 1.1;
        rejects([&] { bakeNormals(roughPlane, roughPlane, relaxedSettings); },
                "bake rejects curvature relaxation outside unit interval");
        for (std::size_t pixel = 0; pixel < automaticSpecialist.rgba.size(); pixel += 4)
            if (automaticSpecialist.rgba[pixel + 3])
                require(automaticSpecialist.ambientRgba[pixel + 3] == 255 &&
                            automaticSpecialist.contactRgba[pixel + 3] == 255 &&
                            automaticSpecialist.cavityRgba[pixel + 3] == 255 &&
                            automaticSpecialist.edgeRgba[pixel + 3] == 255,
                        "surface maps cover the same automatic UV texels as normals");
        const auto mipDirectory = std::filesystem::current_path() / "normal-bake-specialist-mips";
        std::filesystem::create_directories(mipDirectory);
        for (const char *name : {"ambient-occlusion", "contact-occlusion", "signed-curvature",
                                 "cavity", "convex-edge", "crease-distance-mask", "thickness",
                                 "region-id", "signed-height"})
            for (unsigned level = 1; level <= 3; ++level)
            {
                const auto path = mipDirectory / (std::string(name) + "-mip-" +
                                                  std::to_string(level) + ".png");
                std::filesystem::remove(path);
            }
        exportNormalBake(automaticSpecialist, mipDirectory);
        for (const char *name : {"ambient-occlusion", "contact-occlusion", "signed-curvature",
                                 "cavity", "convex-edge", "crease-distance-mask", "thickness",
                                 "region-id", "signed-height"})
            for (unsigned level = 1; level <= 3; ++level)
                require(std::filesystem::exists(mipDirectory / (std::string(name) + "-mip-" +
                                                                 std::to_string(level) + ".png")),
                        "Ucupaint-guided specialist maps export every requested mip level");
        for (const auto *chain : {&automaticSpecialist.specialistMips.ambient,
                                  &automaticSpecialist.specialistMips.contact,
                                  &automaticSpecialist.specialistMips.curvature,
                                  &automaticSpecialist.specialistMips.cavity,
                                  &automaticSpecialist.specialistMips.edge,
                                  &automaticSpecialist.specialistMips.wire,
                                  &automaticSpecialist.specialistMips.thickness,
                                  &automaticSpecialist.specialistMips.region,
                                  &automaticSpecialist.specialistMips.height})
        {
            require(chain->size() == automaticSpecialist.normalMips.size(),
                    "every specialist chain has the same levels as the tangent normal");
            for (std::size_t level = 0; level < chain->size(); ++level)
            {
                require((*chain)[level].size() == automaticSpecialist.normalMips[level].size(),
                        "specialist mip dimensions match the normal mip");
                for (std::size_t pixel = 0; pixel < (*chain)[level].size(); pixel += 4)
                    require((*chain)[level][pixel + 3] ==
                                automaticSpecialist.normalMips[level][pixel + 3],
                            "specialist mip coverage matches the normal mip");
            }
        }
        auto splitRegions = softFold;
        splitRegions.mesh.triangleRegions[1] = Solid::region(99, 123);
        auto splitBake = bakeNormals(splitRegions, splitRegions, automaticSettings);
        require(splitBake.misses == 0 && splitBake.automaticUvIslandCount >= 2,
                "separate region charts bake on the same automatic atlas");
        std::set<std::array<std::uint8_t, 3>> baseRegionColors;
        for (std::size_t pixel = 0; pixel < splitBake.regionRgba.size(); pixel += 4)
            if (splitBake.regionRgba[pixel + 3])
                baseRegionColors.insert({splitBake.regionRgba[pixel], splitBake.regionRgba[pixel + 1],
                                         splitBake.regionRgba[pixel + 2]});
        require(baseRegionColors.size() == 2, "two source regions keep distinct baked IDs");
        for (const auto &mip : splitBake.specialistMips.region)
            for (std::size_t pixel = 0; pixel < mip.size(); pixel += 4)
                if (mip[pixel + 3])
                    require(baseRegionColors.contains({mip[pixel], mip[pixel + 1], mip[pixel + 2]}),
                            "categorical mip never invents a blended region ID");
        auto missingMip = automaticSpecialist;
        missingMip.specialistMips.cavity.pop_back();
        rejects([&] { exportNormalBake(missingMip, mipDirectory); },
                "export rejects a missing specialist mip level");
        auto malformedMip = automaticSpecialist;
        malformedMip.specialistMips.region[0].pop_back();
        rejects([&] { exportNormalBake(malformedMip, mipDirectory); },
                "export rejects malformed categorical mip dimensions");
        auto explicitSettings = settings;
        const auto chartCount = low.mesh.indices.size() / 3;
        const auto columns = unsigned(std::ceil(std::sqrt(double(chartCount))));
        const auto tileSize = settings.resolution / columns;
        for (std::size_t triangle = 0; triangle < chartCount; ++triangle)
            explicitSettings.chartLayout.push_back(
                {unsigned(triangle % columns) * tileSize,
                 unsigned(triangle / columns) * tileSize, tileSize});
        auto explicitFlat = bakeNormals(high, low, explicitSettings);
        require(explicitFlat.rgba == flat.rgba && explicitFlat.cornerUV == flat.cornerUV &&
                    explicitFlat.samples == flat.samples && explicitFlat.misses == flat.misses,
                "explicit equal-size chart layout reproduces legacy UVs and normal map exactly");
        auto variedSettings = settings;
        unsigned chartX = 0, chartY = 0, rowHeight = 0;
        for (std::size_t triangle = 0; triangle < chartCount; ++triangle)
        {
            const unsigned size = triangle == 0 ? 16 : 12;
            if (chartX + size > settings.resolution)
            {
                chartY += rowHeight;
                chartX = rowHeight = 0;
            }
            variedSettings.chartLayout.push_back({chartX, chartY, size});
            chartX += size;
            rowHeight = std::max(rowHeight, size);
        }
        auto varied = bakeNormals(high, low, variedSettings);
        require(varied.misses == 0 && varied.cornerUV != flat.cornerUV &&
                    varied.rgba == bakeNormals(high, low, variedSettings).rgba,
                "different chart sizes produce a deterministic projected atlas");
        auto invalidLayout = variedSettings;
        invalidLayout.chartLayout.pop_back();
        rejects([&] { bakeNormals(high, low, invalidLayout); }, "incomplete chart layout rejected");
        invalidLayout = variedSettings;
        invalidLayout.chartLayout[1] = invalidLayout.chartLayout[0];
        rejects([&] { bakeNormals(high, low, invalidLayout); }, "overlapping charts rejected");
        invalidLayout = variedSettings;
        invalidLayout.chartLayout[0].x = 120;
        rejects([&] { bakeNormals(high, low, invalidLayout); }, "out-of-bounds chart rejected");
        invalidLayout = variedSettings;
        invalidLayout.chartLayout[0].size = 2 * settings.padding + 3;
        rejects([&] { bakeNormals(high, low, invalidLayout); }, "undersized chart rejected");
        for (std::size_t i = 0; i < flat.rgba.size(); i += 4)
            if (flat.rgba[i + 3])
                require(flat.rgba[i] == 128 && flat.rgba[i + 1] == 128 && flat.rgba[i + 2] == 255,
                        "flat source encodes neutral tangent normal");
        settings.specialistMaps = true;
        settings.heightRange = 0.2;
        auto raised = high;
        for (auto &p : raised.mesh.positions)
            p.z += 0.1;
        auto raisedBake = bakeNormals(raised, low, settings);
        require(raisedBake.misses == 0 && raisedBake.heightRgba.size() == raisedBake.rgba.size(),
                "signed high-to-low height map covers the analytic raised plane");
        auto movedRaised = raised, movedLow = low;
        for (auto &p : movedRaised.mesh.positions)
            p = {p.x + 2, p.y + 3, p.z - 4};
        for (auto &p : movedLow.mesh.positions)
            p = {p.x + 2, p.y + 3, p.z - 4};
        auto movedBake = bakeNormals(movedRaised, movedLow, settings);
        require(movedBake.misses == raisedBake.misses && movedBake.rgba == raisedBake.rgba &&
                    movedBake.heightRgba == raisedBake.heightRgba &&
                    movedBake.ambientRgba == raisedBake.ambientRgba &&
                    movedBake.contactRgba == raisedBake.contactRgba &&
                    movedBake.curvatureRgba == raisedBake.curvatureRgba &&
                    movedBake.cavityRgba == raisedBake.cavityRgba &&
                    movedBake.edgeRgba == raisedBake.edgeRgba &&
                    movedBake.wireRgba == raisedBake.wireRgba &&
                    movedBake.thicknessRgba == raisedBake.thicknessRgba &&
                    movedBake.regionRgba == raisedBake.regionRgba,
                "normal and specialist high-to-low maps are invariant under shared translation");
        for (std::size_t i = 0; i < raisedBake.rgba.size(); i += 4)
            if (raisedBake.rgba[i + 3])
                require(raisedBake.heightRgba[i] == 191 && raisedBake.heightRgba[i + 3] == 255,
                        "positive high offset encodes above neutral gray");
        auto lowered = high;
        for (auto &p : lowered.mesh.positions)
            p.z -= 0.1;
        auto loweredBake = bakeNormals(lowered, low, settings);
        for (std::size_t i = 0; i < loweredBake.rgba.size(); i += 4)
            if (loweredBake.rgba[i + 3])
                require(loweredBake.heightRgba[i] == 64,
                        "negative high offset encodes below neutral gray");
        require(std::abs(raisedBake.minimumHeight - 0.1) < 1e-10 &&
                    std::abs(loweredBake.maximumHeight + 0.1) < 1e-10 &&
                    raisedBake.clippedHeightSamples == 0,
                "signed displacement statistics report physical units and no clipping");
        settings.heightRange = 0.05;
        auto clipped = bakeNormals(raised, low, settings);
        require(clipped.clippedHeightSamples == clipped.samples && clipped.misses == 0,
                "height saturation is counted instead of silently hidden");
        settings.heightRange = 0;
        rejects([&] { bakeNormals(high, low, settings); }, "zero height encoding range rejected");
        settings.heightRange = 0.2;
        settings.specialistMaps = false;
        for (auto &p : high.mesh.positions)
            p.z = 0.1 * p.x;
        normals(high);
        auto tilted = bakeNormals(high, low, settings);
        require(tilted.misses == 0 && tilted.rgba != flat.rgba,
                "high surface slope is transferred to normal texture");
        require(tilted.maximumDistance > 0.05 && tilted.maximumDistance < 0.11,
                "projection reports actual high-low distance");
        require(tilted.meanEncodedAngleDegrees < 0.8,
                "encoded texture reconstructs high normals with bounded quantization error");
        const unsigned tile=128/unsigned(std::ceil(std::sqrt(double(low.mesh.indices.size()/3))));
        for(std::size_t triangle=0;triangle<low.mesh.indices.size()/3;++triangle)
        {
            const unsigned columns=128/tile;
            auto x=unsigned(triangle%columns)*tile+3,y=unsigned(triangle/columns)*tile+3;
            auto pixel=(y*128+x)*4;
            auto t=tilted.cornerTangents[triangle*3],b=tilted.cornerBitangents[triangle*3],n=tilted.low.cornerNormals[triangle*3];
            double tx=tilted.rgba[pixel]*(2.0/255)-1,ty=tilted.rgba[pixel+1]*(2.0/255)-1,tz=tilted.rgba[pixel+2]*(2.0/255)-1;
            Vec3d decoded{t.x*tx+b.x*ty+n.x*tz,t.y*tx+b.y*ty+n.y*tz,t.z*tx+b.z*ty+n.z*tz};
            double length=std::hypot(decoded.x,decoded.y,decoded.z);
            require((-0.1*decoded.x+decoded.z)/(length*std::sqrt(1.01))>0.9999,
                    "exported tangent basis reconstructs the analytical high plane normal");
        }
        auto tiltedVaried = bakeNormals(high, low, variedSettings);
        for (std::size_t triangle = 0; triangle < chartCount; ++triangle)
        {
            const auto &chart = variedSettings.chartLayout[triangle];
            auto pixel = ((chart.y + settings.padding + 2) * settings.resolution +
                          chart.x + settings.padding + 2) * 4;
            auto t = tiltedVaried.cornerTangents[triangle * 3];
            auto b = tiltedVaried.cornerBitangents[triangle * 3];
            auto n = tiltedVaried.low.cornerNormals[triangle * 3];
            double tx = tiltedVaried.rgba[pixel] * (2.0 / 255) - 1;
            double ty = tiltedVaried.rgba[pixel + 1] * (2.0 / 255) - 1;
            double tz = tiltedVaried.rgba[pixel + 2] * (2.0 / 255) - 1;
            Vec3d decoded{t.x * tx + b.x * ty + n.x * tz,
                          t.y * tx + b.y * ty + n.y * tz,
                          t.z * tx + b.z * ty + n.z * tz};
            auto length = std::hypot(decoded.x, decoded.y, decoded.z);
            require((-0.1 * decoded.x + decoded.z) / (length * std::sqrt(1.01)) > 0.9999,
                    "nonuniform chart UVs retain the analytical high normal through tangent decoding");
        }
        require(tilted.rgba == bakeNormals(high, low, settings).rgba,
                "normal bake repeats deterministically");
        auto translatedHigh = high, translatedLow = low;
        for (auto &p : translatedHigh.mesh.positions)
            p = {p.x + 2, p.y + 3, p.z - 4};
        for (auto &p : translatedLow.mesh.positions)
            p = {p.x + 2, p.y + 3, p.z - 4};
        auto translatedBake = bakeNormals(translatedHigh, translatedLow, settings);
        require(translatedBake.misses == tilted.misses && translatedBake.rgba == tilted.rgba,
                "normal projection and tangent encoding are invariant under shared source translation");
        auto scaledHigh = high, scaledLow = low;
        for (auto &p : scaledHigh.mesh.positions)
            p = {2 * p.x, 2 * p.y, 2 * p.z};
        for (auto &p : scaledLow.mesh.positions)
            p = {2 * p.x, 2 * p.y, 2 * p.z};
        auto scaledSettings = settings;
        scaledSettings.projectionDistance *= 2;
        auto scaledBake = bakeNormals(scaledHigh, scaledLow, scaledSettings);
        require(scaledBake.misses == tilted.misses && scaledBake.rgba == tilted.rgba,
                "normal projection is scale covariant when the cage scales with the mesh");
        auto rotatedHigh = high, rotatedLow = low;
        for (auto &p : rotatedHigh.mesh.positions)
            p = {-p.y, p.x, p.z};
        for (auto &p : rotatedLow.mesh.positions)
            p = {-p.y, p.x, p.z};
        normals(rotatedHigh);
        auto rotatedBake = bakeNormals(rotatedHigh, rotatedLow, settings);
        require(rotatedBake.misses == tilted.misses && rotatedBake.rgba == tilted.rgba,
                "normal projection and tangent basis rotate with the source geometry");
        require(tilted.low.mesh.indices == low.mesh.indices &&
                    meshHash(tilted.low.mesh) == meshHash(low.mesh),
                "bake does not deform or subdivide low mesh");
        settings.projectionDistance = 0.005;
        require(bakeNormals(high, low, settings).misses > 0, "short projection cage reports misses");
        settings.projectionDistance = 0;
        rejects([&] { bakeNormals(high, low, settings); }, "zero projection cage rejected");
        settings.projectionDistance = 0.2;
        settings.resolution = 16;
        rejects([&] { bakeNormals(high, low, settings); }, "insufficient chart resolution rejected");
        settings.resolution = 128;
        Jerico::CancellationSource source;
        source.requestCancellation();
        auto token = source.token();
        rejects([&] { bakeNormals(high, low, settings, &token); }, "bake cancellation is visible");
        auto broken = high;
        broken.mesh.indices[0] = 999999;
        rejects([&] { bakeNormals(broken, low, settings); }, "malformed source rejected");
        auto reversed = high;
        for (std::size_t t = 0; t < reversed.mesh.indices.size(); t += 3)
            std::swap(reversed.mesh.indices[t + 1], reversed.mesh.indices[t + 2]);
        normals(reversed);
        auto backwards = bakeNormals(reversed, low, settings);
        require(backwards.misses == backwards.samples,
                "back-facing high surface must not become an opaque inverted normal map");
        broken = high;
        auto a = broken.mesh.indices[0], b = broken.mesh.indices[1], c = broken.mesh.indices[2];
        auto p = broken.mesh.positions[a], q = broken.mesh.positions[b];
        broken.mesh.positions[c] = {(p.x + q.x) * 0.5, (p.y + q.y) * 0.5, (p.z + q.z) * 0.5 + 1e-16};
        rejects([&] { bakeNormals(broken, low, settings); },
                "near-degenerate high face rejected with scale-relative criterion");
        auto sheets = low;
        for (auto &p : sheets.mesh.positions)
            p.z = 0.04;
        auto offset = std::uint32_t(sheets.mesh.positions.size());
        for (auto p : low.mesh.positions)
        {
            p.z = 0.12;
            sheets.mesh.positions.push_back(p);
        }
        for (auto i : low.mesh.indices)
            sheets.mesh.indices.push_back(i + offset);
        sheets.mesh.triangleRegions.insert(sheets.mesh.triangleRegions.end(),
                                           low.mesh.triangleRegions.begin(), low.mesh.triangleRegions.end());
        normals(sheets);
        auto nearest = bakeNormals(sheets, low, settings);
        require(nearest.misses == 0 && std::abs(nearest.maximumDistance - 0.04) < 1e-10,
                "projection chooses the nearest compatible front surface to the target, not first cage "
                "intersection");
        settings.projection = NormalProjection::ClosestSurface;
        auto inset = evaluate(recipe, 16);
        for (auto &p : inset.mesh.positions)
            p.x *= 0.9;
        normals(inset);
        require(bakeNormals(inset, low, settings).misses == 0,
                "explicit closest-surface mode covers bounded lateral offset that normal rays miss");
        auto close = bakeNormals(high, low, settings);
        require(close.misses == 0 && close.maximumDistance < 0.11,
                "explicit closest-surface bake transfers tilted high");
        auto reversedClosest = bakeNormals(reversed, low, settings);
        require(reversedClosest.misses == close.samples &&
                    reversedClosest.missesNearbyBackface == reversedClosest.misses,
                "closest-surface mode identifies reversed source faces as orientation misses");
        auto wrongRegionLow = low;
        for (auto &id : wrongRegionLow.mesh.triangleRegions)
            id = Solid::region(123, 987);
        auto regionMisses = bakeNormals(high, wrongRegionLow, settings);
        require(regionMisses.misses == regionMisses.samples &&
                    regionMisses.missesOtherRegionFront == regionMisses.misses,
                "nearby front surface in another region is reported, not silently projected");
        auto shortRange = settings;
        shortRange.projectionDistance = 0.01;
        auto rangeMisses = bakeNormals(raised, low, shortRange);
        require(rangeMisses.misses == rangeMisses.samples &&
                    rangeMisses.missesBeyondRadius == rangeMisses.misses,
                "source outside the cage is distinguished from region and orientation failures");
        auto competing = sheets;
        for (std::size_t t = 0; t < low.mesh.indices.size(); t += 3)
            std::swap(competing.mesh.indices[t + 1], competing.mesh.indices[t + 2]);
        normals(competing);
        auto compatible = bakeNormals(competing, low, settings);
        require(compatible.misses == 0 && std::abs(compatible.maximumDistance - 0.12) < 1e-10,
                "closest projection searches the nearest compatible face instead of stopping on a reversed "
                "candidate");
        Recipe cubeRecipe;
        auto cube = evaluate(cubeRecipe, 4);
        settings.resolution = 512;
        settings.projectionDistance = 0.2;
        settings.specialistMaps = true;
        settings.featureRadius = 4;
        settings.featureSamples = 16;
        settings.curvatureScale = 0.5;
        settings.edgeRadius = 0.5;
        settings.thicknessRange = 4;
        auto specialist = bakeNormals(cube, cube, settings);
        auto pixels = std::size_t(settings.resolution) * settings.resolution * 4;
        require(specialist.ambientRgba.size() == pixels && specialist.contactRgba.size() == pixels &&
                    specialist.curvatureRgba.size() == pixels && specialist.cavityRgba.size() == pixels &&
                    specialist.edgeRgba.size() == pixels && specialist.wireRgba.size() == pixels &&
                    specialist.thicknessRgba.size() == pixels && specialist.regionRgba.size() == pixels &&
                    specialist.heightRgba.size() == pixels,
                "specialist bake returns AO, curvature, thickness, height and semantic region textures");
        bool curved = false, thick = false, convexEdge = false;
        std::set<std::array<unsigned char, 3>> regions;
        for (std::size_t i = 0; i < pixels; i += 4)
            if (specialist.rgba[i + 3])
            {
                curved |= specialist.curvatureRgba[i] > 128;
                convexEdge |= specialist.edgeRgba[i] > 0;
                require(specialist.cavityRgba[i] == 0,
                        "convex cube does not invent a concave cavity mask");
                thick |= specialist.thicknessRgba[i] > 0;
                regions.insert({specialist.regionRgba[i], specialist.regionRgba[i + 1],
                                specialist.regionRgba[i + 2]});
                require(specialist.ambientRgba[i + 3] == 255 &&
                            specialist.contactRgba[i + 3] == 255 &&
                            specialist.curvatureRgba[i + 3] == 255 &&
                            specialist.cavityRgba[i + 3] == 255 &&
                            specialist.edgeRgba[i + 3] == 255 &&
                            specialist.wireRgba[i + 3] == 255 &&
                            specialist.thicknessRgba[i + 3] == 255 && specialist.regionRgba[i + 3] == 255 &&
                            specialist.heightRgba[i + 3] == 255,
                        "specialist maps use the same explicit valid coverage as normal map");
            }
        require(curved && thick && convexEdge && regions.size() == 6,
                "cube bake carries convexity, physical thickness and six semantic face regions");
        MeshResult concaveHinge;
        concaveHinge.mesh.positions = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        concaveHinge.mesh.indices = {0, 1, 2, 1, 0, 3};
        concaveHinge.mesh.triangleRegions.assign(2, cube.mesh.triangleRegions[0]);
        auto hingeSettings = settings;
        hingeSettings.resolution = 128;
        hingeSettings.cavityRadius = 0.5;
        hingeSettings.matchRegions = false;
        auto hingeBake = bakeNormals(concaveHinge, concaveHinge, hingeSettings);
        bool concaveCavity = false;
        for (std::size_t i = 0; i < hingeBake.rgba.size(); i += 4)
            if (hingeBake.rgba[i + 3])
            {
                concaveCavity |= hingeBake.cavityRgba[i] > 0;
                require(hingeBake.edgeRgba[i] == 0,
                        "concave hinge does not become convex edge wear");
            }
        require(concaveCavity, "concave high geometry projects into the cavity map");
        auto specialistRepeat = bakeNormals(cube, cube, settings);
        require(specialist.ambientRgba == specialistRepeat.ambientRgba &&
                    specialist.contactRgba == specialistRepeat.contactRgba &&
                    specialist.curvatureRgba == specialistRepeat.curvatureRgba &&
                    specialist.cavityRgba == specialistRepeat.cavityRgba &&
                    specialist.edgeRgba == specialistRepeat.edgeRgba &&
                    specialist.wireRgba == specialistRepeat.wireRgba &&
                    specialist.thicknessRgba == specialistRepeat.thicknessRgba &&
                    specialist.regionRgba == specialistRepeat.regionRgba &&
                    specialist.heightRgba == specialistRepeat.heightRgba,
                "specialist maps repeat deterministically");
        const auto exportDirectory = std::filesystem::current_path() / "normal-bake-export-guards";
        for (auto channel : {&NormalBake::ambientRgba, &NormalBake::contactRgba,
                             &NormalBake::curvatureRgba, &NormalBake::cavityRgba,
                             &NormalBake::edgeRgba, &NormalBake::wireRgba,
                             &NormalBake::thicknessRgba, &NormalBake::regionRgba,
                             &NormalBake::heightRgba})
        {
            auto invalidExport = specialist;
            (invalidExport.*channel).push_back(0);
            rejects([&] { exportNormalBake(invalidExport, exportDirectory); },
                    "export rejects specialist map dimensions inconsistent with resolution");
            (invalidExport.*channel).resize(1);
            rejects([&] { exportNormalBake(invalidExport, exportDirectory); },
                    "export rejects undersized specialist map before reading pixels");
            (invalidExport.*channel).clear();
            rejects([&] { exportNormalBake(invalidExport, exportDirectory); },
                    "export rejects missing map when specialist maps are enabled");
        }
        std::cout
            << "PASS high-to-low normal bake: coverage, tangent encoding, real high slope, projection "
               "distance, "
               "quantization, determinism, immutable low, misses, budgets, cancellation and invalid input\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
