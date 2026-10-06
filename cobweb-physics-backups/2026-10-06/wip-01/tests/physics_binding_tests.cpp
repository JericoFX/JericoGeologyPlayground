#include "physics.hpp"
#include "damage.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <set>
using namespace web;using namespace web::physics;
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{
 Recipe r;r.preset="corridor";r.density=.9;r.width=.0045;r.broken=0;r.atlas=512;r.quality=20;auto l=generate(r);auto g=buildGraph(l);
 check(!g.edges.empty(),"graph must contain real original structural edges");check(g.particles.size()<=1920&&g.edges.size()<=3584,"hard reserve budget");check(g.anchors.size()==33,"retain all original corridor supports");
 for(size_t i=0;i<g.anchors.size();++i)check(g.anchors[i].id==l.anchors[i].id&&length(g.anchors[i].position-l.anchors[i].position)==0,"original anchors exact");
 std::set<int> anchored;for(auto p:g.particles)if(p.anchor>=0)anchored.insert(p.component);for(auto p:g.particles)check(anchored.count(p.component)>0,"initial physical graph must not contain unattached decorative particles");
 int pins=0;for(auto p:g.particles)if(p.anchor>=0){++pins;check(p.invMass==0,"fixed support");check(length(p.rest-l.anchors[p.anchor].position)==0,"support coordinates exact");}check(pins>=4,"patch support identity retained");
 auto before=bindView(l,g,0,128);check(before.mesh.faces.size()==800,"initial patch topology preserved");check(before.mesh.vertices.size()==441,"tick zero original patch vertex count");
 int target=-1;for(auto e:g.edges)if(g.particles[e.a].anchor<0&&g.particles[e.b].anchor<0){target=e.id;break;}check(target>=0,"interior tear target");
 // Simulate a single committed event without depending on solver implementation.
 auto &e=g.edges[target];e.active=false;g.events.push_back({0,0,e.id,(g.particles[e.a].uv+g.particles[e.b].uv)*.5,(g.particles[e.a].x+g.particles[e.b].x)*.5,1,"test"});
 V2 midpoint=(g.particles[e.a].uv+g.particles[e.b].uv)*.5,along=g.particles[e.b].uv-g.particles[e.a].uv;V2 across{-along.y,along.x};across=across*(.03/length(across));check(segmentCrossesCut(g,midpoint-across,midpoint+across),"visual segment with endpoints outside cut must not bridge its gap");
 auto torn=bindView(l,g,0,128);check(torn.mesh.faces.size()<before.mesh.faces.size(),"tear must remove carrier connectivity");check(torn.mesh.faces.size()>before.mesh.faces.size()/2,"tear must stay local");
 int dark=0;for(size_t i=0;i<torn.coverage.size();++i){check(torn.coverage[i]<=before.coverage[i],"local damage cannot brighten coverage");if(torn.coverage[i]<before.coverage[i])++dark;}check(dark>0,"same break damages local atlas");
 for(auto f:torn.mesh.faces){auto a=torn.mesh.vertices[f[0]].p,b=torn.mesh.vertices[f[1]].p,c=torn.mesh.vertices[f[2]].p;check(length(cross(b-a,c-a))>1e-12,"no degenerate export faces");}
 auto same=buildGraph(l);check(same.particles.size()==g.particles.size(),"same seed same graph");
 auto wider=buildGraph(l,{0,.22,2,.9,1,1,false});check(wider.edges.size()==same.edges.size(),"width changes no topology");for(size_t i=0;i<same.edges.size();++i)check(wider.edges[i].compliance==same.edges[i].compliance&&wider.edges[i].strength==same.edges[i].strength,"width independent from physics");
 auto damagedRecipe=r;damagedRecipe.seed=99;damagedRecipe.broken=.01;auto damagedLab=generate(damagedRecipe);auto damagedGraph=buildGraph(damagedLab);for(auto e:damagedGraph.edges)if(e.active)for(int i=0;i<=3000;++i){auto uv=damagedGraph.particles[e.a].uv+(damagedGraph.particles[e.b].uv-damagedGraph.particles[e.a].uv)*(i/3000.);check(damage(uv,.01,99,0,true,0)>0,"initial invisible hole must never retain resisting connection");}
 auto noCluster=BuildOptions{};noCluster.junctionTolerance=0;auto detailed=buildGraph(l,noCluster);check(g.particles.size()<detailed.particles.size(),"millimetric clustering must reduce only short adjacent junctions");std::set<int> sourceIds;for(auto p:g.particles)for(int id:p.sourceNodes)sourceIds.insert(id);for(auto p:detailed.particles)if(p.sourceNode>=0)check(sourceIds.count(p.sourceNode)>0,"cluster must preserve every original source-node ID");for(auto p:g.particles)for(int id:p.sourceNodes)check(length(p.rest-l.patches[0].sample(l.nodes[id].uv))<=.00500000001,"junction representative stays within declared tolerance");
 auto lightOptions=BuildOptions{};lightOptions.massScale=.001;auto light=buildGraph(l,lightOptions);for(size_t i=0;i<same.particles.size();++i)if(same.particles[i].invMass>0)check(std::abs(light.particles[i].invMass/same.particles[i].invMass-10)<1e-9,"mass scale must independently change inverse mass");
 bool rejected=false;try{buildGraph(l,{0,.001,1,.9,1,1,false});}catch(const std::exception&){rejected=true;}check(rejected,"too dense graph rejects rather than silently truncates");
 std::cout<<"PASS graph binding, "<<g.particles.size()<<" particles "<<g.edges.size()<<" edges; faces "<<before.mesh.faces.size()<<" -> "<<torn.mesh.faces.size()<<"; damaged pixels "<<dark<<"\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
