// Losslessly recompress already-generated native RGB/RGBA16 PNG files.
// Does not sample materials, alter pixels, or invoke any external process.
#include <nlohmann/json.hpp>
#include <zlib.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <stdexcept>
#include <array>
#include <iomanip>
#include <sstream>
using Bytes=std::vector<unsigned char>;
uint32_t be(const Bytes&b,size_t i){return uint32_t(b.at(i))<<24|uint32_t(b.at(i+1))<<16|uint32_t(b.at(i+2))<<8|b.at(i+3);}
void u32(Bytes&b,uint32_t x){b.push_back(x>>24);b.push_back(x>>16);b.push_back(x>>8);b.push_back(x);}
void idat(Bytes&out,const Bytes&data){u32(out,data.size());size_t start=out.size();out.insert(out.end(),{'I','D','A','T'});out.insert(out.end(),data.begin(),data.end());u32(out,crc32(0,out.data()+start,out.size()-start));}
nlohmann::json recompress(const std::filesystem::path&source,const std::filesystem::path&target){
 std::ifstream f(source,std::ios::binary);Bytes in((std::istreambuf_iterator<char>(f)),{});const std::array<unsigned char,8>sig={137,80,78,71,13,10,26,10};
 if(in.size()<33||!std::equal(sig.begin(),sig.end(),in.begin())||in[24]!=16||(in[25]!=2&&in[25]!=6)||in[28]!=0)throw std::runtime_error("Input must be a noninterlaced RGB/RGBA16 native PNG: "+source.string());
 size_t width=be(in,16),height=be(in,20),channels=in[25]==6?4:3;if(width>16384||height>16384)throw std::runtime_error("PNG dimensions exceed native limit");
 Bytes compressed;for(size_t p=8;p<in.size();){size_t n=be(in,p);if(p+12+n>in.size())throw std::runtime_error("Truncated PNG chunk");if(crc32(0,in.data()+p+4,n+4)!=be(in,p+8+n))throw std::runtime_error("PNG CRC mismatch");if(in[p+4]=='I'&&in[p+5]=='D'&&in[p+6]=='A'&&in[p+7]=='T')compressed.insert(compressed.end(),in.begin()+p+8,in.begin()+p+8+n);p+=n+12;}
 uLongf decoded=(width*channels*2+1)*height;Bytes raw(decoded);if(uncompress(raw.data(),&decoded,compressed.data(),compressed.size())!=Z_OK||decoded!=raw.size())throw std::runtime_error("PNG inflate failed");
 uLongf capacity=compressBound(raw.size());Bytes next(capacity);if(compress2(next.data(),&capacity,raw.data(),raw.size(),6)!=Z_OK)throw std::runtime_error("PNG deflate failed");next.resize(capacity);Bytes verify(raw.size());uLongf verified=verify.size();if(uncompress(verify.data(),&verified,next.data(),next.size())!=Z_OK||verified!=raw.size()||verify!=raw)throw std::runtime_error("Decoded byte equality failed");
 Bytes out(sig.begin(),sig.end());bool emitted=false;for(size_t p=8;p<in.size();){size_t n=be(in,p);bool data=in[p+4]=='I'&&in[p+5]=='D'&&in[p+6]=='A'&&in[p+7]=='T';if(data){if(!emitted){idat(out,next);emitted=true;}}else out.insert(out.end(),in.begin()+p,in.begin()+p+n+12);p+=n+12;}
 std::filesystem::create_directories(target.parent_path());std::ofstream of(target,std::ios::binary);of.write(reinterpret_cast<const char*>(out.data()),out.size());if(!of)throw std::runtime_error("Cannot write recompressed PNG");
 std::ostringstream crc;crc<<std::hex<<std::setfill('0')<<std::setw(8)<<crc32(0,raw.data(),raw.size());return {{"source",source.string()},{"target",target.string()},{"input_bytes",in.size()},{"output_bytes",out.size()},{"decoded_bytes",raw.size()},{"decoded_crc32",crc.str()},{"decoded_bytes_equal",true},{"non_IDAT_chunks_preserved",true}};
}
int main(int argc,char**argv){try{if(argc!=3)throw std::invalid_argument("usage: recompress_native_png input-file-or-dir output-file-or-dir");std::filesystem::path in=argv[1],out=argv[2];nlohmann::json report={{"generator","native C++ zlib recompression"},{"zlib_version",zlibVersion()},{"images",nlohmann::json::array()}};if(std::filesystem::is_directory(in)){for(const auto&e:std::filesystem::directory_iterator(in))if(e.is_regular_file()&&e.path().extension()==".png")report["images"].push_back(recompress(e.path(),out/e.path().filename()));}else report["images"].push_back(recompress(in,out));std::cout<<report.dump(2)<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
