#pragma once
#include "rocks/field.hpp"
#include "../../third_party/paris/mc_table.hpp"
#include <array>
#include <bit>
#include <optional>
#include <set>
#include <vector>

namespace rocks::detail {
inline constexpr unsigned cubeFaces[6][4]={{0,1,3,2},{4,5,7,6},{0,1,5,4},{2,3,7,6},{0,2,6,4},{1,3,7,5}};
inline constexpr unsigned cubeEdges[12][2]={{0,1},{2,3},{4,5},{6,7},{0,2},{1,3},{4,6},{5,7},{0,4},{1,5},{2,6},{3,7}};
// Only a face-diagonal pair (or its complement), never interior/multi-face cases.
inline std::optional<unsigned> simpleAmbiguousFace(unsigned mask){
 if(mask>255||(std::popcount(mask)!=2&&std::popcount(mask)!=6))return {};
 std::optional<unsigned> found;
 for(unsigned face=0;face<6;face++){
  unsigned bits=0;for(unsigned k=0;k<4;k++)bits|=((mask>>cubeFaces[face][k])&1)<<k;
  if(bits==5||bits==10){if(found)return {};found=face;}
 }return found;
}
// Decode actual face arcs from the pinned table, avoiding mask-specific rules.
inline std::optional<bool> tableConnectsNegative(unsigned mask,unsigned face){
 if(mask>255||face>=6)return {};
 std::array<unsigned,4> edge{};
 for(unsigned k=0;k<4;k++){
  auto a=cubeFaces[face][k],b=cubeFaces[face][(k+1)%4];bool found=false;
  if(((mask>>a)&1)==((mask>>b)&1))return {};
  for(unsigned e=0;e<12;e++)if((cubeEdges[e][0]==a&&cubeEdges[e][1]==b)||(cubeEdges[e][0]==b&&cubeEdges[e][1]==a)){edge[k]=e;found=true;break;}
  if(!found)return {};
 }
 std::set<std::pair<unsigned,unsigned>> arcs;auto conf=marching_cube_tris[mask];
 for(unsigned t=0;t<(conf&15);t++){
  std::vector<unsigned> onFace;
  for(unsigned k=0;k<3;k++){unsigned e=(conf>>(4+12*t+4*k))&15;for(unsigned j=0;j<4;j++)if(e==edge[j])onFace.push_back(e);}
  if(onFace.size()==2)arcs.insert(std::minmax(onFace[0],onFace[1]));
  else if(onFace.size()>2)return {};
 }
 const std::set<std::pair<unsigned,unsigned>> aroundZero={std::minmax(edge[3],edge[0]),std::minmax(edge[1],edge[2])};
 const std::set<std::pair<unsigned,unsigned>> aroundOne={std::minmax(edge[0],edge[1]),std::minmax(edge[2],edge[3])};
 bool zeroNegative=(mask>>cubeFaces[face][0])&1;
 if(arcs==aroundZero)return !zeroNegative;
 if(arcs==aroundOne)return zeroNegative;
 return {};
}
struct PathEvidence {bool connected=false;unsigned evaluations=0;double minimumClearance=INFINITY;};
// Conservative continuity bound plus an explicit floating-point safety margin.
// This is not interval arithmetic or a proof of absent hidden intra-cell loops.
// It certifies a signed diagonal under the existing MC single-edge-root model.
// Never changes samples, roots or the field; budget/tie/nonfinite => unresolved.
template<class Field> PathEvidence boundedSignedPath(Field field,Vec3 a,Vec3 b,int sign,double lipschitz,double guard,unsigned budget=16384,unsigned maxDepth=24){
 PathEvidence result;if((sign!=1&&sign!=-1)||!finite(a)||!finite(b)||!std::isfinite(lipschitz)||lipschitz<=0||!std::isfinite(guard)||guard<=0||budget==0)return result;
 for(Vec3 p:{a,b}){double value=sign*field(p);result.evaluations++;if(!std::isfinite(value)||value>=-guard)return result;}
 struct Span{Vec3 a,b;unsigned depth;};std::vector<Span> pending{{a,b,0}};
 while(!pending.empty()){
  if(result.evaluations>=budget)return result;
  auto span=pending.back();pending.pop_back();Vec3 middle=(span.a+span.b)*.5;double value=sign*field(middle);result.evaluations++;
  if(!std::isfinite(value)||value>=-guard)return result;
  double upper=value+lipschitz*length(span.b-span.a)*.5+guard;
  if(upper<0){result.minimumClearance=std::min(result.minimumClearance,-upper);continue;}
  if(span.depth>=maxDepth)return result;
  pending.push_back({middle,span.b,span.depth+1});pending.push_back({span.a,middle,span.depth+1});
 }
 result.connected=true;return result;
}
struct FaceDecision {std::optional<bool> negativeConnected;unsigned evaluations=0;double clearance=0;};
inline bool eligibleErosionResolver(const RockField&f,const GridSpec&grid){
 return f.recipe.schemaVersion==2&&f.recipe.generatorVersion=="rocks-v3"&&f.recipe.family=="irregular_beds"&&f.recipe.extractorVersion=="paris-mc-v1"&&f.recipe.wear==0&&grid.cells==64;
}
inline FaceDecision decideErosionFace(const RockField&f,Stage stage,const std::array<Vec3,4>&points,const std::array<double,4>&values){
 FaceDecision result;double guard=1e-8*f.recipe.size;unsigned bits=0;
 for(unsigned i=0;i<4;i++){if(!std::isfinite(values[i])||std::abs(values[i])<=guard||!finite(points[i]))return result;if(values[i]<0)bits|=1u<<i;}
 if(bits!=5&&bits!=10)return result;
 unsigned negative=(bits==5?0:1),positive=1-negative;
 double lo=points[0].z,hi=lo;for(auto p:points){lo=std::min(lo,p.z);hi=std::max(hi,p.z);}
 // Stage Base is exactly an intersection of half-spaces. The protected branch
 // is the same convex host, but only when the ENTIRE face belongs to it.
 if(stage==Stage::Base||hi<=-.20*f.recipe.size){result.negativeConnected=true;result.clearance=std::min(-values[negative],-values[negative+2])-guard;return result;}
 if(lo<=-.20*f.recipe.size)return result; // Do not cross the field's branch.
 // Valid only for the unchanged erosionStudyValue and original cubic fBm.
 // Cubic noise per-coordinate derivative <=3; normalized two-octave fBm <=4.
 // Anisotropic scales give LB<=28.703, LM<=62.208 in3D. Warp norm <=2.797;
 // pockets (p>=2, scaled by min axis) are1-Lipschitz; smooth min is nonexpansive.
 // Resistant-bed + detail terms give total cavity bound <5.229. Variable blend
 // width adds <=.1. L=8 leaves a conservative margin; size normalization cancels.
 auto field=[&](Vec3 p){return fieldValue(f,p,stage);};
 auto neg=boundedSignedPath(field,points[negative],points[negative+2],1,8.,guard);
 auto pos=boundedSignedPath(field,points[positive],points[positive+2],-1,8.,guard);
 result.evaluations=neg.evaluations+pos.evaluations;
 if(neg.connected==pos.connected)return result;
 result.negativeConnected=neg.connected;result.clearance=neg.connected?neg.minimumClearance:pos.minimumClearance;return result;
}
} // namespace rocks::detail
