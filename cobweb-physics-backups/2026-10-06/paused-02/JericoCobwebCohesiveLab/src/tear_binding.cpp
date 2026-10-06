#include "physics.hpp"
#include "cohesive_binding.hpp"
#include "damage.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <stdexcept>
namespace web::physics {
namespace {
double dot2(V2 a,V2 b){return a.x*b.x+a.y*b.y;}
double pointSegment(V2 p,V2 a,V2 b,double* tOut=nullptr){V2 d=b-a;double t=std::clamp(dot2(p-a,d)/std::max(1e-20,dot2(d,d)),0.,1.);if(tOut)*tOut=t;return length(p-(a+d*t));}
double cross2(V2 a,V2 b){return a.x*b.y-a.y*b.x;}
bool inTriangle(V2 p,V2 a,V2 b,V2 c){double x=cross2(b-a,p-a),y=cross2(c-b,p-b),z=cross2(a-c,p-c);return (x>=-1e-10&&y>=-1e-10&&z>=-1e-10)||(x<=1e-10&&y<=1e-10&&z<=1e-10);}
bool capsuleTriangle(V2 x,V2 y,V2 a,V2 b,V2 c){constexpr double radius=.006;for(V2 p:{a,b,c})if(pointSegment(p,x,y)<radius)return true;for(int i=0;i<=32;++i){V2 p=x+(y-x)*(i/32.);if(inTriangle(p,a,b,c)||std::min({pointSegment(p,a,b),pointSegment(p,b,c),pointSegment(p,c,a)})<radius)return true;}return false;}
V2 localUV(const Lab& lab,int patch,V2 uv){int grid=tileGrid(lab);double tile=double(lab.recipe.atlas)/grid;return{(uv.x*lab.recipe.atlas-(patch%grid*tile+3))/(tile-6),(uv.y*lab.recipe.atlas-(patch/grid*tile+3))/(tile-6)};}
int nearestEdge(const Graph& g,V2 uv,bool activeOnly=true){int best=-1;double score=1e100;for(int i=0;i<int(g.edges.size());++i){const auto& e=g.edges[i];if(activeOnly&&!e.active)continue;double d=pointSegment(uv,g.particles[e.a].uv,g.particles[e.b].uv);if(d<score){score=d;best=i;}}return best;}
V3 deformation(const Graph& g,int edge,V2 uv){if(edge<0)return {};const auto& e=g.edges[edge];double t=0;pointSegment(uv,g.particles[e.a].uv,g.particles[e.b].uv,&t);return (g.particles[e.a].x-g.particles[e.a].rest)*(1-t)+(g.particles[e.b].x-g.particles[e.b].rest)*t;}
bool inCut(const Graph& g,V2 uv){for(const auto& e:g.edges)if(!e.active&&pointSegment(uv,g.particles[e.a].uv,g.particles[e.b].uv)<.008)return true;return false;}
Lab localLab(const Lab& lab,int patch,int atlas){Lab local;local.recipe=lab.recipe;local.recipe.atlas=atlas;local.patches={lab.patches[patch]};local.patches[0].id=0;local.anchors=lab.anchors;local.nodes=lab.nodes;for(auto f:lab.fibers)if(f.patch==patch){f.patch=0;local.fibers.push_back(f);}return local;}
std::ofstream output(const std::filesystem::path& p,bool binary=false){std::ofstream s(p,binary?std::ios::binary:std::ios::out);if(!s)throw std::runtime_error("cannot write "+p.string());s<<std::setprecision(17);return s;}
void point(std::ostream& s,V3 p){s<<'['<<p.x<<','<<p.y<<','<<p.z<<']';}
}
V3 deformationAt(const Graph& g,V2 uv){
 const int nearest=nearestEdge(g,uv);if(nearest<0)return {};
 const int component=g.particles[g.edges[nearest].a].component;
 // Blend only local active supports in the chosen connected component. This
 // removes nearest-edge seams without mixing newly separated material pieces.
 std::array<std::pair<double,int>,8> closest;for(auto& item:closest)item={1e100,-1};
 for(int i=0;i<int(g.edges.size());++i){const auto& e=g.edges[i];if(!e.active||g.particles[e.a].component!=component)continue;double distance=pointSegment(uv,g.particles[e.a].uv,g.particles[e.b].uv);if(distance>=closest.back().first)continue;closest.back()={distance,i};for(int k=7;k>0&&closest[k].first<closest[k-1].first;--k)std::swap(closest[k],closest[k-1]);}
 V3 result{};double sum=0;for(auto [distance,edge]:closest)if(edge>=0){double weight=1/(distance*distance+1e-10);result=result+deformation(g,edge,uv)*weight;sum+=weight;}
 return sum>0?result*(1/sum):V3{};
}
bool segmentCrossesCut(const Graph& g,V2 a,V2 b){
 for(const auto& e:g.edges)if(!e.active){V2 c=g.particles[e.a].uv,d=g.particles[e.b].uv;double x=cross2(b-a,c-a),y=cross2(b-a,d-a),u=cross2(d-c,a-c),v=cross2(d-c,b-c);
  if(((x<0&&y>0)||(x>0&&y<0))&&((u<0&&v>0)||(u>0&&v<0)))return true;
  if(std::min({pointSegment(a,c,d),pointSegment(b,c,d),pointSegment(c,a,b),pointSegment(d,a,b)})<.008)return true;
 }
 return false;
}
TearView bindView(const Lab& lab,const Graph& g,int patch,int atlas){
 if(g.materialTears.enabled)return bindCohesiveMaterial(lab,g,patch,atlas);
 if(patch!=0)throw std::invalid_argument("MVP visual binding currently validates corridor patch 0 only");
 if(atlas<64||atlas>1024)throw std::invalid_argument("preview atlas 64..1024 required");
 auto local=localLab(lab,patch,atlas); // Remap original fiber IDs for bake's strand array without changing strand groups.
 std::map<int,int> strands;for(auto& f:local.fibers){int key=f.strand<0?f.id:f.strand;if(!strands.count(key))strands[key]=int(strands.size());f.strand=strands[key];}
 auto base=bake(local);TearView view;view.atlas=atlas;view.coverage.resize(base.coverage.size());for(size_t i=0;i<base.coverage.size();++i)view.coverage[i]=static_cast<unsigned char>(std::round(std::clamp(base.coverage[i],0.f,1.f)*255));
 std::map<int,int> remap;std::vector<V2> uv;std::vector<std::array<V2,3>> removed;
 int grid=tileGrid(lab);
 for(int i=0;i<int(lab.mesh.vertices.size());++i){const auto& v=lab.mesh.vertices[i];int owner=int(v.uv.y*grid)*grid+int(v.uv.x*grid);if(owner!=patch)continue;V2 p=localUV(lab,patch,v.uv);int support=nearestEdge(g,p);int index=int(view.mesh.vertices.size());remap[i]=index;uv.push_back(p);int particle=support>=0?g.edges[support].a:-1;view.supportParticle.push_back(particle);view.mesh.vertices.push_back({v.p+deformationAt(g,p),v.n,atlasUV(local,0,p)});}
 for(int i=0;i<int(lab.mesh.faces.size());++i){auto original=lab.mesh.faces[i];if(!remap.count(original[0]))continue;std::array<int,3> f{remap.at(original[0]),remap.at(original[1]),remap.at(original[2])};V2 a=uv[f[0]],b=uv[f[1]],c=uv[f[2]];bool remove=false;
  for(const auto& e:g.edges)if(!e.active&&capsuleTriangle(g.particles[e.a].uv,g.particles[e.b].uv,a,b,c)){remove=true;break;}
  int component=-1;for(int vi:f){int pi=view.supportParticle[vi];if(pi<0){remove=true;break;}if(component<0)component=g.particles[pi].component;else if(component!=g.particles[pi].component)remove=true;}
  V3 x=view.mesh.vertices[f[0]].p,y=view.mesh.vertices[f[1]].p,z=view.mesh.vertices[f[2]].p;if(length(cross(y-x,z-x))<1e-12)remove=true;
  if(remove)removed.push_back({a,b,c});else{view.mesh.faces.push_back(f);view.sourceFaces.push_back(i);}
 }
 // Atlas and actual deleted faces share the same exact material-space holes.
 for(const auto& tri:removed){double minx=std::min({tri[0].x,tri[1].x,tri[2].x}),maxx=std::max({tri[0].x,tri[1].x,tri[2].x}),miny=std::min({tri[0].y,tri[1].y,tri[2].y}),maxy=std::max({tri[0].y,tri[1].y,tri[2].y});int x0=std::max(0,int(std::floor(3+minx*(atlas-6)))-1),x1=std::min(atlas-1,int(std::ceil(3+maxx*(atlas-6)))+1),y0=std::max(0,int(std::floor(3+miny*(atlas-6)))-1),y1=std::min(atlas-1,int(std::ceil(3+maxy*(atlas-6)))+1);
  for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){V2 p{(x+.5-3)/(atlas-6),(y+.5-3)/(atlas-6)};if(inTriangle(p,tri[0],tri[1],tri[2]))view.coverage[size_t(y)*atlas+x]=0;}}
 return view;
}
void exportFrame(const Lab& lab,const Graph& g,int patch,const std::string& directory,int atlas,const Sphere& sphere){
 std::filesystem::path dir(directory);if(std::filesystem::exists(dir))throw std::runtime_error("immutable frame output already exists: "+dir.string());std::filesystem::create_directories(dir);auto view=bindView(lab,g,patch,atlas);
 auto obj=output(dir/"mesh.obj");obj<<"# Actual deformed carrier; collision uses active silk graph only\no TornSilkCarrier\n";for(auto v:view.mesh.vertices)obj<<"v "<<v.p.x<<' '<<v.p.y<<' '<<v.p.z<<'\n';for(auto v:view.mesh.vertices)obj<<"vt "<<v.uv.x<<' '<<v.uv.y<<'\n';for(auto f:view.mesh.faces)obj<<"f "<<f[0]+1<<'/'<<f[0]+1<<' '<<f[1]+1<<'/'<<f[1]+1<<' '<<f[2]+1<<'/'<<f[2]+1<<'\n';
 auto tga=output(dir/"coverage.tga",true);unsigned char h[18]={};h[2]=2;h[12]=atlas&255;h[13]=atlas>>8;h[14]=h[12];h[15]=h[13];h[16]=32;h[17]=8;tga.write(reinterpret_cast<char*>(h),18);for(auto v:view.coverage){unsigned char p[4]={v,v,v,255};tga.write(reinterpret_cast<char*>(p),4);}
 auto json=output(dir/"frame.json");json<<"{\"solver\":\""<<solverVersion<<"\",\"tick\":"<<g.tick<<",\"particles\":[";bool first=true;for(auto p:g.particles){if(!first)json<<',';first=false;json<<"{\"id\":"<<p.id<<",\"p\":";point(json,p.x);json<<",\"rest\":";point(json,p.rest);json<<",\"uv\":["<<p.uv.x<<','<<p.uv.y<<"],\"anchor\":"<<p.anchor<<",\"component\":"<<p.component<<",\"source_nodes\":[";for(size_t i=0;i<p.sourceNodes.size();++i){if(i)json<<',';json<<p.sourceNodes[i];}json<<"]}";}
 json<<"],\"edges\":[";first=true;for(auto e:g.edges){if(!first)json<<',';first=false;json<<"{\"id\":"<<e.id<<",\"a\":"<<e.a<<",\"b\":"<<e.b<<",\"kind\":"<<e.kind<<",\"width\":"<<e.width<<",\"active\":"<<(e.active?"true":"false")<<",\"load\":"<<e.load<<'}';}
 json<<"],\"events\":[";first=true;for(auto e:g.events){if(!first)json<<',';first=false;json<<"{\"tick\":"<<e.tick<<",\"edge\":"<<e.edge<<",\"uv\":["<<e.uv.x<<','<<e.uv.y<<"],\"cause\":\""<<e.cause<<"\"}";}
 json<<"],\"curves\":[";first=true;auto curve=[&](int kind,double width,const std::vector<V3>& p){if(p.size()<2)return;if(!first)json<<',';first=false;json<<"{\"kind\":"<<kind<<",\"width\":"<<width<<",\"points\":[";for(size_t i=0;i<p.size();++i){if(i)json<<',';point(json,p[i]);}json<<"]}";};
 for(const auto& e:g.edges)if(e.active)curve(e.kind,e.width,{g.particles[e.a].x,g.particles[e.b].x});
 Graph cuts;cuts.particles=g.particles;for(const auto& e:g.edges)if(!e.active)cuts.edges.push_back(e);
 // Decorative fine fibers are component-bound visual detail, not independent colliders.
 for(const auto& f:lab.fibers)if(f.patch==patch&&!f.structural){std::vector<V3> chunk;int previous=-1;V2 previousUV{};for(auto uv:f.uv){int support=nearestEdge(g,uv);bool visible=support>=0&&!inCut(cuts,uv)&&damage(uv,lab.recipe.broken,lab.recipe.seed,patch,true,lab.patches[patch].layer)>.1;int component=support>=0?g.particles[g.edges[support].a].component:-1;
   if(!visible||(previous>=0&&component!=previous)||(!chunk.empty()&&(segmentCrossesCut(cuts,previousUV,uv)||damageMayIntersect(previousUV,uv,lab.recipe.broken,lab.recipe.seed,patch,true,lab.patches[patch].layer)))){curve(f.width<.6?0:1,lab.recipe.width*f.width,chunk);chunk.clear();}if(visible)chunk.push_back(lab.patches[patch].sample(uv)+deformationAt(g,uv));previous=component;previousUV=uv;}
  curve(f.width<.6?0:1,lab.recipe.width*f.width,chunk);}
 json<<"]";if(sphere.enabled){json<<",\"sphere\":{\"center\":";point(json,sphere.center);json<<",\"radius\":"<<sphere.radius<<'}';}json<<",\"mesh_faces\":"<<view.mesh.faces.size()<<",\"note\":\"Artistic VFX profiles; decorative fibers have representative support/contact only. Carrier is never a solid collider.\"}\n";
 auto state=output(dir/"checkpoint.txt");state<<saveState(g);
 if(!obj||!tga||!json||!state)throw std::runtime_error("frame export failed");
}
}
