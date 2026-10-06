#include "jerico/evaluate.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
namespace jerico {
namespace {
using Face=std::vector<size_t>;using Edge=std::pair<size_t,size_t>;
struct Planar {Face vs;V3 n;std::string material,side;};
std::string source_material_side(const Json&p,size_t face,V3 normal){if(p.contains("face_material_sides"))return p["face_material_sides"][face].get<std::string>();return normal[1]<-.9999?"front":normal[1]>.9999?"back":"";}
Edge ordered(size_t a,size_t b){return std::minmax(a,b);}
double len(V3 v){return std::sqrt(dot(v,v));}
std::vector<Planar> merge_faces(const Json&p){const auto&v=p["vertices"];std::vector<Planar> groups;std::vector<double> planes;std::vector<std::map<Edge,int>> es;for(size_t fi=0;fi<p["faces"].size();fi++){Face f=p["faces"][fi];V3 a=v[f[0]],b=v[f[1]],c=v[f[2]];V3 n=norm(cross(sub(b,a),sub(c,a)));double d=dot(n,a);std::string material=p.contains("face_materials")?p["face_materials"][fi].get<std::string>():p["material"].get<std::string>();std::string side=source_material_side(p,fi,n);size_t g=0;for(;g<groups.size();g++)if(dot(n,groups[g].n)>1-1e-10&&std::abs(d-planes[g])<1e-10&&groups[g].material==material&&groups[g].side==side)break;if(g==groups.size()){groups.push_back({{},n,material,side});planes.push_back(d);es.emplace_back();}for(size_t i=0;i<f.size();i++){Edge e{f[i],f[(i+1)%f.size()]},rev{e.second,e.first};auto it=es[g].find(rev);if(it!=es[g].end())es[g].erase(it);else es[g][e]=1;}}
 std::vector<Planar> result;for(size_t g=0;g<groups.size();g++){auto edges=es[g];while(!edges.empty()){Face f;size_t start=edges.begin()->first.first,cur=start;do{f.push_back(cur);auto it=edges.lower_bound({cur,0});if(it==edges.end()||it->first.first!=cur)throw std::runtime_error("native bevel planar boundary is open");cur=it->first.second;edges.erase(it);if(f.size()>100000)throw std::runtime_error("bevel boundary too long");}while(cur!=start);
 // Source cap tessellation vertices on straight boundaries are irrelevant to a fillet.
 bool changed=true;while(changed&&f.size()>3){changed=false;for(size_t i=0;i<f.size();i++){V3 a=v[f[(i+f.size()-1)%f.size()]],b=v[f[i]],c=v[f[(i+1)%f.size()]];if(len(cross(sub(b,a),sub(c,b)))<1e-12&&dot(sub(b,a),sub(c,b))>=0){f.erase(f.begin()+i);changed=true;break;}}}
 result.push_back({f,groups[g].n,groups[g].material,groups[g].side});}}
 return result;
}
std::vector<std::array<size_t,3>> tris3(const Face&f,const std::vector<V3>&vs,V3 n){V3 ref=std::abs(n[0])<.9?V3{1,0,0}:V3{0,1,0};auto u=norm(sub(ref,mul(n,dot(n,ref)))),v=cross(n,u);std::vector<V2> p;for(auto i:f)p.push_back({dot(vs[i],u),dot(vs[i],v)});auto ts=triangulate(p);for(auto&t:ts)for(auto&i:t)i=f[i];return ts;}
}
Json evaluate_legacy(const Json&p){double requested=p.value("bevel",0.0);if(requested<=0){Json out=p;Json normals=Json::array();for(auto&f:p["faces"]){V3 a=p["vertices"][f[0].get<size_t>()],b=p["vertices"][f[1].get<size_t>()],c=p["vertices"][f[2].get<size_t>()];V3 n=norm(cross(sub(b,a),sub(c,a)));Json ns=Json::array();for(size_t i=0;i<f.size();i++)ns.push_back(n);normals.push_back(ns);}out["normals"]=normals;return out;}
 double width=requested,shortest=1e300;for(auto&f:p["faces"])for(size_t i=0;i<f.size();i++)shortest=std::min(shortest,len(sub(p["vertices"][f[i].get<size_t>()].get<V3>(),p["vertices"][f[(i+1)%f.size()].get<size_t>()].get<V3>())));
 if(p.contains("cuts")&&!p["cuts"].empty()&&shortest<width){Json q=p;q["bevel"]=0;auto out=evaluate_legacy(q);out["native_bevel"]={{"requested_m",requested},{"omitted",true},{"reason","Source feature edge smaller than requested decorative bevel radius"}};return out;}
 auto planes=merge_faces(p);bool use_source=false;std::map<size_t,int> counts;for(auto&pl:planes)for(auto v:pl.vs)counts[v]++;for(auto&kv:counts)if(kv.second>3)use_source=true;
 for(auto&pl:planes){try{std::vector<V3> ov=p["vertices"].get<std::vector<V3>>();tris3(pl.vs,ov,pl.n);}catch(const std::exception&){use_source=true;}}
 if(use_source){planes.clear();for(size_t fi=0;fi<p["faces"].size();fi++){Face f=p["faces"][fi];V3 a=p["vertices"][f[0]],b=p["vertices"][f[1]],c=p["vertices"][f[2]];planes.push_back({f,norm(cross(sub(b,a),sub(c,a))),p.contains("face_materials")?p["face_materials"][fi].get<std::string>():p["material"].get<std::string>(),source_material_side(p,fi,norm(cross(sub(b,a),sub(c,a))))});}}
 auto original=p["vertices"].get<std::vector<V3>>();std::map<Edge,std::vector<size_t>> adjacent;std::map<size_t,std::vector<size_t>> incident;for(size_t fi=0;fi<planes.size();fi++)for(size_t k=0;k<planes[fi].vs.size();k++){auto a=planes[fi].vs[k],b=planes[fi].vs[(k+1)%planes[fi].vs.size()];adjacent[ordered(a,b)].push_back(fi);incident[a].push_back(fi);}for(auto&e:adjacent)if(e.second.size()!=2)throw std::runtime_error(p["id"].get<std::string>()+": native bevel requires two-sided edge boundary");
 // Match Blender's overlap protection conservatively. Radius is bounded by each
 // planar corner's bisector movement; all adjustment is recorded for comparison.
 for(auto&face:planes)for(size_t k=0;k<face.vs.size();k++){V3 a=original[face.vs[(k+face.vs.size()-1)%face.vs.size()]],v=original[face.vs[k]],c=original[face.vs[(k+1)%face.vs.size()]];auto u=norm(sub(a,v)),w=norm(sub(c,v));double sinhalf=std::sqrt(std::max(0.0,(1-dot(u,w))/2));width=std::min(width,.45*std::min(len(sub(a,v)),len(sub(c,v)))*sinhalf);}
 std::map<Edge,bool> selected;for(auto&e:adjacent){double angle=std::acos(std::clamp(dot(planes[e.second[0]].n,planes[e.second[1]].n),-1.0,1.0));selected[e.first]=angle>.35;}
 std::vector<V3> verts;std::vector<Face> faces;std::vector<std::vector<V3>> normals;std::vector<std::string> materials,sides;std::map<std::pair<size_t,size_t>,size_t> inset;
 auto vertex=[&](V3 x){verts.push_back(x);return verts.size()-1;};
 auto face=[&](Face f,std::vector<V3>ns,std::string mat,std::string side=""){if(f.size()<3)return;V3 n=cross(sub(verts[f[1]],verts[f[0]]),sub(verts[f[2]],verts[f[0]]));if(len(n)<=1e-14)return;faces.push_back(f);normals.push_back(ns);materials.push_back(mat);sides.push_back(side);};
 for(size_t fi=0;fi<planes.size();fi++){auto&pl=planes[fi];Face f;for(size_t k=0;k<pl.vs.size();k++){size_t prev=pl.vs[(k+pl.vs.size()-1)%pl.vs.size()],cur=pl.vs[k],next=pl.vs[(k+1)%pl.vs.size()];auto a=norm(sub(original[cur],original[prev])),b=norm(sub(original[next],original[cur]));auto ma=cross(pl.n,a),mb=cross(pl.n,b);double wa=selected[ordered(prev,cur)]?width:0,wb=selected[ordered(cur,next)]?width:0,d=dot(ma,mb),den=1-d*d;if(std::abs(den)<1e-12)throw std::runtime_error("bevel singular face corner");V3 offset=add(mul(ma,(wa-d*wb)/den),mul(mb,(wb-d*wa)/den));size_t vi=vertex(add(original[cur],offset));inset[{fi,cur}]=vi;f.push_back(vi);}for(auto t:tris3(f,verts,pl.n))face({t[0],t[1],t[2]},{pl.n,pl.n,pl.n},pl.material,pl.side);}
 const int segments=3;std::map<std::tuple<size_t,size_t,size_t>,std::vector<size_t>> arc;std::map<size_t,V3> vertexNormals;
 for(auto&e:adjacent){size_t a=e.first.first,b=e.first.second,f=e.second[0],g=e.second[1];V3 nf=planes[f].n,ng=planes[g].n;double angle=std::acos(std::clamp(dot(nf,ng),-1.0,1.0));int count=selected[e.first]?segments:1;std::vector<size_t> rows[2];int end=0;for(size_t v:{a,b}){size_t ia=inset[{f,v}],ib=inset[{g,v}];V3 pa=verts[ia],pb=verts[ib],edge=norm(sub(original[b],original[a]));V3 oa=sub(original[v],pa);oa=sub(oa,mul(edge,dot(oa,edge)));V3 ob=sub(original[v],pb);ob=sub(ob,mul(edge,dot(ob,edge)));V3 control=mul(add(add(pa,oa),add(pb,ob)),.5);auto&row=rows[end++];row.push_back(ia);vertexNormals[ia]=nf;for(int j=1;j<count;j++){double frac=double(j)/count;double s0=std::sin((1-frac)*angle*.5),s1=std::sin(frac*angle*.5),t=s1/(s0+s1),weight=std::cos(angle*.5),A=(1-t)*(1-t),B=2*t*(1-t)*weight,C=t*t;V3 point=mul(add(add(mul(pa,A),mul(control,B)),mul(pb,C)),1/(A+B+C));size_t vi=vertex(point);row.push_back(vi);vertexNormals[vi]=norm(add(mul(nf,std::sin((1-frac)*angle)),mul(ng,std::sin(frac*angle))));}row.push_back(ib);vertexNormals[ib]=ng;arc[{v,f,g}]=row;std::reverse(row.begin(),row.end());arc[{v,g,f}]=row;std::reverse(row.begin(),row.end());}
 std::string mat=p.value("bevel_material",planes[f].material);for(int j=0;j<count;j++){Face q{rows[0][j],rows[1][j],rows[1][j+1],rows[0][j+1]};V3 normal=norm(add(vertexNormals[q[0]],vertexNormals[q[2]]));V3 actual=cross(sub(verts[q[1]],verts[q[0]]),sub(verts[q[2]],verts[q[0]]));if(dot(actual,normal)<0)std::reverse(q.begin(),q.end());std::vector<V3> ns;for(auto i:q)ns.push_back(vertexNormals[i]);face(q,ns,mat,p.contains("bevel_material")?"":planes[f].side);}}
 for(auto&iv:incident){size_t v=iv.first;auto fs=iv.second;std::vector<size_t> cycle{fs[0]};std::set<size_t> used{fs[0]};while(cycle.size()<fs.size()){bool found=false;for(size_t candidate:fs){if(used.count(candidate))continue;if(arc.count({v,cycle.back(),candidate})){cycle.push_back(candidate);used.insert(candidate);found=true;break;}}if(!found)throw std::runtime_error("bevel corner adjacency is disconnected");}if(!arc.count({v,cycle.back(),cycle[0]}))throw std::runtime_error("bevel corner does not close");Face ring;V3 sum{};for(size_t i=0;i<cycle.size();i++){auto&row=arc.at({v,cycle[i],cycle[(i+1)%cycle.size()]});ring.insert(ring.end(),row.begin(),row.end()-1);sum=add(sum,planes[cycle[i]].n);}if(len(sum)<1e-10)throw std::runtime_error("bevel corner normal cancels");V3 n=norm(sum);V3 center{};for(auto i:ring)center=add(center,verts[i]);center=mul(center,1.0/ring.size());
 bool orth=cycle.size()==3&&std::abs(dot(planes[cycle[0]].n,planes[cycle[1]].n))<1e-8&&std::abs(dot(planes[cycle[1]].n,planes[cycle[2]].n))<1e-8&&std::abs(dot(planes[cycle[2]].n,planes[cycle[0]].n))<1e-8;
 if(orth){V3 sphere=sub(original[v],mul(sum,width));center=add(sphere,mul(n,width));}
 size_t ci=vertex(center);std::string mat=p.value("bevel_material",planes[cycle[0]].material);for(size_t k=0;k<ring.size();k++){Face tri{ring[k],ring[(k+1)%ring.size()],ci};V3 actual=cross(sub(verts[tri[1]],verts[tri[0]]),sub(verts[tri[2]],verts[tri[0]]));if(dot(actual,n)<0)std::reverse(tri.begin(),tri.end());std::vector<V3> ns;for(auto i:tri)ns.push_back(i==ci?n:vertexNormals[i]);face(tri,ns,mat,p.contains("bevel_material")?"":planes[cycle[0]].side);}}
 // Native numerical cleanup, same 0.1 micrometre weld bound as the reference exporter.
 std::vector<V3> compact;std::vector<size_t> remap(verts.size());for(size_t i=0;i<verts.size();i++){size_t match=compact.size();for(size_t j=0;j<compact.size();j++)if(len(sub(verts[i],compact[j]))<=1e-7){match=j;break;}remap[i]=match;if(match==compact.size())compact.push_back(verts[i]);}for(auto&f:faces)for(auto&i:f)i=remap[i];verts=compact;
 for(size_t fi=0;fi<faces.size();){auto&f=faces[fi];auto&ns=normals[fi];for(size_t k=0;k<f.size()&&f.size()>=2;){if(f[k]==f[(k+1)%f.size()]){f.erase(f.begin()+k);ns.erase(ns.begin()+k);}else k++;}bool deg=f.size()<3;if(!deg){V3 sum{};for(size_t k=1;k+1<f.size();k++)sum=add(sum,cross(sub(verts[f[k]],verts[f[0]]),sub(verts[f[k+1]],verts[f[0]])));deg=len(sum)<1e-14;}if(deg){faces.erase(faces.begin()+fi);normals.erase(normals.begin()+fi);materials.erase(materials.begin()+fi);sides.erase(sides.begin()+fi);}else fi++;}
 // Orient from mesh adjacency, not an averaged normal: the latter reverses
 // portions of concave notch corners even when their boundary is closed.
 std::map<Edge,std::vector<std::pair<size_t,bool>>> links;for(size_t fi=0;fi<faces.size();fi++)for(size_t k=0;k<faces[fi].size();k++){auto a=faces[fi][k],b=faces[fi][(k+1)%faces[fi].size()];links[ordered(a,b)].push_back({fi,a<b});}
 std::vector<int> flips(faces.size(),-1);for(size_t seed=0;seed<faces.size();seed++){if(flips[seed]>=0)continue;flips[seed]=0;std::vector<size_t> todo{seed};while(!todo.empty()){auto fi=todo.back();todo.pop_back();for(size_t k=0;k<faces[fi].size();k++){auto a=faces[fi][k],b=faces[fi][(k+1)%faces[fi].size()];auto&ls=links[ordered(a,b)];for(auto next:ls){if(next.first==fi)continue;int required=flips[fi]^int((a<b)==next.second);if(flips[next.first]<0){flips[next.first]=required;todo.push_back(next.first);}else if(flips[next.first]!=required)throw std::runtime_error("native bevel has non-orientable connectivity");}}}}
 for(size_t fi=0;fi<faces.size();fi++)if(flips[fi]){std::reverse(faces[fi].begin(),faces[fi].end());std::reverse(normals[fi].begin(),normals[fi].end());}
 Json out=p;out["vertices"]=verts;out["faces"]=faces;out["uvs"]=metric_uvs(out["vertices"],out["faces"]);out["normals"]=normals;out["face_materials"]=materials;out["face_material_sides"]=sides;out["bevel"]=0.0;out["native_bevel"]={{"method","native_face_inset_round_edge_corner_patch"},{"segments",segments},{"requested_m",requested},{"effective_m",width},{"angle_limit",.35},{"exact_blender_topology",false}};return out;
}
namespace {
struct FanRegion {size_t source_vertex;std::vector<size_t> faces;V3 normal{},unit_offset{};};
struct SourceEdge {size_t a,b,f,g;bool selected;};
V3 polygon_normal(const Face&face,const std::vector<V3>&vertices){V3 sum{};for(size_t i=1;i+1<face.size();++i)sum=add(sum,cross(sub(vertices[face[i]],vertices[face[0]]),sub(vertices[face[i+1]],vertices[face[0]])));return norm(sum);}

// Conservative classification retains the measured convex baseline, while the
// general path preserves source surface domains (including holes) explicitly.
bool simple_convex_baseline(const Json&p){
 auto planes=merge_faces(p);auto vertices=p["vertices"].get<std::vector<V3>>();std::map<size_t,size_t> degree;
 for(const auto&plane:planes){for(size_t v:plane.vs)++degree[v];double d=dot(plane.n,vertices[plane.vs[0]]);for(const auto&v:vertices)if(dot(plane.n,v)-d>1e-9)return false;
  for(size_t k=0;k<plane.vs.size();++k){auto u=norm(sub(vertices[plane.vs[(k+plane.vs.size()-1)%plane.vs.size()]],vertices[plane.vs[k]]));auto v=norm(sub(vertices[plane.vs[(k+1)%plane.vs.size()]],vertices[plane.vs[k]]));if(std::sqrt(std::max(0.,(1-dot(u,v))/2))<.02)return false;}
 }
 for(const auto&item:degree)if(item.second!=3)return false;
 return true;
}

Json evaluate_fans(const Json&p){
 const double requested=p.at("bevel");const auto original=p["vertices"].get<std::vector<V3>>();const auto source_faces=p["faces"].get<std::vector<Face>>();
 const std::string source_material=p.value("material","");
 const bool continuous_smooth_surface=(source_material=="iron"||source_material=="rust")&&source_faces.size()>12&&std::all_of(source_faces.begin(),source_faces.end(),[](const Face&f){return f.size()<=4;});
 std::vector<V3> source_normals;for(const auto&f:source_faces)source_normals.push_back(polygon_normal(f,original));
 std::map<Edge,std::vector<size_t>> adjacent;std::vector<std::vector<size_t>> incident(original.size());
 for(size_t fi=0;fi<source_faces.size();++fi)for(size_t k=0;k<source_faces[fi].size();++k){size_t a=source_faces[fi][k],b=source_faces[fi][(k+1)%source_faces[fi].size()];adjacent[ordered(a,b)].push_back(fi);incident[a].push_back(fi);}
 std::map<Edge,bool> selected;std::vector<size_t> selected_count(original.size());
 for(const auto&item:adjacent){if(item.second.size()!=2)throw std::runtime_error(p["id"].get<std::string>()+": source edge is not a two-sided manifold");double angle=std::acos(std::clamp(dot(source_normals[item.second[0]],source_normals[item.second[1]]),-1.,1.));bool active=angle>.35;selected[item.first]=active;if(active){++selected_count[item.first.first];++selected_count[item.first.second];}}
 std::vector<FanRegion> regions;std::map<std::pair<size_t,size_t>,size_t> face_region;
 for(size_t v=0;v<original.size();++v){
  std::set<size_t> remaining(incident[v].begin(),incident[v].end());
  while(!remaining.empty()){
   FanRegion region;region.source_vertex=v;std::vector<size_t> queue{*remaining.begin()};remaining.erase(queue[0]);
   while(!queue.empty()){
    size_t fi=queue.back();queue.pop_back();region.faces.push_back(fi);const auto&f=source_faces[fi];auto at=std::find(f.begin(),f.end(),v);size_t k=at-f.begin();
    for(size_t other:{f[(k+f.size()-1)%f.size()],f[(k+1)%f.size()]}){auto edge=ordered(v,other);if(selected[edge])continue;for(size_t neighbor:adjacent[edge])if(remaining.erase(neighbor))queue.push_back(neighbor);}
   }
   std::vector<V3> constraints;
   for(size_t fi:region.faces){const auto&f=source_faces[fi];size_t k=std::find(f.begin(),f.end(),v)-f.begin();size_t prev=f[(k+f.size()-1)%f.size()],next=f[(k+1)%f.size()];V3 u=norm(sub(original[prev],original[v])),w=norm(sub(original[next],original[v]));double angle=std::acos(std::clamp(dot(u,w),-1.,1.));region.normal=add(region.normal,mul(source_normals[fi],angle));
    if(selected[ordered(prev,v)])constraints.push_back(norm(cross(source_normals[fi],norm(sub(original[v],original[prev])))));
    if(selected[ordered(v,next)])constraints.push_back(norm(cross(source_normals[fi],norm(sub(original[next],original[v])))));
   }
   region.normal=norm(region.normal);
   if(selected_count[v]>=2&&!constraints.empty()){
    if(constraints.size()!=2)throw std::runtime_error("selected-edge fan has nonmanifold boundary");
    V3 a=sub(constraints[0],mul(region.normal,dot(constraints[0],region.normal))),b=sub(constraints[1],mul(region.normal,dot(constraints[1],region.normal)));
    double aa=dot(a,a),bb=dot(b,b),ab=dot(a,b),den=aa*bb-ab*ab;
    if(std::abs(den)<1e-12){V3 average=mul(add(a,b),.5);double squared=dot(average,average);if(squared<1e-16)throw std::runtime_error("bevel fan has contradictory offset boundaries");region.unit_offset=mul(average,1/squared);}
    else region.unit_offset=add(mul(a,(bb-ab)/den),mul(b,(aa-ab)/den));
   }
   const size_t gi=regions.size();for(size_t fi:region.faces)face_region[{fi,v}]=gi;regions.push_back(std::move(region));
  }
 }
 // Overlap protection acts on the actual shared surface fan, never on artificial
 // cap triangulation angles. Source triangles must retain positive orientation.
 auto safe_width=[&](double width){
  for(size_t fi=0;fi<source_faces.size();++fi){const auto&f=source_faces[fi];V3 a=add(original[f[0]],mul(regions[face_region.at({fi,f[0]})].unit_offset,width));for(size_t k=1;k+1<f.size();++k){V3 b=add(original[f[k]],mul(regions[face_region.at({fi,f[k]})].unit_offset,width)),c=add(original[f[k+1]],mul(regions[face_region.at({fi,f[k+1]})].unit_offset,width));double area=dot(cross(sub(b,a),sub(c,a)),source_normals[fi]),source_area=dot(cross(sub(original[f[k]],original[f[0]]),sub(original[f[k+1]],original[f[0]])),source_normals[fi]);if(source_area>1e-16&&area<source_area*.02)return false;}}
  return true;
 };
 double width=requested;if(!safe_width(width)){double lo=0,hi=width;for(int i=0;i<48;++i){double mid=(lo+hi)/2;if(safe_width(mid))lo=mid;else hi=mid;}width=lo*.98;}
 std::vector<V3> vertices;std::vector<Face> faces;std::vector<std::vector<V3>> normals;std::vector<std::string> materials,sides;
 std::vector<std::vector<size_t>> vertex_cache(original.size());double coincidence=std::min(1e-11,std::max(width*1e-7,1e-14));
 auto vertex=[&](size_t source,V3 position){for(size_t i:vertex_cache[source])if(len(sub(vertices[i],position))<=coincidence)return i;size_t i=vertices.size();vertices.push_back(position);vertex_cache[source].push_back(i);return i;};
 auto emit=[&](Face f,std::vector<V3>ns,const std::string&material,const std::string&side=""){
  for(size_t i=0;i<f.size()&&f.size()>1;){if(f[i]==f[(i+1)%f.size()]){f.erase(f.begin()+i);ns.erase(ns.begin()+i);}else ++i;}
  if(f.size()<3)return;
  V3 sum{};for(size_t i=1;i+1<f.size();++i)sum=add(sum,cross(sub(vertices[f[i]],vertices[f[0]]),sub(vertices[f[i+1]],vertices[f[0]])));
  if(len(sum)<1e-18)return;
  faces.push_back(std::move(f));normals.push_back(std::move(ns));materials.push_back(material);sides.push_back(side);
 };
 std::vector<size_t> inset(regions.size());for(size_t i=0;i<regions.size();++i)inset[i]=vertex(regions[i].source_vertex,add(original[regions[i].source_vertex],mul(regions[i].unit_offset,width)));
 for(size_t fi=0;fi<source_faces.size();++fi){Face f;for(size_t v:source_faces[fi])f.push_back(inset[face_region.at({fi,v})]);std::string material=p.contains("face_materials")?p["face_materials"][fi].get<std::string>():p["material"].get<std::string>();for(auto t:tris3(f,vertices,source_normals[fi]))emit({t[0],t[1],t[2]},{source_normals[fi],source_normals[fi],source_normals[fi]},material,source_material_side(p,fi,source_normals[fi]));}
 constexpr int segments=3;std::vector<std::vector<Face>> corner_arcs(original.size());std::map<size_t,V3> arc_normals;
 for(const auto&item:adjacent){if(!selected[item.first])continue;size_t f=item.second[0],g=item.second[1],a=item.first.first,b=item.first.second;const auto&source_face=source_faces[f];size_t ak=std::find(source_face.begin(),source_face.end(),a)-source_face.begin();if(source_face[(ak+1)%source_face.size()]!=b)std::swap(a,b);
  double angle=std::acos(std::clamp(dot(source_normals[f],source_normals[g]),-1.,1.));Face rows[2];int endpoint=0;
  for(size_t v:{a,b}){size_t ga=face_region.at({f,v}),gb=face_region.at({g,v}),ia=inset[ga],ib=inset[gb];V3 pa=vertices[ia],pb=vertices[ib],edge=norm(sub(original[b],original[a]));
   // Two selected edges separate the same two smooth surface fans. Their
   // endpoint profile is one shared joint, not two edge-projected profiles
   // plus an artificial, folded lens cap between them.
   double profile_angle=angle;
   if(continuous_smooth_surface&&selected_count[v]==2){edge=norm(cross(regions[ga].normal,regions[gb].normal));profile_angle=std::acos(std::clamp(dot(regions[ga].normal,regions[gb].normal),-1.,1.));}
   V3 oa=sub(original[v],pa);oa=sub(oa,mul(edge,dot(oa,edge)));V3 ob=sub(original[v],pb);ob=sub(ob,mul(edge,dot(ob,edge)));V3 control=mul(add(add(pa,oa),add(pb,ob)),.5);auto&row=rows[endpoint++];row.push_back(ia);arc_normals[ia]=regions[ga].normal;
   for(int j=1;j<segments;++j){double frac=double(j)/segments,s0=std::sin((1-frac)*profile_angle*.5),s1=std::sin(frac*profile_angle*.5),t=s1/(s0+s1),weight=std::cos(profile_angle*.5),A=(1-t)*(1-t),B=2*t*(1-t)*weight,C=t*t;V3 point=mul(add(add(mul(pa,A),mul(control,B)),mul(pb,C)),1/(A+B+C));size_t vi=vertex(v,point);row.push_back(vi);arc_normals[vi]=norm(add(mul(regions[ga].normal,std::sin((1-frac)*profile_angle)),mul(regions[gb].normal,std::sin(frac*profile_angle))));}
   row.push_back(ib);arc_normals[ib]=regions[gb].normal;
  }
  std::string material=p.value("bevel_material",p.contains("face_materials")?p["face_materials"][f].get<std::string>():p["material"].get<std::string>());
  for(int j=0;j<segments;++j){Face q={rows[0][j],rows[0][j+1],rows[1][j+1],rows[1][j]};std::vector<V3>ns;for(size_t i:q)ns.push_back(arc_normals[i]);emit(q,ns,material,p.contains("bevel_material")?"":source_material_side(p,f,source_normals[f]));}
  Face at_a=rows[0];std::reverse(at_a.begin(),at_a.end());corner_arcs[a].push_back(at_a);corner_arcs[b].push_back(rows[1]);
 }
 for(size_t v=0;v<corner_arcs.size();++v){
  std::multiset<Edge> boundary;for(const auto&arc:corner_arcs[v])for(size_t i=0;i+1<arc.size();++i){size_t a=arc[i],b=arc[i+1];if(a==b)continue;auto reverse=boundary.find({b,a});if(reverse!=boundary.end())boundary.erase(reverse);else boundary.insert({a,b});}
  while(!boundary.empty()){
   Face ring;size_t first=boundary.begin()->first,current=first;
   do{ring.push_back(current);auto next=boundary.lower_bound({current,0});if(next==boundary.end()||next->first!=current)throw std::runtime_error("directed bevel corner boundary is open");current=next->second;boundary.erase(next);if(ring.size()>10000)throw std::runtime_error("bevel corner boundary does not terminate");}while(current!=first);
   if(ring.size()<3)continue;
   V3 area_normal{};for(size_t i=1;i+1<ring.size();++i)area_normal=add(area_normal,cross(sub(vertices[ring[i]],vertices[ring[0]]),sub(vertices[ring[i+1]],vertices[ring[0]])));if(len(area_normal)<1e-18)continue;
   V3 center{},sum{};std::set<size_t> groups;for(size_t fi:incident[v])groups.insert(face_region.at({fi,v}));for(size_t group:groups)sum=add(sum,regions[group].normal);V3 n=norm(sum);
   for(size_t i:ring)center=add(center,vertices[i]);
   center=mul(center,1./ring.size());
   bool orth=groups.size()==3;std::vector<size_t> gs(groups.begin(),groups.end());if(orth)orth=std::abs(dot(regions[gs[0]].normal,regions[gs[1]].normal))<1e-8&&std::abs(dot(regions[gs[1]].normal,regions[gs[2]].normal))<1e-8&&std::abs(dot(regions[gs[2]].normal,regions[gs[0]].normal))<1e-8;
   if(orth){V3 sphere=sub(original[v],mul(sum,width));center=add(sphere,mul(n,width));}
   size_t ci=vertices.size();vertices.push_back(center);std::string material=p.value("bevel_material",p.contains("face_materials")?p["face_materials"][incident[v][0]].get<std::string>():p["material"].get<std::string>());
   for(size_t i=0;i<ring.size();++i)emit({ring[i],ring[(i+1)%ring.size()],ci},{arc_normals[ring[i]],arc_normals[ring[(i+1)%ring.size()]],n},material,p.contains("bevel_material")?"":source_material_side(p,incident[v][0],source_normals[incident[v][0]]));
  }
 }
 Json out=p;out["vertices"]=vertices;out["faces"]=faces;out["uvs"]=metric_uvs(out["vertices"],out["faces"]);out["normals"]=normals;out["face_materials"]=materials;out["face_material_sides"]=sides;out["bevel"]=0.;out["native_bevel"]={{"method","native_shared_surface_fans_directed_boundary"},{"segments",segments},{"requested_m",requested},{"effective_m",width},{"angle_limit",.35},{"source_domains_preserved",true},{"exact_blender_topology",false}};return out;
}
} // namespace
Json evaluate_geometry(const Json&p){
 if(p.value("bevel",0.)<=0)return evaluate_legacy(p);
 // This omission is present explicitly in the frozen Blender adapter too.
 if(p.contains("cuts")&&!p["cuts"].empty()){
  double shortest=INFINITY;for(const auto&face:p["faces"])for(size_t i=0;i<face.size();++i)shortest=std::min(shortest,len(sub(p["vertices"][face[i].get<size_t>()].get<V3>(),p["vertices"][face[(i+1)%face.size()].get<size_t>()].get<V3>())));
  if(shortest<p["bevel"].get<double>())return evaluate_legacy(p);
 }
 if(simple_convex_baseline(p))return evaluate_legacy(p);
 return evaluate_fans(p);
}
// Independent implementation of the frozen adapter's FACE_AREA weighted-normal
// policy, verified against Blender 4.3.2 on identical geometry. The default .01
// area threshold groups ranks; weight 40 gives a rank divisor of (40/50)^rank.
// Very small weighted sums use ordinary angle-weighted vertex normals, as the
// reference modifier does. This path has no sharp edges: every source face is
// smooth, and the adapter neither marks sharp edges nor hardens bevel normals.
void smooth_weighted_normals(Json&out,bool weighted){
 auto vertices=out.at("vertices").get<std::vector<V3>>();
 for(auto&v:vertices)for(auto&x:v)x=static_cast<float>(x);
 const auto faces=out.at("faces").get<std::vector<Face>>();
 std::vector<V3> face_normals,angle_sums(vertices.size()),result(vertices.size());
 std::vector<double> areas;std::vector<std::vector<size_t>> incident(vertices.size());
 for(size_t fi=0;fi<faces.size();++fi){
  const auto&f=faces[fi];V3 area{};
  for(size_t k=1;k+1<f.size();++k)area=add(area,cross(sub(vertices[f[k]],vertices[f[0]]),sub(vertices[f[k+1]],vertices[f[0]])));
  const V3 n=norm(area);face_normals.push_back(n);areas.push_back(len(area)*.5);
  for(size_t k=0;k<f.size();++k){
   const size_t v=f[k];incident[v].push_back(fi);
   const V3 a=norm(sub(vertices[f[(k+f.size()-1)%f.size()]],vertices[v]));
   const V3 b=norm(sub(vertices[f[(k+1)%f.size()]],vertices[v]));
   angle_sums[v]=add(angle_sums[v],mul(n,std::acos(std::clamp(dot(a,b),-1.,1.))));
  }
 }
 for(size_t v=0;v<incident.size();++v){
  auto&around=incident[v];if(around.empty())continue;
  V3 sum{};
  if(weighted){
   std::stable_sort(around.begin(),around.end(),[&](size_t a,size_t b){return areas[a]>areas[b];});
   double previous=areas[around[0]];size_t rank=0;
   for(size_t fi:around){if(std::abs(previous-areas[fi])>.01){++rank;previous=areas[fi];}sum=add(sum,mul(face_normals[fi],areas[fi]/std::pow(.8,double(rank))));}
  }
  result[v]=norm(!weighted||len(sum)<1e-6?angle_sums[v]:sum);
 }
 Json normals=Json::array();for(const auto&f:faces){Json ns=Json::array();for(size_t v:f)ns.push_back(result[v]);normals.push_back(ns);}out["normals"]=normals;
 out["native_normals"]={{"method",weighted?"source_face_area_weighted_all_smooth":"source_angle_weighted_all_smooth"},{"weighted_normal_keep_sharp",true},{"weighted_normal_weight",40},{"weighted_normal_threshold",.01},{"custom_short_normal_encoding",false}};
}
Json evaluate_part(const Json&p){
 Json out=evaluate_geometry(p);
 const std::string material=p.value("material","");
 const bool hardware_smooth=(material=="iron"||material=="rust")&&p.at("faces").size()>12;
 if(!hardware_smooth){
  // Frozen adapters leave these source polygons flat. Blender's bevel inherits
  // that flag and Weighted Normal keep_sharp leaves flat loops unchanged.
  const auto vertices=out.at("vertices").get<std::vector<V3>>();Json normals=Json::array();
  for(const auto& item:out.at("faces")){const Face face=item.get<Face>();const V3 n=polygon_normal(face,vertices);Json loops=Json::array();for(size_t i=0;i<face.size();++i)loops.push_back(n);normals.push_back(loops);}
  out["normals"]=normals;
  out["native_normals"]={{"method","source_flat_faces"},{"source_smooth_faces",0},{"weighted_normal_keep_sharp",true},{"weighted_normal_weight",40}};
 }else if(std::all_of(p.at("faces").begin(),p.at("faces").end(),[](const Json&f){return f.size()<=4;})){
  const bool weighted=p.value("bevel",0.)>0&&!out.value("native_bevel",Json::object()).value("omitted",false);
  smooth_weighted_normals(out,weighted);
 }
 return out;
}
Json evaluate_scene(const Json&s){Json out=s;for(auto&p:out["parts"])p=evaluate_part(p);out["metadata"]["native_evaluation"]={{"generator","C++17"},{"bevel",true},{"corner_normals",true},{"blender_generation",false},{"visual_parity_verified",false}};return out;}
void write_obj(const Json&s,const std::string&path){std::ofstream out(path);if(!out)throw std::runtime_error("Cannot write OBJ");out.precision(17);size_t base=1,uvbase=1;for(auto&p:s["parts"]){out<<"o "<<p["id"].get<std::string>()<<'\n';for(auto&v:p["vertices"]){auto w=world_point(s,p["node"],v.get<V3>());out<<"v "<<w[0]<<' '<<w[1]<<' '<<w[2]<<'\n';}for(auto&f:p["uvs"])for(auto&uv:f)out<<"vt "<<uv[0].get<double>()<<' '<<uv[1].get<double>()<<'\n';for(auto&f:p["faces"]){out<<"f";for(auto i:f)out<<' '<<base+i.get<size_t>()<<'/'<<uvbase++;out<<'\n';}base+=p["vertices"].size();}}
}
