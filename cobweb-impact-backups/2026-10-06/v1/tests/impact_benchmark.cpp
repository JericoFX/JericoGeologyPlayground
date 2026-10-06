#include "impact.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

// Reproduce with the documented O2 command, without LTO. No asset generation,
// atlas baking, allocation, checkpointing, export, or rendering is timed.
using namespace web;
using namespace web::impact;

namespace {
volatile uint64_t stateSink = 0;
volatile double geometrySink = 0;
struct Sample { double seconds=0, checksum=0; };
using Clock = std::chrono::steady_clock;
struct Options { uint64_t stateTicks=10000000, deformFrames=2000, batches=5; };

uint64_t positive(const std::string& text) {
    uint64_t value=0;
    const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
    if(parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size() || value==0)
        throw std::invalid_argument("benchmark counts must be positive integers");
    return value;
}
Options options(int argc,char** argv) {
    Options value;
    for(int i=1;i<argc;i+=2) {
        if(i+1>=argc) throw std::invalid_argument("expected --state-ticks N --deform-frames N --batches N");
        const std::string key=argv[i];
        const uint64_t count=positive(argv[i+1]);
        if(key=="--state-ticks") value.stateTicks=count;
        else if(key=="--deform-frames") value.deformFrames=count;
        else if(key=="--batches") value.batches=count;
        else throw std::invalid_argument("unknown benchmark option: "+key);
    }
    if(value.batches<3 || value.batches>100) throw std::invalid_argument("use 3..100 batches");
    if(value.deformFrames>std::numeric_limits<uint64_t>::max()/441)
        throw std::invalid_argument("vertex evaluation count would overflow");
    return value;
}
Sample measureState(uint64_t ticks,bool active) {
    Settings s; s.breakable=false;
    // Synthetic long ramp isolates the indentation branch throughout the run.
    // This duration is not the proposed visual effect's duration.
    if(active) s.breakDelay=double(ticks)/120.0+1.0;
    Panel panel(s);
    if(active) panel.receiveImpact(1,.9,{.5,.5});
    const auto begin=Clock::now();
    for(uint64_t n=0;n<ticks;++n) panel.step();
    const double seconds=std::chrono::duration<double>(Clock::now()-begin).count();
    if(panel.state().tick!=ticks || panel.state().phase!=(active?Phase::Indenting:Phase::Intact))
        throw std::runtime_error("state benchmark did not exercise its declared branch");
    // The observable state read and volatile sink are outside the timed region.
    stateSink=panel.state().tick+panel.state().eventCount;
    geometrySink=panel.state().dent;
    return {seconds,double(stateSink)+geometrySink};
}
Patch representativeQuad() {
    Patch p;
    p.corners={V3{-1.6,0,.25},V3{1.6,.04,.35},V3{1.45,.02,3.1},V3{-1.4,-.08,3.2}};
    p.bows={V3{0,.03,.19},V3{-.18,0,0},V3{0,.03,-.2},V3{.15,0,0}};
    p.bulge={0,.1,-.03};
    return p;
}
struct GridInput {
    Patch patch=representativeQuad();
    std::array<V2,441> uv{};
    Settings settings;
    std::vector<State> states;
    explicit GridInput(bool active) {
        for(size_t j=0;j<21;++j) for(size_t i=0;i<21;++i) uv[j*21+i]={double(i)/20,double(j)/20};
        settings.breakable=false;
        if(active) { settings.windVelocity={.8,.2,.5}; settings.windAmplitude=.025; }
        Panel panel(settings);
        if(!active) { states.push_back(panel.state()); return; }
        // Precompute actual replay states, keeping stepping outside deformation timing.
        for(uint64_t n=0;n<120;++n) {
            if(n%20==0) panel.receiveImpact(n,1,{.45,.55});
            panel.step();
            states.push_back(panel.state());
        }
    }
};
Sample measureGrid(uint64_t frames,const GridInput& input) {
    double checksum=0;
    const auto begin=Clock::now();
    for(uint64_t frame=0;frame<frames;++frame) {
        const State& state=input.states[frame%input.states.size()];
        for(V2 uv:input.uv) {
            const V3 p=deformedPoint(input.patch,uv,state,input.settings);
            checksum+=p.x*.17+p.y*.31+p.z*.53;
        }
    }
    const double seconds=std::chrono::duration<double>(Clock::now()-begin).count();
    if(!std::isfinite(checksum)) throw std::runtime_error("nonfinite deformation checksum");
    geometrySink=checksum;
    return {seconds,geometrySink};
}
void reportSample(const std::string& mode,uint64_t batch,uint64_t units,const Sample& sample,bool grid) {
    const double ns=sample.seconds*1e9/double(units);
    std::cout << "sample mode=" << mode << " batch=" << batch << " units=" << units
              << " elapsed_seconds=" << sample.seconds << " ns_per_unit=" << ns;
    if(grid) std::cout << " us_per_441_vertex_grid=" << ns*441/1000;
    std::cout << " checksum=" << sample.checksum << '\n';
}
void reportSummary(const std::string& mode,std::vector<double> ns,bool grid) {
    std::sort(ns.begin(),ns.end());
    const size_t mid=ns.size()/2;
    const double median=ns.size()%2?ns[mid]:(ns[mid-1]+ns[mid])/2;
    std::cout << "summary mode=" << mode << " batches=" << ns.size()
              << " min_ns_per_unit=" << ns.front() << " median_ns_per_unit=" << median
              << " max_ns_per_unit=" << ns.back();
    if(grid) std::cout << " median_us_per_441_vertex_grid=" << median*441/1000;
    std::cout << '\n';
}
} // namespace

