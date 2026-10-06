#include "cohesive_binding.hpp"
#include "uv_cage.hpp"
#include "uv_clip.hpp"
#include "material_tears.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <tuple>
namespace web::physics {
namespace {
Atlas patchAtlas(const Lab& lab,int patch,int atlas){
 Lab local;local.recipe=lab.recipe;local.recipe.atlas=atlas;local.patches={lab.patches.at(patch)};local.patches[0].id=0;
 std::map<int,int> strands;
 for(auto f:lab.fibers)if(f.patch==patch){f.patch=0;int key=f.strand<0?f.id:f.strand;if(!strands.count(key))strands[key]=int(strands.size());f.strand=strands[key];local.fibers.push_back(std::move(f));}
 return bake(local);
}
using Bary=std::array<double,3>;
}
TearView bindCohesiveMaterial(const Lab& lab,const Graph& graph,int patch,int atlas){
 if(!graph.cohesive.enabled)throw std::invalid_argument("cohesive material binding requires rebuilt cohesive asset metadata");
 if(patch!=0||atlas<64||atlas>1024)throw std::invalid_argument("cohesive material binding validates patch0 and atlas64..1024");
 const auto cage=buildRestCage(graph);Mesh source;std::vector<int> owners,sourceCageFaces;
 std::map<std::tuple<int,long long,long long>,int> vertices;
 auto vertex=[&](const std::array<int,3>& tri,Bary b,int owner){
  V2 uv{};V3 rest{},delta{};double total=0;
  for(int k=0;k<3;++k){uv=uv+cage.uv()[tri[k]]*b[k];rest=rest+cage.rest()[tri[k]]*b[k];if(graph.particles[tri[k]].component==owner){delta=delta+(graph.particles[tri[k]].x-graph.particles[tri[k]].rest)*b[k];total+=b[k];}}
  if(total<=1e-14)throw std::runtime_error("material face has no support in its component");
  V3 position=rest+delta*(1/total);auto key=std::make_tuple(owner,std::llround(uv.x*1e12),std::llround(uv.y*1e12));auto it=vertices.find(key);
  if(it!=vertices.end()){if(length(source.vertices[it->second].p-position)>1e-7)throw std::runtime_error("cohesive material seam disagrees in world position");return it->second;}
  int id=int(source.vertices.size());source.vertices.push_back({position,{0,0,1},uv});vertices[key]=id;return id;
 };
 auto polygon=[&](int face,const std::array<int,3>& tri,int owner,const std::vector<Bary>& points){
  std::vector<int> ids;for(auto b:points)ids.push_back(vertex(tri,b,owner));int support=-1;for(int p:tri)if(graph.particles[p].component==owner){support=p;break;}
  for(size_t k=1;k+1<ids.size();++k){source.faces.push_back({ids[0],ids[k],ids[k+1]});owners.push_back(support);sourceCageFaces.push_back(face);}
 };
 for(int fi=0;fi<int(cage.triangles().size());++fi){auto tri=cage.triangles()[fi];int c0=graph.particles[tri[0]].component,c1=graph.particles[tri[1]].component,c2=graph.particles[tri[2]].component;
  if(c0==c1&&c1==c2)polygon(fi,tri,c0,{{1,0,0},{0,1,0},{0,0,1}});
  else for(int i=0;i<3;++i){Bary corner{},next{},previous{};corner[i]=1;next[i]=next[(i+1)%3]=.5;previous[i]=previous[(i+2)%3]=.5;polygon(fi,tri,graph.particles[tri[i]].component,{corner,next,{1./3,1./3,1./3},previous});}
 }
 if(source.faces.size()>8400||source.vertices.size()>25200)throw std::runtime_error("component-partitioned carrier exceeded render budget");
 std::vector<uvclip::ConvexCut> cuts;for(const auto& cut:graph.materialTears.cuts)cuts.push_back(cut.polygon);
 auto clipped=uvclip::subtract(source,cuts);TearView result;result.mesh=std::move(clipped.mesh);result.atlas=atlas;result.supportParticle.assign(result.mesh.vertices.size(),-1);
 for(size_t i=0;i<result.mesh.faces.size();++i){const int base=int(clipped.sourceFaces[i]),support=owners.at(base);result.sourceFaces.push_back(sourceCageFaces.at(base));for(int vi:result.mesh.faces[i]){int prior=result.supportParticle[vi];if(prior>=0&&graph.particles[prior].component!=graph.particles[support].component)throw std::runtime_error("clipping joined separate physical components");result.supportParticle[vi]=support;}}
 auto base=patchAtlas(lab,patch,atlas);result.coverage.resize(base.coverage.size());
 for(int y=0;y<atlas;++y)for(int x=0;x<atlas;++x){V2 uv{(x+.5-3)/(atlas-6),(y+.5-3)/(atlas-6)};size_t i=size_t(y)*atlas+x;double alpha=pointInMaterialCut(graph,uv)?0:base.coverage[i];result.coverage[i]=static_cast<unsigned char>(std::round(std::clamp(alpha,0.,1.)*255));}
 for(auto& v:result.mesh.vertices)v.uv={(3+v.uv.x*(atlas-6))/atlas,(3+v.uv.y*(atlas-6))/atlas};
 return result;
}
}
