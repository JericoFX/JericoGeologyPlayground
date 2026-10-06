#include "cobweb.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
int main(int argc,char** argv){if(argc!=2)return 2;web::Recipe r;r.preset="corridor";r.density=.9;r.width=.0045;r.broken=0;r.atlas=512;r.quality=20;auto lab=web::generate(r);std::ofstream out(argv[1]);out<<std::setprecision(17)<<441<<' '<<800<<'\n';int grid=web::tileGrid(lab);double tile=double(r.atlas)/grid;for(int i=0;i<441;++i){auto v=lab.mesh.vertices[i];v.uv={(v.uv.x*r.atlas-3)/(tile-6),(v.uv.y*r.atlas-3)/(tile-6)};out<<v.p.x<<' '<<v.p.y<<' '<<v.p.z<<' '<<v.n.x<<' '<<v.n.y<<' '<<v.n.z<<' '<<v.uv.x<<' '<<v.uv.y<<'\n';}for(int i=0;i<800;++i){auto f=lab.mesh.faces[i];out<<f[0]<<' '<<f[1]<<' '<<f[2]<<'\n';}std::cout<<"Frozen original patch0: 441 vertices, 800 faces; local UV, original 3D and normals\n";return out?0:1;}
