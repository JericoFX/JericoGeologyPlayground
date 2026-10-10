#include "../core/src/face_resolution.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace rocks;using namespace rocks::detail;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 unsigned supported=0;
 for(unsigned mask=0;mask<256;mask++){
  auto face=simpleAmbiguousFace(mask);if(!face)continue;supported++;
  auto a=tableConnectsNegative(mask,*face),b=tableConnectsNegative(255-mask,*face);
  CHECK(a.has_value()&&b.has_value());
  // Complement swaps signs. Relative to ORIGINAL negative corners its
  // connectivity is the inverse of b, so these must be opposite alternatives.
  CHECK(*a==*b);
  // All referenced roots must lie on sign-changing cube edges, in both tables.
  for(unsigned lookup:{mask,255-mask}){
   auto table=marching_cube_tris[lookup];CHECK((table&15)>0);
   for(unsigned i=0;i<(table&15)*3;i++){unsigned e=(table>>(4+i*4))&15;CHECK(e<12);CHECK(((mask>>cubeEdges[e][0])&1)!=((mask>>cubeEdges[e][1])&1));}
  }
 }
 CHECK(supported==24);
 for(unsigned mask:{24,36,66,126,129,189,219,231,105,150})CHECK(!simpleAmbiguousFace(mask));
 auto negative=[](Vec3){return -1.;};CHECK(boundedSignedPath(negative,Vec3{0,0,0},Vec3{1,0,0},1,8.,1e-8).connected);
 CHECK(!boundedSignedPath(negative,Vec3{0,0,0},Vec3{1,0,0},-1,8.,1e-8).connected);
 CHECK(!boundedSignedPath(negative,Vec3{0,0,0},Vec3{1,0,0},1,8.,1e-8,2).connected);
 auto island=[](Vec3 p){return .1-4*(p.x-.5)*(p.x-.5);};CHECK(!boundedSignedPath(island,Vec3{0,0,0},Vec3{1,0,0},1,4.,1e-8).connected);
 auto nan=[](Vec3){return std::numeric_limits<double>::quiet_NaN();};CHECK(!boundedSignedPath(nan,Vec3{0,0,0},Vec3{1,0,0},1,8.,1e-8).connected);
 auto tie=[](Vec3){return -1e-12;};CHECK(!boundedSignedPath(tie,Vec3{0,0,0},Vec3{1,0,0},1,8.,1e-8).connected);
 Recipe r;r.schemaVersion=2;r.generatorVersion="rocks-v3";r.family="irregular_beds";r.extractorVersion="paris-mc-v1";r.wear=0;r.detail=1;auto f=makeField(r);
 CHECK(eligibleErosionResolver(f,makeGrid(r,64)));CHECK(!eligibleErosionResolver(f,makeGrid(r,128)));auto other=f;other.recipe.family="fractured_wall";CHECK(!eligibleErosionResolver(other,makeGrid(r,64)));
 for(auto version:{"rocks-v2","rocks-v4"}){auto outside=f;outside.recipe.generatorVersion=version;CHECK(!eligibleErosionResolver(outside,makeGrid(r,64)));}
 for(double wear:{.01,1.}){auto outside=f;outside.recipe.wear=wear;CHECK(!eligibleErosionResolver(outside,makeGrid(r,64)));}
 auto schema=f;schema.recipe.schemaVersion=1;CHECK(!eligibleErosionResolver(schema,makeGrid(r,64)));
 auto extractor=f;extractor.recipe.extractorVersion="polyhedral-v1";CHECK(!eligibleErosionResolver(extractor,makeGrid(r,64)));
 std::array<Vec3,4> crossing={Vec3{0,0,-.41},Vec3{0,.1,-.41},Vec3{0,.1,-.39},Vec3{0,0,-.39}};std::array<double,4> vals={-.1,.1,-.1,.1};CHECK(!decideErosionFace(f,Stage::Stratified,crossing,vals).negativeConnected);
 vals[0]=0;CHECK(!decideErosionFace(f,Stage::Base,crossing,vals).negativeConnected);
 std::cout<<"24 orientations/complements, edge roots, rejected ambiguity classes, nonfinite/tie/budget/interrupted-path/branch guards PASS\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
