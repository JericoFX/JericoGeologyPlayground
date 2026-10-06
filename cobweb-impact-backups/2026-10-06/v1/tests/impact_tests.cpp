#include "impact.hpp"
#include <charconv>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace web;
using namespace web::impact;

namespace {
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F operation) {
    bool rejected = false;
    try { operation(); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "invalid input must throw invalid_argument");
}
bool exact(V3 a, V3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
bool finite(V3 a) {
    return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z);
}
Patch patch() {
    Patch p;
    p.corners = {V3{-1,0,-1}, V3{1,0,-1}, V3{1,0,1}, V3{-1,0,1}};
    p.bulge = {0,.08,0};
    return p;
}
void thresholdBoundaries() {
    for (double score : {std::nextafter(1.0,0.0), 1.0,
                         std::nextafter(1.0,2.0)}) {
        Panel p;
        require(p.receiveImpact(0,score,{.5,.5}), "first hit ID zero is valid");
        require(p.state().phase == Phase::Indenting, "hit begins indentation");
        for (int n = 0; n < 9; ++n) {
            require(!p.step(), "no removal before delay");
            require(p.collisionEnabled() && p.resistanceEnabled(), "intact contact stays enabled");
        }
        require(p.state().dent > 0, "short visible dent before removal");
        bool breaks = score >= 1.0;
        require(p.step() == breaks, "threshold equality breaks on tick 10");
        require(p.collisionEnabled() == !breaks, "collision follows removal state immediately");
        require(p.resistanceEnabled() == !breaks, "resistance follows removal state immediately");
        for (int n = 0; n < 200; ++n) require(!p.step(), "only one permanent break event");
        require(p.state().eventCount == (breaks ? 1u : 0u), "correct permanent event count");
        require(p.state().phase == (breaks ? Phase::Detached : Phase::Intact), "nonbreak recovers");
        require(p.state().dent == 0, "no stale dent after recovery or detachment");
    }
}
void unbreakable() {
    Settings s; s.breakable = false;
    Panel p(s);
    p.receiveImpact(1, 1e100, {.2,.7});
    for (int n = 0; n < 1000; ++n) {
        require(!p.step(), "unbreakable never emits removal");
        require(p.collisionEnabled() && p.resistanceEnabled(), "unbreakable keeps all contact");
    }
    require(p.state().phase == Phase::Intact && p.state().eventCount == 0,
            "unbreakable recovers without event");
    require(p.state().tick == 1000, "fixed tick advances");
}
void delayQuantization() {
    for (const auto& sample : std::vector<std::pair<double,int>>{{fixedDt,2},{.001,2},{.025,3},{.1,12}}) {
        Settings s; s.breakDelay=sample.first;
        Panel p(s); p.receiveImpact(1,1,{.5,.5});
        for (int n=1;n<sample.second;++n) {
            require(!p.step(),"delay includes at least one visible indentation step");
            require(p.state().dent>0,"positive hit exposes indentation before removal");
        }
        require(p.step(),"a whole-tick decimal delay must not gain a spurious floating-point tick");
    }
}
void idempotent() {
    Panel p;
    p.receiveImpact(7,.5,{.5,.5});
    p.step();
    const std::string before = p.checkpoint();
    require(!p.receiveImpact(7,2,{.2,.2}), "duplicate ID cannot change an earlier hit");
    require(before == p.checkpoint(), "duplicate is exactly inert");
    require(!p.receiveImpact(6,2,{.2,.2}), "old ID is ignored");
    require(before == p.checkpoint(), "out-of-order delivery is inert");
    require(p.receiveImpact(8,1,{.5,.5}), "new qualifying hit accepted");
    for (int n = 0; n < 9; ++n) {
        require(!p.step(), "scheduled break waits");
        require(!p.receiveImpact(9+n,.1,{.1,.1}), "later hits cannot postpone break");
    }
    require(p.step(), "break occurs on original schedule");
    const auto detached = p.checkpoint();
    require(!p.receiveImpact(100,20,{.5,.5}), "detached panel rejects hits");
    require(detached == p.checkpoint(), "detached hit cannot change state");
    for (int n = 0; n < 100; ++n) require(!p.step(), "event cannot repeat");
}
void exactAnchorsAndZeroWind() {
    Settings s; s.breakable = false; s.windVelocity = {2,3,-1};
    Panel p(s); p.receiveImpact(1,.7,{.5,.5});
    for (int n = 0; n < 5; ++n) p.step();
    const Patch mesh = patch();
    require(!exact(deformedPoint(mesh,{.5,.5},p.state(),s),mesh.sample({.5,.5})),
            "indentation actually deforms center");
    const std::vector<V2> corners = {{0,0},{1,0},{1,1},{0,1}};
    for (V2 uv : corners) {
        require(exact(deformedPoint(mesh,uv,p.state(),s),mesh.sample(uv)), "zero-wind anchors exact");
        Settings windy = s; windy.windAmplitude = .09;
        require(exact(deformedPoint(mesh,uv,p.state(),windy),mesh.sample(uv)), "wind anchors exact");
    }
    Settings still = s; still.windVelocity = {}; still.windAmplitude = 10;
    for (int i=0;i<=10;++i) for (int j=0;j<=10;++j) {
        V2 uv{double(i)/10,double(j)/10};
        require(exact(deformedPoint(mesh,uv,p.state(),s),deformedPoint(mesh,uv,p.state(),still)),
                "zero amplitude and zero velocity share exact old dent path");
    }
    Panel untouched;
    for (int i=0;i<=10;++i) {
        V2 uv{double(i)/10,.37};
        require(exact(deformedPoint(mesh,uv,untouched.state(),untouched.settings()),mesh.sample(uv)),
                "zero dent and wind preserve old sample exactly");
    }
}
void dentDirection() {
    Settings s; s.breakable=false;
    Panel p(s); p.receiveImpact(1,1,{.5,.5});
    for(int n=0;n<5;++n) p.step();
    const Patch mesh=patch();
    const V3 normal=normalized(cross(mesh.corners[1]-mesh.corners[0],mesh.corners[3]-mesh.corners[0]));
    const V3 base=mesh.sample({.5,.5});
    const V3 deformed=deformedPoint(mesh,{.5,.5},p.state(),s);
    const V3 delta=deformed-base;
    require(delta.x*normal.x+delta.y*normal.y+delta.z*normal.z<0,
            "dent follows a stone approaching from +normal along -normal");
    require(exact(deformed,base-normal*p.state().dent),
            "center dent is exactly opposite cross(corner1-corner0,corner3-corner0)");
}
void boundedWindAndDent() {
    Settings s; s.breakable = false; s.windVelocity={3,-2,1}; s.windAmplitude=.09;
    Panel p(s); p.receiveImpact(1,.9,{.43,.58});
    const Patch mesh=patch();
    bool sawWind=false;
    for (int n=0;n<240;++n) {
        p.step();
        for (int i=0;i<=12;++i) for (int j=0;j<=12;++j) {
            V2 uv{double(i)/12,double(j)/12};
            const V3 value=deformedPoint(mesh,uv,p.state(),s);
            require(finite(value), "wind/dent outputs finite");
            require(length(value-mesh.sample(uv)) <= s.maxIndent+s.windAmplitude+1e-12,
                    "combined displacement respects additive bounds");
            Settings calm=s; calm.windAmplitude=0;
            const V3 noWind=deformedPoint(mesh,uv,p.state(),calm);
            const double wind=length(value-noWind);
            require(wind <= s.windAmplitude+1e-12, "wind displacement bounded");
            sawWind = sawWind || wind>1e-8;
        }
    }
    require(sawWind, "nonzero wind creates visible displacement");
}
void deterministicReplay() {
    Settings s; s.windVelocity={3,0,1}; s.windAmplitude=.05;
    Panel a(s), b(s);
    const Patch mesh=patch();
    for (int n=0;n<100;++n) {
        if(n==2 || n==45) { a.receiveImpact(n,n==2?.7:2,{.3,.6}); b.receiveImpact(n,n==2?.7:2,{.3,.6}); }
        require(a.step()==b.step(), "identical event replay");
        require(a.checkpoint()==b.checkpoint(), "identical exact state replay");
        require(exact(deformedPoint(mesh,{.3,.5},a.state(),s),deformedPoint(mesh,{.3,.5},b.state(),s)),
                "identical analytic geometry replay");
    }
    require(a.state().phase==Phase::Detached, "replay reaches break");
    Panel saved(s); saved.receiveImpact(91,2,{.2345678901234567,.7234567890123456});
    for(int n=0;n<4;++n) saved.step();
    Panel loaded; loaded.restore(saved.checkpoint());
    require(loaded.checkpoint()==saved.checkpoint(), "checkpoint includes settings and exact doubles");
    for(int n=0;n<40;++n) {
        require(saved.step()==loaded.step(), "restored replay emits same event");
        require(saved.checkpoint()==loaded.checkpoint(), "restored replay stays exact");
    }
    saved.reset();
    require(saved.checkpoint()==Panel(s).checkpoint(), "reset exactly restores initial state");
}
void invalidInputs() {
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    for(double bad : {-1.0,nan,inf}) {
        Settings s; s.threshold=bad; rejects([&]{Panel p(s);});
        s={}; s.maxIndent=bad; rejects([&]{Panel p(s);});
        s={}; s.windAmplitude=bad; rejects([&]{Panel p(s);});
        s={}; s.gustFrequency=bad; rejects([&]{Panel p(s);});
    }
    for(double bad : {0.0,-1.0,nan,inf}) {
        Settings s; s.breakDelay=bad; rejects([&]{Panel p(s);});
    }
    Settings s; s.windVelocity.x=nan; rejects([&]{Panel p(s);});
    Panel p;
    const std::string initial=p.checkpoint();
    for(double bad : {-1.0,nan,inf}) rejects([&]{p.receiveImpact(1,bad,{.5,.5});});
    for(V2 bad : {V2{-1,.5},V2{.5,1.1},V2{nan,.5},V2{.5,inf}})
        rejects([&]{p.receiveImpact(1,1,bad);});
    require(initial==p.checkpoint(), "invalid hits do not partially mutate");
    for(const std::string& bad : {std::string{}, std::string("not a checkpoint"), initial+" garbage"}) {
        rejects([&]{p.restore(bad);});
        require(initial==p.checkpoint(), "bad restore is atomic");
    }
    rejects([&]{deformedPoint(patch(),{nan,.5},p.state(),p.settings());});
    Patch bad=patch(); bad.corners[0].x=inf;
    rejects([&]{deformedPoint(bad,{.5,.5},p.state(),p.settings());});
    bad=patch(); bad.triangular=true;
    rejects([&]{deformedPoint(bad,{.5,.5},p.state(),p.settings());});
    State badState; badState.dent=inf;
    rejects([&]{deformedPoint(patch(),{.5,.5},badState,p.settings());});
}
void checkpointIntegrityAndExtremes() {
    Panel p;
    const std::string original=p.checkpoint();
    std::istringstream input(original);
    std::vector<std::string> tokens;
    for(std::string token;input>>token;) tokens.push_back(token);
    // Fields: version, bool, unsigned seed/tick, phase, dent/event/hasHit/nonce/tick.
    for(const auto& mutation:std::vector<std::pair<int,std::string>>{
        {1,"2"},{2,"2"},{11,"-1"},{12,"-1"},{13,"3"},{17,"nan"},
        {18,"1"},{19,"1"},{20,"18446744073709551616"},{21,"1"}}) {
        auto changed=tokens; changed.at(mutation.first)=mutation.second;
        std::string broken;
        for(const auto& token:changed) broken+=token+" ";
        rejects([&]{p.restore(broken);});
        require(original==p.checkpoint(),"corrupt field cannot partially restore");
    }
    auto exhausted=tokens; exhausted[12]="18446744073709551615";
    std::string lastTick;
    for(const auto& token:exhausted) lastTick+=token+" ";
    p.restore(lastTick);
    const std::string atEnd=p.checkpoint();
    bool overflow=false;
    try { p.step(); } catch(const std::overflow_error&) { overflow=true; }
    require(overflow && p.checkpoint()==atEnd,"tick overflow is rejected atomically");

    Settings s;
    s.threshold=std::numeric_limits<double>::denorm_min();
    s.maxIndent=std::numeric_limits<double>::denorm_min();
    s.windVelocity={std::numeric_limits<double>::max(),std::numeric_limits<double>::max(),1};
    s.windAmplitude=.1;
    s.gustFrequency=std::numeric_limits<double>::max();
    s.seed=std::numeric_limits<uint64_t>::max();
    Panel tiny(s), restored;
    restored.restore(tiny.checkpoint());
    require(tiny.checkpoint()==restored.checkpoint(),"extreme finite settings round-trip exactly");
    tiny.receiveImpact(0,std::numeric_limits<double>::max(),{.5,.5});
    for(int n=0;n<5;++n) tiny.step();
    require(finite(deformedPoint(patch(),{.5,.5},tiny.state(),s)),"extreme finite wind remains finite");
    restored.restore(tiny.checkpoint());
    require(tiny.checkpoint()==restored.checkpoint(),"extreme indentation checkpoint round-trips");

    s={}; s.windVelocity={1,2,3}; s.windAmplitude=.1;
    Panel windy(s); windy.step();
    const V3 first=deformedPoint(patch(),{.5,.5},windy.state(),s);
    windy.step();
    require(!exact(first,deformedPoint(patch(),{.5,.5},windy.state(),s)),"wind evolves with ticks");
    Settings other=s; ++other.seed;
    require(!exact(deformedPoint(patch(),{.5,.5},windy.state(),s),
                   deformedPoint(patch(),{.5,.5},windy.state(),other)),"seed changes analytic wind");
}
void benchmark(uint64_t ticks) {
    for(bool active:{false,true}) {
        Settings s; s.breakable=false;
        // A deliberately long synthetic ramp keeps the active branch occupied.
        // It is a microbenchmark configuration, not a proposed effect duration.
        if(active) s.breakDelay=double(ticks)/120.0+1.0;
        Panel panel(s);
        if(active) panel.receiveImpact(1,.9,{.5,.5});
        const auto start=std::chrono::steady_clock::now();
        for(uint64_t n=0;n<ticks;++n) panel.step();
        const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        require(panel.state().tick==ticks,"benchmark update count");
        require(panel.state().phase==(active?Phase::Indenting:Phase::Intact),"benchmark branch remains occupied");
        std::cout << "state_only_mode=" << (active?"synthetic_active_ramp":"intact_idle")
                  << " ticks=" << ticks << " elapsed_seconds=" << seconds
                  << " ns_per_tick=" << seconds*1e9/double(ticks)
                  << " final_tick=" << panel.state().tick
                  << " (fixed-step state only; excludes impact delivery, deformation, export, render; no engine real-time claim)\n";
    }
}
}