int main(int argc,char** argv) {
    try {
#ifndef __OPTIMIZE__
        throw std::runtime_error("compile this benchmark with g++ -O2, without LTO");
#endif
        const Options opt=options(argc,argv);
        const GridInput rest(false),deformed(true);
        const std::array<std::string,4> modes={"state_intact_idle","state_synthetic_active_ramp",
            "deform_rest_zero_wind","deform_dent_and_wind"};
        std::array<std::vector<double>,4> results;
        std::cout << std::setprecision(12)
                  << "impact_benchmark compiler=" << __VERSION__ << " cplusplus=" << __cplusplus
                  << " grid_vertices=441 fixed_dt=" << fixedDt
                  << " state_ticks_per_batch=" << opt.stateTicks << " deform_frames_per_batch=" << opt.deformFrames
                  << " batches=" << opt.batches << '\n'
                  << "scope=state_step_and_separate_quad_deformation; units=ticks_for_state,vertices_for_deformation; "
                  << "includes=loop_and_geometry_checksum_overhead; excludes=impact_delivery,grid_preparation,allocation,generation,bake,export,render; "
                  << "no_engine_realtime_claim; timings_are_machine_and_load_dependent\n";
        // Untimed warmup. Every measured mode is repeated in each batch.
        measureState(1000,false); measureState(1000,true);
        measureGrid(8,rest); measureGrid(8,deformed);
        for(uint64_t batch=1;batch<=opt.batches;++batch) {
            const std::array<Sample,4> samples={measureState(opt.stateTicks,false),measureState(opt.stateTicks,true),
                measureGrid(opt.deformFrames,rest),measureGrid(opt.deformFrames,deformed)};
            for(size_t mode=0;mode<modes.size();++mode) {
                const bool grid=mode>=2;
                const uint64_t units=grid?opt.deformFrames*441:opt.stateTicks;
                reportSample(modes[mode],batch,units,samples[mode],grid);
                results[mode].push_back(samples[mode].seconds*1e9/double(units));
            }
        }
        for(size_t mode=0;mode<modes.size();++mode) reportSummary(modes[mode],results[mode],mode>=2);
        return 0;
    } catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
