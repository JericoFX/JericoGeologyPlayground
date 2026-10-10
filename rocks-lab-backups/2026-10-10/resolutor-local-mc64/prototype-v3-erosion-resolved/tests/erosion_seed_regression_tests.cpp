#include "rocks/mesh.hpp"
#include <iostream>
using namespace rocks;
int main(){unsigned failed=0;for(unsigned seed:{4,5,8,14,25,29,31})for(auto stage:{Stage::Base,Stage::Stratified,Stage::Eroded}){
 Recipe r;r.schemaVersion=2;r.generatorVersion="rocks-v3";r.extractorVersion="paris-mc-v1";r.family="irregular_beds";r.seed=seed;r.wear=0;r.detail=1;
 auto ex=extract(makeField(r),makeGrid(r,64),stage);auto q=validate(ex.mesh,r.size);
 bool ok=q.accepted&&!ex.boundaryTouched&&ex.unresolvedAmbiguousCells==0&&ex.resolvedAmbiguousCells==ex.ambiguousCells;
 std::cout<<"seed="<<seed<<" stage="<<int(stage)<<" accepted="<<ok<<" ambiguous="<<ex.ambiguousCells<<" unresolved="<<ex.unresolvedAmbiguousCells<<" degenerate="<<q.degenerateFaces<<" badVertices="<<q.badVertices<<" contractedEdges="<<ex.contractedEdges<<" distance="<<ex.maxContractionDistance<<'\n';failed+=!ok;
}return failed?1:0;}