int main(int argc,char** argv) {
    try {
        if(argc>1) {
            require(std::string(argv[1])=="--benchmark", "expected --benchmark [positive ticks]");
            uint64_t ticks=1000000;
            if(argc>2) {
                const std::string value=argv[2];
                const auto parsed=std::from_chars(value.data(),value.data()+value.size(),ticks);
                require(parsed.ec==std::errc{} && parsed.ptr==value.data()+value.size(),"positive integral ticks required");
            }
            require(ticks>0 && argc<=3,"positive benchmark ticks required");
            benchmark(ticks); return 0;
        }
        const std::vector<std::pair<std::string,std::function<void()>>> tests={
            {"threshold boundaries",thresholdBoundaries},{"unbreakable",unbreakable},
            {"delay quantization",delayQuantization},
            {"idempotence",idempotent},{"exact anchors and zero wind",exactAnchorsAndZeroWind},
            {"dent direction",dentDirection},
            {"bounded wind and dent",boundedWindAndDent},{"deterministic replay/reset/checkpoint",deterministicReplay},
            {"invalid inputs",invalidInputs},{"checkpoint integrity and extremes",checkpointIntegrityAndExtremes}};
        int failed=0;
        for(const auto& test:tests) {
            try { test.second(); std::cout << "PASS " << test.first << '\n'; }
            catch(const std::exception& e) { ++failed; std::cerr << "FAIL " << test.first << ": " << e.what() << '\n'; }
        }
        std::cout << tests.size()-failed << '/' << tests.size() << " test groups passed\n";
        return failed?1:0;
    } catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
