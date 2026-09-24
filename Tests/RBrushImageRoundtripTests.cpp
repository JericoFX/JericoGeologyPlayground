#include "RBrushInterop.hpp"
#include <Jerico/AssetPipeline/RBrushContainer.hpp>
#include <Jerico/AssetPipeline/RBrushHash.hpp>
#include <Jerico/AssetPipeline/RBrushImageImporter.hpp>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace A = Jerico::Tools::AssetPipeline;
using Playground::makeRBrushLocalField;
using Playground::sampleRBrushLocalField;

void require(bool valid, const char *message)
{
    if (!valid)
        throw std::runtime_error(message);
}
std::string hex(const A::RBrushDigest &digest)
{
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (auto byte : digest)
        output << std::setw(2) << std::to_integer<unsigned>(byte);
    return output.str();
}
int main(int argc, char **argv)
try
{
    require(argc >= 2, "CC0 displacement input path required");
    std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
    require(bool(file), "CC0 source cannot be opened");
    auto size = file.tellg();
    require(size > 0 && size < 8 * 1024 * 1024, "CC0 source size outside fixture bounds");
    std::vector<std::byte> encoded(std::size_t(size), std::byte{0});
    file.seekg(0);
    file.read(reinterpret_cast<char *>(encoded.data()), size);
    require(bool(file), "CC0 source read failed");
    require(hex(A::hashRBrushBytes(encoded)) ==
                "2861630bab9f8a17014915ff624be83281185b04ff3f4b64a123db60d5e160d3",
            "CC0 cliff displacement provenance hash changed");
    Jerico::Guid assetGuid;
    assetGuid.bytes[0] = std::byte{0x71};
    A::RBrushImageProcessing processing;
    processing.canvasMode = A::RBrushImageCanvasMode::PreserveOriginal;
    processing.bitDepth = A::RBrushImageBitDepth::ForceR8;
    processing.padding = 0;
    auto start = std::chrono::steady_clock::now();
    auto imported = A::importRBrushImages(
        {.assetGuid = assetGuid,
         .sources = {{.logicalName = "cliff_side-disp-1k.png",
                      .encodedBytes = encoded,
                      .extractions = {{.channel = A::RBrushImageChannel::Luminance,
                                       .presetName = "CC0 cliff displacement fixture",
                                       .processing = processing}}}}});
    if (!imported.hasValue())
        throw std::runtime_error("Official RBrush image import failed: " + imported.error().message);
    auto importedAt = std::chrono::steady_clock::now();
    const auto &pack = imported.value();
    require(pack.tips.size() == 1 && pack.presets.size() == 1 &&
                pack.tips.front().pixelFormat == A::RBrushPixelFormat::R8Unorm &&
                pack.tips.front().width == 1024 && pack.tips.front().height == 1024,
            "official importer must create one canonical R8 cliff mask");
    auto encodedPack = A::writeRBrushContainer(pack);
    if (!encodedPack.hasValue())
        throw std::runtime_error("Official RBrush container write failed: " +
                                 encodedPack.error().message);
    auto decoded = A::readRBrushContainer(encodedPack.value());
    if (!decoded.hasValue())
        throw std::runtime_error("Official RBrush container read failed: " +
                                 decoded.error().message);
    auto tipOnly = A::readRBrushContainerTip(encodedPack.value(), pack.tips.front().tipGuid);
    require(tipOnly.hasValue() && decoded.value() == pack &&
                tipOnly.value() == pack.tips.front(),
            "official RBrush container and selective tip read preserve exact IR");
    auto packedAt = std::chrono::steady_clock::now();
    require(!decoded.value().presets.front().sdfDefaults.compatible,
            "imported RBrush keeps editable SDF defaults opt-in");
    auto field = makeRBrushLocalField(decoded.value().tips.front(),
                                      decoded.value().presets.front(), assetGuid,
                                      {}, 4, 4, 1, 0.25);
    require(field.sdf().insideTexels > 0 &&
                field.sdf().insideTexels < std::uint64_t(field.source().width) * field.source().height,
            "real-image-derived cliff mask needs interior and exterior texels");
    double total = 0;
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            total += sampleRBrushLocalField(
                field, {-2 + (x + 0.5) / 16.0, -2 + (y + 0.5) / 16.0, 0});
    require(total > 0 && total < 4096, "CC0 mask projects to a bounded, nontrivial local field");
    auto sampledAt = std::chrono::steady_clock::now();
    auto customPreset = decoded.value().presets.front();
    customPreset.sdfDefaults.compatible = true;
    customPreset.sdfDefaults.threshold = 0.8f;
    auto customSdf = Playground::deriveRBrushTipSdf(decoded.value().tips.front(), customPreset);
    auto geologyField = makeRBrushLocalField(decoded.value().tips.front(), customPreset,
                                             assetGuid, {}, 4, 4, 1, 0.25);
    require(customSdf.insideTexels != field.sdf().insideTexels &&
                geologyField.sdf() == field.sdf(),
            "Engine geology RBrush local field uses its fixed 0.5 threshold, not preset SDF defaults");
    auto thresholdAt = std::chrono::steady_clock::now();
    double previewMilliseconds = 0;
    if (argc > 2)
    {
        auto previewStart = std::chrono::steady_clock::now();
        std::filesystem::create_directories(argv[2]);
        auto output = std::filesystem::path(argv[2]) / "cliff-side-cc0.rbrush";
        std::ofstream out(output, std::ios::binary);
        out.write(reinterpret_cast<const char *>(encodedPack.value().data()),
                  std::streamsize(encodedPack.value().size()));
        require(bool(out), "RBrush fixture output failed");
        constexpr std::uint32_t width = 512, height = 512, stride = width * 3;
        std::vector<std::uint8_t> bitmap(54 + stride * height);
        auto write16 = [&](std::size_t at, std::uint16_t value)
        {
            bitmap[at] = std::uint8_t(value);
            bitmap[at + 1] = std::uint8_t(value >> 8);
        };
        auto write32 = [&](std::size_t at, std::uint32_t value)
        {
            write16(at, std::uint16_t(value));
            write16(at + 2, std::uint16_t(value >> 16));
        };
        bitmap[0] = 'B';
        bitmap[1] = 'M';
        write32(2, std::uint32_t(bitmap.size()));
        write32(10, 54);
        write32(14, 40);
        write32(18, width);
        write32(22, height);
        write16(26, 1);
        write16(28, 24);
        write32(34, stride * height);
        for (bool inverted : {false, true})
        {
            for (std::uint32_t y = 0; y < height; ++y)
                for (std::uint32_t x = 0; x < width; ++x)
                {
                    auto weight = sampleRBrushLocalField(
                        field, {-2 + (x + 0.5) / 128.0, -2 + (y + 0.5) / 128.0, 0},
                        inverted);
                    auto pixel = std::uint8_t(std::lround(std::clamp(weight, 0.0, 1.0) * 255));
                    auto at = 54 + y * stride + x * 3;
                    bitmap[at] = bitmap[at + 1] = bitmap[at + 2] = pixel;
                }
            auto filename = inverted ? "local-field-weight-inverted.bmp" :
                                       "local-field-weight.bmp";
            std::ofstream preview(std::filesystem::path(argv[2]) / filename,
                                  std::ios::binary);
            preview.write(reinterpret_cast<const char *>(bitmap.data()),
                          std::streamsize(bitmap.size()));
            require(bool(preview), "RBrush local-field preview output failed");
        }
        previewMilliseconds = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - previewStart).count();
    }
    std::cout << "PASS CC0 PNG -> canonical RBrush pack -> container -> local field; "
              << "tip=" << field.source().width << 'x' << field.source().height
              << " inside=" << field.sdf().insideTexels
              << " presetThreshold08Inside=" << customSdf.insideTexels
              << " tipHash=" << hex(field.source().canonicalTipHash)
              << " packBytes=" << encodedPack.value().size()
              << " importMs=" << std::chrono::duration<double, std::milli>(importedAt - start).count()
              << " roundtripMs=" << std::chrono::duration<double, std::milli>(packedAt - importedAt).count()
              << " fieldMs=" << std::chrono::duration<double, std::milli>(sampledAt - packedAt).count()
              << " thresholdGateMs=" <<
                     std::chrono::duration<double, std::milli>(thresholdAt - sampledAt).count()
              << " preview524288SamplesMs=" << previewMilliseconds
              << '\n';
    return 0;
}
catch (const std::exception &error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
