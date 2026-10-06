#include "rocks/mesh.hpp"
#include <array>
#include <chrono>
#include <map>
#include <numbers>
#include <stdexcept>

namespace rocks {
namespace {
using Edge=std::pair<std::uint32_t,std::uint32_t>;
struct Face{std::array<std::uint32_t,3> v;unsigned group;};
struct Segment{Vec3 a,b;};
double ridge(double phase){double t=phase/(2*std::numbers::pi);t-=std::floor(t);return 1-4*std::abs(t-.5);}
double smooth(double x){x=std::clamp(x,0.,1.);return x*x*(3-2*x);}
double segmentDistance(Vec3 p,Segment edge){Vec3 d=edge.b-edge.a;double t=std::clamp(dot(p-edge.a,d)/dot(d,d),0.,1.);return length(p-(edge.a+d*t));}
double random(std::uint32_t seed,unsigned i){std::uint32_t x=seed+0x9e3779b9U*i;x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;x^=x>>16;return double(x)/4294967295.;}
}

void applySurfaceDetail(Extraction&result,const RockField&field){
 auto start=std::chrono::steady_clock::now();auto&mesh=result.mesh;const double size=field.recipe.size;
 const double amplitude=.008*size*field.recipe.detail,targetEdge=.020*size,edgeFade=.030*size;
 std::vector<Plane> groups;std::vector<Face> faces;std::map<Edge,std::vector<Vec3>> incident;
 for(std::size_t i=0;i<mesh.indices.size();i+=3){
  std::array<std::uint32_t,3> ids{mesh.indices[i],mesh.indices[i+1],mesh.indices[i+2]};
  Vec3 a=mesh.positions[ids[0]],b=mesh.positions[ids[1]],c=mesh.positions[ids[2]],n=normalized(cross(b-a,c-a));double offset=dot(n,a);
  unsigned group=0;for(;group<groups.size();group++)if(dot(groups[group].normal,n)>1-1e-10&&std::abs(groups[group].offset-offset)<1e-8*size)break;
  if(group==groups.size())groups.push_back({n,offset});
  faces.push_back({ids,group});
  for(int j=0;j<3;j++)incident[std::minmax(ids[j],ids[(j+1)%3])].push_back(n);
 }
 std::vector<Segment> sharp;
 for(auto&[edge,normals]:incident)if(normals.size()==2&&dot(normals[0],normals[1])<1-1e-8)sharp.push_back({mesh.positions[edge.first],mesh.positions[edge.second]});
 if(sharp.empty())throw std::runtime_error("surface detail requires a closed faceted macro mesh");
 // Every midpoint is indexed once for both incident triangles. The 1/2/3
 // edge split cases preserve winding and never create hanging vertices.
 for(unsigned pass=0;pass<12;pass++){
  std::map<Edge,std::uint32_t> midpoints;
  for(auto face:faces)for(int j=0;j<3;j++){
   auto key=std::minmax(face.v[j],face.v[(j+1)%3]);if(midpoints.contains(key))continue;
   if(std::max(mesh.positions[key.first].z,mesh.positions[key.second].z)>-.08*size&&length(mesh.positions[key.first]-mesh.positions[key.second])>targetEdge){auto id=std::uint32_t(mesh.positions.size());mesh.positions.push_back((mesh.positions[key.first]+mesh.positions[key.second])*.5);midpoints[key]=id;}
  }
  if(midpoints.empty())break;
  std::vector<Face> refined;refined.reserve(faces.size()*3);
  for(auto face:faces){
   std::array<int,3> midpoint{-1,-1,-1};int count=0;
   for(int j=0;j<3;j++){auto it=midpoints.find(std::minmax(face.v[j],face.v[(j+1)%3]));if(it!=midpoints.end()){midpoint[j]=int(it->second);count++;}}
   auto add=[&](std::uint32_t a,std::uint32_t b,std::uint32_t c){refined.push_back({{a,b,c},face.group});};auto v=face.v;
   if(count==0){refined.push_back(face);continue;}
   if(count==1){int j=0;while(midpoint[j]<0)j++;auto m=std::uint32_t(midpoint[j]);add(v[j],m,v[(j+2)%3]);add(m,v[(j+1)%3],v[(j+2)%3]);}
   else if(count==2){int j=0;while(!(midpoint[j]>=0&&midpoint[(j+1)%3]>=0))j++;auto m=std::uint32_t(midpoint[j]),n=std::uint32_t(midpoint[(j+1)%3]);add(m,v[(j+1)%3],n);add(v[j],m,v[(j+2)%3]);add(m,n,v[(j+2)%3]);}
   else{auto a=std::uint32_t(midpoint[0]),b=std::uint32_t(midpoint[1]),c=std::uint32_t(midpoint[2]);add(v[0],a,c);add(a,v[1],b);add(c,b,v[2]);add(a,b,c);}
  }
  faces=std::move(refined);if(faces.size()>250000)throw std::runtime_error("surface detail refinement budget exceeded");
  if(pass==11)throw std::runtime_error("surface detail did not resolve requested wavelengths");
 }
 const auto rest=mesh.positions;
 const double wavelengths[6]={.18,.135,.09,.21,.12,.10},weights[6]={.27,.18,.10,.21,.15,.09};
 std::array<Vec3,6> waves;std::array<double,6> phases;double noiseDerivativeBound=0;
 for(unsigned i=0;i<6;i++){Vec3 direction{random(field.recipe.seed,1500+i*3)-.5,random(field.recipe.seed,1501+i*3)-.5,random(field.recipe.seed,1502+i*3)-.5};
  if(field.recipe.family=="strata")direction={direction.x*.32,.65+direction.y*.3,direction.z*.4};
  else if(field.recipe.family=="cliff")direction={.6+direction.x*.3,direction.y*.45,direction.z*.45};
  waves[i]=normalized(direction)*(2*std::numbers::pi/(wavelengths[i]*size));phases[i]=2*std::numbers::pi*random(field.recipe.seed,1600+i);noiseDerivativeBound+=weights[i]*length(waves[i]);}
 // A single fixed direction gives a rank-one displacement Jacobian. The
 // conservative global Lipschitz bound below is <1, hence the continuous
 // warp is injective. Triangle orientation and intersections remain audited
 // separately because a piecewise-linear sampled mesh is an approximation.
 const Vec3 direction=normalized({.13,.19,1});
 double gradientBound=amplitude*(noiseDerivativeBound+1.5/edgeFade+1.5/(.25*size));result.jacobianLowerBound=1-gradientBound;
 if(result.jacobianLowerBound<=0)throw std::runtime_error("surface-detail displacement loses injectivity bound");
 for(std::size_t i=0;i<rest.size();i++){
  Vec3 p=rest[i];double distance=INFINITY;for(auto edge:sharp)distance=std::min(distance,segmentDistance(p,edge));
  if(distance<=1e-9*size){result.preservedSharpVertices++;continue;}
  double exposure=smooth((p.z+.08*size)/(.25*size)),mask=smooth(distance/edgeFade),noise=0;
  for(unsigned k=0;k<6;k++)noise+=weights[k]*ridge(dot(waves[k],p)+phases[k]);
  Vec3 displacement=direction*(amplitude*exposure*mask*noise);mesh.positions[i]=p+displacement;result.maxDisplacement=std::max(result.maxDisplacement,length(displacement));
 }
 mesh.indices.clear();mesh.cornerNormals.clear();mesh.normals.assign(mesh.positions.size(),{});mesh.resistance.clear();
 std::map<std::pair<std::uint32_t,unsigned>,std::vector<Vec3>> groupNormals;std::vector<Vec3> faceNormals;
 for(auto face:faces){auto v=face.v;Vec3 before=cross(rest[v[1]]-rest[v[0]],rest[v[2]]-rest[v[0]]),after=cross(mesh.positions[v[1]]-mesh.positions[v[0]],mesh.positions[v[2]]-mesh.positions[v[0]]);if(dot(before,after)<=0)result.flippedTriangles++;
  faceNormals.push_back(normalized(after));for(auto id:v){mesh.indices.push_back(id);mesh.normals[id]=mesh.normals[id]+after;auto key=std::pair{id,face.group};groupNormals[key].push_back(after);}
 }
 if(result.flippedTriangles)throw std::runtime_error("surface detail inverted a triangle");
 for(std::size_t fi=0;fi<faces.size();fi++)for(auto id:faces[fi].v){Vec3 sum{};for(auto normal:groupNormals[{id,faces[fi].group}])if(dot(normalized(normal),faceNormals[fi])>.985)sum=sum+normal;mesh.cornerNormals.push_back(normalized(sum));}
 for(std::size_t i=0;i<mesh.positions.size();i++){mesh.normals[i]=normalized(mesh.normals[i]);mesh.resistance.push_back(resistance(field,mesh.positions[i]));}
 result.surfaceDetailMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 result.coreBufferBytes=(mesh.positions.capacity()+mesh.normals.capacity()+mesh.cornerNormals.capacity())*sizeof(Vec3)+mesh.indices.capacity()*sizeof(std::uint32_t)+mesh.resistance.capacity()*sizeof(double);
}
}
