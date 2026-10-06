#include "physics.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace web;using namespace web::physics;
int main(){int failed=0;for(unsigned seed:{42u,99u})for(int regime:{0,1}){Recipe r;r.preset="corridor";r.seed=seed;r.density=.9;r.width=.0045;r.broken=0;r.quality=20;auto l=generate(r);BuildOptions o;o.massScale=regime?1:.01;o.strength=regime?50:1;o.stiffness=regime?10:1;auto g=buildGraph(l,o);const double e0=energy(g);double peak=e0,displacement=0;std::string error;try{for(int t=0;t<10000;++t){step(g);peak=std::max(peak,energy(g));for(auto&p:g.particles)displacement=std::max(displacement,length(p.x-p.rest));if(!std::isfinite(peak))throw std::runtime_error("nonfinite energy");}}catch(const std::exception&e){error=e.what();}const bool ok=error.empty()&&g.events.empty()&&peak<=e0*1.03&&displacement<.5;failed+=!ok;std::cout<<(ok?"PASS":"FAIL")<<" seed="<<seed<<" mass="<<o.massScale<<" strength="<<o.strength<<" stiffness="<<o.stiffness<<" particles="<<g.particles.size()<<" edges="<<g.edges.size()<<" peak_energy_ratio="<<peak/e0<<" max_displacement="<<displacement<<" breaks="<<g.events.size()<<" error="<<error<<std::endl;}return failed?1:0;}
