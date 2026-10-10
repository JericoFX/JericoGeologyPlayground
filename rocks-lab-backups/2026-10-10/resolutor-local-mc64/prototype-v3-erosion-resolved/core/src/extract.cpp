#include "rocks/mesh.hpp"
#include "face_resolution.hpp"
#include <map>
#include <tuple>
#include "../../third_party/paris/mc_table.hpp"
#include <array>
#include <chrono>
#include <stdexcept>
#include <set>
namespace rocks {
static constexpr int edges[12][2]={{0,1},{2,3},{4,5},{6,7},{0,2},{1,3},{4,6},{5,7},{0,4},{1,5},{2,6},{3,7}};
bool ambiguousCase(unsigned mask){
 // Conservative: reject alternating-sign faces and disconnected positive OR negative corner edge graphs.
 const int faces[6][4]={{0,1,3,2},{4,5,7,6},{0,1,5,4},{2,3,7,6},{0,2,6,4},{1,3,7,5}};
 for(auto &f:faces){unsigned b=0;for(int k=0;k<4;k++)b|=((mask>>f[k])&1)<<k;if(b==5||b==10)return true;}
 for(int sign=0;sign<2;sign++){unsigned seen=0;int groups=0;for(int i=0;i<8;i++)if(int((mask>>i)&1)==sign&&!(seen&(1u<<i))){groups++;unsigned front=1u<<i;while(front){unsigned next=0;seen|=front;for(auto&e:edges){if((front&(1u<<e[0]))&&int((mask>>e[1])&1)==sign)next|=1u<<e[1];if((front&(1u<<e[1]))&&int((mask>>e[0])&1)==sign)next|=1u<<e[0];}front=next&~seen;}}if(groups>1)return true;}return false;
}
// Explicit, metered contraction of microscopic MC edges. This is confined
// to v7 weathered and v12 granite; older recipes retain their behavior.
// Keep an existing field root, enforce the manifold link condition and reject
// every local orientation reversal. Two incident faces disappear per collapse.
void contractSubresolutionEdges(Extraction&ex,double size){
 auto&m=ex.mesh;const double areaLimit=1e-12*size*size,distanceLimit=1e-5*size;
 for(unsigned iteration=0;iteration<64;iteration++){
  std::size_t bad=m.indices.size();
  for(std::size_t i=0;i<m.indices.size();i+=3)if(length(cross(m.positions[m.indices[i+1]]-m.positions[m.indices[i]],m.positions[m.indices[i+2]]-m.positions[m.indices[i]]))*.5<=areaLimit){bad=i;break;}
  if(bad==m.indices.size())break;
  std::array<std::pair<double,std::pair<unsigned,unsigned>>,3> edges;
  for(unsigned k=0;k<3;k++){unsigned a=m.indices[bad+k],b=m.indices[bad+(k+1)%3];edges[k]={length(m.positions[a]-m.positions[b]),{a,b}};}
  std::sort(edges.begin(),edges.end());bool collapsed=false;
  for(auto [distance,edge]:edges){
   if(distance>distanceLimit)continue;
   for(unsigned direction=0;direction<2&&!collapsed;direction++){
    unsigned keep=direction?edge.second:edge.first,remove=direction?edge.first:edge.second;std::set<unsigned> nk,nr,opposite;std::set<std::pair<unsigned,unsigned>> lk,lr;unsigned incident=0;
    for(std::size_t i=0;i<m.indices.size();i+=3){bool hasKeep=false,hasRemove=false;for(unsigned k=0;k<3;k++){hasKeep|=m.indices[i+k]==keep;hasRemove|=m.indices[i+k]==remove;}
     for(unsigned k=0;k<3;k++){auto v=m.indices[i+k];if(hasKeep&&v!=keep)nk.insert(v);if(hasRemove&&v!=remove)nr.insert(v);if(hasKeep&&hasRemove&&v!=keep&&v!=remove)opposite.insert(v);}
     for(unsigned k=0;k<3;k++){auto a=m.indices[i+(k+1)%3],b=m.indices[i+(k+2)%3];if(m.indices[i+k]==keep)lk.insert(std::minmax(a,b));if(m.indices[i+k]==remove)lr.insert(std::minmax(a,b));}
     incident+=hasKeep&&hasRemove;
    }
    std::set<unsigned> common;for(auto v:nk)if(nr.contains(v))common.insert(v);
    if(incident!=2||opposite.size()!=2||common!=opposite)continue;
    // The edge link consists only of its two opposite vertices. Endpoint
    // links must not share an edge as well (the tetrahedron exception).
    bool sharedLinkEdge=false;for(auto e:lk)if(lr.contains(e)){sharedLinkEdge=true;break;}if(sharedLinkEdge)continue;
    bool safe=true;std::set<std::array<unsigned,3>> faces;std::vector<std::uint32_t> indices;indices.reserve(m.indices.size()-6);
    for(std::size_t i=0;i<m.indices.size();i+=3){std::array<unsigned,3> before{m.indices[i],m.indices[i+1],m.indices[i+2]},after=before;for(auto&v:after)if(v==remove)v=keep;
     if(after[0]==after[1]||after[1]==after[2]||after[0]==after[2])continue;
     auto canonical=after;std::sort(canonical.begin(),canonical.end());if(!faces.insert(canonical).second){safe=false;break;}
     Vec3 oldNormal=cross(m.positions[before[1]]-m.positions[before[0]],m.positions[before[2]]-m.positions[before[0]]),newNormal=cross(m.positions[after[1]]-m.positions[after[0]],m.positions[after[2]]-m.positions[after[0]]);
     if(dot(oldNormal,newNormal)<=0){safe=false;break;}indices.insert(indices.end(),after.begin(),after.end());
    }
    if(!safe||indices.size()+6!=m.indices.size())continue;
    m.indices=std::move(indices);ex.contractedEdges++;ex.contractedFaces+=2;ex.maxContractionDistance=std::max(ex.maxContractionDistance,distance);collapsed=true;
   }if(collapsed)break;
  }
  if(!collapsed)throw std::runtime_error("microscopic MC edge cannot be contracted with topology/orientation guarantees");
  if(iteration==63)throw std::runtime_error("microscopic MC edge contraction budget exceeded");
 }
 if(ex.contractedEdges){
  std::vector<bool> used(m.positions.size());for(auto v:m.indices)used[v]=true;Mesh compact;std::vector<unsigned> mapping(m.positions.size());
  for(std::size_t i=0;i<m.positions.size();i++)if(used[i]){mapping[i]=unsigned(compact.positions.size());compact.positions.push_back(m.positions[i]);compact.normals.push_back(m.normals[i]);compact.resistance.push_back(m.resistance[i]);}
  compact.indices=m.indices;for(auto&v:compact.indices)v=mapping[v];m=std::move(compact);
 }
}
Extraction extract(const RockField&f,const GridSpec&g,Stage stage){auto errors=validateRecipe(f.recipe,g);if(!errors.empty())throw std::invalid_argument(errors.front());using Clock=std::chrono::steady_clock;auto start=Clock::now();Extraction ex;auto &m=ex.mesh;unsigned n=g.cells,N=n+1;std::size_t total=std::size_t(N)*N*N;std::vector<double> values(total);std::vector<int32_t> cache(total*3,-1);double h=(g.max.x-g.min.x)/n;auto id=[&](unsigned x,unsigned y,unsigned z){return (std::size_t(z)*N+y)*N+x;};auto point=[&](unsigned x,unsigned y,unsigned z){return g.min+Vec3{double(x),double(y),double(z)}*h;};
 for(unsigned z=0;z<N;z++)for(unsigned y=0;y<N;y++)for(unsigned x=0;x<N;x++){double v=fieldValue(f,point(x,y,z),stage);if(!std::isfinite(v))throw std::runtime_error("nonfinite field sample");values[id(x,y,z)]=v;if((x==0||y==0||z==0||x==n||y==n||z==n)&&v<=0)ex.boundaryTouched=true;}
 auto sampled=Clock::now();ex.fieldMs=std::chrono::duration<double,std::milli>(sampled-start).count();
 // One canonical entry per grid face; both incident cells consume one decision.
 using FaceKey=std::tuple<unsigned,unsigned,unsigned,unsigned>;
 std::map<FaceKey,detail::FaceDecision> faceDecisions;
 const bool resolveFaces=detail::eligibleErosionResolver(f,g);
 for(unsigned z=0;z<n;z++)for(unsigned y=0;y<n;y++)for(unsigned x=0;x<n;x++){
 std::array<std::size_t,8> ids;std::array<double,8> v;unsigned mask=0;for(unsigned k=0;k<8;k++){ids[k]=id(x+(k&1),y+((k>>1)&1),z+((k>>2)&1));v[k]=values[ids[k]];if(v[k]<0)mask|=1u<<k;}if(mask==0||mask==255)continue;
 bool alternate=false;
 if(ambiguousCase(mask)){
  ex.ambiguousCells++;bool resolved=false;auto resolutionStart=Clock::now();
  if(resolveFaces)if(auto face=detail::simpleAmbiguousFace(mask)){
   unsigned axis=*face<2?2:*face<4?1:0;
   FaceKey key=axis==0?FaceKey{axis,x+(*face==5),y,z}:axis==1?FaceKey{axis,y+(*face==3),x,z}:FaceKey{axis,z+(*face==1),x,y};
   // A shared face is atomic: never change one incident cell while its
   // neighbor remains on an unsupported configuration and a different pairing.
   int nx=int(x),ny=int(y),nz=int(z);int delta=(*face%2)?1:-1;
   if(axis==0)nx+=delta;else if(axis==1)ny+=delta;else nz+=delta;
   bool neighborCompatible=nx>=0&&ny>=0&&nz>=0&&nx<int(n)&&ny<int(n)&&nz<int(n);
   if(neighborCompatible){
    unsigned neighborMask=0;for(unsigned k=0;k<8;k++)if(values[id(unsigned(nx)+(k&1),unsigned(ny)+((k>>1)&1),unsigned(nz)+((k>>2)&1))]<0)neighborMask|=1u<<k;
    auto neighborFace=detail::simpleAmbiguousFace(neighborMask);
    neighborCompatible=neighborFace.has_value()&&*neighborFace==(*face^1u);
   }
   if(neighborCompatible){
   auto it=faceDecisions.find(key);
   if(it==faceDecisions.end()){
    std::array<Vec3,4> points;std::array<double,4> faceValues;
    for(unsigned j=0;j<4;j++){unsigned k=detail::cubeFaces[*face][j];points[j]=point(x+(k&1),y+((k>>1)&1),z+((k>>2)&1));faceValues[j]=v[k];}
    auto decision=detail::decideErosionFace(f,stage,points,faceValues);ex.faceCertificateSamples+=decision.evaluations;
    if(decision.negativeConnected.has_value())ex.resolvedAmbiguousFaces++;
    it=faceDecisions.emplace(key,std::move(decision)).first;
   }
   auto original=detail::tableConnectsNegative(mask,*face),complement=detail::tableConnectsNegative(255-mask,*face);
   if(it->second.negativeConnected.has_value()&&original.has_value()&&complement.has_value()&&*original!=(!*complement)){alternate=*original!=*it->second.negativeConnected;resolved=true;}
   }
  }
  if(resolved)ex.resolvedAmbiguousCells++;else ex.unresolvedAmbiguousCells++;
  ex.ambiguityResolutionMs+=std::chrono::duration<double,std::milli>(Clock::now()-resolutionStart).count();
 }
 std::uint32_t vi[12]{};for(unsigned e=0;e<12;e++){int a=edges[e][0],b=edges[e][1];if((v[a]<0)==(v[b]<0))continue;unsigned axis=e<4?0:e<8?1:2;auto key=ids[a]*3+axis;if(cache[key]<0){double t=v[a]/(v[a]-v[b]);Vec3 pa=point(x+(a&1),y+((a>>1)&1),z+((a>>2)&1));Vec3 pb=point(x+(b&1),y+((b>>1)&1),z+((b>>2)&1));double lo=0,hi=1; for(int refine=0;refine<32;refine++){double mid=(lo+hi)*.5;double vm=fieldValue(f,pa+(pb-pa)*mid,stage);if((vm<0)==(v[a]<0))lo=mid;else hi=mid;} t=(lo+hi)*.5;Vec3 p=pa+(pb-pa)*t;cache[key]=int32_t(m.positions.size());m.positions.push_back(p);auto sample=evaluate(f,p,stage);m.normals.push_back(normalized(sample.gradient));m.resistance.push_back(sample.resistance);}vi[e]=std::uint32_t(cache[key]);}
 auto conf=marching_cube_tris[alternate?255-mask:mask];unsigned count=conf&15;for(unsigned j=0;j<count;j++){std::uint32_t a=vi[(conf>>(4+12*j))&15],b=vi[(conf>>(8+12*j))&15],c=vi[(conf>>(12+12*j))&15];// Complement configuration swaps inside/outside, so reverse ALL its faces.
// The unambiguous branch retains the exact original table and vertex order.
if(alternate)std::swap(b,c);
m.indices.insert(m.indices.end(),{a,b,c});}}
 if((f.recipe.generatorVersion=="rocks-v7"||(f.recipe.generatorVersion=="rocks-v12"&&f.recipe.family=="jointed_granite"))&&!ex.ambiguousCells&&!ex.boundaryTouched)contractSubresolutionEdges(ex,f.recipe.size);
 ex.extractNormalsMs=std::chrono::duration<double,std::milli>(Clock::now()-sampled).count();ex.coreBufferBytes=values.capacity()*sizeof(double)+cache.capacity()*sizeof(int32_t)+(m.positions.capacity()+m.normals.capacity())*sizeof(Vec3)+m.resistance.capacity()*sizeof(double)+m.indices.capacity()*sizeof(std::uint32_t);return ex;
}
}
