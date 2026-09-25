#include "Renderer.hpp"
#include "NormalBake.hpp"
#include <Buffer.h>
#include <DeviceContext.h>
#include <EngineFactoryVk.h>
#include <ImGuiImplDiligent.hpp>
#include <Jerico/ProceduralAuthoring/Geology/GeologyNoise.hpp>
#include <MapHelper.hpp>
#include <PipelineState.h>
#include <Query.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <ShaderResourceBinding.h>
#include <SwapChain.h>
#include <Texture.h>
#include <Windows.h>
#include <algorithm>
#include <backends/imgui_impl_win32.h>
#include <chrono>
#include <cmath>
#include <imgui.h>
#include <stb_image_write.h>
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdexcept>

namespace Playground
{
using namespace Diligent;
namespace
{
struct Vertex
{
    float p[3], n[3], field, uv[2], tangent[3], bitangent[3];
};
struct Constants
{
    float camera[4];
    float target[4];
    float range[4];
};
const char *shader = R"(
cbuffer Constants {float4 Camera;float4 Target;float4 Range;};
Texture2D NormalMap; SamplerState NormalMap_sampler;
Texture2D ColorMap; SamplerState ColorMap_sampler;
Texture2D AmbientMap; SamplerState AmbientMap_sampler;
Texture2D ContactMap; SamplerState ContactMap_sampler;
Texture2D CavityMap; SamplerState CavityMap_sampler;
Texture2D EdgeMap; SamplerState EdgeMap_sampler;
struct VIn {float3 Position:ATTRIB0;float3 Normal:ATTRIB1;float Field:ATTRIB2;float2 UV:ATTRIB3;float3 Tangent:ATTRIB4;float3 Bitangent:ATTRIB5;};
struct VOut {float4 Position:SV_POSITION;float3 Normal:NORMAL;float3 World:TEXCOORD0;float Field:TEXCOORD1;float2 UV:TEXCOORD2;float3 Tangent:TEXCOORD3;float3 Bitangent:TEXCOORD4;};
VOut VSMain(VIn input) {
 VOut o;float cy=cos(Camera.x),sy=sin(Camera.x),cp=cos(Camera.y),sp=sin(Camera.y);
 float3 right=float3(cy,0,-sy),forward=float3(sy*cp,sp,cy*cp),up=cross(forward,right);
 float3 q=input.Position-Target.xyz;float depth=dot(q,forward)+Camera.z;float scale=2.4;
 o.Position=float4(dot(q,right)*scale/Camera.w,dot(q,up)*scale,(depth-0.05)*100.0/99.95,depth);
 o.Normal=input.Normal;o.World=input.Position;o.Field=input.Field;o.UV=input.UV;o.Tangent=input.Tangent;o.Bitangent=input.Bitangent;return o;
}
float4 PSMain(VOut input):SV_TARGET {
 if(Target.w>3.5 && Target.w<4.5){float v=NormalMap.Sample(NormalMap_sampler,float2(input.UV.x,1-input.UV.y)).r;return float4(v,v,v,1);}
 float3 n=normalize(input.Normal);
 if(Range.z>0.5) {
  float3 mapped=NormalMap.Sample(NormalMap_sampler,float2(input.UV.x,1-input.UV.y)).xyz*2-1;
  float3 t=normalize(input.Tangent-n*dot(input.Tangent,n));
  float3 b=cross(n,t)*sign(dot(cross(n,t),input.Bitangent));
  n=normalize(t*mapped.x+b*mapped.y+n*mapped.z);
 }
 if(Target.w>0.5 && Target.w<1.5)return float4(n*.5+.5,1);
 if(Target.w>4.5 && Target.w<5.5){
  float2 uv=float2(input.UV.x,1-input.UV.y);
  float ao=AmbientMap.Sample(AmbientMap_sampler,uv).r;
  float contact=ContactMap.Sample(ContactMap_sampler,uv).r;
  float cavity=CavityMap.Sample(CavityMap_sampler,uv).r;
  float edge=EdgeMap.Sample(EdgeMap_sampler,uv).r;
  float3 weight=pow(abs(normalize(input.Normal)),4);
  weight/=max(weight.x+weight.y+weight.z,0.00001);
  float3 albedo=ColorMap.Sample(ColorMap_sampler,input.World.yz*.8).rgb*weight.x+
                ColorMap.Sample(ColorMap_sampler,input.World.xz*.8).rgb*weight.y+
                ColorMap.Sample(ColorMap_sampler,input.World.xy*.8).rgb*weight.z;
  albedo*=1-.18*max(contact*.6,cavity*.4);
  albedo=lerp(albedo,float3(.62,.58,.52),edge*.12);
  float3 light=normalize(float3(-.5,.85,-.7));
  float cy=cos(Camera.x),sy=sin(Camera.x),cp=cos(Camera.y),sp=sin(Camera.y);
  float3 forward=float3(sy*cp,sp,cy*cp);
  float3 viewDirection=normalize(Target.xyz-forward*Camera.z-input.World);
  float3 halfVector=normalize(light+viewDirection);
  float nl=saturate(dot(n,light)),nv=saturate(dot(n,viewDirection));
  float nh=saturate(dot(n,halfVector)),vh=saturate(dot(viewDirection,halfVector));
  float roughness=lerp(.82,.68,edge*.35);
  float a=roughness*roughness,a2=a*a;
  float d=a2/(3.14159265*pow(nh*nh*(a2-1)+1,2)+.00001);
  float k=pow(roughness+1,2)/8;
  float g=nv/(nv*(1-k)+k+.00001)*nl/(nl*(1-k)+k+.00001);
  float3 fresnel=.04+(1-.04)*pow(1-vh,5);
  float3 specular=d*g*fresnel/(4*nv*nl+.00001);
  float3 direct=(albedo*(1-fresnel)/3.14159265+specular)*2.8*nl;
  float3 ambient=albedo*.6*(1-.85*ao);
  return float4(pow(saturate(direct+ambient),1.0/2.2),1);
 }
 if(Target.w>2.5){float t=saturate((input.Field-Range.x)/max(0.00001,Range.y-Range.x));
  float3 color=lerp(float3(.07,.17,.38),float3(.1,.8,.6),saturate(t*2));color=lerp(color,float3(1,.55,.12),saturate((t-.5)*2));return float4(color,1);}
 float key=max(0,dot(n,normalize(float3(-.5,.85,-.7))));float rim=max(0,dot(n,normalize(float3(.8,.3,.6))));
 float3 color=float3(.48,.43,.36)*(0.22+key*.95)+float3(.1,.14,.2)*rim;
 return float4(pow(color,1.0/2.2),1);
})";
const char *computeShader = R"(
cbuffer Params {uint Count;uint Seed;uint Octaves;float Frequency;float Lacunarity;float Gain;float2 Pad;float4 Scale;};
StructuredBuffer<float4> Positions;RWStructuredBuffer<float> Values;
uint hash(uint v){v^=v>>16;v*=0x7feb352d;v^=v>>15;v*=0x846ca68b;return v^(v>>16);}
float lattice(int3 p,uint seed){uint v=seed^asuint(p.x)*0x8da6b343;v^=asuint(p.y)*0xd8163841;v^=asuint(p.z)*0xcb1ab31f;return (hash(v)&0x00ffffff)*(2.0/16777215.0)-1;}
float noise(float3 p,uint seed){int3 b=int3(floor(p));float3 w=p-floor(p);w=w*w*(3-2*w);
 float a=lerp(lattice(b,seed),lattice(b+int3(1,0,0),seed),w.x);
 float c=lerp(lattice(b+int3(0,1,0),seed),lattice(b+int3(1,1,0),seed),w.x);
 float d=lerp(lattice(b+int3(0,0,1),seed),lattice(b+int3(1,0,1),seed),w.x);
 float e=lerp(lattice(b+int3(0,1,1),seed),lattice(b+int3(1,1,1),seed),w.x);return lerp(lerp(a,c,w.y),lerp(d,e,w.y),w.z);}
[numthreads(64,1,1)] void CSMain(uint3 tid:SV_DispatchThreadID){if(tid.x>=Count)return;
 float3 p=Positions[tid.x].xyz*Frequency*Scale.xyz;float sum=0,weight=1,weights=0;
 for(uint o=0;o<Octaves;++o){sum+=noise(p,Seed+o*0x9e3779b9)*weight;weights+=weight;p*=Lacunarity;weight*=Gain;}
 Values[tid.x]=sum/weights;
})";
void require(bool ok, const char *message)
{
    if (!ok)
        throw std::runtime_error(message);
}
std::vector<std::vector<std::uint8_t>> colorMips(const std::vector<std::uint8_t> &base,
                                                  unsigned width, unsigned height)
{
    std::vector<std::vector<std::uint8_t>> chain;
    while (width > 1 || height > 1)
    {
        const auto &previous = chain.empty() ? base : chain.back();
        const auto nextWidth = std::max(1u, width / 2), nextHeight = std::max(1u, height / 2);
        std::vector<std::uint8_t> next(std::size_t(nextWidth) * nextHeight * 4);
        for (unsigned y = 0; y < nextHeight; ++y)
            for (unsigned x = 0; x < nextWidth; ++x)
            {
                double linear[3]{};
                for (unsigned dy = 0; dy < 2; ++dy)
                    for (unsigned dx = 0; dx < 2; ++dx)
                    {
                        const auto source = 4 * (std::size_t(std::min(2 * y + dy, height - 1)) * width +
                                                 std::min(2 * x + dx, width - 1));
                        for (int channel = 0; channel < 3; ++channel)
                        {
                            const double encoded = previous[source + channel] / 255.0;
                            linear[channel] += encoded <= 0.04045 ? encoded / 12.92 :
                                std::pow((encoded + 0.055) / 1.055, 2.4);
                        }
                    }
                const auto target = 4 * (std::size_t(y) * nextWidth + x);
                for (int channel = 0; channel < 3; ++channel)
                {
                    const double value = linear[channel] * 0.25;
                    const double encoded = value <= 0.0031308 ? 12.92 * value :
                        1.055 * std::pow(value, 1.0 / 2.4) - 0.055;
                    next[target + channel] = std::uint8_t(std::lround(std::clamp(encoded, 0.0, 1.0) * 255));
                }
                next[target + 3] = 255;
            }
        chain.push_back(std::move(next));
        width = nextWidth;
        height = nextHeight;
    }
    return chain;
}
} // namespace
struct Renderer::Impl
{
    RefCntAutoPtr<IEngineFactoryVk> factory;
    RefCntAutoPtr<IRenderDevice> device;
    RefCntAutoPtr<IDeviceContext> context;
    RefCntAutoPtr<ISwapChain> swap;
    std::unique_ptr<ImGuiImplDiligent> ui;
    RefCntAutoPtr<IPipelineState> solid, wire, compute;
    RefCntAutoPtr<IShaderResourceBinding> binding;
    RefCntAutoPtr<IBuffer> constants, vertices, indices, parameters;
    RefCntAutoPtr<ITexture> color, depth, normalMap;
    std::array<RefCntAutoPtr<ITexture>, 5> materialMaps;
    bool baked{}, material{};
    unsigned indexCount{}, viewWidth{}, viewHeight{};
    std::size_t bytes{};
    float low{}, high{1};
    RefCntAutoPtr<IShader> makeShader(const char *source, const char *entry, SHADER_TYPE type)
    {
        ShaderCreateInfo ci;
        ci.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ci.ShaderCompiler = SHADER_COMPILER_DXC;
        ci.Desc.ShaderType = type;
        ci.Desc.Name = entry;
        ci.EntryPoint = entry;
        ci.Source = source;
        ci.Desc.UseCombinedTextureSamplers = true;
        RefCntAutoPtr<IShader> result;
        device->CreateShader(ci, &result);
        require(bool(result), "Diligent shader creation failed; inspect console");
        return result;
    }
};
Renderer::Renderer(void *window, unsigned width, unsigned height) : impl(std::make_unique<Impl>())
{
    auto &r = *impl;
    r.factory = LoadAndGetEngineFactoryVk();
    require(bool(r.factory), "Diligent Vulkan DLL unavailable");
    EngineVkCreateInfo ci;
    ci.EnableValidation = false;
    ci.Features.WireframeFill = DEVICE_FEATURE_STATE_OPTIONAL;
    ci.Features.ComputeShaders = DEVICE_FEATURE_STATE_ENABLED;
    ci.Features.DurationQueries = DEVICE_FEATURE_STATE_OPTIONAL;
    r.factory->CreateDeviceAndContextsVk(ci, &r.device, &r.context);
    require(bool(r.device) && bool(r.context), "Could not create Vulkan device");
    SwapChainDesc sd;
    sd.Width = width;
    sd.Height = height;
    sd.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM;
    sd.DepthBufferFormat = TEX_FORMAT_D32_FLOAT;
    Win32NativeWindow nw{static_cast<HWND>(window)};
    r.factory->CreateSwapChainVk(r.device, r.context, sd, nw, &r.swap);
    require(bool(r.swap), "Could not create swap chain");
    r.ui = std::make_unique<ImGuiImplDiligent>(ImGuiDiligentCreateInfo{r.device, r.swap->GetDesc()});
    require(ImGui_ImplWin32_Init(window), "ImGui Win32 initialization failed");
    BufferDesc bd;
    bd.Name = "Playground camera";
    bd.Size = sizeof(Constants);
    bd.Usage = USAGE_DYNAMIC;
    bd.BindFlags = BIND_UNIFORM_BUFFER;
    bd.CPUAccessFlags = CPU_ACCESS_WRITE;
    r.device->CreateBuffer(bd, nullptr, &r.constants);
    require(bool(r.constants), "Camera buffer allocation failed");
    auto vs = r.makeShader(shader, "VSMain", SHADER_TYPE_VERTEX),
         ps = r.makeShader(shader, "PSMain", SHADER_TYPE_PIXEL);
    GraphicsPipelineStateCreateInfo p;
    p.PSODesc.Name = "Playground mesh";
    p.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
    p.pVS = vs;
    p.pPS = ps;
    p.GraphicsPipeline.NumRenderTargets = 1;
    p.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM;
    p.GraphicsPipeline.DSVFormat = TEX_FORMAT_D32_FLOAT;
    p.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    p.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    p.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
    LayoutElement layout[] = {{0, 0, 3, VT_FLOAT32, false}, {1, 0, 3, VT_FLOAT32, false},
                              {2, 0, 1, VT_FLOAT32, false}, {3, 0, 2, VT_FLOAT32, false},
                              {4, 0, 3, VT_FLOAT32, false}, {5, 0, 3, VT_FLOAT32, false}};
    p.GraphicsPipeline.InputLayout.LayoutElements = layout;
    p.GraphicsPipeline.InputLayout.NumElements = 6;
    ShaderResourceVariableDesc variables[] = {
        {SHADER_TYPE_PIXEL, "NormalMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "ColorMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "AmbientMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "ContactMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "CavityMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "EdgeMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}};
    p.PSODesc.ResourceLayout.Variables = variables;
    p.PSODesc.ResourceLayout.NumVariables = std::size(variables);
    SamplerDesc sampler;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = TEXTURE_ADDRESS_CLAMP;
    SamplerDesc colorSampler = sampler;
    colorSampler.AddressU = colorSampler.AddressV = colorSampler.AddressW = TEXTURE_ADDRESS_WRAP;
    ImmutableSamplerDesc samplers[] = {
        {SHADER_TYPE_PIXEL, "NormalMap", sampler},
        {SHADER_TYPE_PIXEL, "ColorMap", colorSampler},
        {SHADER_TYPE_PIXEL, "AmbientMap", sampler},
        {SHADER_TYPE_PIXEL, "ContactMap", sampler},
        {SHADER_TYPE_PIXEL, "CavityMap", sampler},
        {SHADER_TYPE_PIXEL, "EdgeMap", sampler}};
    p.PSODesc.ResourceLayout.ImmutableSamplers = samplers;
    p.PSODesc.ResourceLayout.NumImmutableSamplers = std::size(samplers);
    r.device->CreateGraphicsPipelineState(p, &r.solid);
    require(bool(r.solid), "Graphics pipeline failed");
    r.solid->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(r.constants);
    r.solid->GetStaticVariableByName(SHADER_TYPE_PIXEL, "Constants")->Set(r.constants);
    r.solid->CreateShaderResourceBinding(&r.binding, true);
    const char *names[] = {"ColorMap", "AmbientMap", "ContactMap", "CavityMap", "EdgeMap"};
    for (std::size_t i = 0; i < std::size(names); ++i)
    {
        const std::uint8_t neutral[4] = {i == 0 ? std::uint8_t(180) : std::uint8_t(0),
                                         i == 0 ? std::uint8_t(170) : std::uint8_t(0),
                                         i == 0 ? std::uint8_t(150) : std::uint8_t(0), 255};
        TextureDesc d;
        d.Name = names[i];
        d.Type = RESOURCE_DIM_TEX_2D;
        d.Width = d.Height = 1;
        d.MipLevels = 1;
        d.Format = i == 0 ? TEX_FORMAT_RGBA8_UNORM_SRGB : TEX_FORMAT_RGBA8_UNORM;
        d.Usage = USAGE_IMMUTABLE;
        d.BindFlags = BIND_SHADER_RESOURCE;
        TextureSubResData subresource{neutral, 4};
        TextureData initial{&subresource, 1};
        r.device->CreateTexture(d, &initial, &r.materialMaps[i]);
        require(bool(r.materialMaps[i]), "Material fallback texture allocation failed");
        r.binding->GetVariableByName(SHADER_TYPE_PIXEL, names[i])
            ->Set(r.materialMaps[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    }
    if (r.device->GetDeviceInfo().Features.WireframeFill == DEVICE_FEATURE_STATE_ENABLED)
    {
        p.GraphicsPipeline.RasterizerDesc.FillMode = FILL_MODE_WIREFRAME;
        r.device->CreateGraphicsPipelineState(p, &r.wire);
    }
    if (r.wire)
    {
        r.wire->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(r.constants);
        r.wire->GetStaticVariableByName(SHADER_TYPE_PIXEL, "Constants")->Set(r.constants);
    }
}
Renderer::~Renderer()
{
    if (impl)
    {
        impl->context->WaitForIdle();
        ImGui_ImplWin32_Shutdown();
        impl->ui.reset();
    }
}
void Renderer::begin(unsigned width, unsigned height)
{
    auto &r = *impl;
    if (r.swap->GetDesc().Width != width || r.swap->GetDesc().Height != height)
        r.swap->Resize(width, height);
    ImGui_ImplWin32_NewFrame();
    r.ui->NewFrame(width, height, r.swap->GetDesc().PreTransform);
}
void Renderer::upload(const MeshResult &mesh, const NormalBake *bake,
                      const std::vector<std::uint8_t> *diagnosticMap,
                      const std::vector<std::vector<std::uint8_t>> *diagnosticMips)
{
    auto &r = *impl;
    std::vector<Vertex> vertices;
    auto indices = mesh.mesh.indices;
    bool corners = mesh.cornerNormals.size() == mesh.mesh.indices.size();
    if (bake)
        require(corners && bake->cornerUV.size() == mesh.mesh.indices.size() &&
                    bake->cornerTangents.size() == mesh.mesh.indices.size() &&
                    bake->cornerBitangents.size() == mesh.mesh.indices.size() &&
                    bake->rgba.size() == std::size_t(bake->resolution) * bake->resolution * 4 &&
                    (!diagnosticMap || diagnosticMap->size() == bake->rgba.size()),
                "Normal bake renderer input mismatch");
    require(!diagnosticMap || bake, "Diagnostic map requires the baked low UVs");
    require(!diagnosticMips || diagnosticMap, "Diagnostic mips require a diagnostic map");
    auto count = corners ? indices.size() : mesh.mesh.positions.size();
    vertices.reserve(count);
    r.low = 1e20f;
    r.high = -1e20f;
    for (std::size_t i = 0; i < count; ++i)
    {
        auto source = corners ? mesh.mesh.indices[i] : i;
        auto p = mesh.mesh.positions[source], n = corners ? mesh.cornerNormals[i] : mesh.normals[source];
        float f = mesh.field[source];
        if (corners)
            indices[i] = std::uint32_t(i);
        vertices.push_back({{float(p.x), float(p.y), float(p.z)}, {float(n.x), float(n.y), float(n.z)}, f});
        if (bake)
        {
            auto &v = vertices.back();
            auto uv = bake->cornerUV[i];
            auto t = bake->cornerTangents[i], b = bake->cornerBitangents[i];
            v.uv[0] = uv[0];
            v.uv[1] = uv[1];
            v.tangent[0] = float(t.x);
            v.tangent[1] = float(t.y);
            v.tangent[2] = float(t.z);
            v.bitangent[0] = float(b.x);
            v.bitangent[1] = float(b.y);
            v.bitangent[2] = float(b.z);
        }
        r.low = std::min(r.low, f);
        r.high = std::max(r.high, f);
    }
    r.vertices.Release();
    r.indices.Release();
    BufferDesc d;
    d.Name = "Playground mesh vertices";
    d.Size = vertices.size() * sizeof(Vertex);
    d.Usage = USAGE_IMMUTABLE;
    d.BindFlags = BIND_VERTEX_BUFFER;
    BufferData data{vertices.data(), d.Size};
    r.device->CreateBuffer(d, &data, &r.vertices);
    d.Name = "Playground mesh indices";
    d.Size = mesh.mesh.indices.size() * sizeof(std::uint32_t);
    d.BindFlags = BIND_INDEX_BUFFER;
    data = {indices.data(), d.Size};
    r.device->CreateBuffer(d, &data, &r.indices);
    require(bool(r.vertices) && bool(r.indices), "Mesh buffer allocation failed");
    r.indexCount = unsigned(mesh.mesh.indices.size());
    r.bytes = vertices.size() * sizeof(Vertex) + d.Size;
    const std::uint8_t neutral[4] = {128, 128, 255, 255};
    TextureDesc texture;
    texture.Name = "Playground baked texture";
    texture.Type = RESOURCE_DIM_TEX_2D;
    texture.Width = texture.Height = bake ? bake->resolution : 1;
    const auto *mips = diagnosticMap ? diagnosticMips : bake ? &bake->normalMips : nullptr;
    std::size_t maxMipCount = 0;
    for (auto dimension = texture.Width; dimension > 1; dimension >>= 1)
        ++maxMipCount;
    require(!mips || mips->size() <= maxMipCount, "Baked texture mip count exceeds its dimensions");
    texture.MipLevels = mips ? Uint32(mips->size() + 1) : 1;
    texture.Format = TEX_FORMAT_RGBA8_UNORM;
    texture.Usage = USAGE_IMMUTABLE;
    texture.BindFlags = BIND_SHADER_RESOURCE;
    std::vector<TextureSubResData> subresources(texture.MipLevels);
    subresources[0].pData = diagnosticMap ? diagnosticMap->data() : bake ? bake->rgba.data() : neutral;
    subresources[0].Stride = texture.Width * 4;
    for (Uint32 level = 1; level < texture.MipLevels; ++level)
    {
        auto size = texture.Width >> level;
        require((*mips)[level - 1].size() == std::size_t(size) * size * 4,
                "Baked texture mip dimensions mismatch");
        subresources[level].pData = (*mips)[level - 1].data();
        subresources[level].Stride = size * 4;
    }
    TextureData initial;
    initial.pSubResources = subresources.data();
    initial.NumSubresources = texture.MipLevels;
    r.normalMap.Release();
    r.device->CreateTexture(texture, &initial, &r.normalMap);
    require(bool(r.normalMap), "Normal texture allocation failed");
    r.binding->GetVariableByName(SHADER_TYPE_PIXEL, "NormalMap")
        ->Set(r.normalMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    r.baked = bake != nullptr && diagnosticMap == nullptr;
    r.material = false;
    for (Uint32 level = 0; level < texture.MipLevels; ++level)
        r.bytes += std::size_t(texture.Width >> level) * (texture.Height >> level) * 4;
}
void Renderer::uploadMaterial(const NormalBake *bake, const std::filesystem::path &diffusePath,
                              bool useDiffuseMips)
{
    auto &r = *impl;
    require(!bake || (r.baked && bake->settings.specialistMaps && bake->settings.automaticUv),
            "Material preview requires an uploaded automatic-UV specialist bake");
    int width = 0, height = 0, channels = 0;
    auto *decoded = stbi_load(diffusePath.string().c_str(), &width, &height, &channels, 4);
    if (!decoded || width <= 0 || height <= 0 || width > 4096 || height > 4096)
    {
        stbi_image_free(decoded);
        throw std::runtime_error("Material preview diffuse texture could not be decoded");
    }
    std::vector<std::uint8_t> color(decoded, decoded + std::size_t(width) * height * 4);
    stbi_image_free(decoded);
    auto diffuseMips = useDiffuseMips ? colorMips(color, unsigned(width), unsigned(height)) :
        std::vector<std::vector<std::uint8_t>>{};
    const std::vector<std::uint8_t> neutral{0, 0, 0, 255};
    const std::vector<std::uint8_t> *maps[] = {
        &color, bake ? &bake->ambientRgba : &neutral, bake ? &bake->contactRgba : &neutral,
        bake ? &bake->cavityRgba : &neutral, bake ? &bake->edgeRgba : &neutral};
    const std::vector<std::vector<std::uint8_t>> *mips[] = {
        &diffuseMips, bake ? &bake->specialistMips.ambient : nullptr,
        bake ? &bake->specialistMips.contact : nullptr,
        bake ? &bake->specialistMips.cavity : nullptr,
        bake ? &bake->specialistMips.edge : nullptr};
    const char *names[] = {"ColorMap", "AmbientMap", "ContactMap", "CavityMap", "EdgeMap"};
    for (std::size_t i = 0; i < std::size(maps); ++i)
    {
        const auto size = i == 0 ? unsigned(width) : bake ? bake->resolution : 1;
        const auto mapHeight = i == 0 ? unsigned(height) : bake ? bake->resolution : 1;
        require(maps[i]->size() == std::size_t(size) * mapHeight * 4,
                "Material preview map dimensions mismatch");
        TextureDesc d;
        d.Name = names[i];
        d.Type = RESOURCE_DIM_TEX_2D;
        d.Width = size;
        d.Height = mapHeight;
        d.MipLevels = mips[i] ? Uint32(mips[i]->size() + 1) : 1;
        d.Format = i == 0 ? TEX_FORMAT_RGBA8_UNORM_SRGB : TEX_FORMAT_RGBA8_UNORM;
        d.Usage = USAGE_IMMUTABLE;
        d.BindFlags = BIND_SHADER_RESOURCE;
        std::vector<TextureSubResData> levels(d.MipLevels);
        levels[0].pData = maps[i]->data();
        levels[0].Stride = size * 4;
        for (Uint32 level = 1; level < d.MipLevels; ++level)
        {
            const auto mipWidth = std::max(1u, d.Width >> level);
            const auto mipHeight = std::max(1u, d.Height >> level);
            require((*mips[i])[level - 1].size() == std::size_t(mipWidth) * mipHeight * 4,
                    "Material preview mip dimensions mismatch");
            levels[level].pData = (*mips[i])[level - 1].data();
            levels[level].Stride = mipWidth * 4;
        }
        TextureData initial{levels.data(), d.MipLevels};
        r.materialMaps[i].Release();
        r.device->CreateTexture(d, &initial, &r.materialMaps[i]);
        require(bool(r.materialMaps[i]), "Material preview texture allocation failed");
        r.binding->GetVariableByName(SHADER_TYPE_PIXEL, names[i])
            ->Set(r.materialMaps[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        for (Uint32 level = 0; level < d.MipLevels; ++level)
            r.bytes += std::size_t(d.Width >> level) * (d.Height >> level) * 4;
    }
    r.material = true;
}
void *Renderer::draw(unsigned width, unsigned height, const Camera &camera, int mode)
{
    auto &r = *impl;
    require(mode != 5 || r.material, "Material preview needs baked maps and diffuse texture");
    width = std::max(32U, width);
    height = std::max(32U, height);
    if (width != r.viewWidth || height != r.viewHeight)
    {
        r.context->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        r.color.Release();
        r.depth.Release();
        TextureDesc d;
        d.Name = "Playground viewport";
        d.Type = RESOURCE_DIM_TEX_2D;
        d.Width = width;
        d.Height = height;
        d.Format = TEX_FORMAT_RGBA8_UNORM;
        d.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
        r.device->CreateTexture(d, nullptr, &r.color);
        d.Name = "Playground depth";
        d.Format = TEX_FORMAT_D32_FLOAT;
        d.BindFlags = BIND_DEPTH_STENCIL;
        r.device->CreateTexture(d, nullptr, &r.depth);
        require(bool(r.color) && bool(r.depth), "Viewport target allocation failed");
        r.viewWidth = width;
        r.viewHeight = height;
    }
    auto *color = r.color->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    auto *depth = r.depth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
    r.context->SetRenderTargets(1, &color, depth, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    const float clear[] = {0.055f, 0.068f, 0.086f, 1};
    r.context->ClearRenderTarget(color, clear, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    r.context->ClearDepthStencil(depth, CLEAR_DEPTH_FLAG, 1, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    if (r.vertices)
    {
        MapHelper<Constants> cb(r.context, r.constants, MAP_WRITE, MAP_FLAG_DISCARD);
        *cb = Constants{{camera.yaw, camera.pitch, camera.distance, float(width) / height},
                        {camera.target[0], camera.target[1], camera.target[2], float(mode)},
                        {r.low, r.high, r.baked ? 1.0f : 0.0f, 0}};
        cb.Unmap();
        auto *pso = (mode == 2 && r.wire) ? r.wire.RawPtr() : r.solid.RawPtr();
        r.context->SetPipelineState(pso);
        r.context->CommitShaderResources(r.binding, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        IBuffer *buffer = r.vertices;
        Uint64 offset = 0;
        r.context->SetVertexBuffers(0, 1, &buffer, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                    SET_VERTEX_BUFFERS_FLAG_RESET);
        r.context->SetIndexBuffer(r.indices, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        DrawIndexedAttribs draw;
        draw.IndexType = VT_UINT32;
        draw.NumIndices = r.indexCount;
        draw.Flags = DRAW_FLAG_VERIFY_ALL;
        r.context->DrawIndexed(draw);
    }
    r.context->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    StateTransitionDesc barrier{r.color, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE,
                                STATE_TRANSITION_FLAG_UPDATE_STATE};
    r.context->TransitionResourceStates(1, &barrier);
    return r.color->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
}
void Renderer::present()
{
    auto &r = *impl;
    auto *rt = r.swap->GetCurrentBackBufferRTV();
    r.context->SetRenderTargets(1, &rt, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    const float clear[] = {0.04f, 0.04f, 0.05f, 1};
    r.context->ClearRenderTarget(rt, clear, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    r.ui->Render(r.context);
    r.swap->Present(1);
}
void Renderer::capture(const std::filesystem::path &path)
{
    auto &r = *impl;
    if (!r.color)
        return;
    TextureDesc desc = r.color->GetDesc();
    desc.Name = "Playground capture staging";
    desc.Usage = USAGE_STAGING;
    desc.BindFlags = BIND_NONE;
    desc.CPUAccessFlags = CPU_ACCESS_READ;
    RefCntAutoPtr<ITexture> staging;
    r.device->CreateTexture(desc, nullptr, &staging);
    CopyTextureAttribs copy;
    copy.pSrcTexture = r.color;
    copy.pDstTexture = staging;
    copy.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    copy.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    r.context->CopyTexture(copy);
    r.context->WaitForIdle();
    MappedTextureSubresource mapped;
    r.context->MapTextureSubresource(staging, 0, 0, MAP_READ, MAP_FLAG_DO_NOT_WAIT, nullptr, mapped);
    require(mapped.pData != nullptr, "Capture map failed");
    int ok =
        stbi_write_png(path.string().c_str(), desc.Width, desc.Height, 4, mapped.pData, int(mapped.Stride));
    r.context->UnmapTextureSubresource(staging, 0, 0);
    require(ok != 0, "Capture write failed");
}
GpuComparison Renderer::compareFbm(const MeshResult &mesh, const Layer &layer)
{
    auto &r = *impl;
    GpuComparison result;
    result.samples = mesh.mesh.positions.size();
    if (!result.samples)
        return result;
    if (!r.compute)
    {
        auto cs = r.makeShader(computeShader, "CSMain", SHADER_TYPE_COMPUTE);
        ComputePipelineStateCreateInfo ci;
        ci.PSODesc.Name = "Playground FBM parity";
        ci.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
        ci.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        ci.pCS = cs;
        r.device->CreateComputePipelineState(ci, &r.compute);
        require(bool(r.compute), "Compute pipeline failed");
    }
    struct Params
    {
        Uint32 count, seed, octaves;
        float frequency, lacunarity, gain, pad[2], scale[4];
    };
    Params params{Uint32(result.samples),
                  layer.seed,
                  Uint32(layer.octaves),
                  layer.frequency,
                  layer.lacunarity,
                  layer.roughness,
                  {0, 0},
                  {layer.coordinateScale[0], layer.coordinateScale[1], layer.coordinateScale[2], 0}};
    std::vector<std::array<float, 4>> positions;
    for (auto p : mesh.mesh.positions)
        positions.push_back({float(p.x), float(p.y), float(p.z), 0});
    RefCntAutoPtr<IBuffer> input, output, constant, staging;
    BufferDesc d;
    d.Name = "FBM samples";
    d.Size = positions.size() * 16;
    d.BindFlags = BIND_SHADER_RESOURCE;
    d.Mode = BUFFER_MODE_STRUCTURED;
    d.ElementByteStride = 16;
    d.Usage = USAGE_IMMUTABLE;
    BufferData data{positions.data(), d.Size};
    r.device->CreateBuffer(d, &data, &input);
    d.Name = "FBM values";
    d.Size = positions.size() * 4;
    d.BindFlags = BIND_UNORDERED_ACCESS;
    d.ElementByteStride = 4;
    d.Usage = USAGE_DEFAULT;
    r.device->CreateBuffer(d, nullptr, &output);
    d.Name = "FBM readback";
    d.Usage = USAGE_STAGING;
    d.BindFlags = BIND_NONE;
    d.CPUAccessFlags = CPU_ACCESS_READ;
    r.device->CreateBuffer(d, nullptr, &staging);
    d = {};
    d.Name = "FBM parameters";
    d.Size = sizeof(params);
    d.Usage = USAGE_IMMUTABLE;
    d.BindFlags = BIND_UNIFORM_BUFFER;
    data = {&params, sizeof(params)};
    r.device->CreateBuffer(d, &data, &constant);
    require(bool(input) && bool(output) && bool(staging) && bool(constant), "FBM buffer allocation failed");
    RefCntAutoPtr<IShaderResourceBinding> binding;
    r.compute->CreateShaderResourceBinding(&binding, true);
    binding->GetVariableByName(SHADER_TYPE_COMPUTE, "Params")->Set(constant);
    binding->GetVariableByName(SHADER_TYPE_COMPUTE, "Positions")
        ->Set(input->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
    binding->GetVariableByName(SHADER_TYPE_COMPUTE, "Values")
        ->Set(output->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS));
    RefCntAutoPtr<IQuery> query;
    if (r.device->GetDeviceInfo().Features.DurationQueries == DEVICE_FEATURE_STATE_ENABLED)
    {
        QueryDesc q;
        q.Name = "FBM GPU duration";
        q.Type = QUERY_TYPE_DURATION;
        r.device->CreateQuery(q, &query);
    }
    r.context->SetPipelineState(r.compute);
    r.context->CommitShaderResources(binding, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    if (query)
        r.context->BeginQuery(query);
    DispatchComputeAttribs dispatch;
    dispatch.ThreadGroupCountX = Uint32((positions.size() + 63) / 64);
    r.context->DispatchCompute(dispatch);
    if (query)
        r.context->EndQuery(query);
    r.context->CopyBuffer(output, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, staging, 0,
                          positions.size() * 4, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    r.context->WaitForIdle();
    if (query)
    {
        QueryDataDuration q;
        if (query->GetData(&q, sizeof(q)) && q.Frequency)
            result.gpuMilliseconds = double(q.Duration) * 1000 / q.Frequency;
    }
    MapHelper<float> values(r.context, staging, MAP_READ, MAP_FLAG_DO_NOT_WAIT);
    auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < positions.size(); ++i)
    {
        Vec3d p{double(positions[i][0]) * layer.frequency * layer.coordinateScale[0],
                double(positions[i][1]) * layer.frequency * layer.coordinateScale[1],
                double(positions[i][2]) * layer.frequency * layer.coordinateScale[2]};
        auto cpu =
            PA::evaluateGeologyFbmReference(p, layer.seed, layer.octaves, layer.lacunarity, layer.roughness);
        result.maxError = std::max(result.maxError, std::abs(cpu - values[i]));
    }
    result.cpuMilliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return result;
}
std::size_t Renderer::bufferBytes() const
{
    return impl->bytes + std::size_t(impl->viewWidth) * impl->viewHeight * 8;
}
std::string Renderer::adapter() const
{
    return impl->device->GetAdapterInfo().Description;
}
} // namespace Playground
