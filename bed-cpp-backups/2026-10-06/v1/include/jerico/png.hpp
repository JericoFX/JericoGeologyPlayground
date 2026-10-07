#pragma once
#include "scene.hpp"
namespace jerico {
// Pixels are linear RGB doubles, in bottom-left row order. PNG rows are top-down.
// srgb=true applies the exact sRGB transfer function before 16-bit quantization.
void write_png16(const std::string&path,int width,int height,const std::vector<V3>&pixels,bool srgb=false,const std::vector<double>&alpha={});
}
