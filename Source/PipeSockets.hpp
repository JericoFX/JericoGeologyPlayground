#pragma once
#include "SplineConstruction.hpp"
#include <memory>

namespace Playground::Pipes
{
enum class Role { Connection, StructuralAnchor, Effect };
struct Socket
{
    std::string name{"Socket"};
    Vec3d position{}, rotation{}; // Local XYZ Euler degrees, composed Z * Y * X.
    double diameter{.2};
    Role role{Role::Connection};
    std::vector<std::string> capabilities{"Pipe"};
};
struct Module { std::string moduleType{"Unclassified"}; std::vector<Socket> sockets; };
using Quaternion=std::array<double,4>; // x, y, z, w
struct Pose { Vec3d position{}; Quaternion rotation{0,0,0,1}; };
struct Frame { Vec3d position, forward, up; };
struct TextureImage
{
    std::string name;
    unsigned width{},height{},bitsPerChannel{8};
    std::vector<std::uint16_t> rgba16;
    std::vector<std::uint8_t> encoded;
    std::vector<std::uint8_t> rgba; // Procedural images already decoded.
    int wrapS{10497},wrapT{10497},minFilter{9987},magFilter{9729};
};
struct PipeMaterial
{
    std::string name;
    std::shared_ptr<const TextureImage> baseColor,normal,metallicRoughness,occlusion,emissive;
    std::array<float,4> baseColorFactor{1,1,1,1};
    std::array<float,3> emissiveFactor{};
    float metallicFactor{1},roughnessFactor{1},normalScale{1},occlusionStrength{1},alphaCutoff{.5f};
    bool doubleSided{},alphaMask{},unified{},alphaBlend{};
    std::array<unsigned,5> textureUVSets{}; // Base color, normal, metallic/roughness, occlusion, emissive.
    std::shared_ptr<const TextureImage> transmission,clearcoat,specularWeight;
    float transmissionFactor{},ior{1.5f},clearcoatFactor{},clearcoatRoughness{.03f},specularFactor{1},thickness{};
};
struct MaterialRange
{
    std::uint32_t firstIndex{},indexCount{};
    std::shared_ptr<const PipeMaterial> material;
};
struct ImportedPart {std::string localId,nodeId;std::uint32_t firstIndex{},indexCount{};};
struct ImportedMesh
{
    MeshResult mesh;
    std::vector<std::array<float,2>> cornerUV;
    std::vector<MaterialRange> materials;
    std::vector<std::array<float,2>> cornerUV1;
    std::vector<std::array<float,4>> cornerTangents;
    std::vector<ImportedPart> parts;
};
std::vector<std::uint8_t> decodeTexture(const TextureImage&);
std::vector<std::uint16_t> decodeTexture16(const TextureImage&);
std::shared_ptr<const PipeMaterial> straightMaterial();
enum class ConnectionState { Compatible, NeedsCorrection, Incompatible };
struct ConnectionCheck
{
    ConnectionState state{ConnectionState::Incompatible};
    std::string reason;
    double positionError{}, angleErrorDegrees{}, diameterError{};
};
struct StraightSettings
{
    Vec3d start{0,0,0},end{0,0,2};
    double diameter{.2},wallThickness{.02},repeatMeters{1};
};
Quaternion quaternion(Vec3d degrees);
Vec3d eulerAngles(Quaternion);
Quaternion rotateWorld(Quaternion,int axis,double degrees);
Vec3d faceDirection(Vec3d forward);
Vec3d rotate(Quaternion rotation,Vec3d point);
Vec3d transform(const Pose&,Vec3d point);
Frame worldFrame(const Socket&,const Pose&);
void validate(const Socket&);
std::vector<std::filesystem::path> discover(const std::filesystem::path& directory);
ImportedMesh loadGlb(const std::filesystem::path&);
// Direct export of the renderer's Y-up metre mesh; no regeneration. Legacy PBR.
void writeGlb(const std::filesystem::path&,const ImportedMesh&);
Module loadModule(const std::filesystem::path& asset);
void saveModule(const std::filesystem::path& asset,const Module&);
Pose snap(const Socket& fixed,const Pose& fixedPose,const Socket& moving);
ConnectionCheck connection(const Socket&,const Pose&,const Socket&,const Pose&,
                           double positionTolerance=.01,double angleToleranceDegrees=5,
                           double diameterTolerance=.001);
Construction::ConstructionResult straight(const StraightSettings&);
// Same ring profile as straight(), swept along a routed polyline whose ends follow the
// socket axes. The module carries a socket at each end.
struct RoutedResult {
    Construction::ConstructionResult mesh;
    Module module;
};
RoutedResult routed(const std::vector<Vec3d>& path,const StraightSettings& settings);
Module straightModule(const StraightSettings&);
void append(ImportedMesh& destination,const ImportedMesh& source,const Pose&);
}
