#include "rocks/mesh.hpp"
#include <array>
#include <chrono>
#include <map>
#include <tuple>
#include <stdexcept>

namespace rocks {
namespace {
using Polygon=std::vector<Vec3>;
using Clock=std::chrono::steady_clock;

Polygon clip(const Polygon& polygon,Plane plane,double tolerance){
 Polygon result;
 if(polygon.empty())return result;
 for(std::size_t i=0;i<polygon.size();++i){
  Vec3 a=polygon[i],b=polygon[(i+1)%polygon.size()];
  double da=planeValue(plane,a),db=planeValue(plane,b);
  bool insideA=da<=tolerance,insideB=db<=tolerance;
  if(insideA)result.push_back(a);
  if(insideA!=insideB){
   double denominator=da-db;
   if(std::abs(denominator)>tolerance*.01){
    double t=std::clamp(da/denominator,0.,1.);
    result.push_back(a+(b-a)*t);
   }
  }
 }
 Polygon clean;
 for(auto p:result)if(clean.empty()||length(p-clean.back())>tolerance)clean.push_back(p);
 if(clean.size()>1&&length(clean.front()-clean.back())<=tolerance)clean.pop_back();
 return clean;
}
Vec3 areaVector(const Polygon&p){Vec3 area{};for(std::size_t i=1;i+1<p.size();++i)area=area+cross(p[i]-p[0],p[i+1]-p[0]);return area*.5;}
Vec3 centroid(const Polygon&p){Vec3 c{};for(auto v:p)c=c+v;return c/double(p.size());}
Plane canonical(Plane p){for(double component:{p.normal.x,p.normal.y,p.normal.z})if(std::abs(component)>1e-10){if(component<0){p.normal=p.normal*-1;p.offset=-p.offset;}break;}return p;}
}

Extraction extractPolyhedral(const RockField&field,Stage stage){
 const double size=field.recipe.size,tol=1e-10*size,weldTolerance=1e-9*size;
 if(field.recipe.wear!=0)throw std::invalid_argument("direct polyhedral route requires wear=0; curved wear is not planar");
 if(field.planes.empty())throw std::invalid_argument("direct polyhedral host requires planes");
 auto errors=validateRecipe(field.recipe,makeGrid(field.recipe,128));if(!errors.empty())throw std::invalid_argument(errors.front());
 auto start=Clock::now();Extraction result;
 std::vector<Plane> planes=field.planes;
 if(stage!=Stage::Base)for(const auto&mass:field.masses)planes.insert(planes.end(),mass.begin(),mass.end());
 if(stage!=Stage::Base)for(const auto&cut:field.fractures)planes.insert(planes.end(),cut.begin(),cut.end());
 std::vector<Plane> unique;
 for(auto plane:planes){
  if(!finite(plane.normal)||!std::isfinite(plane.offset)||std::abs(length(plane.normal)-1)>1e-9)throw std::invalid_argument("polyhedral plane is nonfinite or nonunit");
  plane=canonical(plane);bool found=false;
  for(auto q:unique)if(length(plane.normal-q.normal)<1e-11){double separation=std::abs(plane.offset-q.offset);if(separation<1e-13*size){found=true;break;}if(separation<1e-8*size)throw std::invalid_argument("near-coincident planes below direct-polyhedral feature tolerance");}
  if(!found)unique.push_back(plane);
 }
 if(unique.size()>64)throw std::invalid_argument("direct polyhedral comparison supports at most64 unique planes");
 std::vector<Polygon> boundary;
 for(auto plane:unique){
  Vec3 axis=std::abs(plane.normal.x)<.8?Vec3{1,0,0}:Vec3{0,1,0};
  Vec3 u=normalized(cross(plane.normal,axis)),v=cross(plane.normal,u),origin=plane.normal*plane.offset;
  Polygon face={origin-u*(2*size)-v*(2*size),origin+u*(2*size)-v*(2*size),origin+u*(2*size)+v*(2*size),origin-u*(2*size)+v*(2*size)};
  // A cutter contributes boundary only within the original convex host.
  for(auto hostPlane:field.planes)face=clip(face,hostPlane,tol);
  if(face.size()<3||length(areaVector(face))<1e-14*size*size)continue;
  std::vector<Polygon> cells{face};
  for(auto divider:unique){
   std::vector<Polygon> next;
   for(const auto&cell:cells){
    double low=INFINITY,high=-INFINITY;
    for(auto p:cell){double d=planeValue(divider,p);low=std::min(low,d);high=std::max(high,d);}
    if(low< -tol&&high>tol){
     auto a=clip(cell,divider,tol),b=clip(cell,{divider.normal*-1,-divider.offset},tol);
     for(auto*part:{&a,&b})if(part->size()>=3&&length(areaVector(*part))>1e-14*size*size)next.push_back(std::move(*part));
    }else next.push_back(cell);
   }
   cells=std::move(next);
   if(cells.size()>20000)throw std::runtime_error("polyhedral arrangement budget exceeded");
  }
  for(auto&cell:cells){
   auto t=Clock::now();Vec3 p=centroid(cell);double epsilon=1e-7*size;
   bool minus=fieldValue(field,p-plane.normal*epsilon,stage)<0,plus=fieldValue(field,p+plane.normal*epsilon,stage)<0;
   result.fieldMs+=std::chrono::duration<double,std::milli>(Clock::now()-t).count();
   if(minus==plus)continue;
   Vec3 outward=minus?plane.normal:plane.normal*-1;
   if(dot(areaVector(cell),outward)<0)std::reverse(cell.begin(),cell.end());
   boundary.push_back(std::move(cell));
  }
 }
 if(boundary.empty())throw std::invalid_argument("empty direct-polyhedral boundary; unsupported/empty solid");
 auto&mesh=result.mesh;
 using Key=std::tuple<long long,long long,long long>;
 std::map<Key,std::vector<std::uint32_t>> buckets;
 auto key=[&](Vec3 p){return Key{std::llround(p.x/weldTolerance),std::llround(p.y/weldTolerance),std::llround(p.z/weldTolerance)};};
 auto vertex=[&](Vec3 p){
  if(!finite(p)||std::max({std::abs(p.x),std::abs(p.y),std::abs(p.z)})>=.75*size)throw std::invalid_argument("direct boundary must remain inside padded finite domain");
  auto [x,y,z]=key(p);
  for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)for(int dz=-1;dz<=1;dz++){
   auto it=buckets.find({x+dx,y+dy,z+dz});if(it==buckets.end())continue;
   for(auto id:it->second)if(length(mesh.positions[id]-p)<=weldTolerance)return id;
  }
  auto id=std::uint32_t(mesh.positions.size());mesh.positions.push_back(p);buckets[{x,y,z}].push_back(id);return id;
 };
 std::vector<std::vector<std::uint32_t>> polygons;
 for(auto&face:boundary){std::vector<std::uint32_t> ids;for(auto p:face)ids.push_back(vertex(p));polygons.push_back(std::move(ids));}
 const auto boundaryVertexCount=mesh.positions.size();
 // Split every shared edge at all collinear boundary vertices. This closes
 // T-junctions introduced by a plane subdividing only one incident polygon.
 for(const auto&polygon:polygons){
  std::vector<std::uint32_t> edgeLoop;
  for(std::size_t i=0;i<polygon.size();i++){
   auto a=polygon[i],b=polygon[(i+1)%polygon.size()];Vec3 pa=mesh.positions[a],pb=mesh.positions[b],direction=pb-pa;double len=length(direction);
   if(len<=weldTolerance)throw std::runtime_error("collapsed polyhedral edge");
   std::vector<std::pair<double,std::uint32_t>> subdivisions{{0,a}};
   for(std::uint32_t j=0;j<boundaryVertexCount;j++){
    if(j==a||j==b)continue;
    Vec3 q=mesh.positions[j]-pa;double t=dot(q,direction)/(len*len);
    if(t>weldTolerance/len&&t<1-weldTolerance/len&&length(cross(q,direction))/len<=weldTolerance)subdivisions.push_back({t,j});
   }
   std::sort(subdivisions.begin(),subdivisions.end());for(auto[t,j]:subdivisions){(void)t;if(edgeLoop.empty()||edgeLoop.back()!=j)edgeLoop.push_back(j);}
  }
  Vec3 center{};for(auto j:edgeLoop)center=center+mesh.positions[j];center=center/double(edgeLoop.size());auto middle=vertex(center);
  for(std::size_t i=0;i<edgeLoop.size();i++)mesh.indices.insert(mesh.indices.end(),{middle,edgeLoop[i],edgeLoop[(i+1)%edgeLoop.size()]});
 }
 mesh.normals.assign(mesh.positions.size(),{});mesh.resistance.reserve(mesh.positions.size());
 for(std::size_t i=0;i<mesh.indices.size();i+=3){auto a=mesh.indices[i],b=mesh.indices[i+1],c=mesh.indices[i+2];Vec3 normal=cross(mesh.positions[b]-mesh.positions[a],mesh.positions[c]-mesh.positions[a]);for(auto id:{a,b,c}){mesh.normals[id]=mesh.normals[id]+normal;mesh.cornerNormals.push_back(normalized(normal));}}
 for(std::size_t i=0;i<mesh.positions.size();i++){
  if(length(mesh.normals[i])<1e-14*size*size)throw std::runtime_error("undefined polyhedral vertex normal");
  mesh.normals[i]=normalized(mesh.normals[i]);mesh.resistance.push_back(resistance(field,mesh.positions[i]));
 }
 result.coreBufferBytes=(mesh.positions.capacity()+mesh.normals.capacity()+mesh.cornerNormals.capacity())*sizeof(Vec3)+mesh.indices.capacity()*sizeof(std::uint32_t)+mesh.resistance.capacity()*sizeof(double);
 for(auto&p:boundary)result.coreBufferBytes+=p.capacity()*sizeof(Vec3);
 for(auto&p:polygons)result.coreBufferBytes+=p.capacity()*sizeof(std::uint32_t);
 result.extractNormalsMs=std::chrono::duration<double,std::milli>(Clock::now()-start).count()-result.fieldMs;
 if(field.recipe.generatorVersion=="rocks-v6"&&field.recipe.detail>0&&stage!=Stage::Base)applySurfaceDetail(result,field);
 return result;
}
// Select actual exposed sharp mesh edges, rather than drilling arbitrary
// points on broad faces. Every chip remains a finite convex subtraction.
void addEdgeChips(RockField&field){
 auto macro=extractPolyhedral(field,Stage::Stratified).mesh;
 struct Edge{std::uint32_t a,b;std::vector<Vec3> normals;double score=0;};
 std::map<std::pair<std::uint32_t,std::uint32_t>,Edge> edges;
 for(std::size_t i=0;i<macro.indices.size();i+=3){
  auto a=macro.positions[macro.indices[i]],b=macro.positions[macro.indices[i+1]],c=macro.positions[macro.indices[i+2]];auto normal=normalized(cross(b-a,c-a));
  for(int j=0;j<3;j++){auto u=macro.indices[i+j],v=macro.indices[i+(j+1)%3];auto key=std::minmax(u,v);auto& edge=edges[key];edge.a=key.first;edge.b=key.second;edge.normals.push_back(normal);}
 }
 auto random=[&](unsigned i){std::uint32_t x=field.recipe.seed+0x9e3779b9U*i;x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;x^=x>>16;return double(x)/4294967295.;};
 std::vector<Edge> candidates;const double size=field.recipe.size;
 for(auto [key,e]:edges){(void)key;if(e.normals.size()!=2||dot(e.normals[0],e.normals[1])>.92)continue;
  double len=length(macro.positions[e.a]-macro.positions[e.b]);if(len<.045*size)continue;
  e.score=len*(.5+random(e.a*31+e.b*17));candidates.push_back(e);
 }
 std::stable_sort(candidates.begin(),candidates.end(),[](const Edge&a,const Edge&b){return a.score>b.score;});
 std::vector<Vec3> selected;
 for(auto edge:candidates){
  if(selected.size()>=4)break;
  unsigned i=unsigned(selected.size());
  Vec3 a=macro.positions[edge.a],b=macro.positions[edge.b];Vec3 p=a+(b-a)*(.25+.5*random(1200+i));
  bool separated=true;for(auto q:selected)if(length(p-q)<.17*size)separated=false;if(!separated)continue;
  Vec3 n=normalized(edge.normals[0]+edge.normals[1]),u=normalized(b-a),v=normalized(cross(n,u));
  double amount=field.recipe.detail,span=(.038+.025*random(1210+i))*size*amount,depth=(.016+.012*random(1220+i))*size*amount;
  std::vector<Plane> cut;auto add=[&](Vec3 normal,double distance){double l=length(normal);cut.push_back({normal/l,(dot(normal,p)+distance)/l});};
  add(u-n*.28,span);add(u*-1-n*.18,span*.63);
  add(v+u*.23-n*.36,span*.71);add(v*-1-u*.11-n*.25,span*.49);
  add(n*-1,depth);field.fractures.push_back(std::move(cut));selected.push_back(p);
 }
}

}
