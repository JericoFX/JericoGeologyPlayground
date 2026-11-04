#pragma once
// Shared persistence for the existing DamagePathCutter. Geometry stays in DamageField.
#include "DamageField.hpp"
#include <Windows.h>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace Playground::DamageIO {
using Json=nlohmann::json;
inline constexpr std::size_t maximumFileBytes=1024*1024;
inline Json vectorJson(Vec3d p){return Json::array({p.x,p.y,p.z});}
inline Vec3d readVector(const Json& value){
 if(!value.is_array()||value.size()!=3)throw std::runtime_error("Damage vector must contain three numbers");
 Vec3d p{value.at(0).get<double>(),value.at(1).get<double>(),value.at(2).get<double>()};
 if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))throw std::runtime_error("Damage vector must be finite");
 return p;
}
inline std::uint64_t readSeed(const Json& value){
 if(!value.is_number_integer()||(!value.is_number_unsigned()&&value.get<std::int64_t>()<0))
  throw std::runtime_error("Damage contour seed must be an unsigned 64-bit integer");
 return value.get<std::uint64_t>();
}
inline Json writePathCutter(const DamagePathCutter& cutter){
 sampleDamagePathField(cutter.projector.center,cutter);
 Json points=Json::array();for(auto point:cutter.path)points.push_back(vectorJson(point));
 return {{"enabled",cutter.enabled},{"amount",cutter.amount},{"radius",cutter.radius},{"depth",cutter.depth},
  {"path",points},{"irregularity",cutter.irregularity},{"separationMeters",cutter.separationMeters},{"contourSeed",cutter.seed},
  {"projector",{{"center",vectorJson(cutter.projector.center)},{"tangent",vectorJson(cutter.projector.tangent)},
  {"bitangent",vectorJson(cutter.projector.bitangent)},{"normal",vectorJson(cutter.projector.normal)},
  {"scale",vectorJson(cutter.projector.scale)},{"rotationRadians",cutter.projector.rotationRadians}}}};
}
inline DamagePathCutter readPathCutter(const Json& value){
 DamagePathCutter cutter;cutter.enabled=value.at("enabled").get<bool>();cutter.amount=value.at("amount").get<double>();
 cutter.radius=value.at("radius").get<double>();cutter.depth=value.at("depth").get<double>();
 cutter.irregularity=value.value("irregularity",0.0);cutter.separationMeters=value.value("separationMeters",0.0);
 cutter.seed=value.contains("contourSeed")?readSeed(value.at("contourSeed")):0;
 const auto& points=value.at("path");
 if(!points.is_array()||points.size()<2||points.size()>maximumDamagePathPoints)throw std::runtime_error("Invalid damage path points");
 for(const auto& point:points)cutter.path.push_back(readVector(point));
 const auto& p=value.at("projector");
 cutter.projector.center=readVector(p.at("center"));cutter.projector.tangent=readVector(p.at("tangent"));
 cutter.projector.bitangent=readVector(p.at("bitangent"));cutter.projector.normal=readVector(p.at("normal"));
 cutter.projector.scale=readVector(p.at("scale"));cutter.projector.rotationRadians=p.at("rotationRadians").get<double>();
 sampleDamagePathField(cutter.projector.center,cutter);return cutter;
}
inline std::string readFile(const std::filesystem::path& path){
 if(std::filesystem::file_size(path)>maximumFileBytes)throw std::runtime_error("Damage recipe file too large");
 std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("Could not open damage recipe");
 std::string text(maximumFileBytes+1,'\0');
 in.read(text.data(),std::streamsize(text.size()));text.resize(std::size_t(in.gcount()));
 if(in.bad()||text.size()>maximumFileBytes)throw std::runtime_error("Could not read bounded damage recipe");
 return text;
}
inline void writeFile(const std::string& text,const std::filesystem::path& path){
 if(text.size()+1>maximumFileBytes)throw std::runtime_error("Damage recipe file too large");
 static std::atomic_uint64_t serial{};
 auto temporary=path;temporary+=L".tmp."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64())+L"."+std::to_wstring(serial++);
 HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Could not create temporary damage recipe");
 try{
  const auto payload=text+'\n';DWORD written{};
  if(!WriteFile(file,payload.data(),DWORD(payload.size()),&written,nullptr)||written!=payload.size()||!FlushFileBuffers(file))
   throw std::runtime_error("Could not write damage recipe");
  CloseHandle(file);file=INVALID_HANDLE_VALUE;
  if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
   throw std::runtime_error("Could not replace damage recipe file");
 }catch(...){if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);std::error_code ignored;std::filesystem::remove(temporary,ignored);throw;}
}
}
