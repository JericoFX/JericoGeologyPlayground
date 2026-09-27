#include "Renderer.hpp"
#include "NormalBake.hpp"
#include "WebLod.hpp"
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
#include <numeric>
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
    float normalBias[4];
    float webA[4];
    float webB[4];
    float webC[4];
    float webD[4];
    float webE[4];
    float webF[4];
    float webG[4];
};
const char *shader = R"(
cbuffer Constants {float4 Camera;float4 Target;float4 Range;float4 NormalBias;float4 WebA;float4 WebB;float4 WebC;float4 WebD;float4 WebE;float4 WebF;float4 WebG;};
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
 float3 position=input.Position;
 if(WebF.w>.5){
  float weight=Target.w>10.5 && Target.w<15.5?
    sin(3.14159265*input.UV.x)*sin(3.14159265*input.UV.y):input.Field;
  position+=WebF.xyz*saturate(weight);
 }
 float3 q=position-Target.xyz;float depth=dot(q,forward)+Camera.z;float scale=2.4;
 o.Position=float4(dot(q,right)*scale/Camera.w,dot(q,up)*scale,(depth-0.05)*100.0/99.95,depth);
 o.Normal=input.Normal;o.World=position;o.Field=input.Field;o.UV=input.UV;o.Tangent=input.Tangent;o.Bitangent=input.Bitangent;return o;
}
float4 PSMain(VOut input,uint primitiveId:SV_PrimitiveID):SV_TARGET {
 if(Target.w>10.5 && Target.w<15.5){
  float2 uv=float2(input.UV.x,1-input.UV.y);
  float4 coverage=AmbientMap.Sample(AmbientMap_sampler,uv);
  float phi=ContactMap.Sample(ContactMap_sampler,uv).r;
  float alpha=coverage.a;
  if(Target.w>14.5){
   float signedPhi=(phi-.5)*.5;
   float aa=max(.0001,max(.1,WebB.y)*fwidth(signedPhi));
   alpha=1-smoothstep(-aa,aa,signedPhi);
  }
  if((Target.w<11.5 || Target.w>14.5) && alpha<1.0/255.0 && phi>.53)
   discard;
  float3 mapped=NormalMap.Sample(NormalMap_sampler,uv).xyz*2-1;
  if(Target.w<11.5 || Target.w>14.5){
   float3 surfaceNormal=normalize(input.Normal);
   float3 n=surfaceNormal;
   float3 dx=ddx(input.World),dy=ddy(input.World);
   float2 ux=ddx(input.UV),uy=ddy(input.UV);
   float det=ux.x*uy.y-ux.y*uy.x;
   float3 tangent=abs(det)>1e-6?normalize((dx*uy.y-dy*ux.y)/det):float3(1,0,0);
   float3 bitangent=normalize(cross(n,tangent));
   if(dot(bitangent,(dy*ux.x-dx*uy.x)/max(abs(det),1e-6))*sign(det)<0)bitangent=-bitangent;
   n=normalize(tangent*mapped.x+bitangent*mapped.y+n*mapped.z);
   float3 lightDelta=WebE.xyz-input.World;
   float lightDistance=length(lightDelta);
   float3 light=lightDelta/max(lightDistance,.0001);
   float3 forward=float3(sin(Camera.x)*cos(Camera.y),sin(Camera.y),cos(Camera.x)*cos(Camera.y));
   float3 eye=Target.xyz-forward*Camera.z;
   float3 viewDirection=normalize(eye-input.World);
   if(dot(n,viewDirection)<0){n=-n;surfaceNormal=-surfaceNormal;}
   float3 halfway=normalize(light+viewDirection);
   float attenuation=saturate(1-lightDistance/max(WebE.w,.001));
   float wet=saturate(WebA.z);
   float rough=saturate(WebA.x*(1-.72*wet));
   float key=sqrt(saturate(dot(n,light)))*attenuation;
   float rim=saturate(1-dot(n,viewDirection));
   float3 strandTangent=normalize(cross(surfaceNormal,n)+float3(.001,.002,.003));
   float tangentGlint=pow(saturate(1-abs(dot(strandTangent,halfway))),
                          lerp(36,8,rough));
   float normalGlint=pow(saturate(dot(n,halfway)),lerp(80,8,rough));
   float glint=tangentGlint*normalGlint*attenuation;
   float signedPhi=(phi-.5)*.5;
   float sigma=max(.001,WebB.y*fwidth(signedPhi)*2);
   float field=exp(-pow(max(signedPhi,0)/sigma,2));
   float droplet=0;
   if(WebD.z>0){
    float2 dropletCell=uv/max(WebD.w,.002);
    float2 cell=floor(dropletCell);
    float random=frac(sin(dot(cell,float2(127.1,311.7))+WebD.y)*43758.5453);
    float2 center=float2(frac(sin(dot(cell,float2(269.5,183.3))+WebD.y)*27471.3),
                         frac(sin(dot(cell,float2(113.5,271.9))+WebD.y)*13579.7));
    float dropRadius=lerp(WebG.x/max(WebD.w,.002),1,
       frac(sin(dot(cell,float2(41.3,97.7))+WebD.y)*17831.3))*.36;
    droplet=step(1-saturate(WebD.z),random)*
       (1-smoothstep(dropRadius*.25,dropRadius,
         length(frac(dropletCell)-center)))*field;
   }
   float3 silk=float3(.80,.84,.86)+float3(.12,.11,.10)*key;
   silk=lerp(silk,float3(.86,.91,.95),wet*.45);
   silk=lerp(silk,float3(.92,.95,1),saturate(WebA.w)*.65);
   silk=lerp(silk,float3(.57,.51,.42),saturate(WebB.x)*.5);
   float response=saturate(WebC.x+WebC.y*glint+
      WebC.z*(.85*key+.15*glint)+WebC.w*rim*key+
      wet*(.12+.25*key));
   float glintEnergy=WebD.x*(glint*(.25+wet*.75)+
      droplet*(.15+.85*attenuation)*(.3+wet*.7));
   silk=WebA.y*(silk+float3(.94,.97,1)*glintEnergy);
   alpha=saturate(alpha*response+droplet*(.15+.45*attenuation)*wet);
   alpha*=saturate((WebB.w-Camera.z)/max(WebB.w-WebB.z,.001));
   return float4(pow(saturate(silk),1.0/2.2),alpha);
  }
  if(Target.w<12.5)return float4(coverage.aaa,1);
  if(Target.w<13.5)return float4(phi,phi,phi,1);
  return float4(mapped*.5+.5,coverage.a);
 }
 if(Target.w>15.5 && Target.w<16.5){
  float grain=sin(input.World.x*3.1+sin(input.World.z*1.7))*
              sin(input.World.y*4.2+input.World.z*2.3);
  float shade=.12+.025*grain+.045*saturate(dot(normalize(input.Normal),
                                             normalize(float3(-.3,.8,-.4))));
  return float4(shade*.85,shade*.86,shade*.81,1);
 }
 if(Target.w>5.5 && Target.w<7.5){
  uint packed;
  if(Target.w<6.5){
   float cy=cos(Camera.x),sy=sin(Camera.x),cp=cos(Camera.y),sp=sin(Camera.y);
   float3 forward=float3(sy*cp,sp,cy*cp);
   float viewDepth=dot(input.World-Target.xyz,forward)+Camera.z;
   packed=(uint)round(saturate(viewDepth/100.0)*16777215.0);
  }else packed=primitiveId+1u;
  return float4(float3(float((packed>>16)&255u),float((packed>>8)&255u),
                       float(packed&255u))/255.0,1.0);
 }
 if(Target.w>3.5 && Target.w<4.5){float v=NormalMap.Sample(NormalMap_sampler,float2(input.UV.x,1-input.UV.y)).r;return float4(v,v,v,1);}
 float3 n=normalize(input.Normal);
 float normalLength=1;
 if(Range.z>0.5) {
  float3 mapped=NormalMap.SampleBias(NormalMap_sampler,float2(input.UV.x,1-input.UV.y),NormalBias.x).xyz*2-1;
  normalLength=saturate(length(mapped));
  float3 t=normalize(input.Tangent-n*dot(input.Tangent,n));
  float3 b=cross(n,t)*sign(dot(cross(n,t),input.Bitangent));
  n=normalize(t*mapped.x+b*mapped.y+n*mapped.z);
 }
 if(Target.w>0.5 && Target.w<1.5)return float4(n*.5+.5,1);
 if(Target.w>7.5 && Target.w<8.5){
  float2 grid=floor(input.UV*2.0);
  float checker=fmod(abs(grid.x+grid.y),2.0);
  float2 seam=abs(frac(input.UV)-0.5);
  float gridLine=step(.47,max(seam.x,seam.y));
  float3 c=lerp(float3(.12,.16,.18),float3(.84,.81,.69),checker);
  return float4(lerp(c,float3(.12,.45,.9),gridLine*.65),1);
 }
 if(Target.w>8.5 && Target.w<9.5){
  float seam=step(.5,input.Field);
  return float4(lerp(float3(.52,.48,.41),float3(1,.05,.1),seam),1);
 }
 if(Target.w>9.5 && Target.w<10.5){
  if(input.Field>.5 && input.Field<1.5)return float4(1,.16,.14,1);
  if(input.Field>1.5 && input.Field<2.5)return float4(.16,1,.24,1);
  if(input.Field>2.5)return float4(.15,.45,1,1);
  return float4(.33,.31,.28,1);
 }
 if(Target.w>4.5 && Target.w<5.5){
  float2 uv=float2(input.UV.x,1-input.UV.y);
  float ao=AmbientMap.Sample(AmbientMap_sampler,uv).r;
  float contact=ContactMap.Sample(ContactMap_sampler,uv).r;
  float cavity=CavityMap.Sample(CavityMap_sampler,uv).r;
  float edge=EdgeMap.Sample(EdgeMap_sampler,uv).r;
  float3 weight=pow(abs(n),4);
  weight/=max(weight.x+weight.y+weight.z,0.00001);
  float materialScale=Range.w>3.5?.22:.8;
  float3 albedo=ColorMap.Sample(ColorMap_sampler,input.World.yz*materialScale).rgb*weight.x+
                ColorMap.Sample(ColorMap_sampler,input.World.xz*materialScale).rgb*weight.y+
                ColorMap.Sample(ColorMap_sampler,input.World.xy*materialScale).rgb*weight.z;
  if(Range.w>1.5 && Range.w<3.5){
   float grey=dot(albedo,float3(.2126,.7152,.0722));
   albedo=Range.w>2.5?grey*float3(.48,.50,.51):grey*float3(.76,.83,.87);
  }
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
  if(NormalBias.y>0.5 && Range.z>0.5)
   roughness=sqrt(saturate(roughness*roughness+max(0,1-normalLength)));
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
    RefCntAutoPtr<IPipelineState> solid, wire, webSolid, compute;
    RefCntAutoPtr<IShaderResourceBinding> binding;
    RefCntAutoPtr<IBuffer> constants, vertices, indices, parameters;
    RefCntAutoPtr<IBuffer> webStructureVertices,webStructureIndices;
    RefCntAutoPtr<IBuffer> webBackdropVertices,webBackdropIndices;
    RefCntAutoPtr<ITexture> color, depth, normalMap;
    RefCntAutoPtr<IQuery> timing;
    std::array<RefCntAutoPtr<ITexture>, 5> materialMaps;
    bool baked{}, material{}, materialColorMips{},web{};
    WebAppearance webAppearance;
    Vec3d webDisplacement{};
    bool webDeformable{};
    int materialAppearance{};
    std::filesystem::path materialColorPath;
    std::size_t materialColorBytes{};
    unsigned indexCount{}, viewWidth{}, viewHeight{};
    unsigned webStructureIndexCount{};
    unsigned webBackdropIndexCount{};
    std::size_t bytes{}, meshBytes{};
    float low{}, high{1};
    float normalMipBias{};
    bool normalMomentMode{};
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
    p.PSODesc.Name="Playground transparent web";
    auto &blend=p.GraphicsPipeline.BlendDesc.RenderTargets[0];
    blend.BlendEnable=true;
    blend.SrcBlend=BLEND_FACTOR_SRC_ALPHA;
    blend.DestBlend=BLEND_FACTOR_INV_SRC_ALPHA;
    p.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable=false;
    r.device->CreateGraphicsPipelineState(p,&r.webSolid);
    require(bool(r.webSolid),"Web pipeline creation failed");
    r.webSolid->GetStaticVariableByName(SHADER_TYPE_VERTEX,"Constants")->Set(r.constants);
    r.webSolid->GetStaticVariableByName(SHADER_TYPE_PIXEL,"Constants")->Set(r.constants);
    p.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable=true;
    p.GraphicsPipeline.BlendDesc.RenderTargets[0]=RenderTargetBlendDesc{};
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
                      const std::vector<std::vector<std::uint8_t>> *diagnosticMips,
                      bool useNormalMoments,
                      const std::vector<std::array<float, 2>> *cornerUV)
{
    auto &r = *impl;
    r.web=false;
    r.webDeformable=false;
    r.webStructureVertices.Release();
    r.webStructureIndices.Release();
    r.webStructureIndexCount=0;
    r.webBackdropVertices.Release();
    r.webBackdropIndices.Release();
    r.webBackdropIndexCount=0;
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
    require(!cornerUV || (corners && cornerUV->size() == mesh.mesh.indices.size()),
            "Analytic UV count must match mesh corners");
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
        else if (cornerUV)
        {
            vertices.back().uv[0] = (*cornerUV)[i][0];
            vertices.back().uv[1] = (*cornerUV)[i][1];
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
    if (useNormalMoments && (!bake || diagnosticMap || bake->normalMomentMips.empty()))
        throw std::runtime_error("Normal moment mode requires moment mips in the bake");
    const auto *mips = diagnosticMap ? diagnosticMips : bake ?
        (useNormalMoments ? &bake->normalMomentMips : &bake->normalMips) : nullptr;
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
    r.normalMomentMode = useNormalMoments;
    r.material = false;
    for (Uint32 level = 0; level < texture.MipLevels; ++level)
        r.bytes += std::size_t(texture.Width >> level) * (texture.Height >> level) * 4;
    r.meshBytes = r.bytes;
}
void Renderer::uploadWeb(const Web::Result &web,bool averagedCoverageMips,
                         const Web::Result *structural,const MeshResult *backdrop)
{
    require(web.cardAccepted && !web.mips.empty() &&
            web.cardUV.size()==web.card.mesh.indices.size(),
            "Web card and baked texture are required");
    MeshResult subdivided;
    std::vector<std::array<float,2>> subdividedUV;
    const bool deformable=structural && web.patches.size()==1;
    if (deformable)
    {
        const auto &patch=web.patches.front();
        for (unsigned y=0;y<3;++y)
            for (unsigned x=0;x<3;++x)
            {
                const double u=patch.minU+(patch.maxU-patch.minU)*x*.5;
                const double v=patch.minV+(patch.maxV-patch.minV)*y*.5;
                subdivided.mesh.positions.push_back({
                    patch.center.x+patch.axisU.x*u+patch.axisV.x*v,
                    patch.center.y+patch.axisU.y*u+patch.axisV.y*v,
                    patch.center.z+patch.axisU.z*u+patch.axisV.z*v});
                subdivided.normals.push_back(patch.normal);
                subdivided.field.push_back(1);
            }
        for (unsigned y=0;y<2;++y)
            for (unsigned x=0;x<2;++x)
            {
                const unsigned a=y*3+x,b=a+1,c=a+3,d=c+1;
                for (unsigned vertex:{a,b,d,a,d,c})
                {subdivided.mesh.indices.push_back(vertex);
                 subdivided.cornerNormals.push_back(patch.normal);
                 subdividedUV.push_back({float(vertex%3)*.5f,
                                         float(vertex/3)*.5f});}
            }
    }
    upload(deformable?subdivided:web.card,nullptr,nullptr,nullptr,false,
           deformable?&subdividedUV:&web.cardUV);
    auto &r=*impl;
    r.webDeformable=deformable;
    std::vector<std::vector<std::uint8_t>> average;
    if (averagedCoverageMips)
    {
        average.reserve(web.mips.size());
        average.push_back(web.mips.front().coverageRgba);
        for (std::size_t level=1;level<web.mips.size();++level)
        {
            const auto &previous=web.mips[level-1],&current=web.mips[level];
            average.emplace_back(std::size_t(current.width)*current.height*4);
            for (unsigned y=0;y<current.height;++y)
                for (unsigned x=0;x<current.width;++x)
                {
                    unsigned sum{};
                    for (unsigned dy=0;dy<2;++dy)
                        for (unsigned dx=0;dx<2;++dx)
                            sum+=average[level-1][4*(std::size_t(2*y+dy)*previous.width+
                                2*x+dx)+3];
                    const auto value=std::uint8_t(sum/4);
                    const auto index=4*(std::size_t(y)*current.width+x);
                    for (unsigned channel=0;channel<4;++channel)
                        average.back()[index+channel]=value;
                }
        }
    }
    auto bind=[&](const char *name,const char *label,
                  const std::vector<std::uint8_t> Web::TextureLevel::*member,
                  RefCntAutoPtr<ITexture> &texture)
    {
        const auto &base=web.mips.front();
        TextureDesc description;
        description.Name=label;
        description.Type=RESOURCE_DIM_TEX_2D;
        description.Width=base.width;
        description.Height=base.height;
        description.MipLevels=Uint32(web.mips.size());
        description.Format=TEX_FORMAT_RGBA8_UNORM;
        description.Usage=USAGE_IMMUTABLE;
        description.BindFlags=BIND_SHADER_RESOURCE;
        std::vector<TextureSubResData> subresources(web.mips.size());
        for (std::size_t level=0;level<web.mips.size();++level)
        {
            const auto &mip=web.mips[level];
            const auto &pixels=averagedCoverageMips && member==
                &Web::TextureLevel::coverageRgba?average[level]:mip.*member;
            require(mip.width==std::max(1u,base.width>>level) &&
                    mip.height==std::max(1u,base.height>>level) &&
                    pixels.size()==std::size_t(mip.width)*mip.height*4,
                    "Web texture mip dimensions mismatch");
            subresources[level].pData=pixels.data();
            subresources[level].Stride=mip.width*4;
            r.bytes+=pixels.size();
        }
        TextureData data{subresources.data(),Uint32(subresources.size())};
        texture.Release();
        r.device->CreateTexture(description,&data,&texture);
        require(bool(texture),"Web texture allocation failed");
        r.binding->GetVariableByName(SHADER_TYPE_PIXEL,name)->Set(
            texture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    };
    bind("NormalMap","Web pseudo normal",&Web::TextureLevel::normalRgba,r.normalMap);
    bind("AmbientMap","Web coverage",&Web::TextureLevel::coverageRgba,r.materialMaps[1]);
    bind("ContactMap","Web distance",&Web::TextureLevel::distanceRgba,r.materialMaps[2]);
    if (structural && !structural->structural.mesh.indices.empty())
    {
        const auto &mesh=structural->structural;
        require(mesh.cornerNormals.size()==mesh.mesh.indices.size(),
                "Web structure corner normals mismatch");
        double extent=0;
        const auto hub=web.graph.nodes.front();
        for (auto point:web.graph.nodes)
            extent=std::max(extent,std::hypot(point.x-hub.x,point.y-hub.y,
                                               point.z-hub.z));
        std::vector<Vertex> vertices;
        vertices.reserve(mesh.mesh.indices.size());
        std::vector<std::uint32_t> indices;
        indices.reserve(mesh.mesh.indices.size());
        for (std::size_t i=0;i<mesh.mesh.indices.size();++i)
        {
            const auto source=mesh.mesh.indices[i];
            const auto p=mesh.mesh.positions[source],n=mesh.cornerNormals[i];
            const auto distance=std::hypot(p.x-hub.x,p.y-hub.y,p.z-hub.z);
            const float weight=float(std::clamp(1-distance/std::max(extent,.001),0.0,1.0));
            vertices.push_back({{float(p.x),float(p.y),float(p.z)},
                                {float(n.x),float(n.y),float(n.z)},weight});
            indices.push_back(std::uint32_t(i));
        }
        BufferDesc description;
        description.Name="Web structural vertices";
        description.Size=vertices.size()*sizeof(Vertex);
        description.Usage=USAGE_IMMUTABLE;
        description.BindFlags=BIND_VERTEX_BUFFER;
        BufferData initial{vertices.data(),description.Size};
        r.device->CreateBuffer(description,&initial,&r.webStructureVertices);
        description.Name="Web structural indices";
        description.Size=indices.size()*sizeof(std::uint32_t);
        description.BindFlags=BIND_INDEX_BUFFER;
        initial={indices.data(),description.Size};
        r.device->CreateBuffer(description,&initial,&r.webStructureIndices);
        require(bool(r.webStructureVertices) && bool(r.webStructureIndices),
                "Web structure buffer allocation failed");
        r.webStructureIndexCount=unsigned(indices.size());
        r.bytes+=vertices.size()*sizeof(Vertex)+indices.size()*sizeof(std::uint32_t);
    }
    if (backdrop && !backdrop->mesh.indices.empty())
    {
        require(backdrop->normals.size()==backdrop->mesh.positions.size(),
                "Web backdrop normals mismatch");
        std::vector<Vertex> vertices;
        vertices.reserve(backdrop->mesh.positions.size());
        for (std::size_t i=0;i<backdrop->mesh.positions.size();++i)
        {
            const auto p=backdrop->mesh.positions[i],n=backdrop->normals[i];
            vertices.push_back({{float(p.x),float(p.y),float(p.z)},
                                {float(n.x),float(n.y),float(n.z)},0});
        }
        BufferDesc description;
        description.Name="Web corridor vertices";
        description.Size=vertices.size()*sizeof(Vertex);
        description.Usage=USAGE_IMMUTABLE;
        description.BindFlags=BIND_VERTEX_BUFFER;
        BufferData initial{vertices.data(),description.Size};
        r.device->CreateBuffer(description,&initial,&r.webBackdropVertices);
        description.Name="Web corridor indices";
        description.Size=backdrop->mesh.indices.size()*sizeof(std::uint32_t);
        description.BindFlags=BIND_INDEX_BUFFER;
        initial={backdrop->mesh.indices.data(),description.Size};
        r.device->CreateBuffer(description,&initial,&r.webBackdropIndices);
        require(bool(r.webBackdropVertices) && bool(r.webBackdropIndices),
                "Web corridor buffer allocation failed");
        r.webBackdropIndexCount=unsigned(backdrop->mesh.indices.size());
        r.bytes+=vertices.size()*sizeof(Vertex)+description.Size;
    }
    r.web=true;
}
void Renderer::clearMesh()
{
    auto &r=*impl;
    r.vertices.Release();
    r.indices.Release();
    r.webStructureVertices.Release();
    r.webStructureIndices.Release();
    r.webStructureIndexCount=0;
    r.webBackdropVertices.Release();
    r.webBackdropIndices.Release();
    r.webBackdropIndexCount=0;
    r.indexCount=0;
    r.web=false;
    r.webDeformable=false;
    r.bytes=0;
    r.meshBytes=0;
}
void Renderer::setWebAppearance(const WebAppearance &appearance)
{
    for (float value:{appearance.roughness,appearance.brightness,appearance.wetness,
                      appearance.frost,appearance.dust,appearance.distanceSoftness,
                      appearance.baseVisibility,appearance.specularResponse,
                      appearance.lightRevealStrength,appearance.viewAngleResponse,
                      appearance.dropletDensity,appearance.dropletSizeMin,
                      appearance.dropletSizeMax,appearance.lightReach,
                      appearance.shaderOnlyDistance,appearance.cullDistance,
                      appearance.lightPosition[0],appearance.lightPosition[1],
                      appearance.lightPosition[2]})
        require(std::isfinite(value),"Web material parameter is non-finite");
    auto &r=*impl;
    r.webAppearance=appearance;
}
void Renderer::setWebDisplacement(Vec3d displacement)
{
    require(std::isfinite(displacement.x) && std::isfinite(displacement.y) &&
            std::isfinite(displacement.z),"Web displacement is non-finite");
    impl->webDisplacement=displacement;
}
void Renderer::uploadMaterial(const NormalBake *bake, const std::filesystem::path &diffusePath,
                              bool useDiffuseMips, int appearance)
{
    auto &r = *impl;
    require(!bake || (r.baked && bake->settings.specialistMaps && bake->settings.automaticUv),
            "Material preview requires an uploaded automatic-UV specialist bake");
    require(appearance >= 0 && appearance <= 4, "Unknown material appearance");
    r.bytes = r.meshBytes;
    int width = 0, height = 0, channels = 0;
    const bool reuseColor = r.materialColorPath == diffusePath &&
        r.materialColorMips == useDiffuseMips && bool(r.materialMaps[0]);
    std::vector<std::uint8_t> color;
    if (!reuseColor)
    {
        auto *decoded = stbi_load(diffusePath.string().c_str(), &width, &height, &channels, 4);
        if (!decoded || width <= 0 || height <= 0 || width > 4096 || height > 4096)
        {
            stbi_image_free(decoded);
            throw std::runtime_error("Material preview diffuse texture could not be decoded");
        }
        color.assign(decoded, decoded + std::size_t(width) * height * 4);
        stbi_image_free(decoded);
    }
    auto diffuseMips = !reuseColor && useDiffuseMips ? colorMips(color, unsigned(width), unsigned(height)) :
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
        if (i == 0 && reuseColor)
        {
            r.bytes += r.materialColorBytes;
            continue;
        }
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
        if (i == 0)
            r.materialColorBytes = std::size_t(width) * height * 4 +
                std::accumulate(diffuseMips.begin(), diffuseMips.end(), std::size_t(0),
                                [](std::size_t total, const auto &level) { return total + level.size(); });
    }
    r.materialColorPath = diffusePath;
    r.materialColorMips = useDiffuseMips;
    r.materialAppearance = appearance;
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
    const bool diagnostic = mode == 6 || mode == 7;
    const float clear[] = {diagnostic ? 0.0f : 0.055f, diagnostic ? 0.0f : 0.068f,
                           diagnostic ? 0.0f : 0.086f, diagnostic ? 0.0f : 1.0f};
    r.context->ClearRenderTarget(color, clear, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    r.context->ClearDepthStencil(depth, CLEAR_DEPTH_FLAG, 1, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    if (r.vertices)
    {
        const Constants values{{camera.yaw, camera.pitch, camera.distance, float(width) / height},
                        {camera.target[0], camera.target[1], camera.target[2], float(mode)},
                        {r.low, r.high, r.baked ? 1.0f : 0.0f, float(r.materialAppearance)},
                        {r.normalMipBias, r.normalMomentMode ? 1.0f : 0.0f, 0.0f, 0.0f},
                        {r.webAppearance.roughness,r.webAppearance.brightness,
                         r.webAppearance.wetness,r.webAppearance.frost},
                        {r.webAppearance.dust,r.webAppearance.distanceSoftness,
                         r.webAppearance.shaderOnlyDistance,
                         r.webAppearance.cullDistance},
                        {r.webAppearance.baseVisibility,r.webAppearance.specularResponse,
                         r.webAppearance.lightRevealStrength,
                         r.webAppearance.viewAngleResponse},
                        {r.webAppearance.specularResponse,
                         float(r.webAppearance.dropletSeed),
                         r.webAppearance.dropletDensity,
                         r.webAppearance.dropletSizeMax},
                        {r.webAppearance.lightPosition[0],r.webAppearance.lightPosition[1],
                         r.webAppearance.lightPosition[2],r.webAppearance.lightReach},
                        {float(r.webDisplacement.x),float(r.webDisplacement.y),
                         float(r.webDisplacement.z),
                         r.web && r.webDeformable?1.0f:0.0f},
                        {r.webAppearance.dropletSizeMin,0,0,0}};
        {
            MapHelper<Constants> cb(r.context,r.constants,MAP_WRITE,MAP_FLAG_DISCARD);
            *cb=values;
        }
        if (r.timing) r.context->BeginQuery(r.timing);
        if (r.webBackdropIndexCount && mode==11)
        {
            auto background=values;
            background.target[3]=16;
            MapHelper<Constants> bg(r.context,r.constants,MAP_WRITE,MAP_FLAG_DISCARD);
            *bg=background;
            bg.Unmap();
            r.context->SetPipelineState(r.solid);
            r.context->CommitShaderResources(r.binding,
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            IBuffer *buffer=r.webBackdropVertices;
            Uint64 offset=0;
            r.context->SetVertexBuffers(0,1,&buffer,&offset,
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                SET_VERTEX_BUFFERS_FLAG_RESET);
            r.context->SetIndexBuffer(r.webBackdropIndices,0,
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            DrawIndexedAttribs backgroundDraw;
            backgroundDraw.IndexType=VT_UINT32;
            backgroundDraw.NumIndices=r.webBackdropIndexCount;
            backgroundDraw.Flags=DRAW_FLAG_VERIFY_ALL;
            r.context->DrawIndexed(backgroundDraw);
            MapHelper<Constants> restore(r.context,r.constants,MAP_WRITE,MAP_FLAG_DISCARD);
            *restore=values;
        }
        auto *pso = (mode == 2 && r.wire) ? r.wire.RawPtr() :
                    (r.web && mode>=11 && mode<=15)?r.webSolid.RawPtr():r.solid.RawPtr();
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
        if (r.webStructureIndexCount && mode==11)
        {
            auto geometryValues=values;
            geometryValues.target[3]=0;
            {
                MapHelper<Constants> cb(r.context,r.constants,MAP_WRITE,MAP_FLAG_DISCARD);
                *cb=geometryValues;
            }
            r.context->SetPipelineState(r.solid);
            IBuffer *structure=r.webStructureVertices;
            r.context->SetVertexBuffers(0,1,&structure,&offset,
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION,SET_VERTEX_BUFFERS_FLAG_RESET);
            r.context->SetIndexBuffer(r.webStructureIndices,0,
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            draw.NumIndices=r.webStructureIndexCount;
            r.context->DrawIndexed(draw);
        }
        if (r.timing) r.context->EndQuery(r.timing);
    }
    r.context->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    StateTransitionDesc barrier{r.color, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE,
                                STATE_TRANSITION_FLAG_UPDATE_STATE};
    r.context->TransitionResourceStates(1, &barrier);
    return r.color->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
}
double Renderer::measureDrawGpu(unsigned width,unsigned height,const Camera &camera,int mode)
{
    auto &r=*impl;
    if (r.device->GetDeviceInfo().Features.DurationQueries!=DEVICE_FEATURE_STATE_ENABLED)
        return -1;
    QueryDesc description;
    description.Name="Playground web draw duration";
    description.Type=QUERY_TYPE_DURATION;
    r.device->CreateQuery(description,&r.timing);
    require(bool(r.timing),"Web draw duration query failed");
    draw(width,height,camera,mode);
    r.context->WaitForIdle();
    QueryDataDuration data{};
    const auto ready=r.timing->GetData(&data,sizeof(data));
    r.timing.Release();
    return ready && data.Frequency?double(data.Duration)*1000/data.Frequency:-1;
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
std::optional<std::uint32_t> Renderer::pickTriangle(unsigned width,unsigned height,
    const Camera &camera,unsigned pixelX,unsigned pixelY)
{
    if (pixelX>=width || pixelY>=height) return std::nullopt;
    draw(width,height,camera,7);
    auto &r=*impl;
    TextureDesc desc=r.color->GetDesc();
    desc.Name="Playground part pick staging";
    desc.Usage=USAGE_STAGING;
    desc.BindFlags=BIND_NONE;
    desc.CPUAccessFlags=CPU_ACCESS_READ;
    RefCntAutoPtr<ITexture> staging;
    r.device->CreateTexture(desc,nullptr,&staging);
    require(bool(staging),"Part pick staging allocation failed");
    CopyTextureAttribs copy;
    copy.pSrcTexture=r.color;
    copy.pDstTexture=staging;
    copy.SrcTextureTransitionMode=RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    copy.DstTextureTransitionMode=RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    r.context->CopyTexture(copy);
    r.context->WaitForIdle();
    MappedTextureSubresource mapped;
    r.context->MapTextureSubresource(staging,0,0,MAP_READ,MAP_FLAG_DO_NOT_WAIT,nullptr,mapped);
    require(mapped.pData!=nullptr,"Part pick readback failed");
    const auto *pixel=static_cast<const std::uint8_t *>(mapped.pData)+
        std::size_t(pixelY)*mapped.Stride+std::size_t(pixelX)*4;
    const std::uint32_t packed=(std::uint32_t(pixel[0])<<16)|
        (std::uint32_t(pixel[1])<<8)|std::uint32_t(pixel[2]);
    r.context->UnmapTextureSubresource(staging,0,0);
    if (!packed) return std::nullopt;
    return packed-1;
}
void Renderer::setNormalMipBias(float bias)
{
    require(std::isfinite(bias) && bias >= -2.0f && bias <= 0.0f,
            "Normal mip bias must be finite and between -2 and zero");
    impl->normalMipBias = bias;
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
