#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
Mesh tetra(){Mesh m;m.positions={{1,1,1},{-1,-1,1},{-1,1,-1},{1,-1,-1}};for(auto p:m.positions)m.normals.push_back(normalized(p));m.resistance.resize(4,.5);m.indices={0,2,1,0,1,3,0,3,2,1,2,3};return m;}
int main(){try{auto m=tetra();CHECK(validate(m,2).accepted);
 for(int k=0;k<8;k++){auto a=m;if(k==0)std::swap(a.indices[0],a.indices[1]);if(k==1)a.indices.insert(a.indices.end(),{0,2,1});if(k==2)a.positions[0]=a.positions[1];if(k==3)a.positions[0].x=NAN;if(k==4)a.indices[0]=99;if(k==5)a.indices.resize(9);if(k==6)a.normals[0]=a.normals[0]*-1;if(k==7){for(unsigned j=1;j<4;j++){a.positions.push_back(m.positions[j]+Vec3{4,0,0});a.normals.push_back(m.normals[j]);a.resistance.push_back(.5);}for(auto i:m.indices)a.indices.push_back(i?i+3:0);}CHECK(!validate(a,2).accepted);}
 for(unsigned n:{64,128})for(Stage s:{Stage::Base,Stage::Stratified,Stage::Eroded}){Recipe r;auto ex=extract(makeField(r),makeGrid(r,n),s);auto v=validate(ex.mesh,r.size);if(!v.accepted)for(auto e:v.errors)std::cerr<<e<<'\n';CHECK(ex.ambiguousCells==0);CHECK(!ex.boundaryTouched);CHECK(v.accepted);CHECK(v.components==1);for(auto p:ex.mesh.positions){CHECK(std::abs(p.x)<=r.size*.5+1e-8);CHECK(std::abs(p.y)<=r.size*.35+1e-8);CHECK(std::abs(p.z)<=r.size*.4+1e-8);}std::cout<<n<<" stage "<<int(s)<<" vertices "<<ex.mesh.positions.size()<<" faces "<<ex.mesh.indices.size()/3<<" volume "<<v.signedVolume<<'\n';}
 Mesh tri;tri.positions={{0,0,0},{1,0,0},{0,1,0}};tri.indices={0,1,2};auto identical=compareSurfaces(tri,tri);CHECK(identical.maxDistance<1e-12);auto moved=tri;for(auto &p:moved.positions)p.z+=.1;auto diff=compareSurfaces(tri,moved);CHECK(std::abs(diff.maxDistance-.1)<1e-12);CHECK(std::abs(diff.rmsDistance-.1)<1e-12);CHECK(diff.sampleCount==8);
 CHECK(ambiguousCase(0x09));CHECK(!ambiguousCase(0x0f));std::cout<<"mesh fixtures: PASS\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
