#include "rocks/mesh.hpp"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <array>
#include <iomanip>
using namespace rocks;using json=nlohmann::json;
static constexpr unsigned faces[6][4]={{0,1,3,2},{4,5,7,6},{0,1,5,4},{2,3,7,6},{0,2,6,4},{1,3,7,5}};
static Vec3 off(unsigned k){return {double(k&1),double((k>>1)&1),double((k>>2)&1)};}
static json vj(Vec3 v){return {v.x,v.y,v.z};}
int main(int argc,char**argv){json all=json::array();for(unsigned seed:{48,117,1,305,999})for(auto stage:{Stage::Base,Stage::Stratified}){
 Recipe r;r.schemaVersion=2;r.generatorVersion="rocks-v3";r.family="irregular_beds";r.extractorVersion="paris-mc-v1";r.wear=0;r.detail=1;r.seed=seed;auto f=makeField(r);auto grid=makeGrid(r,64);double h=(grid.max.x-grid.min.x)/grid.cells;unsigned N=65;std::vector<double> values(N*N*N);auto id=[&](unsigned x,unsigned y,unsigned z){return (z*N+y)*N+x;};
 for(unsigned z=0;z<N;z++)for(unsigned y=0;y<N;y++)for(unsigned x=0;x<N;x++)values[id(x,y,z)]=fieldValue(f,grid.min+Vec3{double(x),double(y),double(z)}*h,stage);
 json record={{"seed",seed},{"stage",int(stage)},{"voxelPitchMeters",h},{"cells",json::array()}};
 for(unsigned z=0;z<64;z++)for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){
  unsigned mask=0;std::array<double,8> vs;for(unsigned k=0;k<8;k++){vs[k]=values[id(x+(k&1),y+((k>>1)&1),z+((k>>2)&1))];if(vs[k]<0)mask|=1u<<k;}if(!mask||mask==255||!ambiguousCase(mask))continue;
  Vec3 lo=grid.min+Vec3{double(x),double(y),double(z)}*h,center=lo+Vec3{h*.5,h*.5,h*.5};json cell={{"gridIndex",{x,y,z}},{"mask",mask},{"min",vj(lo)},{"center",vj(center)},{"cornerValues",vs},{"fieldCenter",fieldValue(f,center,stage)},{"baseCenter",fieldValue(f,center,Stage::Base)},{"alternatingFaces",json::array()},{"refinement",json::array()}};
  for(unsigned face=0;face<6;face++){unsigned faceMask=0;Vec3 fc{};for(unsigned k=0;k<4;k++){faceMask|=((mask>>faces[face][k])&1)<<k;fc=fc+lo+off(faces[face][k])*h;}fc=fc/4.;if(faceMask==5||faceMask==10){auto a=vs[faces[face][0]],b=vs[faces[face][1]],c=vs[faces[face][2]],d=vs[faces[face][3]];cell["alternatingFaces"].push_back({{"face",face},{"corners",faces[face]},{"faceMask",faceMask},{"exactCenterValue",fieldValue(f,fc,stage)},{"bilinearCenterValue",(a+b+c+d)*.25},{"bilinearDeterminant",a*c-b*d}});}}
  std::vector<std::pair<double,unsigned>> active;for(unsigned i=0;i<f.planes.size();i++)active.push_back({planeValue(f.planes[i],center),i});std::sort(active.begin(),active.end(),std::greater<>());
  for(unsigned i=0;i<3;i++){auto [val,index]=active[i];cell["closestHostPlanes"].push_back({{"index",index},{"valueAtCenter",val},{"normal",vj(f.planes[index].normal)},{"offset",f.planes[index].offset}});}
  for(unsigned divisions:{2,4,8,16}){unsigned bad=0,activeCells=0;double sh=h/divisions;for(unsigned zz=0;zz<divisions;zz++)for(unsigned yy=0;yy<divisions;yy++)for(unsigned xx=0;xx<divisions;xx++){unsigned submask=0;for(unsigned k=0;k<8;k++){auto p=lo+(Vec3{double(xx),double(yy),double(zz)}+off(k))*sh;if(fieldValue(f,p,stage)<0)submask|=1u<<k;}if(submask&&submask!=255){activeCells++;bad+=ambiguousCase(submask);}}cell["refinement"].push_back({{"divisions",divisions},{"activeCells",activeCells},{"ambiguous",bad}});}
  record["cells"].push_back(cell);
 }
 auto ex=extract(f,grid,stage);auto valid=validate(ex.mesh,r.size);record["metrics"]={{"ambiguousCells",ex.ambiguousCells},{"meshValidatorAccepted",valid.accepted},{"triangles",ex.mesh.indices.size()/3},{"components",valid.components},{"degenerateFaces",valid.degenerateFaces},{"badEdges",valid.badEdges},{"badVertices",valid.badVertices},{"signedVolume",valid.signedVolume}};std::cout<<"seed="<<seed<<" stage="<<int(stage)<<" ambiguous="<<ex.ambiguousCells<<" meshValid="<<valid.accepted<<"\n";all.push_back(record);
 }
 std::ofstream(argc>1?argv[1]:"ambiguity.json")<<all.dump(2)<<'\n';
}
