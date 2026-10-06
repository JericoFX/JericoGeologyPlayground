#include "physics.hpp"
#include "damage.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
namespace web::physics {
namespace {
constexpr double uvTolerance=1e-10;
double cross2(V2 a,V2 b){return a.x*b.y-a.y*b.x;}
double dot2(V2 a,V2 b){return a.x*b.x+a.y*b.y;}
struct Crossing {double a,b;V2 uv;};
std::vector<Crossing> crossings(V2 a,V2 b,V2 c,V2 d){
 const V2 u=b-a,v=d-c,w=c-a;const double uu=dot2(u,u),vv=dot2(v,v),den=cross2(u,v);
 if(uu<1e-24||vv<1e-24)return {};
 if(std::abs(den)>1e-12*std::sqrt(uu*vv)){
  const double t=cross2(w,v)/den,s=cross2(w,u)/den;
  if(t< -uvTolerance||t>1+uvTolerance||s< -uvTolerance||s>1+uvTolerance)return {};
  const double ct=std::clamp(t,0.,1.),cs=std::clamp(s,0.,1.);return {{ct,cs,a+u*ct}};
 }
 if(std::abs(cross2(w,u))>uvTolerance*std::sqrt(uu))return {};
 std::vector<Crossing> result;
 auto endpoint=[&](V2 point,double t,double s){if(t>=-uvTolerance&&t<=1+uvTolerance&&s>=-uvTolerance&&s<=1+uvTolerance){for(const auto& h:result)if(length(h.uv-point)<=uvTolerance)return;result.push_back({std::clamp(t,0.,1.),std::clamp(s,0.,1.),point});}};
 endpoint(a,0,dot2(a-c,v)/vv);endpoint(b,1,dot2(b-c,v)/vv);endpoint(c,dot2(c-a,u)/uu,0);endpoint(d,dot2(d-a,u)/uu,1);return result;
}
void makeCohesive(Graph& g,const Patch& patch,const BuildOptions& opt,const std::array<Profile,3>& prof){
 g.cohesive.enabled=true;g.cohesive.patch=opt.patch;g.cohesive.layer=patch.layer;g.cohesive.depthTolerance=opt.cohesiveDepthTolerance;
 const auto original=g.edges;
 std::vector<int> parent(g.particles.size());std::iota(parent.begin(),parent.end(),0);
 auto root=[&](int n){while(parent[n]!=n){parent[n]=parent[parent[n]];n=parent[n];}return n;};
 std::vector<std::vector<std::pair<double,int>>> split(original.size());
 for(size_t i=0;i<original.size();++i)split[i]={{0,original[i].a},{1,original[i].b}};
 auto endpoint=[&](const Edge&e,double t){return t<=uvTolerance?e.a:(t>=1-uvTolerance?e.b:-1);};
 for(size_t i=0;i<original.size();++i)for(size_t j=i+1;j<original.size();++j){
  const auto&a=original[i];const auto&b=original[j];
  for(auto hit:crossings(g.particles[a.a].uv,g.particles[a.b].uv,g.particles[b.a].uv,g.particles[b.b].uv)){
   const V3 rest=patch.sample(hit.uv),ra=g.particles[a.a].rest*(1-hit.a)+g.particles[a.b].rest*hit.a,rb=g.particles[b.a].rest*(1-hit.b)+g.particles[b.b].rest*hit.b;
   const double gap=length(ra-rb),error=std::max(length(rest-ra),length(rest-rb));
   g.cohesive.maxDepthGap=std::max(g.cohesive.maxDepthGap,gap);
   // A UV overlap is adhesive only inside the same sampled rest-depth sheet.
   // Different patches/layers never enter this per-patch builder together.
   if(gap>opt.cohesiveDepthTolerance||error>opt.cohesiveDepthTolerance){++g.cohesive.depthRejected;continue;}
   std::vector<int> candidates;int ea=endpoint(a,hit.a),eb=endpoint(b,hit.b);
   if(ea>=0)candidates.push_back(root(ea));
   if(eb>=0)candidates.push_back(root(eb));
   for(size_t n=0;n<g.particles.size();++n)if(root(int(n))==int(n)&&length(g.particles[n].uv-hit.uv)<=uvTolerance&&length(g.particles[n].rest-rest)<=opt.cohesiveDepthTolerance)candidates.push_back(int(n));
   std::sort(candidates.begin(),candidates.end());candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
   int anchor=-1;bool conflict=false;for(int n:candidates)if(g.particles[n].anchor>=0){if(anchor>=0&&anchor!=g.particles[n].anchor)conflict=true;anchor=g.particles[n].anchor;}
   if(conflict){++g.cohesive.anchorRejected;continue;}
   int chosen=-1;for(int n:candidates)if(chosen<0||g.particles[n].anchor>=0)chosen=n;
   if(chosen<0){Particle p;p.id=int(g.particles.size());p.uv=hit.uv;p.rest=p.x=rest;g.particles.push_back(p);parent.push_back(p.id);chosen=p.id;++g.cohesive.junctions;}
   for(int n:candidates)if(n!=chosen){parent[n]=chosen;auto& aliases=g.particles[chosen].sourceNodes;aliases.insert(aliases.end(),g.particles[n].sourceNodes.begin(),g.particles[n].sourceNodes.end());}
   auto& aliases=g.particles[chosen].sourceNodes;std::sort(aliases.begin(),aliases.end());aliases.erase(std::unique(aliases.begin(),aliases.end()),aliases.end());if(g.particles[chosen].sourceNode<0&&!aliases.empty())g.particles[chosen].sourceNode=aliases.front();
   split[i].push_back({hit.a,chosen});split[j].push_back({hit.b,chosen});g.cohesive.maxRestShapeError=std::max(g.cohesive.maxRestShapeError,error);
  }
 }
 std::vector<int> mapping(g.particles.size(),-1);std::vector<Particle> particles;
 for(int i=0;i<int(g.particles.size());++i)if(root(i)==i){auto p=g.particles[i];p.id=int(particles.size());mapping[i]=p.id;particles.push_back(p);}
 for(int i=0;i<int(mapping.size());++i)mapping[i]=mapping[root(i)];
 g.particles=std::move(particles);g.edges.clear();
 std::vector<int> edgeParents;
 for(size_t i=0;i<original.size();++i){auto& list=split[i];std::sort(list.begin(),list.end());
  int previous=-1;for(auto entry:list){const int next=mapping[entry.second];if(next==previous)continue;if(previous>=0){Edge e=original[i];e.a=previous;e.b=next;e.id=int(g.edges.size());e.rest=length(g.particles[e.b].rest-g.particles[e.a].rest);e.compliance=prof[e.kind].compliance*e.rest;if(e.rest>1e-10){g.edges.push_back(e);edgeParents.push_back(original[i].id);}}previous=next;}
 }
 for(const auto&p:g.particles)g.cohesive.vertices.push_back({p.uv,p.rest,p.id});
 for(const auto&e:g.edges)g.cohesive.segments.push_back({e.a,e.b,e.id,edgeParents[e.id]});
}
void assignMass(Graph& g,const BuildOptions& opt,const std::array<Profile,3>& prof){
 std::vector<double> mass(g.particles.size(),0);
 for(const auto&e:g.edges){mass[e.a]+=.5*e.rest*prof[e.kind].massPerLength;mass[e.b]+=.5*e.rest*prof[e.kind].massPerLength;}
 for(size_t i=0;i<mass.size();++i)g.particles[i].invMass=g.particles[i].anchor>=0?0:1/(std::max(.001,mass[i])*opt.massScale);
}
}

Graph buildGraph(const Lab& lab,const BuildOptions& opt){
 if(opt.patch<0||opt.patch>=int(lab.patches.size()))throw std::invalid_argument("invalid patch");
 for(double v:{opt.sampleLength,opt.width,opt.stiffness,opt.strength,opt.massScale})if(!std::isfinite(v)||v<=0)throw std::invalid_argument("positive finite graph options required");
 if(!std::isfinite(opt.junctionTolerance)||opt.junctionTolerance<0||opt.junctionTolerance>.01||opt.massScale<.0001||opt.massScale>100||opt.sampleLength<.01||opt.sampleLength>.5||opt.width>10||opt.stiffness>1000||opt.strength>1e6||!std::isfinite(opt.density)||opt.density<0||opt.density>1)throw std::invalid_argument("graph options outside validated range");
 if(opt.cohesive&&(!std::isfinite(opt.cohesiveDepthTolerance)||opt.cohesiveDepthTolerance<0||opt.cohesiveDepthTolerance>.1))throw std::invalid_argument("cohesive rest-depth tolerance outside [0,.1]");
 const auto& patch=lab.patches[opt.patch];Graph g;g.anchors=lab.anchors;auto prof=profiles(opt.width,opt.stiffness,opt.strength);
 std::vector<const Fiber*> selected;std::set<int> retained;
 for(const auto& f:lab.fibers)if(f.patch==opt.patch&&f.structural)selected.push_back(&f);
 // A small declared subset of original non-structural fibers supplies representative thin/medium contact.
 std::set<int> structuralNodes;for(auto f:selected){structuralNodes.insert(f->nodeA);structuralNodes.insert(f->nodeB);}
 int count=0;for(const auto& f:lab.fibers)if(f.patch==opt.patch&&!f.structural&&structuralNodes.count(f.nodeA)&&structuralNodes.count(f.nodeB)&&f.id%29==0&&count<24){selected.push_back(&f);retained.insert(f.nodeA);retained.insert(f.nodeB);++count;}
 std::map<int,std::vector<int>> adj;for(int i=0;i<int(selected.size());++i){adj[selected[i]->nodeA].push_back(i);adj[selected[i]->nodeB].push_back(i);}
 for(const auto& [n,es]:adj){if(es.size()!=2||selected[es[0]]->strand!=selected[es[1]]->strand)retained.insert(n);for(const auto& a:lab.anchors)if(length(patch.sample(lab.nodes[n].uv)-a.position)<1e-10)retained.insert(n);}
 std::map<int,int> nodeParticle;
 auto particle=[&](int source,V2 uv){if(source>=0&&nodeParticle.count(source))return nodeParticle.at(source);Particle p;p.id=int(g.particles.size());p.sourceNode=source;if(source>=0)p.sourceNodes={source};p.uv=uv;p.rest=p.x=patch.sample(uv);p.invMass=1;
  for(const auto& a:lab.anchors)if(length(p.rest-a.position)<1e-10){p.anchor=a.id;p.rest=p.x=a.position;p.invMass=0;break;}
  g.particles.push_back(p);if(source>=0)nodeParticle[source]=p.id;return p.id;};
 std::vector<bool> used(selected.size(),false);
 auto walk=[&](int start,int first){int n=start,index=first,strand=selected[index]->strand;double visWidth=selected[index]->width;bool structural=selected[index]->structural;std::vector<V2> path;
  while(true){if(used[index])break;used[index]=true;const auto& f=*selected[index];bool forward=f.nodeA==n;
   if(forward){for(size_t j=path.empty()?0:1;j<f.uv.size();++j)path.push_back(f.uv[j]);n=f.nodeB;}
   else{for(int j=int(f.uv.size())-1-(path.empty()?0:1);j>=0;--j)path.push_back(f.uv[j]);n=f.nodeA;}
   if(retained.count(n)||n==start)break;
   const auto& links=adj.at(n);int next=links[0]==index?links[1]:links[0];if(used[next])break;index=next;
  }
  if(path.size()<2)return;
  int end=n;std::vector<double> dist(path.size(),0);for(size_t j=1;j<path.size();++j)dist[j]=dist[j-1]+length(patch.sample(path[j])-patch.sample(path[j-1]));
  const double total=dist.back();if(total<1e-8)return;int steps=std::max(1,int(std::ceil(total/opt.sampleLength)));int prev=particle(start,path.front());
  int kind=structural?(visWidth>=2.?2:1):(visWidth<.6?0:1);
  for(int j=1;j<=steps;++j){double d=total*double(j)/steps;auto it=std::lower_bound(dist.begin(),dist.end(),d);int k=std::min(int(dist.size())-1,std::max(1,int(it-dist.begin())));double t=(d-dist[k-1])/std::max(1e-15,dist[k]-dist[k-1]);V2 uv=path[k-1]*(1-t)+path[k]*t;int next=particle(j==steps?end:-1,uv);if(next==prev)continue;
   Edge e;e.id=int(g.edges.size());e.a=prev;e.b=next;e.kind=kind;e.strand=strand;e.rest=length(g.particles[next].rest-g.particles[prev].rest);e.compliance=prof[kind].compliance*e.rest;e.strength=prof[kind].strength;e.width=prof[kind].width;
   if(e.rest>1e-8)g.edges.push_back(e);
   prev=next;
  }
 };
 for(int n:retained)for(int i:adj[n])if(!used[i])walk(n,i);
 for(int i=0;i<int(selected.size());++i)if(!used[i]){retained.insert(selected[i]->nodeA);walk(selected[i]->nodeA,i);}
 if(opt.cohesive)makeCohesive(g,patch,opt,prof);
 if(g.particles.size()+128>2048||g.edges.size()+512>4096)throw std::runtime_error("physical budget exceeded (2048 particles / 4096 constraints, reserves included)");
 std::vector<double> mass(g.particles.size(),0);for(const auto& e:g.edges){mass[e.a]+=.5*e.rest*prof[e.kind].massPerLength;mass[e.b]+=.5*e.rest*prof[e.kind].massPerLength;}for(size_t i=0;i<mass.size();++i)if(g.particles[i].anchor<0)g.particles[i].invMass=1/(std::max(.001,mass[i])*opt.massScale);
 // Contract only physically connected sub-millimetric/millimetric junctions.
 // Original source-node IDs remain listed; the visual asset itself is untouched.
 if(opt.junctionTolerance>0){
  std::vector<int> parent(g.particles.size());std::iota(parent.begin(),parent.end(),0);
  std::vector<std::vector<int>> members(g.particles.size());for(int i=0;i<int(members.size());++i)members[i]={i};
  auto root=[&](int i){while(parent[i]!=i){parent[i]=parent[parent[i]];i=parent[i];}return i;};
  for(const auto& e:g.edges)if(e.rest<opt.junctionTolerance){int a=root(e.a),b=root(e.b);if(a==b)continue;bool allowed=true;int anchorA=-1,anchorB=-1;
   for(int i:members[a])if(g.particles[i].anchor>=0)anchorA=g.particles[i].anchor;
   for(int i:members[b])if(g.particles[i].anchor>=0)anchorB=g.particles[i].anchor;
   if(anchorA>=0&&anchorB>=0&&anchorA!=anchorB)allowed=false;
   for(int i:members[a])for(int j:members[b]){
    if(length(g.particles[i].rest-g.particles[j].rest)>opt.junctionTolerance)allowed=false;
    if(opt.initialDamage&&lab.recipe.broken>0&&damageMayIntersect(g.particles[i].uv,g.particles[j].uv,lab.recipe.broken,lab.recipe.seed,opt.patch,lab.recipe.preset=="corridor",patch.layer))allowed=false;
   }
   if(!allowed)continue;
   if(a>b)std::swap(a,b);
   parent[b]=a;members[a].insert(members[a].end(),members[b].begin(),members[b].end());members[b].clear();
  }
  std::vector<int> mapping(g.particles.size(),-1);std::vector<Particle> particles;
  for(int i=0;i<int(g.particles.size());++i)if(root(i)==i){int representative=i;double mass=0;for(int member:members[i]){if(g.particles[member].anchor>=0)representative=member;else mass+=1/g.particles[member].invMass;}
   Particle p=g.particles[representative];p.id=int(particles.size());p.sourceNodes.clear();
   for(int member:members[i]){mapping[member]=p.id;const auto& aliases=g.particles[member].sourceNodes;p.sourceNodes.insert(p.sourceNodes.end(),aliases.begin(),aliases.end());}
   std::sort(p.sourceNodes.begin(),p.sourceNodes.end());p.sourceNodes.erase(std::unique(p.sourceNodes.begin(),p.sourceNodes.end()),p.sourceNodes.end());
   if(p.sourceNode<0&&!p.sourceNodes.empty())p.sourceNode=p.sourceNodes.front();
   p.invMass=p.anchor>=0?0:1/mass;particles.push_back(p);
  }
  if(opt.cohesive)for(auto& vertex:g.cohesive.vertices)vertex.particle=mapping[vertex.particle];
  std::vector<int> edgeMapping(g.edges.size(),-1);
  std::vector<Edge> edges;for(auto e:g.edges){e.a=mapping[e.a];e.b=mapping[e.b];if(e.a==e.b)continue;double rest=length(particles[e.b].rest-particles[e.a].rest);if(rest<1e-10)throw std::runtime_error("junction clustering made an invalid zero-length connection");e.compliance*=rest/e.rest;e.rest=rest;edgeMapping[e.id]=int(edges.size());e.id=int(edges.size());edges.push_back(e);}
  if(opt.cohesive)for(auto& segment:g.cohesive.segments)segment.edge=edgeMapping[segment.edge];
  g.particles=std::move(particles);g.edges=std::move(edges);
 }
 if(opt.cohesive)assignMass(g,opt,prof);
 // R3 damage is applied to physical connections before tick zero. No hidden resistant strand crosses its mask.
 if(opt.initialDamage&&lab.recipe.broken>0)for(auto& e:g.edges){V2 a=g.particles[e.a].uv,b=g.particles[e.b].uv;bool damaged=damageMayIntersect(a,b,lab.recipe.broken,lab.recipe.seed,opt.patch,lab.recipe.preset=="corridor",patch.layer);if(damaged){e.active=false;g.events.push_back({0,0,e.id,(a+b)*.5,(g.particles[e.a].x+g.particles[e.b].x)*.5,0,"initial-mask"});}}
 components(g);validateGraph(g);return g;
}
}
