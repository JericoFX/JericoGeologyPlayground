#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
namespace rocks {
struct Vec3 { double x{},y{},z{}; bool operator==(const Vec3&)const=default; };
inline Vec3 operator+(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 operator-(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 operator*(Vec3 a,double b){return {a.x*b,a.y*b,a.z*b};}
inline Vec3 operator/(Vec3 a,double b){return a*(1/b);}
inline double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double length(Vec3 a){return std::sqrt(dot(a,a));}
inline Vec3 normalized(Vec3 a){double l=length(a);return l>0?a/l:Vec3{0,1,0};}
inline bool finite(Vec3 a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
enum class Stage {Base,Stratified,Eroded};
struct Recipe {std::uint32_t schemaVersion=1,seed=48;std::string units="m",generatorVersion="rocks-v2",extractorVersion="paris-mc-v1";double size=2,layerThickness=.4,contrast=.85,wear=1;std::string family="stratified";double detail=0;std::uint32_t columnCount=5;double studySpacing=.45,roundness=.12,chipping=.35,columnLean=.08,jointDepth=.4;};
struct GridSpec{std::uint32_t cells;Vec3 min,max;};
struct Sample{double value;Vec3 gradient;double resistance;};
}
