#include "physics.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
using namespace web;using namespace web::physics;
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("usage: bend_probe OUT stiffness (0=off)");
 std::string out=argv[1];if(std::filesystem::exists(out))throw std::runtime_error("immutable output exists");
 double stiffness=std::stod(argv[2]);
 Recipe r;r.preset="corridor";r.seed=42;r.density=.9;r.width=.0045;r.broken=0;r.quality=20;r.atlas=512;auto lab=generate(r);BuildOptions o;o.massScale=.01;o.strength=1.25;o.stiffness=1;o.junctionTolerance=.005;auto g=buildGraph(lab,o);g.settings.substeps=8;g.settings.iterations=12;addStrandBends(g,stiffness);
 std::filesystem::create_directories(out);std::ofstream csv(out+"/metrics.csv");csv<<std::setprecision(17)<<"tick,energy,anchor_drift,new_breaks,max_speed\n";
 std::ofstream topo(out+"/topology.csv");topo<<"id,a,b,strand,rest,compliance,active\n"<<std::setprecision(17);for(auto e:g.edges)topo<<e.id<<','<<e.a<<','<<e.b<<','<<e.strand<<','<<e.rest<<','<<e.compliance<<','<<e.active<<'\n';
 for(int t=0;t<=150;++t){double drift=0,speed=0;for(auto p:g.particles){if(p.anchor>=0)drift=std::max(drift,length(p.x-p.rest));speed=std::max(speed,length(p.v));}csv<<g.tick<<','<<energy(g)<<','<<drift<<','<<g.events.size()<<','<<speed<<'\n';if(t%30==0)exportFrame(lab,g,0,out+"/tick_"+std::to_string(t),512);if(t<150)step(g,{},t>=30?std::vector<Force>{{459,{.015,0,0}}}:std::vector<Force>{});}
 std::cout<<"particles="<<g.particles.size()<<" bends="<<g.bends.size()<<" edges="<<g.edges.size()<<" breaks="<<g.events.size()<<" E="<<energy(g)<<'\n';
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
