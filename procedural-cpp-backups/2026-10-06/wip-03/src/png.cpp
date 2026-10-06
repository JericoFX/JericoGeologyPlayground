#include "jerico/png.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <cstdint>
#include <zlib.h>
namespace jerico {
namespace {
using Bytes=std::vector<unsigned char>;
void append_u32(Bytes&b,uint32_t v){b.push_back(v>>24);b.push_back(v>>16);b.push_back(v>>8);b.push_back(v);}
uint32_t crc32(const unsigned char*data,size_t length){return ::crc32(0,data,static_cast<uInt>(length));}
void chunk(std::ofstream&f,const std::array<char,4>&name,const Bytes&data){Bytes header;append_u32(header,uint32_t(data.size()));for(char c:name)header.push_back(c);f.write(reinterpret_cast<const char*>(header.data()),header.size());f.write(reinterpret_cast<const char*>(data.data()),data.size());Bytes crcdata;crcdata.reserve(4+data.size());for(char c:name)crcdata.push_back(c);crcdata.insert(crcdata.end(),data.begin(),data.end());Bytes crc;append_u32(crc,crc32(crcdata.data(),crcdata.size()));f.write(reinterpret_cast<const char*>(crc.data()),4);}
double transfer(double v){return v<=.0031308?12.92*v:1.055*std::pow(v,1/2.4)-.055;}
}
void write_png16(const std::string&path,int width,int height,const std::vector<V3>&pixels,bool srgb,const std::vector<double>&alpha){

 if(width<=0||height<=0||width>16384||height>16384||pixels.size()!=size_t(width)*height||(!alpha.empty()&&alpha.size()!=pixels.size()))throw std::invalid_argument("Invalid PNG16 dimensions or pixel buffer");
 const bool hasalpha=!alpha.empty();Bytes raw;raw.reserve((size_t(width)*(hasalpha?8:6)+1)*height);
 for(int y=height-1;y>=0;y--){raw.push_back(0);for(int x=0;x<width;x++){size_t i=size_t(y)*width+x;for(double v:pixels[i]){if(!std::isfinite(v))throw std::invalid_argument("Nonfinite PNG16 channel");v=std::clamp(srgb?transfer(v):v,0.,1.);uint16_t q=uint16_t(std::lround(v*65535));raw.push_back(q>>8);raw.push_back(q);}if(hasalpha){double v=alpha[i];if(!std::isfinite(v))throw std::invalid_argument("Nonfinite PNG16 alpha");uint16_t q=uint16_t(std::lround(std::clamp(v,0.,1.)*65535));raw.push_back(q>>8);raw.push_back(q);}}}
 // Native C++ lossless zlib encoding, directly from the 16-bit scanlines.
 // No Python, Blender, source image, or 8-bit intermediate is involved.
 uLongf bound=compressBound(raw.size());Bytes z(bound);int status=compress2(z.data(),&bound,raw.data(),raw.size(),6);if(status!=Z_OK)throw std::runtime_error("PNG16 zlib compression failed");z.resize(bound);
 std::ofstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Cannot create PNG16: "+path);const unsigned char sig[]={137,80,78,71,13,10,26,10};f.write(reinterpret_cast<const char*>(sig),8);Bytes ihdr;append_u32(ihdr,width);append_u32(ihdr,height);ihdr.insert(ihdr.end(),{16,static_cast<unsigned char>(hasalpha?6:2),0,0,0});chunk(f,{'I','H','D','R'},ihdr);if(srgb)chunk(f,{'s','R','G','B'},Bytes{0});chunk(f,{'I','D','A','T'},z);chunk(f,{'I','E','N','D'},{});if(!f)throw std::runtime_error("Cannot write PNG16: "+path);
}
}
