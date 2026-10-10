#include "rocks/mesh.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace rocks;
int main(int argc,char**argv){if(argc!=4)return 2;Recipe r;r.schemaVersion=2;r.generatorVersion="rocks-v3";r.family="irregular_beds";r.extractorVersion="paris-mc-v1";r.wear=0;r.detail=1;r.seed=std::stoul(argv[1]);auto stage=Stage(std::stoi(argv[2]));auto ex=extract(makeField(r),makeGrid(r,64),stage);auto v=validate(ex.mesh,r.size);std::ofstream out(argv[3]);out<<"# DIAGNOSTIC ONLY: old rejected MC connectivity; rawAmbiguous="<<ex.ambiguousCells<<"\n"<<std::setprecision(17);for(auto p:ex.mesh.positions)out<<"v "<<p.x<<' '<<p.y<<' '<<p.z<<'\n';for(auto n:ex.mesh.normals)out<<"vn "<<n.x<<' '<<n.y<<' '<<n.z<<'\n';for(size_t i=0;i<ex.mesh.indices.size();i+=3){out<<"f";for(unsigned k=0;k<3;k++){auto x=ex.mesh.indices[i+k]+1;out<<' '<<x<<"//"<<x;}out<<'\n';}std::cout<<"rawAmbiguous="<<ex.ambiguousCells<<" topologyAccepted="<<v.accepted<<'\n';}
