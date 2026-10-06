#include "impact.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace web;
namespace fs=std::filesystem;
namespace {
double dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
V3 rotate(V3 v,V3 axis,double angle){axis=normalized(axis);return v*std::cos(angle)+cross(axis,v)*std::sin(angle)+axis*(dot(axis,v)*(1-std::cos(angle)));}
std::ofstream file(const fs::path& p,bool binary=false){std::ofstream f(p,binary?std::ios::binary:std::ios::out);if(!f)throw std::runtime_error("cannot write "+p.string());f<<std::setprecision(17);return f;}
void vector(std::ostream& f,V3 v){f<<'['<<v.x<<','<<v.y<<','<<v.z<<']';}
void obj(const Mesh& m,const fs::path& p){auto f=file(p);f<<"o WebCarrier\n";for(auto v:m.vertices)f<<"v "<<v.p.x<<' '<<v.p.y<<' '<<v.p.z<<'\n';for(auto v:m.vertices)f<<"vt "<<v.uv.x<<' '<<v.uv.y<<'\n';for(auto t:m.faces)f<<"f "<<t[0]+1<<'/'<<t[0]+1<<' '<<t[1]+1<<'/'<<t[1]+1<<' '<<t[2]+1<<'/'<<t[2]+1<<'\n';}
void tga(const Atlas& a,const fs::path& p){auto f=file(p,true);unsigned char h[18]={};h[2]=2;h[12]=a.size&255;h[13]=a.size>>8;h[14]=h[12];h[15]=h[13];h[16]=32;h[17]=8;f.write(reinterpret_cast<char*>(h),18);for(auto c:a.coverage){auto v=static_cast<unsigned char>(std::round(std::clamp(c,0.f,1.f)*255));unsigned char px[4]={v,v,v,255};f.write(reinterpret_cast<char*>(px),4);}}
Lab panelAsset(const Recipe& recipe){auto all=generate(recipe);Lab local;local.recipe=recipe;local.patches={all.patches[0]};local.patches[0].id=0;local.mainPatches=1;std::map<int,int> strands,vertices;for(auto f:all.fibers)if(f.patch==0){int key=f.strand<0?f.id:f.strand;if(!strands.count(key))strands[key]=int(strands.size());f.strand=strands[key];local.fibers.push_back(std::move(f));}int grid=tileGrid(all);double tile=double(recipe.atlas)/grid;for(int i=0;i<int(all.mesh.vertices.size());++i){auto v=all.mesh.vertices[i];if(int(v.uv.x*grid)!=0||int(v.uv.y*grid)!=0)continue;V2 uv{(v.uv.x*recipe.atlas-3)/(tile-6),(v.uv.y*recipe.atlas-3)/(tile-6)};v.uv=atlasUV(local,0,uv);vertices[i]=int(local.mesh.vertices.size());local.mesh.vertices.push_back(v);}for(auto f:all.mesh.faces)if(vertices.count(f[0]))local.mesh.faces.push_back({vertices.at(f[0]),vertices.at(f[1]),vertices.at(f[2])});for(auto a:all.anchors)for(auto c:local.patches[0].corners)if(length(a.position-c)<1e-10){local.anchors.push_back(a);break;}return local;}
V2 materialUV(V2 atlas,int size){return{(atlas.x*size-3)/(size-6),(atlas.y*size-3)/(size-6)};}
Mesh deformedMesh(const Lab& lab,const impact::Panel& panel){Mesh mesh;if(panel.state().phase==impact::Phase::Detached)return mesh;mesh=lab.mesh;if(panel.state().dent==0 && (panel.settings().windAmplitude==0 || length(panel.settings().windVelocity)==0))return mesh;for(auto& v:mesh.vertices)v.p=impact::deformedPoint(lab.patches[0],materialUV(v.uv,lab.recipe.atlas),panel.state(),panel.settings());return mesh;}
struct Stone {V3 center{};double radius=.18,angle=0;V3 axis{.6,1,.2};};
Mesh remnants(const Stone& stone,V3 normal,double attachAngle,int atlas){Mesh m;V3 up{0,0,1},side=normalized(cross(up,normal));up=normalized(cross(normal,side));
 const std::array<V3,4> directions{{normal,normalized(normal*.35+side),normalized(normal*.4-side*.9+up*.2),normalized(normal*.3+up)}};
 // Sparse crops from the same seed42 atlas: visible threads plus >70% gaps.
 const std::array<V2,4> centers{{V2{.72,.20},V2{.60,.48},V2{.44,.80},V2{.72,.68}}};
 const std::array<V2,4> cropSize{{V2{.28,.15},V2{.19,.15},V2{.19,.22},V2{.19,.15}}};
 for(int piece=0;piece<4;++piece){V3 n=rotate(directions[piece],stone.axis,-attachAngle);V3 tangent=normalized(cross(std::abs(n.z)>.9?V3{0,1,0}:V3{0,0,1},n));V3 vertical=normalized(cross(n,tangent));int offset=int(m.vertices.size());
  for(int y=0;y<=4;++y)for(int x=0;x<=4;++x){double u=x/4.,v=y/4.;double angle=std::atan2(v-.5,u-.5),phase=.73*piece+.2;double edgeShape=.88+.10*std::sin(3*angle+phase)+.08*std::cos(5*angle+.7*phase);V3 local=normalized(n+tangent*((u-.5)*1.05*edgeShape)+vertical*((v-.5)*.62*edgeShape))*(stone.radius*1.03);V2 uv{centers[piece].x+(u-.5)*cropSize[piece].x,centers[piece].y+(v-.5)*cropSize[piece].y};m.vertices.push_back({stone.center+rotate(local,stone.axis,stone.angle),n,{(3+uv.x*(atlas-6))/atlas,(3+uv.y*(atlas-6))/atlas}});}
  for(int y=0;y<4;++y)for(int x=0;x<4;++x){double u=(x+.5)/4.-.5,v=(y+.5)/4.-.5;if(u*u+v*v>.20)continue;int a=offset+y*5+x;m.faces.push_back({a,a+1,a+5});m.faces.push_back({a+1,a+6,a+5});}
 }
 return m;
}
const char* phaseName(impact::Phase p){return p==impact::Phase::Intact?"intact":p==impact::Phase::Indenting?"indenting":"detached";}
double number(const std::string& s){size_t n=0;double v=std::stod(s,&n);if(n!=s.size()||!std::isfinite(v))throw std::invalid_argument("finite numeric option required");return v;}
}
int main(int argc,char** argv){try{
 impact::Settings settings;settings.threshold=1;settings.breakDelay=.08;settings.maxIndent=.16;double score=2;std::string output;uint64_t seed=42;
 for(int i=1;i<argc;++i){std::string k=argv[i];if(k=="--help"){std::cout<<"impact_demo --out NEW_DIR [--breakable 0|1] [--score N] [--threshold N] [--seed N] [--wind 0..0.05]\n";return 0;}if(i+1>=argc)throw std::invalid_argument("missing value for "+k);std::string v=argv[++i];if(k=="--out")output=v;else if(k=="--breakable"){if(v!="0"&&v!="1")throw std::invalid_argument("breakable must be0 or1");settings.breakable=v=="1";}else if(k=="--score")score=number(v);else if(k=="--threshold")settings.threshold=number(v);else if(k=="--wind"){settings.windAmplitude=number(v);settings.windVelocity={1,0,0};}else if(k=="--seed"){if(v.empty()||v.find_first_not_of("0123456789")!=std::string::npos)throw std::invalid_argument("seed requires unsigned digits");try{seed=std::stoull(v);}catch(...){throw std::invalid_argument("seed outside uint64 range");}}else throw std::invalid_argument("unknown option "+k);}
 if(output.empty()||score<0||score>1000||settings.threshold>1000||settings.windAmplitude>.05)throw std::invalid_argument("require new output and bounded score/threshold/wind");
 if(fs::exists(output))throw std::runtime_error("output exists; choose a new directory");
 settings.seed=seed;impact::Panel panel(settings);Recipe recipe;recipe.preset="corridor";recipe.seed=seed;recipe.density=.9;recipe.width=.0045;recipe.broken=0;recipe.atlas=512;auto asset=panelAsset(recipe);auto atlas=bake(asset);const Patch& patch=asset.patches[0];V2 hitUV{.28,.43};int px=int(3+hitUV.x*(atlas.size-6)),py=int(3+hitUV.y*(atlas.size-6));if(atlas.coverage[size_t(py)*atlas.size+px]<.05)throw std::runtime_error("scripted impact point has no silk; choose another point");
 const fs::path outputPath(output);if(!outputPath.parent_path().empty())fs::create_directories(outputPath.parent_path());if(!fs::create_directory(outputPath))throw std::runtime_error("output already claimed; choose a new directory");tga(atlas,fs::path(output)/"coverage.tga");obj(asset.mesh,fs::path(output)/"reference_panel.obj");
 V3 normal=normalized(cross(patch.corners[1]-patch.corners[0],patch.corners[3]-patch.corners[0]));const V3 gravity{0,0,-9.81};const int hitTick=42;const double hitTime=hitTick*impact::fixedDt;Stone stone;V3 contactCenter=patch.sample(hitUV)+normal*stone.radius;V3 initialVelocity=normal*-2.6-gravity*(.5*hitTime);V3 initialPosition=contactCenter-initialVelocity*hitTime-gravity*(.5*hitTime*hitTime);V3 impactVelocity=initialVelocity+gravity*hitTime;V3 releasePosition{},lastAttached=contactCenter;int releaseTick=-1;double stateMicroseconds=0;std::vector<int> frames{0,47,52,70,106};std::vector<std::string> names{"01_before","02_indent","03_response","04_remnants","05_later"};auto trace=file(fs::path(output)/"trace.csv");trace<<"tick,phase,collision,resistance,break_events,dent\n";
 for(int tick=0;tick<=106;++tick){if(tick==hitTick){if(!panel.receiveImpact(1,score,hitUV))throw std::runtime_error("first impact rejected");}
  double time=tick*impact::fixedDt;stone.angle=time*2;
  if(tick<hitTick)stone.center=initialPosition+initialVelocity*time+gravity*(.5*time*time);
  else if(panel.state().phase==impact::Phase::Detached){double t=(tick-releaseTick)*impact::fixedDt;stone.center=releasePosition+impactVelocity*(.85*t)+gravity*(.5*t*t);}
  else if(settings.breakable&&score>=settings.threshold){stone.center=impact::deformedPoint(patch,hitUV,panel.state(),settings)+normal*stone.radius;lastAttached=stone.center;}
  else {double t=time-hitTime;double vn=dot(impactVelocity,normal);V3 bounce=impactVelocity-normal*(1.3*vn);stone.center=contactCenter+bounce*t+gravity*(.5*t*t);}
  // Ground handling is a simple offline clamp, not an engine rigid-body integration.
  stone.center.z=std::max(stone.radius,stone.center.z);
  if(panel.state().phase==impact::Phase::Detached&&(panel.collisionEnabled()||panel.resistanceEnabled()))throw std::runtime_error("detached panel retained contact");
  trace<<tick<<','<<phaseName(panel.state().phase)<<','<<panel.collisionEnabled()<<','<<panel.resistanceEnabled()<<','<<panel.state().eventCount<<','<<panel.state().dent<<'\n';
  auto frame=std::find(frames.begin(),frames.end(),tick);if(frame!=frames.end()){
   fs::path dir=fs::path(output)/names[size_t(frame-frames.begin())];fs::create_directory(dir);auto mesh=deformedMesh(asset,panel);obj(mesh,dir/"mesh.obj");fs::copy_file(fs::path(output)/"coverage.tga",dir/"coverage.tga");Mesh scraps;if(panel.state().phase==impact::Phase::Detached){scraps=remnants(stone,normal,releaseTick*impact::fixedDt*2,atlas.size);obj(scraps,dir/"remnants.obj");}
   auto json=file(dir/"frame.json");json<<"{\"tick\":"<<tick<<",\"time\":"<<time<<",\"phase\":\""<<phaseName(panel.state().phase)<<"\",\"breakable\":"<<(settings.breakable?"true":"false")<<",\"impact_score\":"<<score<<",\"threshold\":"<<settings.threshold<<",\"impact_units\":\"arbitrary artistic impact units\",\"collision_active\":"<<(panel.collisionEnabled()?"true":"false")<<",\"resistance_active\":"<<(panel.resistanceEnabled()?"true":"false")<<",\"break_events\":"<<panel.state().eventCount<<",\"mesh_faces\":"<<mesh.faces.size()<<",\"remnant_count\":"<<(scraps.faces.empty()?0:4)<<",\"remnant_faces\":"<<scraps.faces.size()<<",\"particles\":[";bool first=true;for(auto a:asset.anchors){if(!first)json<<',';first=false;json<<"{\"id\":"<<a.id<<",\"anchor\":"<<a.id<<",\"p\":";vector(json,a.position);json<<",\"rest\":";vector(json,a.position);json<<'}';}json<<"],\"edges\":[],\"curves\":[],\"rock\":{\"center\":";vector(json,stone.center);json<<",\"radius\":"<<stone.radius<<",\"axis\":";vector(json,normalized(stone.axis));json<<",\"angle\":"<<stone.angle<<"},\"remnants_attachment\":\"stone-local cosmetic cards; no cloth wrapping simulation\",\"benchmark\":\"scripted known-point impact; ballistic stone before/after, kinematic during indentation; no Jolt integration\"}\n";auto cp=file(dir/"panel.checkpoint");cp<<panel.checkpoint();
  }
  if(tick<106){auto start=std::chrono::steady_clock::now();bool detached=panel.step();stateMicroseconds+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();if(detached){releaseTick=int(panel.state().tick);releasePosition=lastAttached;}}
 }
 auto checkpoint=file(fs::path(output)/"panel.checkpoint");checkpoint<<panel.checkpoint();auto summary=file(fs::path(output)/"summary.json");summary<<"{\"model\":\"cheap whole-panel impact v1\",\"seed\":"<<seed<<",\"breakable\":"<<(settings.breakable?"true":"false")<<",\"break_tick\":"<<releaseTick<<",\"break_events\":"<<panel.state().eventCount<<",\"panel_triangles_before\":"<<asset.mesh.faces.size()<<",\"max_cosmetic_patches\":4,\"state_update_us_total\":"<<stateMicroseconds<<",\"state_updates\":106,\"physics_particles\":0,\"physics_constraints\":0,\"dynamic_texture_bakes\":0,\"impact_detection\":\"scripted material point with coverage check\",\"engine_integration\":\"none\"}\n";
 auto rec=file(fs::path(output)/"recipe.txt");rec<<"seed="<<seed<<"\nbreakable="<<settings.breakable<<"\nimpact_score="<<score<<"\nthreshold="<<settings.threshold<<"\nwind_amplitude="<<settings.windAmplitude<<"\nimpact_tick=42\n";
 std::cout<<"Whole-panel prototype: "<<panel.state().eventCount<<" break event, release tick "<<releaseTick<<", "<<stateMicroseconds<<" us for106 state updates. No engine integration. -> "<<output<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<"error: "<<e.what()<<'\n';return 1;}}
