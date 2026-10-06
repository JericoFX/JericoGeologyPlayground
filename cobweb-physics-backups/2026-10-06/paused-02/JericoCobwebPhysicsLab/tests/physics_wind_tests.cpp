#include "wind.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace web;
using namespace web::physics;

namespace {
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
double norm(V3 value){return std::hypot(value.x,value.y,value.z);}
bool same(V3 a,V3 b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
bool same(const std::vector<Force>& a,const std::vector<Force>& b){
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i)if(a[i].particle!=b[i].particle||!same(a[i].value,b[i].value))return false;
    return true;
}
void rejects(const std::function<void()>& call,const std::string& message){
    bool rejected=false;
    try{call();}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,message);
}
Graph segment(){
    Graph g;
    g.settings.gravity=0;
    g.particles={{10,-1,-1,{-1,0,1},{-1,0,1},{},{0,0},1,0},
                 {20,-1,-1,{1,0,1},{1,0,1},{},{1,0},1,0}};
    g.edges={{30,0,1,1,0,2,1e-3,1,0,0,0,.002,true}};
    return g;
}
WindSettings steady(V3 velocity={0,2,0}){
    WindSettings settings;
    settings.velocity=velocity;
    settings.gust=0;
    return settings;
}
void zero_exact_test(){
    auto a=strandBench(1,100,6),b=a;
    a.particles[3].v=b.particles[3].v={.02,-.03,.01};
    const WindSettings disabled;
    const auto before=saveState(a);
    check(windForces(a,disabled).empty(),"zero wind must return no forces, including on moving particles");
    check(saveState(a)==before,"wind helper mutated the graph");
    for(int tick=0;tick<120;++tick){step(a);step(b,{},windForces(b,disabled));}
    check(saveState(a)==saveState(b),"disabled wind changed the old trajectory bits");
}
void transverse_quadratic_test(){
    const auto g=segment();
    const auto first=windForces(g,steady());
    check(first.size()==2,"active segment should distribute wind to both endpoints");
    check(first[0].particle==0&&first[1].particle==1,"force targets must use particle indices, not IDs");
    // 0.5 * drag * current length * width * |v_perp| * v_perp.
    const double expected=.5*.02*2*.002*2*2;
    for(const auto& force:first)check(std::abs(force.value.y-expected)<1e-16&&force.value.x==0&&force.value.z==0,"incorrect quadratic transverse line drag");
    const auto doubled=windForces(g,steady({0,4,0}));
    for(size_t i=0;i<first.size();++i)check(std::abs(doubled[i].value.y-first[i].value.y*4)<1e-16,"drag is not quadratic in transverse relative speed");
    check(windForces(g,steady({2,0,0})).empty(),"axial wind should not create transverse silk drag");
}
void relative_velocity_test(){
    auto g=segment();
    for(auto& particle:g.particles)particle.v={0,2,0};
    check(windForces(g,steady()).empty(),"co-moving air and silk should have no relative drag");
    for(auto& particle:g.particles)particle.v={0,3,0};
    const auto forces=windForces(g,steady());
    check(forces.size()==2&&forces[0].value.y<0&&forces[1].value.y<0,"air drag must oppose relative transverse motion");
}
void seeded_gust_test(){
    auto g=segment();
    WindSettings settings=steady();settings.gust=.4;
    const auto first=windForces(g,settings);
    check(!first.empty(),"nonzero gusting wind yielded no force");
    for(int i=0;i<20;++i)check(same(first,windForces(g,settings)),"render-like repeated evaluation changed wind");
    ++settings.seed;
    check(!same(first,windForces(g,settings)),"distinct seeds should produce distinct gust phases");
    settings.seed=42;
    g.tick=1;
    const auto next=windForces(g,settings);
    check(!same(first,next),"gust did not advance with physical time");
    check(norm(next[0].value-first[0].value)<norm(first[0].value)*.1,"gust changes discontinuously between adjacent ticks");
    settings.gust=0;
    const auto steadyForce=windForces(g,settings);
    settings.seed=std::numeric_limits<std::uint64_t>::max();
    check(same(steadyForce,windForces(g,settings)),"seed altered gust-disabled wind");
}
void physical_clock_test(){
    auto a=segment(),b=a;
    a.tick=120;b.tick=60;b.settings.dt=1./60.;
    WindSettings settings=steady();settings.gust=.4;
    check(!windForces(a,settings).empty(),"physical clock test needs active gusting force");
    check(same(windForces(a,settings),windForces(b,settings)),"gust phase uses ticks without physical dt");
    b=a;b.settings.substeps=8;b.settings.iterations=12;
    check(same(windForces(a,settings),windForces(b,settings)),"gust depends on solver passes or substeps");
}
void accumulated_cap_test(){
    auto g=segment();
    for(int i=0;i<20;++i){auto edge=g.edges.front();edge.id=31+i;g.edges.push_back(edge);}
    auto settings=steady({0,12,0});settings.drag=10;settings.maxParticleForce=.003;
    const auto forces=windForces(g,settings);
    check(forces.size()==2,"wind must accumulate at most one force per particle");
    for(const auto& force:forces)check(norm(force.value)<=settings.maxParticleForce*(1+1e-14)&&norm(force.value)>=settings.maxParticleForce*.999,"accumulated per-particle cap was not applied");
}
void sparse_active_edges_test(){
    auto g=segment();
    Particle isolated=g.particles.front();isolated.id=90;isolated.x=isolated.rest={0,3,1};g.particles.push_back(isolated);
    auto forces=windForces(g,steady());
    check(forces.size()==2,"isolated particle received wind without silk support");
    cut(g,g.edges.front().id);
    check(windForces(g,steady()).empty(),"inactive cut silk still receives wind");
}
void fixed_anchors_test(){
    auto g=strandBench(1,100,4);
    const auto before=saveState(g);
    const auto forces=windForces(g,steady());
    check(!forces.empty(),"anchored strand's dynamic silk received no force");
    for(const auto& force:forces)check(g.particles[force.particle].anchor<0&&g.particles[force.particle].invMass>0,"wind targeted a fixed anchor");
    check(saveState(g)==before,"force evaluation changed anchor or graph state");
    auto settings=steady();
    for(int tick=0;tick<60;++tick)step(g,{},windForces(g,settings));
    check(same(g.particles.front().x,g.particles.front().rest)&&same(g.particles.front().v,{}),"fixed anchor moved under wind");
    auto immobile=segment();immobile.particles[0].invMass=0;
    const auto movable=windForces(immobile,steady());
    check(movable.size()==1&&movable.front().particle==1,"zero-inverse-mass endpoint received force");
}
void edge_order_test(){
    auto a=segment();
    for(int i=0;i<12;++i){auto edge=a.edges.front();edge.id=31+i;edge.width=.0001*(i+1);a.edges.push_back(edge);}
    auto b=a;std::reverse(b.edges.begin(),b.edges.end());
    check(windForces(a,steady({0,1,1})).size()==2,"edge-order test needs accumulated nonzero forces");
    check(same(windForces(a,steady({0,1,1})),windForces(b,steady({0,1,1}))),"edge storage order changed force sums");
}
void collapsed_and_zero_area_test(){
    auto g=segment();g.particles[1].x=g.particles[0].x;
    check(windForces(g,steady()).empty(),"collapsed edge caused nonfinite or nonzero drag");
    g=segment();g.edges[0].width=0;
    check(windForces(g,steady()).empty(),"zero-area edge received drag");
    auto settings=steady();settings.drag=0;
    check(windForces(segment(),settings).empty(),"zero drag did not disable forces");
    settings=steady();settings.maxParticleForce=0;
    check(windForces(segment(),settings).empty(),"zero cap did not disable forces");
}
void invalid_settings_test(){
    const auto g=segment();
    const double nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity();
    for(V3 velocity:{V3{nan,0,0},V3{0,inf,0},V3{0,0,-inf},V3{51,0,0},V3{40,40,0},V3{1e308,0,0}}){
        auto settings=steady(velocity);rejects([&]{windForces(g,settings);},"invalid or unbounded velocity accepted");
    }
    for(auto member:{&WindSettings::gust,&WindSettings::frequency,&WindSettings::drag,&WindSettings::maxParticleForce}){
        for(double value:{nan,inf,-1.}){auto settings=steady();settings.*member=value;rejects([&]{windForces(g,settings);},"invalid scalar wind parameter accepted");}
    }
    for(auto pair:{std::pair<double WindSettings::*,double>{&WindSettings::gust,1.01},{&WindSettings::frequency,10.01},{&WindSettings::drag,10.01},{&WindSettings::maxParticleForce,1.01}}){
        auto settings=steady();settings.*pair.first=pair.second;rejects([&]{windForces(g,settings);},"out-of-range wind parameter accepted");
    }
    WindSettings disabled;disabled.gust=nan;
    rejects([&]{windForces(g,disabled);},"zero-wind fast path skipped settings validation");
}
void invalid_graph_test(){
    auto g=segment();g.particles[1].v.x=std::numeric_limits<double>::quiet_NaN();
    rejects([&]{windForces(g,steady());},"nonfinite graph velocity accepted");
    g=segment();g.edges[0].b=99;
    rejects([&]{windForces(g,steady());},"invalid edge index accepted");
    g=segment();g.settings.dt=std::numeric_limits<double>::infinity();
    rejects([&]{windForces(g,steady());},"invalid physical timestep accepted");
    g=segment();g.tick=-1;
    rejects([&]{windForces(g,steady());},"negative physical time accepted");
}
void trajectory_determinism_test(){
    auto a=strandBench(1,100,5),b=a;
    WindSettings settings=steady({0,.5,.1});settings.gust=.25;
    check(!windForces(a,settings).empty(),"deterministic trajectory test needs active wind");
    for(int tick=0;tick<120;++tick){
        // Sampling for visualization must have no effect on the next physics tick.
        for(int sample=0;sample<tick%5;++sample)(void)windForces(b,settings);
        step(a,{},windForces(a,settings));step(b,{},windForces(b,settings));
    }
    check(saveState(a)==saveState(b),"render-like extra wind samples changed the deterministic simulation");
    Graph restored;restoreState(restored,saveState(a));
    for(int tick=0;tick<60;++tick){step(a,{},windForces(a,settings));step(restored,{},windForces(restored,settings));}
    check(saveState(a)==saveState(restored),"restored physical clock lost wind phase");
}
void panel_stability_test(){
    for(unsigned seed:{42u,99u}){
        Recipe recipe;recipe.preset="corridor";recipe.seed=seed;recipe.density=.9;recipe.width=.0045;recipe.broken=0;recipe.quality=20;
        const auto lab=generate(recipe);
        BuildOptions options;options.massScale=.01;options.strength=1.25;
        auto windy=buildGraph(lab,options),baseline=windy;
        windy.settings.substeps=baseline.settings.substeps=8;
        windy.settings.iterations=baseline.settings.iterations=12;
        WindSettings settings;settings.velocity={.8,.3,.1};settings.seed=seed;
        double peakSpeed=0,peakForce=0;
        for(int tick=0;tick<1000;++tick){
            const auto forces=windForces(windy,settings);
            check(!forces.empty(),"moderate panel wind produced no forces");
            for(const auto& force:forces)peakForce=std::max(peakForce,norm(force.value));
            step(windy,{},forces);step(baseline);
            for(const auto& particle:windy.particles){
                check(std::isfinite(norm(particle.x))&&std::isfinite(norm(particle.v)),"nonfinite windy panel");
                peakSpeed=std::max(peakSpeed,norm(particle.v));
                if(particle.anchor>=0)check(same(particle.x,particle.rest)&&same(particle.v,{}),"windy panel anchor drifted");
            }
        }
        check(windy.events.empty(),"moderate wind caused spontaneous panel tear");
        check(std::isfinite(energy(windy))&&peakSpeed<10,"moderate wind produced unstable panel energy/speed");
        double difference=0;
        for(size_t i=0;i<windy.particles.size();++i)difference=std::max(difference,norm(windy.particles[i].x-baseline.particles[i].x));
        check(difference>1e-5,"panel did not visibly respond to moderate wind");
        std::cout<<"METRIC wind_panel seed="<<seed<<" ticks=1000 breaks="<<windy.events.size()<<" peak_speed="<<peakSpeed<<" peak_force="<<peakForce<<" displacement_vs_calm="<<difference<<'\n';
    }
}
}

