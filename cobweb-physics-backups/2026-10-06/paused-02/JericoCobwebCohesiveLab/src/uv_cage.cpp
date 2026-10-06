#include "uv_cage.hpp"
#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace web::physics {
namespace {
using Triangle=std::array<int,3>;
using EdgeKey=std::pair<int,int>;
EdgeKey key(int a,int b){return std::minmax(a,b);}
long double orient(V2 a,V2 b,V2 c){return ((long double)b.x-a.x)*((long double)c.y-a.y)-((long double)b.y-a.y)*((long double)c.x-a.x);}
double dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
bool finite(V2 p){return std::isfinite(p.x)&&std::isfinite(p.y);}
bool finite(V3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
bool properCross(V2 a,V2 b,V2 c,V2 d){const auto x=orient(a,b,c),y=orient(a,b,d),z=orient(c,d,a),w=orient(c,d,b);return ((x>0&&y<0)||(x<0&&y>0))&&((z>0&&w<0)||(z<0&&w>0));}
Triangle ccw(Triangle t,const std::vector<V2>&p){if(orient(p[t[0]],p[t[1]],p[t[2]])<0)std::swap(t[1],t[2]);return t;}
long double incircle(V2 a,V2 b,V2 c,V2 p){
 const long double ax=(long double)a.x-p.x,ay=(long double)a.y-p.y,bx=(long double)b.x-p.x,by=(long double)b.y-p.y,cx=(long double)c.x-p.x,cy=(long double)c.y-p.y;
 const long double al=ax*ax+ay*ay,bl=bx*bx+by*by,cl=cx*cx+cy*cy;
 const long double determinant=al*(bx*cy-by*cx)-bl*(ax*cy-ay*cx)+cl*(ax*by-ay*bx);
 const long double permanent=al*(std::abs(bx*cy)+std::abs(by*cx))+bl*(std::abs(ax*cy)+std::abs(ay*cx))+cl*(std::abs(ax*by)+std::abs(ay*bx));
 // Conservative long-double roundoff filter. Treat unresolved cocircular cases
 // as ties; they affect only diagonal quality, never point inclusion/topology.
 return std::abs(determinant)<=128*std::numeric_limits<long double>::epsilon()*permanent?0:determinant;
}
void legalize(std::vector<Triangle>&triangles,const std::vector<V2>&p,const std::set<EdgeKey>&protectedEdges){
 // Deterministic Lawson legalization, with filtered long-double predicates.
 // Cocircular edges retain their prior diagonal, avoiding flip oscillation.
 std::map<EdgeKey,std::set<int>> adjacency;std::set<EdgeKey> pending;
 auto attach=[&](int f){const auto t=triangles[f];for(int k=0;k<3;++k){const auto e=key(t[k],t[(k+1)%3]);adjacency[e].insert(f);pending.insert(e);}};
 auto detach=[&](int f){const auto t=triangles[f];for(int k=0;k<3;++k){const auto e=key(t[k],t[(k+1)%3]);auto found=adjacency.find(e);found->second.erase(f);if(found->second.empty())adjacency.erase(found);}};
 for(int f=0;f<int(triangles.size());++f)attach(f);
 size_t flips=0;while(!pending.empty()){const auto edge=*pending.begin();pending.erase(pending.begin());if(protectedEdges.count(edge))continue;auto entry=adjacency.find(edge);if(entry==adjacency.end()||entry->second.size()!=2)continue;auto it=entry->second.begin();const int f=*it++,g=*it;int c=-1,d=-1;for(int v:triangles[f])if(v!=edge.first&&v!=edge.second)c=v;for(int v:triangles[g])if(v!=edge.first&&v!=edge.second)d=v;
  if(!properCross(p[edge.first],p[edge.second],p[c],p[d]))continue;
  const auto t=triangles[f];if(incircle(p[t[0]],p[t[1]],p[t[2]],p[d])<=0)continue;
  if(++flips>p.size()*p.size()*8)throw std::runtime_error("UV cage Delaunay legalization did not converge");
  detach(f);detach(g);triangles[f]=ccw({c,d,edge.first},p);triangles[g]=ccw({d,c,edge.second},p);attach(f);attach(g);
 }
}
std::vector<Triangle> triangulatePoints(const std::vector<V2>&p){
 // Start at the actual convex hull. A finite artificial supertriangle can eat
 // valid hull slivers when a real point lies close to a boundary edge.
 std::vector<int> order;for(int i=0;i<int(p.size());++i)order.push_back(i);
 std::sort(order.begin(),order.end(),[&](int a,int b){return std::make_pair(p[a].x,p[a].y)<std::make_pair(p[b].x,p[b].y);});
 std::vector<int> hull;for(int v:order){while(hull.size()>=2&&orient(p[hull[hull.size()-2]],p[hull.back()],p[v])<=0)hull.pop_back();hull.push_back(v);}
 const size_t lower=hull.size();for(int j=int(order.size())-2;j>=0;--j){int v=order[j];while(hull.size()>lower&&orient(p[hull[hull.size()-2]],p[hull.back()],p[v])<=0)hull.pop_back();hull.push_back(v);}hull.pop_back();
 if(hull.size()<3)throw std::invalid_argument("UV cage particles are collinear");
 std::set<int> onHull(hull.begin(),hull.end());std::vector<Triangle> triangles;
 for(size_t i=1;i+1<hull.size();++i)triangles.push_back({hull[0],hull[i],hull[i+1]});
 for(int v=0;v<int(p.size());++v)if(!onHull.count(v)){std::vector<Triangle> next;bool inserted=false;
  for(auto t:triangles){bool contains=true;for(int k=0;k<3;++k)if(orient(p[t[k]],p[t[(k+1)%3]],p[v])<0)contains=false;
   if(!contains){next.push_back(t);continue;}inserted=true;
   for(int k=0;k<3;++k)if(orient(p[t[k]],p[t[(k+1)%3]],p[v])>0)next.push_back({t[k],t[(k+1)%3],v});
  }
  if(!inserted)throw std::runtime_error("UV cage point insertion failed");
  triangles=std::move(next);
 }
 legalize(triangles,p,{});
 return triangles;
}
bool hasEdge(const std::vector<Triangle>&triangles,int a,int b){for(auto t:triangles)for(int k=0;k<3;++k)if(key(t[k],t[(k+1)%3])==key(a,b))return true;return false;}
void recoverConstraint(std::vector<Triangle>&triangles,int a,int b,const std::vector<V2>&p){
 if(hasEdge(triangles,a,b))return;
 // Recover by flipping only edges crossed by the requested constraint. Unlike
 // replacing a crossed-triangle cavity by its boundary, this cannot swallow
 // low-valence interior particles or already-recovered noncrossing constraints.
 std::map<EdgeKey,std::set<int>> adjacency;
 auto attach=[&](int f){const auto t=triangles[f];for(int k=0;k<3;++k)adjacency[key(t[k],t[(k+1)%3])].insert(f);};
 auto detach=[&](int f){const auto t=triangles[f];for(int k=0;k<3;++k){const auto edge=key(t[k],t[(k+1)%3]);auto it=adjacency.find(edge);it->second.erase(f);if(it->second.empty())adjacency.erase(it);}};
 for(int f=0;f<int(triangles.size());++f)attach(f);
 auto crosses=[&](EdgeKey e){return e.first!=a&&e.first!=b&&e.second!=a&&e.second!=b&&properCross(p[a],p[b],p[e.first],p[e.second]);};
 std::vector<std::pair<long double,EdgeKey>> intersections;const V2 direction=p[b]-p[a];
 for(const auto&entry:adjacency)if(crosses(entry.first)){const auto edge=entry.first;const V2 q=p[edge.first]-p[a],d=p[edge.second]-p[edge.first];const long double den=(long double)direction.x*d.y-(long double)direction.y*d.x;intersections.push_back({((long double)q.x*d.y-(long double)q.y*d.x)/den,edge});}
 std::sort(intersections.begin(),intersections.end());std::deque<EdgeKey> pending;for(const auto&entry:intersections)pending.push_back(entry.second);
 size_t attempts=0,stalled=0;while(!pending.empty()){
  if(++attempts>triangles.size()*triangles.size()*8)throw std::runtime_error("UV cage constraint recovery did not converge");
  const auto edge=pending.front();pending.pop_front();auto it=adjacency.find(edge);if(it==adjacency.end()||!crosses(edge))continue;
  if(it->second.size()!=2)throw std::runtime_error("UV cage source constraint crosses hull boundary");
  auto face=it->second.begin();const int f=*face++,g=*face;int c=-1,d=-1;for(int v:triangles[f])if(v!=edge.first&&v!=edge.second)c=v;for(int v:triangles[g])if(v!=edge.first&&v!=edge.second)d=v;
  if(!properCross(p[edge.first],p[edge.second],p[c],p[d])){pending.push_back(edge);if(++stalled>pending.size())throw std::runtime_error("UV cage source constraint cannot be recovered");continue;}
  stalled=0;detach(f);detach(g);triangles[f]=ccw({c,d,edge.first},p);triangles[g]=ccw({d,c,edge.second},p);attach(f);attach(g);if(crosses(key(c,d)))pending.push_back(key(c,d));
 }
 if(!hasEdge(triangles,a,b))throw std::runtime_error("UV cage failed to recover a source constraint");
}
std::array<double,3> barycentric(const RestCage&c,int face,V2 p){const auto t=c.triangles().at(size_t(face));const V2 a=c.uv()[t[0]],b=c.uv()[t[1]],d=c.uv()[t[2]];const long double den=orient(a,b,d);return {double(orient(b,d,p)/den),double(orient(d,a,p)/den),double(orient(a,b,p)/den)};}
void validateBinding(const RestCage&c,const CageBinding&b){if((b.offsetMode!=CageOffsetMode::WorldFixed&&b.offsetMode!=CageOffsetMode::TriangleFrame)||b.triangle<0||size_t(b.triangle)>=c.triangles().size()||!finite(b.offsetCoordinates)||!finite(b.worldRestOffset))throw std::invalid_argument("invalid UV cage binding");double sum=0;for(double w:b.barycentric){if(!std::isfinite(w)||w< -1e-10||w>1+1e-10)throw std::invalid_argument("invalid UV cage barycentric weight");sum+=w;}if(std::abs(sum-1)>1e-10)throw std::invalid_argument("UV cage weights must sum to one");}
V3 evaluate(const std::array<V3,3>&x,const CageBinding&b){V3 result=x[0]*b.barycentric[0]+x[1]*b.barycentric[1]+x[2]*b.barycentric[2];if(b.offsetMode==CageOffsetMode::WorldFixed)return result+b.worldRestOffset;if(b.offsetCoordinates.x==0&&b.offsetCoordinates.y==0&&b.offsetCoordinates.z==0)return result;const V3 e1=x[1]-x[0],e2=x[2]-x[0],normal=cross(e1,e2);const double n=length(normal);if(n<=1e-20)throw std::runtime_error("cannot transport carrier offset through collapsed cage triangle");const V3 u=e1*(1/length(e1)),w=normal*(1/n),v=cross(w,u);return result+u*b.offsetCoordinates.x+v*b.offsetCoordinates.y+w*b.offsetCoordinates.z;}
}

RestCage buildRestCage(const Graph&g,const CageOptions&opt){
 if(g.particles.size()<3)throw std::invalid_argument("UV cage needs at least three particles");
 if(opt.maxFaces==0||opt.maxVertices==0||opt.maxFaces>8400||opt.maxVertices>25200||g.particles.size()>opt.maxVertices)throw std::runtime_error("UV cage vertex/face budget exceeded");
 RestCage c;for(const auto&p:g.particles){if(!finite(p.uv)||!finite(p.rest))throw std::invalid_argument("UV cage requires finite rest data");c.uv_.push_back(p.uv);c.rest_.push_back(p.rest);}
 for(size_t i=0;i<c.uv_.size();++i)for(size_t j=0;j<i;++j)if(length(c.uv_[i]-c.uv_[j])<=1e-13)throw std::invalid_argument("UV cage has duplicate UV particles; consolidate physical junctions first");
 std::map<EdgeKey,std::vector<int>> constraints;
 for(size_t ei=0;ei<g.edges.size();++ei){const auto&e=g.edges[ei];if(e.a<0||e.b<0||size_t(e.a)>=c.uv_.size()||size_t(e.b)>=c.uv_.size()||e.a==e.b)throw std::invalid_argument("UV cage has invalid source edge");const V2 a=c.uv_[e.a],b=c.uv_[e.b],d=b-a;const long double dd=(long double)d.x*d.x+(long double)d.y*d.y;std::vector<std::pair<long double,int>> ordered{{0,e.a},{1,e.b}};
  // A source segment may have a third physical node in its interior (T junction
  // or overlapping fibers). Preserve it, and deduplicate only geometric spans.
  for(int i=0;i<int(c.uv_.size());++i)if(i!=e.a&&i!=e.b){const V2 q=c.uv_[i]-a;const long double along=((long double)q.x*d.x+(long double)q.y*d.y)/dd;if(along>0&&along<1&&std::abs(orient(a,b,c.uv_[i]))<=1e-14L*std::sqrt(dd))ordered.push_back({along,i});}
  std::sort(ordered.begin(),ordered.end());for(size_t i=1;i<ordered.size();++i)constraints[key(ordered[i-1].second,ordered[i].second)].push_back(int(ei));
 }
 for(const auto&entry:constraints)c.constraints_.push_back({{entry.first.first,entry.first.second},entry.second});
 for(size_t i=0;i<c.constraints_.size();++i)for(size_t j=0;j<i;++j){const auto a=c.constraints_[i].particles,b=c.constraints_[j].particles;if(a[0]==b[0]||a[0]==b[1]||a[1]==b[0]||a[1]==b[1])continue;if(properCross(c.uv_[a[0]],c.uv_[a[1]],c.uv_[b[0]],c.uv_[b[1]]))throw std::invalid_argument("UV cage source constraints cross without a shared physical particle");}
 c.triangles_=triangulatePoints(c.uv_);
 for(const auto&e:c.constraints_)recoverConstraint(c.triangles_,e.particles[0],e.particles[1],c.uv_);
 std::set<EdgeKey> protectedEdges;for(const auto&e:c.constraints_)protectedEdges.insert(key(e.particles[0],e.particles[1]));
 legalize(c.triangles_,c.uv_,protectedEdges);
 if(c.triangles_.empty())throw std::invalid_argument("UV cage particles are collinear");
 if(c.triangles_.size()>opt.maxFaces||c.triangles_.size()>opt.maxVertices/3)throw std::runtime_error("UV cage vertex/face budget exceeded");
 // Canonical CCW indexing makes triangle IDs independent of cavity bookkeeping.
 std::vector<unsigned char> represented(c.uv_.size(),0);std::set<EdgeKey> meshEdges;
 for(auto&t:c.triangles_){const auto first=std::min_element(t.begin(),t.end());std::rotate(t.begin(),first,t.end());if(orient(c.uv_[t[0]],c.uv_[t[1]],c.uv_[t[2]])<=0)throw std::runtime_error("UV cage emitted a degenerate face");for(int k=0;k<3;++k){represented[t[k]]=1;meshEdges.insert(key(t[k],t[(k+1)%3]));}}
 std::sort(c.triangles_.begin(),c.triangles_.end());
 if(std::find(represented.begin(),represented.end(),0)!=represented.end())throw std::runtime_error("UV cage triangulation lost a physical particle");
 for(auto e:c.constraints_)if(!meshEdges.count(key(e.particles[0],e.particles[1])))throw std::runtime_error("UV cage triangulation lost a source constraint");
 return c;
}

std::optional<CageBinding> bindCageTriangle(const RestCage&c,int triangle,V2 uv,std::optional<V3> carrier,CageOffsetMode mode){
 if((mode!=CageOffsetMode::WorldFixed&&mode!=CageOffsetMode::TriangleFrame)||triangle<0||size_t(triangle)>=c.triangles().size()||!finite(uv)||(carrier&&!finite(*carrier)))throw std::invalid_argument("invalid UV cage query");
 CageBinding result;result.triangle=triangle;result.offsetMode=mode;result.barycentric=barycentric(c,triangle,uv);for(double w:result.barycentric)if(w< -1e-11||w>1+1e-11)return std::nullopt;
 // Snap only round-off at exact edges/vertices, never select alternative support.
 for(double&w:result.barycentric)if(std::abs(w)<1e-14)w=0;
 double sum=result.barycentric[0]+result.barycentric[1]+result.barycentric[2];for(double&w:result.barycentric)w/=sum;
 if(carrier){const auto t=c.triangles()[triangle];const V3 a=c.rest()[t[0]],e1=c.rest()[t[1]]-a,e2=c.rest()[t[2]]-a,base=a*result.barycentric[0]+c.rest()[t[1]]*result.barycentric[1]+c.rest()[t[2]]*result.barycentric[2],offset=*carrier-base,normal=cross(e1,e2);result.worldRestOffset=offset;const double n2=dot(normal,normal);if(n2<=1e-40)throw std::invalid_argument("UV cage rest triangle collapsed in world space");const V3 u=e1*(1/length(e1)),w=normal*(1/std::sqrt(n2)),v=cross(w,u);result.offsetCoordinates={dot(offset,u),dot(offset,v),dot(offset,w)};}
 return result;
}
std::optional<CageBinding> bindCageUV(const RestCage&c,V2 uv,std::optional<V3> carrier,const std::vector<unsigned char>*enabled,CageOffsetMode mode){
 if(enabled&&enabled->size()!=c.triangles().size())throw std::invalid_argument("UV cage face mask size mismatch");
 if(!finite(uv)||(carrier&&!finite(*carrier)))throw std::invalid_argument("invalid UV cage query");
 for(int f=0;f<int(c.triangles().size());++f)if(!enabled||(*enabled)[f]){auto binding=bindCageTriangle(c,f,uv,carrier,mode);if(binding)return binding;}
 return std::nullopt;
}
std::optional<CageBinding> bindPatchUV(const RestCage&c,const Patch&p,V2 uv,const std::vector<unsigned char>*enabled,CageOffsetMode mode){return bindCageUV(c,uv,p.sample(uv),enabled,mode);}
V3 evaluateCage(const RestCage&c,const CageBinding&b,const Graph&g){validateBinding(c,b);if(g.particles.size()<c.uv().size())throw std::invalid_argument("UV cage current particle count mismatch");const auto t=c.triangles()[b.triangle];std::array<V3,3>x;for(int k=0;k<3;++k){x[k]=g.particles[t[k]].x;if(!finite(x[k]))throw std::invalid_argument("UV cage current particle is not finite");}return evaluate(x,b);}
V3 cageRestPosition(const RestCage&c,const CageBinding&b){validateBinding(c,b);const auto t=c.triangles()[b.triangle];return evaluate({c.rest()[t[0]],c.rest()[t[1]],c.rest()[t[2]]},b);}
int cageTriangleComponent(const RestCage&c,int face,const Graph&g){if(face<0||size_t(face)>=c.triangles().size()||g.particles.size()<c.uv().size())throw std::invalid_argument("invalid UV cage component query");const auto t=c.triangles()[face];const int owner=g.particles[t[0]].component;return g.particles[t[1]].component==owner&&g.particles[t[2]].component==owner?owner:-1;}
}
