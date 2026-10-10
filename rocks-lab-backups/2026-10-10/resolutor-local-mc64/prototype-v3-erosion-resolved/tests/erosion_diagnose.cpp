#include "rocks/mesh.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
using namespace rocks;
int main(int argc,char**argv){Recipe r;r.schemaVersion=2;r.generatorVersion="rocks-v3";r.family="irregular_beds";r.extractorVersion="paris-mc-v1";r.wear=0;r.detail=1;auto f=makeField(r);auto g=makeGrid(r,64);double h=(g.max.x-g.min.x)/g.cells;
for(unsigned z=0;z<64;z++)for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){unsigned mask=0;for(unsigned k=0;k<8;k++){auto p=g.min+Vec3{double(x+(k&1)),double(y+((k>>1)&1)),double(z+((k>>2)&1))}*h;if(fieldValue(f,p,Stage::Stratified)<0)mask|=1u<<k;}if(ambiguousCase(mask)&&mask&&mask!=255){auto p=g.min+Vec3{x+.5,y+.5,z+.5}*h;std::cout<<"ambiguous "<<mask<<" center "<<p.x<<","<<p.y<<","<<p.z<<" host="<<fieldValue(f,p,Stage::Base)<<" eroded="<<fieldValue(f,p,Stage::Stratified)<<"\n";}}
if(argc>1){auto ex=extract(f,g,Stage::Stratified);auto q=validate(ex.mesh,r.size);std::ofstream out(argv[1]);out<<"# REJECTED DIAGNOSTIC GEOMETRY: ambiguous="<<ex.ambiguousCells<<" topologyAccepted="<<q.accepted<<"\n"<<std::setprecision(17);for(auto v:ex.mesh.positions)out<<"v "<<v.x<<" "<<v.y<<" "<<v.z<<"\n";for(auto n:ex.mesh.normals)out<<"vn "<<n.x<<" "<<n.y<<" "<<n.z<<"\n";for(size_t i=0;i<ex.mesh.indices.size();i+=3){out<<"f";for(unsigned j=0;j<3;j++){auto id=ex.mesh.indices[i+j]+1;out<<" "<<id<<"//"<<id;}out<<"\n";}}}