int main(){
    const std::pair<const char*,void(*)()> cases[]={
        {"zero_wind_exact_old_trajectory",zero_exact_test},{"quadratic_transverse_drag",transverse_quadratic_test},
        {"relative_air_velocity",relative_velocity_test},{"seeded_smooth_gust",seeded_gust_test},
        {"physical_tick_clock",physical_clock_test},{"accumulated_particle_cap",accumulated_cap_test},
        {"sparse_active_silk_only",sparse_active_edges_test},{"fixed_anchors_untouched",fixed_anchors_test},
        {"stable_edge_order",edge_order_test},{"degenerate_and_disabled",collapsed_and_zero_area_test},
        {"reject_invalid_settings",invalid_settings_test},{"reject_invalid_graph",invalid_graph_test},
        {"trajectory_and_replay_determinism",trajectory_determinism_test},{"1000_tick_stable_panel",panel_stability_test}
    };
    int failed=0;
    for(const auto& test:cases)try{test.second();std::cout<<"PASS "<<test.first<<std::endl;}catch(const std::exception& error){++failed;std::cerr<<"FAIL "<<test.first<<": "<<error.what()<<std::endl;}
    std::cout<<"WIND_TESTS "<<std::size(cases)-failed<<'/'<<std::size(cases)<<" passed\n";
    return failed?1:0;
}
