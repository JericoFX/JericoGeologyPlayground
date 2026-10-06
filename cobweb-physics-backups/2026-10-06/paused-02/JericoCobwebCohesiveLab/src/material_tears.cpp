#include "material_tears.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <istream>
#include <limits>
#include <map>
#include <ostream>
#include <set>
#include <stdexcept>

namespace web::physics {
namespace {
constexpr double epsilon=1e-12;
void require(bool condition,const char* message){if(!condition)throw std::invalid_argument(message);}
bool finite(V2 p){return std::isfinite(p.x)&&std::isfinite(p.y);}
double cross2(V2 a,V2 b){return a.x*b.y-a.y*b.x;}
double length2(V2 a){return std::sqrt(a.x*a.x+a.y*a.y);}
const MaterialEdgeLineage* lineage(const Graph& g,int id){
 const auto& l=g.materialTears.lineage;
 auto it=std::lower_bound(l.begin(),l.end(),id,[](const auto& a,int b){return a.edge<b;});
 return it!=l.end()&&it->edge==id?&*it:nullptr;
}
MaterialCutRegion region(V2 center,double radius,int id,int edge,int tick,int substep){
 const double q=std::sqrt(.5);
 MaterialCutRegion r;r.id=id;r.primaryEdge=edge;r.tick=tick;r.substep=substep;
 for(V2 p:std::vector<V2>{{1,0},{q,q},{0,1},{-q,q},{-1,0},{-q,-q},{0,-1},{q,-q}})r.polygon.push_back(center+p*radius);
 return r;
}
struct Split {std::size_t edge;double lo,hi;};
void commitRegion(Graph& g,int primary,const std::string& cause,int substep,std::size_t contactScalars){
 auto found=std::find_if(g.edges.begin(),g.edges.end(),[&](const Edge&e){return e.id==primary;});
 require(found!=g.edges.end(),"material cut edge ID does not exist");
 if(!found->active)return;
 auto& state=g.materialTears;
 require(state.nextCutId<std::numeric_limits<int>::max(),"material cut ID exhausted");
 const auto primaryUV=materialEdgeUV(g,*found);
 const auto cut=region((primaryUV[0]+primaryUV[1])*.5,state.radius,state.nextCutId,primary,g.tick,substep);
 std::vector<Split> splits;
 std::vector<std::size_t> order;
 for(std::size_t i=0;i<g.edges.size();++i)if(g.edges[i].active)order.push_back(i);
 std::sort(order.begin(),order.end(),[&](std::size_t a,std::size_t b){return g.edges[a].id<g.edges[b].id;});
 std::vector<double> incident(g.particles.size(),0);
 // Canonical reduction matters at shared junctions with unequal edge lengths.
 for(std::size_t i:order){const auto&e=g.edges[i];
  incident[e.a]+=e.rest;incident[e.b]+=e.rest;
  const auto uv=materialEdgeUV(g,e);auto interval=materialCutInterval(cut,uv[0],uv[1]);
  if(interval)splits.push_back({i,(*interval)[0],(*interval)[1]});
 }
 std::size_t added=0;
 for(auto s:splits){if(s.lo>epsilon)++added;if(s.hi<1-epsilon)++added;}
 if(g.particles.size()+added>static_cast<std::size_t>(std::min(g.settings.maxParticles,state.initialParticles+state.particleReserve)))throw std::runtime_error("material tear particle budget exceeded: need "+std::to_string(g.particles.size()+added)+", limit "+std::to_string(std::min(g.settings.maxParticles,state.initialParticles+state.particleReserve))+" (reserve "+std::to_string(state.particleReserve)+", hard limit 2048); no topology committed");
 if(g.edges.size()+added+contactScalars>static_cast<std::size_t>(g.settings.maxConstraints))throw std::runtime_error("material tear scalar budget exceeded: need "+std::to_string(g.edges.size()+added+contactScalars)+", limit "+std::to_string(g.settings.maxConstraints)+" including current contacts; no topology committed");
 require(added<=static_cast<std::size_t>(std::numeric_limits<int>::max()-state.nextParticleId)&&added<=static_cast<std::size_t>(std::numeric_limits<int>::max()-state.nextEdgeId),"material particle or edge ID exhausted");
 // Conservative artistic lumping: a new end receives half of its retained
 // endpoint's edge-weighted lump, scaled by the surviving fraction. Removed UV
 // material's mass stays in existing lumps. This conserves free-particle mass,
 // but interpolated end velocity does NOT guarantee momentum/energy conservation.
 // A pinned endpoint has no finite dynamic mass. Its end is funded by a counted
 // pinned reservoir, estimated from the other endpoint (or nominal profile when
 // both endpoints are pinned). The anchor itself is never moved or unpinned.
 std::vector<double> originalMass(g.particles.size(),0), transfers(g.particles.size(),0);
 for(std::size_t i=0;i<g.particles.size();++i)if(g.particles[i].invMass>0)originalMass[i]=1/g.particles[i].invMass;
 for(auto split:splits){
  const Edge parent=g.edges[split.edge];const Particle a=g.particles[parent.a],b=g.particles[parent.b];
  const auto parentUV=materialEdgeUV(g,parent);const auto parentLine=*lineage(g,parent.id);
  const std::string eventCause=parent.id==primary?cause:"secondary-material-tear";
  const double eventT=parent.id==primary?.5:.5*(split.lo+split.hi);
  g.events.push_back({g.tick,substep,parent.id,parentUV[0]+(parentUV[1]-parentUV[0])*eventT,a.x+(b.x-a.x)*eventT,parent.load,eventCause});
  auto& dead=g.edges[split.edge];dead.active=false;dead.lambda=dead.load=dead.overload=0;
  if(parent.id==primary)++state.primaryCuts;else ++state.secondaryCuts;
  auto piece=[&](double t0,double t1,bool left){
   const double fraction=t1-t0;if(fraction<=epsilon)return;
   require(parent.rest*fraction>1e-10,"material tear leaves a fragment below solver minimum rest length");
   const double t=left?t1:t0;const int donor=left?parent.a:parent.b,other=left?parent.b:parent.a;
   double donated=0;
   if(originalMass[donor]>0){
    donated=.5*originalMass[donor]*(parent.rest/incident[donor])*fraction;
    transfers[donor]+=donated;state.transferredMass+=donated;
   }else{
    const double lump=originalMass[other]>0?originalMass[other]*(parent.rest/incident[other]):.5*parent.rest*profiles()[parent.kind].massPerLength;
    donated=.5*lump*fraction;state.anchorSuppliedMass+=donated;
   }
   require(std::isfinite(donated)&&donated>=1e-12,"material free-end mass below solver inverse-mass limit");
   Particle p;p.id=state.nextParticleId++;p.rest=a.rest+(b.rest-a.rest)*t;p.x=a.x+(b.x-a.x)*t;p.v=a.v+(b.v-a.v)*t;p.uv=parentUV[0]+(parentUV[1]-parentUV[0])*t;p.invMass=1/donated;
   const int index=static_cast<int>(g.particles.size());g.particles.push_back(p);
   Edge child=parent;child.id=state.nextEdgeId++;child.a=left?parent.a:index;child.b=left?index:parent.b;
   child.rest*=fraction;child.compliance*=fraction;child.active=true;child.lambda=child.load=child.overload=0;
   g.edges.push_back(child);
   state.lineage.push_back({child.id,parent.id,parentLine.rootEdge,cut.id,parentUV[0]+(parentUV[1]-parentUV[0])*t0,parentUV[0]+(parentUV[1]-parentUV[0])*t1,t0,t1});
  };
  piece(0,split.lo,true);piece(split.hi,1,false);
 }
 for(std::size_t i=0;i<originalMass.size();++i)if(transfers[i]>0){
  const double remainder=originalMass[i]-transfers[i];require(std::isfinite(remainder)&&remainder>=1e-12,"material donor mass below solver inverse-mass limit");g.particles[i].invMass=1/remainder;
 }
 state.cuts.push_back(cut);++state.nextCutId;
}
}

void enableMaterialTears(Graph& g,double radius,int reserve,MaterialChartSource chartSource){
 require(chartSource==MaterialChartSource::ExactAuthoringUV||chartSource==MaterialChartSource::ParticleUV,"invalid material chart source");
 require(std::isfinite(radius)&&radius>0&&radius<=1,"material cut radius must be finite in (0,1]");
 require(reserve>=0&&reserve<=128,"material particle reserve must be within [0,128]");
 validateGraph(g);
 if(g.materialTears.enabled){require(g.materialTears.radius==radius&&g.materialTears.particleReserve==reserve&&g.materialTears.chartSource==chartSource,"material tears already enabled with different options");return;}
 MaterialTearState state;state.enabled=true;state.chartSource=chartSource;state.radius=radius;state.particleReserve=reserve;state.initialParticles=static_cast<int>(g.particles.size());
 for(const auto&p:g.particles){require(p.id<std::numeric_limits<int>::max(),"material particle ID exhausted");state.nextParticleId=std::max(state.nextParticleId,p.id+1);}
 for(const auto&e:g.edges){require(e.id<std::numeric_limits<int>::max(),"material edge ID exhausted");state.nextEdgeId=std::max(state.nextEdgeId,e.id+1);
  std::array<V2,2> uv{g.particles[e.a].uv,g.particles[e.b].uv};
  if(chartSource==MaterialChartSource::ExactAuthoringUV&&g.cohesive.enabled)for(const auto&s:g.cohesive.segments)if(s.edge==e.id){
   require(s.a>=0&&s.b>=0&&static_cast<std::size_t>(s.a)<g.cohesive.vertices.size()&&static_cast<std::size_t>(s.b)<g.cohesive.vertices.size(),"invalid cohesive material segment");
   const auto&a=g.cohesive.vertices[s.a];const auto&b=g.cohesive.vertices[s.b];
   if(a.particle==e.a&&b.particle==e.b)uv={a.uv,b.uv};
   else if(a.particle==e.b&&b.particle==e.a)uv={b.uv,a.uv};
   else throw std::invalid_argument("cohesive material segment does not match physical endpoints");
   break;
  }
  state.lineage.push_back({e.id,-1,e.id,-1,uv[0],uv[1],0,1});
 }
 std::sort(state.lineage.begin(),state.lineage.end(),[](const auto&a,const auto&b){return a.edge<b.edge;});
 Graph next=g;next.materialTears=std::move(state);validateMaterialTears(next);g=std::move(next);
}

MaterialTearResult applyMaterialTear(Graph& g,int edge,const std::string& cause,int substep){return applyMaterialTearBatch(g,{{edge,cause}},substep);}
MaterialTearResult applyMaterialTearBatch(Graph& g,const std::vector<MaterialFailure>& failures,int substep,std::size_t contactScalars){
 require(g.materialTears.enabled,"material tears are not enabled");validateGraph(g);
 require(substep>=0&&substep<g.settings.substeps,"invalid material tear substep");
 require(contactScalars<=512,"material contact scalar count exceeds solver limit");
 auto ordered=failures;std::sort(ordered.begin(),ordered.end(),[](const auto&a,const auto&b){return a.edge<b.edge;});
 int previous=-1;for(const auto& f:ordered){require(f.edge>=0&&f.edge!=previous&&!f.cause.empty()&&f.cause.size()<=128&&f.cause!="secondary-material-tear","invalid or duplicate primary material failure");previous=f.edge;require(std::any_of(g.edges.begin(),g.edges.end(),[&](const auto&e){return e.id==f.edge;}),"material cut edge ID does not exist");}
 Graph next=g;for(const auto& f:ordered)commitRegion(next,f.edge,f.cause,substep,contactScalars);
 components(next);validateGraph(next);
 MaterialTearResult result{int(next.materialTears.cuts.size()-g.materialTears.cuts.size()),next.materialTears.primaryCuts-g.materialTears.primaryCuts,next.materialTears.secondaryCuts-g.materialTears.secondaryCuts,int(next.particles.size()-g.particles.size()),int(next.edges.size()-g.edges.size())};
 g=std::move(next);return result;
}

std::array<V2,2> materialEdgeUV(const Graph& g,const Edge& e){
 if(g.materialTears.enabled){const auto* l=lineage(g,e.id);require(l!=nullptr,"missing material edge lineage");return {l->uvA,l->uvB};}
 return {g.particles.at(e.a).uv,g.particles.at(e.b).uv};
}
std::optional<std::array<double,2>> materialCutInterval(const MaterialCutRegion& r,V2 a,V2 b){
 require(r.polygon.size()>=3&&finite(a)&&finite(b),"invalid material interval input");
 double lo=0,hi=1;const V2 delta=b-a;
 for(std::size_t i=0;i<r.polygon.size();++i){const V2 p=r.polygon[i],edge=r.polygon[(i+1)%r.polygon.size()]-p;
  const double len=length2(edge);require(len>0,"degenerate material polygon edge");
  const double origin=cross2(edge,a-p)/len,slope=cross2(edge,delta)/len;
  if(std::abs(slope)<1e-15){if(origin<=epsilon)return std::nullopt;continue;}
  const double t=-origin/slope;if(slope>0)lo=std::max(lo,t);else hi=std::min(hi,t);
  if(hi-lo<=epsilon)return std::nullopt;
 }
 if(hi-lo<=epsilon)return std::nullopt;
 return std::array<double,2>{std::clamp(lo,0.,1.),std::clamp(hi,0.,1.)};
}
bool pointInMaterialCut(const MaterialCutRegion& r,V2 p,double tolerance){
 require(r.polygon.size()>=3&&finite(p)&&std::isfinite(tolerance),"invalid material point input");
 for(std::size_t i=0;i<r.polygon.size();++i){auto edge=r.polygon[(i+1)%r.polygon.size()]-r.polygon[i];if(cross2(edge,p-r.polygon[i])< -tolerance*length2(edge))return false;}return true;
}
bool pointInMaterialCut(const Graph& g,V2 p,double tolerance){for(const auto&r:g.materialTears.cuts)if(pointInMaterialCut(r,p,tolerance))return true;return false;}

void validateMaterialTears(const Graph& g){
 const auto&s=g.materialTears;
 require(s.chartSource==MaterialChartSource::ExactAuthoringUV||s.chartSource==MaterialChartSource::ParticleUV,"invalid material chart source");
 if(!s.enabled){require(s.cuts.empty()&&s.lineage.empty(),"disabled material tears retain dynamic chart");return;}
 require(std::isfinite(s.radius)&&s.radius>0&&s.radius<=1&&s.particleReserve>=0&&s.particleReserve<=128,"invalid material tear options");
 require(s.initialParticles>0&&s.initialParticles<=int(g.particles.size())&&g.particles.size()<=static_cast<std::size_t>(s.initialParticles+s.particleReserve),"invalid material particle reserve state");
 require(s.lineage.size()==g.edges.size()&&s.cuts.size()<=g.edges.size(),"invalid material lineage or cut count");
 require(s.primaryCuts>=0&&s.secondaryCuts>=0&&s.primaryCuts==int(s.cuts.size())&&s.nextCutId==int(s.cuts.size()),"invalid material cut counters");
 require(std::isfinite(s.transferredMass)&&s.transferredMass>=0&&std::isfinite(s.anchorSuppliedMass)&&s.anchorSuppliedMass>=0,"invalid material mass accounting");
 std::map<int,const Edge*> edges;for(const auto&e:g.edges){edges[e.id]=&e;require(e.id<s.nextEdgeId,"invalid next material edge ID");}
 for(const auto&p:g.particles)require(p.id<s.nextParticleId,"invalid next material particle ID");
 int previous=-1,children=0;for(const auto& l:s.lineage){
  require(l.edge>previous&&edges.count(l.edge)&&finite(l.uvA)&&finite(l.uvB),"invalid material lineage edge or UV");previous=l.edge;
  if(s.chartSource==MaterialChartSource::ParticleUV){const auto* edge=edges.at(l.edge);require(edge->a>=0&&edge->b>=0&&static_cast<std::size_t>(edge->a)<g.particles.size()&&static_cast<std::size_t>(edge->b)<g.particles.size(),"invalid particle-chart endpoint");require(length2(l.uvA-g.particles[edge->a].uv)<=epsilon&&length2(l.uvB-g.particles[edge->b].uv)<=epsilon,"ParticleUV lineage differs from particle chart");}
  require(std::isfinite(l.parentT0)&&std::isfinite(l.parentT1)&&l.parentT0>=0&&l.parentT1<=1&&l.parentT0<l.parentT1,"invalid material lineage interval");
  require(edges.count(l.rootEdge)&&l.rootEdge<=l.edge,"invalid material root edge");
  if(l.parentEdge<0)require(l.parentEdge==-1&&l.rootEdge==l.edge&&l.cut==-1&&l.parentT0==0&&l.parentT1==1,"invalid original material lineage");
  else {
   ++children;require(edges.count(l.parentEdge)&&l.parentEdge<l.edge&&!edges.at(l.parentEdge)->active&&l.cut>=0&&l.cut<int(s.cuts.size()),"invalid material parent lineage");
   const auto* parent=lineage(g,l.parentEdge);require(parent&&l.rootEdge==parent->rootEdge,"material child lost root lineage");
   const V2 span=parent->uvB-parent->uvA;
   require(length2(l.uvA-(parent->uvA+span*l.parentT0))<=epsilon&&length2(l.uvB-(parent->uvA+span*l.parentT1))<=epsilon,"material child UV differs from parent interval");
   const auto* edge=edges.at(l.edge);const auto* parentEdge=edges.at(l.parentEdge);
   require((l.parentT0==0&&edge->a==parentEdge->a)||(l.parentT1==1&&edge->b==parentEdge->b),"material child lost surviving endpoint");
  }
 }
 require(children==int(g.particles.size())-s.initialParticles,"material free-end and child counts differ");
 for(std::size_t i=0;i<s.cuts.size();++i){const auto&r=s.cuts[i];
  require(r.id==int(i)&&edges.count(r.primaryEdge)&&!edges.at(r.primaryEdge)->active&&r.tick>=0&&r.tick<=g.tick&&r.substep>=0&&r.substep<g.settings.substeps,"invalid material cut metadata");
  require(r.polygon.size()>=3&&r.polygon.size()<=32,"invalid material polygon count");
  for(std::size_t j=0;j<r.polygon.size();++j){const auto a=r.polygon[j],b=r.polygon[(j+1)%r.polygon.size()],c=r.polygon[(j+2)%r.polygon.size()];require(finite(a)&&std::isfinite(cross2(b-a,c-b))&&cross2(b-a,c-b)>0,"material polygon must be finite, strictly convex and CCW");
   for(const auto&p:r.polygon)require(cross2(b-a,p-a)>=-epsilon*length2(b-a),"self-intersecting or nonconvex material polygon");}
 }
 int primary=0,secondary=0;for(const auto&e:g.events){if(e.cause=="secondary-material-tear")++secondary;else for(const auto&r:s.cuts)if(e.edge==r.primaryEdge){++primary;break;}}
 require(primary==s.primaryCuts&&secondary==s.secondaryCuts,"material events disagree with counters");
 // Check chart consistency independently of inactive parent spans. Cheap box
 // rejection keeps this postcondition local even in a large mostly intact web.
 for(const auto&r:s.cuts){double xmin=r.polygon[0].x,xmax=xmin,ymin=r.polygon[0].y,ymax=ymin;for(const auto&p:r.polygon){xmin=std::min(xmin,p.x);xmax=std::max(xmax,p.x);ymin=std::min(ymin,p.y);ymax=std::max(ymax,p.y);}
  for(const auto&l:s.lineage)if(edges.at(l.edge)->active){
   if(std::max(l.uvA.x,l.uvB.x)<xmin||std::min(l.uvA.x,l.uvB.x)>xmax||std::max(l.uvA.y,l.uvB.y)<ymin||std::min(l.uvA.y,l.uvB.y)>ymax)continue;
   require(!materialCutInterval(r,l.uvA,l.uvB),"active material segment crosses stored cut region");
  }
 }
}
void writeMaterialTearState(std::ostream& out,const Graph& g){
 const auto&s=g.materialTears;require(s.enabled,"cannot serialize disabled material extension");
 out<<"MATERIAL_TEARS 2\n"<<s.radius<<' '<<s.particleReserve<<' '<<s.initialParticles<<' '<<s.nextCutId<<' '<<s.nextParticleId<<' '<<s.nextEdgeId<<' '<<s.primaryCuts<<' '<<s.secondaryCuts<<' '<<s.transferredMass<<' '<<s.anchorSuppliedMass<<' '<<static_cast<int>(s.chartSource)<<'\n';
 out<<s.cuts.size()<<' '<<s.lineage.size()<<'\n';
 for(const auto&r:s.cuts){out<<r.id<<' '<<r.primaryEdge<<' '<<r.tick<<' '<<r.substep<<' '<<r.polygon.size();for(const auto&p:r.polygon)out<<' '<<p.x<<' '<<p.y;out<<'\n';}
 for(const auto&l:s.lineage)out<<l.edge<<' '<<l.parentEdge<<' '<<l.rootEdge<<' '<<l.cut<<' '<<l.uvA.x<<' '<<l.uvA.y<<' '<<l.uvB.x<<' '<<l.uvB.y<<' '<<l.parentT0<<' '<<l.parentT1<<'\n';
 out<<"END_MATERIAL_TEARS\n";
}
void readMaterialTearState(std::istream& in,Graph& g){
 std::string tag;int version=-1;in>>tag>>version;require(!in.fail()&&tag=="MATERIAL_TEARS"&&(version==1||version==2),"unknown checkpoint extension");
 auto&s=g.materialTears;s.enabled=true;
 in>>s.radius>>s.particleReserve>>s.initialParticles>>s.nextCutId>>s.nextParticleId>>s.nextEdgeId>>s.primaryCuts>>s.secondaryCuts>>s.transferredMass>>s.anchorSuppliedMass;
 if(version==2){int chart=-1;in>>chart;require(!in.fail()&&(chart==0||chart==1),"invalid checkpoint material chart source");s.chartSource=static_cast<MaterialChartSource>(chart);}
 else s.chartSource=MaterialChartSource::ExactAuthoringUV;
 long long cuts=-1,lineages=-1;in>>cuts>>lineages;require(!in.fail()&&cuts>=0&&cuts<=4096&&lineages==static_cast<long long>(g.edges.size()),"invalid material checkpoint counts");
 s.cuts.resize(static_cast<std::size_t>(cuts));s.lineage.resize(static_cast<std::size_t>(lineages));
 for(auto&r:s.cuts){long long points=-1;in>>r.id>>r.primaryEdge>>r.tick>>r.substep>>points;require(!in.fail()&&points>=3&&points<=32,"invalid material checkpoint polygon count");r.polygon.resize(static_cast<std::size_t>(points));for(auto&p:r.polygon)in>>p.x>>p.y;}
 for(auto&l:s.lineage)in>>l.edge>>l.parentEdge>>l.rootEdge>>l.cut>>l.uvA.x>>l.uvA.y>>l.uvB.x>>l.uvB.y>>l.parentT0>>l.parentT1;
 in>>tag;require(!in.fail()&&tag=="END_MATERIAL_TEARS","truncated material checkpoint extension");
 validateMaterialTears(g);
}
}
