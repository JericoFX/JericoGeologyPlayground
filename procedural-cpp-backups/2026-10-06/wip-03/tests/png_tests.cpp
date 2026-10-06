#include "jerico/png.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <iterator>
#include <cmath>
#include <zlib.h>
using namespace jerico;
uint32_t be(const std::vector<unsigned char>&b,size_t i){return uint32_t(b.at(i))<<24|uint32_t(b.at(i+1))<<16|uint32_t(b.at(i+2))<<8|b.at(i+3);}
void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
int main(){try{
 std::string path="/tmp/jerico_png16_test.png";write_png16(path,2,2,{{.5,.5,1},{0,0,0},{1,.25,0},{.1,.2,.3}});
 std::ifstream f(path,std::ios::binary);require(bool(f),"native PNG encoder did not create file");std::vector<unsigned char>b((std::istreambuf_iterator<char>(f)),{});
 require(b.size()>50&&b[0]==137&&b[1]=='P'&&b[2]=='N'&&b[3]=='G',"PNG signature");require(be(b,16)==2&&be(b,20)==2&&b[24]==16&&b[25]==2,"PNG16 RGB header");
 std::vector<unsigned char>z;for(size_t i=8;i<b.size();){uint32_t len=be(b,i);if(b.at(i+4)=='I'&&b.at(i+5)=='D'&&b.at(i+6)=='A'&&b.at(i+7)=='T')z.insert(z.end(),b.begin()+i+8,b.begin()+i+8+len);i+=12+len;}
 require(z.size()>10&&z[0]==0x78,"zlib header");std::vector<unsigned char>raw(26);uLongf decoded=raw.size();require(uncompress(raw.data(),&decoded,z.data(),z.size())==Z_OK&&decoded==26,"independent zlib inflate");
 require(raw.size()==26,"PNG scanline byte count");require(raw[0]==0&&raw[1]==255&&raw[2]==255&&raw[3]==64&&raw[4]==0,"PNG top-down orientation and16bit values");require(raw[13]==0&&raw[14]==128&&raw[15]==0&&raw[16]==128&&raw[17]==0,"neutral normal16 retains32768");
 bool rejected=false;try{write_png16(path,0,1,{});}catch(const std::invalid_argument&){rejected=true;}require(rejected,"invalid PNG dimensions rejected");

 write_png16("/tmp/jerico_png16_alpha_test.png",1,1,{{1,1,1}},false,{.2});std::ifstream af("/tmp/jerico_png16_alpha_test.png",std::ios::binary);std::vector<unsigned char>ab((std::istreambuf_iterator<char>(af)),{});require(ab.at(25)==6,"RGBA16 specular alpha channel missing");

 std::vector<V3> flat(64*64,{.5,.5,1});write_png16("/tmp/jerico_png16_compressed_test.png",64,64,flat);std::ifstream cf("/tmp/jerico_png16_compressed_test.png",std::ios::binary|std::ios::ate);require(cf.tellg()<1000,"lossless PNG compression is missing");
 std::cout<<"PNG16 native encoder tests passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
