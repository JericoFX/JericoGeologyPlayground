#include "physics.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
using namespace web;
using namespace web::physics;
namespace {
void check(bool ok,const std::string& msg){if(!ok)throw std::runtime_error(msg);}
double norm(V3 p){return std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);}
bool same(V3 a,V3 b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
void rejects(const std::function<void()>& f,const std::string& msg){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,msg);}
Graph freeSegment(){Graph g;g.settings.gravity=0;g.settings.damping=0;g.settings.extremeStrain=100;g.particles={{10,-1,-1,{-1,1,0},{-1,1,0},{},{0,0},1,0},{20,-1,-1,{1,1,0},{1,1,0},{},{1,0},1,0}};g.edges={{30,0,1,1,0,2,1e3,1e6,0,0,0,.002,true}};return g;}
Graph anchoredPair(double load,double delay,int substeps){Graph g=freeSegment();g.settings.substeps=substeps;g.settings.breakDelay=delay;g.anchors={{5,g.particles[0].rest},{8,g.particles[1].rest}};for(int i=0;i<2;++i){g.particles[i].anchor=g.anchors[i].id;g.particles[i].invMass=0;}g.edges[0].rest=2-load*.001;g.edges[0].compliance=.001;g.edges[0].strength=1;return g;}
void profile_test(){const auto p=profiles();check(p[0].strength==.25&&p[1].strength==1&&p[2].strength==6,"three explicit relative strengths");const auto wider=profiles(3,1,1);const auto stiff=profiles(1,2,1);const auto strong=profiles(1,1,2);for(int i=0;i<3;++i){check(wider[i].width==p[i].width*3&&wider[i].massPerLength==p[i].massPerLength&&wider[i].compliance==p[i].compliance&&wider[i].strength==p[i].strength,"visual width must not change mass/stiffness/strength");check(stiff[i].compliance==p[i].compliance/2&&stiff[i].strength==p[i].strength,"stiffness independent from strength");check(strong[i].strength==p[i].strength*2&&strong[i].compliance==p[i].compliance,"strength independent from stiffness");}}
void anchors_test(){auto g=strandBench(1);const auto pos=g.particles.front().x;for(int i=0;i<120;++i)step(g,{},{{static_cast<int>(g.particles.size()-1),{.1,0,0}}});check(same(g.particles.front().x,pos)&&same(g.particles.front().v,{}),"anchor drift");check(g.tick==120,"fixed tick clock did not advance");}
void tensile_test(){auto g=strandBench(1,1,1);g.settings.gravity=0;g.settings.damping=12;g.particles[1].x=g.particles[0].x+(g.particles[1].rest-g.particles[0].rest)*.5;for(int i=0;i<12;++i)step(g);check(norm(g.particles[1].x-g.particles[0].x)<.51,"compressed string behaves like a rigid bar");check(g.edges[0].load==0,"compression produces tensile load");}
void recovery_test(){auto g=strandBench(1,1,1);g.settings.gravity=0;g.settings.damping=15;for(int i=0;i<240;++i)step(g,{},{{1,{.15,0,0}}});const double loaded=norm(g.particles[1].x-g.particles[0].x)-g.edges[0].rest;check(loaded>1e-5,"small force did not stretch the strand");check(g.events.empty(),"small force broke strand");for(int i=0;i<240;++i)step(g);double after=std::max(0.,norm(g.particles[1].x-g.particles[0].x)-g.edges[0].rest);check(after<loaded*.1,"strand failed to recover tensile extension");std::cout<<"METRIC recovery loaded="<<loaded<<" residual="<<after<<'\n';}
void strength_test(){double previous=0;for(int k=0;k<3;++k){auto low=strandBench(k,1,1);low.settings.gravity=0;low.settings.damping=8;auto high=low;double threshold=profiles()[k].strength;for(int t=0;t<600;++t)step(low,{},{{1,{threshold*.8*std::min(1.,t/360.),0,0}}});check(low.events.empty(),"below-threshold load broke class "+std::to_string(k));for(int t=0;t<360&&high.events.empty();++t)step(high,{},{{1,{threshold*1.3,0,0}}});check(!high.events.empty(),"above-threshold load did not break class "+std::to_string(k));check(high.events[0].cause=="overload","threshold failure was not force based");check(high.events[0].load>previous,"three break loads not increasing");previous=high.events[0].load;std::cout<<"METRIC class="<<k<<" nominal="<<threshold<<" break_load="<<previous<<" tick="<<high.events[0].tick<<'\n';}}
void overload_clock_test(){for(int subs:{4,8}){auto g=anchoredPair(2,.025,subs);step(g);step(g);check(g.events.empty(),"overload delay counted iterations rather than physical time");step(g);check(g.events.size()==1,"sustained force did not break after physical delay");check(g.events[0].tick==2,"break event tick not deterministic");check(g.events[0].substep==subs-1,"physical break time differs between substep counts");}}
void cut_test(){auto g=freeSegment();cut(g,30);check(!g.edges[0].active&&g.edges[0].load==0&&g.edges[0].lambda==0,"cut retained resistance");check(g.events.size()==1,"cut event missing");cut(g,30);check(g.events.size()==1,"cut not idempotent");check(g.particles[0].component!=g.particles[1].component,"cut did not separate components");g.particles[0].v={-1,0,0};g.particles[1].v={1,0,0};step(g);check(g.particles[0].x.x<-1&&g.particles[1].x.x>1,"cut endpoints still resist separation");}
void fall_test(){auto g=strandBench(1,1,3);g.settings.gravity=-9.81;cut(g,g.edges.front().id);double before=g.particles.back().x.z;for(int i=0;i<30;++i)step(g);check(g.particles.back().x.z<before-.1,"unsupported component did not fall");check(g.particles.back().component!=g.particles.front().component,"unsupported component linked to anchor");}
void floor_test(){auto g=strandBench(1,1,1);cut(g,0);g.particles[1].v={0,0,-1};g.settings.gravity=-9.81;for(int i=0;i<180;++i)step(g);check(g.particles[1].x.z==0&&g.particles[1].v.z==0,"floor failed to stop detached component at z=0");}
void contact_test(){auto g=freeSegment();g.edges[0].width=0;Sphere s{true,{0,.96,0},{0,1.04,0},.02};auto cutGraph=g;cut(cutGraph,30);step(g,s);step(cutGraph,s);check(g.particles[0].x.y>1.01&&g.particles[1].x.y>1.01,"swept sphere tunneled through active segment");check(same(cutGraph.particles[0].x,cutGraph.particles[0].rest)&&same(cutGraph.particles[1].x,cutGraph.particles[1].rest),"cut segment produced ghost contact");const auto before=saveState(cutGraph);Sphere fast=s;fast.center.y=2;rejects([&]{step(cutGraph,fast);},"unvalidated fast sphere accepted");check(saveState(cutGraph)==before,"rejected sphere mutated state");Sphere tiny=s;tiny.radius=.0001;rejects([&]{step(cutGraph,tiny);},"unvalidated tiny radius accepted");}
void overlap_test(){auto g=freeSegment();Sphere s{true,{0,1,0},{0,1,0},.12};step(g,s);check(norm((g.particles[0].x+g.particles[1].x)*.5-s.center)>=s.radius,"initial overlap was left unresolved");check(norm(g.particles[0].v)<1e-9&&norm(g.particles[1].v)<1e-9,"initial overlap correction injected kinetic energy");check(energy(g)<1e-8,"static overlap injected elastic or kinetic energy");}
void determinism_test(){auto a=strandBench(1,10,8),b=a;for(int i=0;i<300;++i){const std::vector<Force> f{{8,{.01*std::sin(i*.13),-.1,0}}};step(a,{},f);step(b,{},f);}check(saveState(a)==saveState(b),"same execution differs byte for byte");auto reordered=a;std::reverse(reordered.edges.begin(),reordered.edges.end());for(int i=0;i<30;++i){step(a);step(reordered);}std::sort(reordered.edges.begin(),reordered.edges.end(),[](const Edge&x,const Edge&y){return x.id<y.id;});check(saveState(a)==saveState(reordered),"edge storage order changes solver results");}
void checkpoint_test(){auto a=strandBench(2,1,6);a.settings.gravity=0;const auto initial=saveState(a);for(int i=0;i<111;++i)step(a,{},{{6,{.2,.1,0}}});const auto checkpoint=saveState(a);Graph resumed;restoreState(resumed,checkpoint);check(saveState(resumed)==checkpoint,"checkpoint changed round-trip bits");for(int i=0;i<150;++i){step(a,{},{{6,{.2,.1,0}}});step(resumed,{},{{6,{.2,.1,0}}});}check(saveState(a)==saveState(resumed),"resumed run differs from continuous run");restoreState(resumed,initial);check(saveState(resumed)==initial,"reset did not restore full initial state");for(const std::string& bad:{std::string("nonsense"),checkpoint.substr(0,checkpoint.size()/2),checkpoint+"garbage"}){const auto before=saveState(resumed);rejects([&]{restoreState(resumed,bad);},"invalid checkpoint accepted");check(saveState(resumed)==before,"failed restore changed live state");}}
void refinement_test(){auto a=strandBench(1,100,1),b=a;a.settings.gravity=0;b.settings.gravity=0;a.settings.damping=8;b.settings.damping=8;b.settings.substeps=8;for(int i=0;i<360;++i){step(a,{},{{1,{.5,0,0}}});step(b,{},{{1,{.5,0,0}}});}double da=norm(a.particles[1].x-a.particles[0].x)-a.edges[0].rest,db=norm(b.particles[1].x-b.particles[0].x)-b.edges[0].rest;double loadError=std::abs(a.edges[0].load-b.edges[0].load)/b.edges[0].load,stretchError=std::abs(da-db)/db;check(loadError<.05&&stretchError<.05,"4/8 substeps diverge beyond 5% equilibrium tolerance");std::cout<<"METRIC substeps load_relative="<<loadError<<" stretch_relative="<<stretchError<<'\n';}
void endurance_test(){for(unsigned seed:{42u,99u}){auto g=strandBench(2,100,16);g.settings.gravity=0;g.settings.damping=2;std::mt19937 rng(seed);std::uniform_real_distribution<double>d(-.015,.015);for(size_t i=1;i<g.particles.size();++i)g.particles[i].v={d(rng),d(rng),d(rng)};const double start=energy(g);double peak=start;for(int i=0;i<10000;++i){step(g);const double e=energy(g);check(std::isfinite(e)&&e>=0,"non-finite or negative energy");peak=std::max(peak,e);for(const auto&p:g.particles)check(std::isfinite(norm(p.x))&&std::isfinite(norm(p.v)),"non-finite particle after endurance");}check(g.events.empty(),"unforced endurance graph broke");check(peak<=start*1.03+1e-12,"unforced solver injected >3% energy");std::cout<<"METRIC seed="<<seed<<" ticks=10000 initial_energy="<<start<<" peak_energy="<<peak<<" final_energy="<<energy(g)<<'\n';}}
void ramp_threshold_test(){
    std::array<Graph,3> graphs{strandBench(0,1,1),strandBench(1,1,1),strandBench(2,1,1)};
    std::array<double,3> applied{};
    for(auto&g:graphs)g.settings.damping=20;
    for(int t=0;t<1800;++t){
        const double force=t*.005;
        for(int k=0;k<3;++k)if(graphs[k].events.empty()){
            step(graphs[k],{},{{1,{force,0,0}}});
            if(!graphs[k].events.empty())applied[k]=force;
        }
    }
    for(int k=0;k<3;++k){
        check(!graphs[k].events.empty(),"common load ramp failed to break class");
        check(std::abs(applied[k]/profiles()[k].strength-1)<.15,"quasi-static break threshold differs >15% from profile");
        if(k)check(graphs[k-1].events[0].tick<graphs[k].events[0].tick,"class order under identical ramp is wrong");
        std::cout<<"METRIC common_ramp class="<<k<<" applied_at_break="<<applied[k]<<" measured_load="<<graphs[k].events[0].load<<'\n';
    }
}
void checkpoint_damage_test(){
    auto g=anchoredPair(2,.025,4);step(g);Graph resumed;restoreState(resumed,saveState(g));
    for(int i=0;i<6;++i){step(g);step(resumed);}
    check(g.events.size()==1&&saveState(g)==saveState(resumed),"checkpoint lost in-progress physical overload timer or break event");
    Graph postBreak;restoreState(postBreak,saveState(g));
    check(saveState(postBreak)==saveState(g),"checkpoint lost cut topology or events");
}
void grazing_contact_test(){auto g=freeSegment();g.edges[0].width=0;Sphere s{true,{-.04,1.0201,0},{.04,1.0201,0},.02};step(g,s);check(same(g.particles[0].x,g.particles[0].rest)&&same(g.particles[1].x,g.particles[1].rest),"grazing non-contact sweep moved material");}
void provenance_checkpoint_test(){
    auto g=strandBench(1,1,2);
    g.particles[1].sourceNode=7;
    g.particles[1].sourceNodes={7,11,25};
    const auto checkpoint=saveState(g);
    Graph restored;
    restoreState(restored,checkpoint);
    check(restored.particles[1].sourceNodes==g.particles[1].sourceNodes,"checkpoint lost welded-junction source IDs");
    check(saveState(restored)==checkpoint,"provenance checkpoint is not exact");
    auto bad=g;bad.particles[1].sourceNodes={7,7};
    rejects([&]{validateGraph(bad);},"duplicate source IDs accepted");
    bad=g;bad.particles[1].sourceNodes={11,7};
    rejects([&]{validateGraph(bad);},"unordered source IDs accepted");
    bad=g;bad.particles[1].sourceNodes={11,25};
    rejects([&]{validateGraph(bad);},"primary source missing from provenance accepted");
    bad=g;bad.particles[1].sourceNodes={-1,7};
    rejects([&]{validateGraph(bad);},"negative source ID accepted");
    std::string corrupt=checkpoint;
    const auto marker=corrupt.find("3 7 11 25\n");
    check(marker!=std::string::npos,"checkpoint did not encode source list count");
    corrupt.replace(marker,1,"999999999");
    const auto before=saveState(restored);
    rejects([&]{restoreState(restored,corrupt);},"oversized provenance count accepted");
    check(saveState(restored)==before,"invalid provenance restore changed live graph");
}
void invalid_test(){auto g=strandBench(1);auto checkBad=[&](Graph bad){rejects([&]{validateGraph(bad);},"invalid graph accepted");const int tick=bad.tick;rejects([&]{step(bad);},"step accepted invalid graph");check(bad.tick==tick,"invalid graph advanced tick");};auto bad=g;bad.edges[0].b=999999;checkBad(bad);bad=g;bad.particles[1].x.x=std::numeric_limits<double>::quiet_NaN();checkBad(bad);bad=g;bad.edges[0].rest=0;checkBad(bad);bad=g;bad.edges[0].compliance=-1;checkBad(bad);bad=g;bad.particles[1].id=bad.particles[0].id;checkBad(bad);bad=g;bad.settings.substeps=0;checkBad(bad);bad=g;bad.settings.dt=std::numeric_limits<double>::infinity();checkBad(bad);bad=g;bad.settings.maxParticles=1;checkBad(bad);bad=g;bad.settings.dt=std::numeric_limits<double>::denorm_min();checkBad(bad);bad=g;bad.settings.dt=1e-9;checkBad(bad);rejects([&]{profiles(0);},"zero width accepted");rejects([&]{strandBench(3);},"invalid class accepted");rejects([&]{strandBench(1,1,0);},"empty strand accepted");const auto before=saveState(g);rejects([&]{step(g,{},{{9999,{1,0,0}}});},"invalid force target accepted");rejects([&]{cut(g,9999);},"invalid cut accepted");check(saveState(g)==before,"invalid action mutated graph");}
}
int main(){int failed=0;const std::pair<const char*,void(*)()> cases[]={{"profiles",profile_test},{"anchors",anchors_test},{"tensile_only",tensile_test},{"small_load_recovery",recovery_test},{"three_break_thresholds",strength_test},{"physical_overload_time",overload_clock_test},{"cut_topology",cut_test},{"component_fall",fall_test},{"floor_contact",floor_test},{"swept_contact_and_cut",contact_test},{"initial_overlap",overlap_test},{"stable_determinism",determinism_test},{"checkpoint_reset_atomic",checkpoint_test},{"substep_refinement",refinement_test},{"10000_tick_energy_seeds",endurance_test},{"common_ramp_thresholds",ramp_threshold_test},{"checkpoint_damage_timer",checkpoint_damage_test},{"grazing_sweep",grazing_contact_test},{"junction_provenance_checkpoint",provenance_checkpoint_test},{"invalid_inputs",invalid_test}};for(auto c:cases)try{c.second();std::cout<<"PASS "<<c.first<<'\n';}catch(const std::exception&e){++failed;std::cerr<<"FAIL "<<c.first<<": "<<e.what()<<'\n';}std::cout<<"PHYSICS_TESTS "<<(20-failed)<<"/20 passed\n";return failed?1:0;}
