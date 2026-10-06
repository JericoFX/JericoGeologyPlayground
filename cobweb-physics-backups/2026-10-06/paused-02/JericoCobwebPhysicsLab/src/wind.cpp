#include "wind.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace web::physics {
namespace {
constexpr double tau=6.283185307179586476925286766559;
double dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
double norm(V3 value){return std::hypot(value.x,value.y,value.z);}
bool finite(V3 value){return std::isfinite(value.x)&&std::isfinite(value.y)&&std::isfinite(value.z);}
void require(bool condition,const char* message){if(!condition)throw std::invalid_argument(message);}

// Stateless integer mixing: independent of renderer calls and library RNG state.
std::uint64_t mix(std::uint64_t value){
    value+=UINT64_C(0x9e3779b97f4a7c15);
    value=(value^(value>>30))*UINT64_C(0xbf58476d1ce4e5b9);
    value=(value^(value>>27))*UINT64_C(0x94d049bb133111eb);
    return value^(value>>31);
}
double phase(std::uint64_t value){return static_cast<double>(mix(value)>>11)*(1./9007199254740992.)*tau;}

void validate(const WindSettings& settings){
    require(finite(settings.velocity)&&norm(settings.velocity)<=50,"wind velocity must be finite with magnitude at most 50");
    require(std::isfinite(settings.gust)&&settings.gust>=0&&settings.gust<=1,"wind gust must be finite in [0, 1]");
    require(std::isfinite(settings.frequency)&&settings.frequency>=0&&settings.frequency<=10,"wind frequency must be finite in [0, 10] Hz");
    require(std::isfinite(settings.drag)&&settings.drag>=0&&settings.drag<=10,"wind drag must be finite in [0, 10]");
    require(std::isfinite(settings.maxParticleForce)&&settings.maxParticleForce>=0&&settings.maxParticleForce<=1,"wind particle force cap must be finite in [0, 1]");
}
}

std::vector<Force> windForces(const Graph& graph,const WindSettings& settings){
    validate(settings);
    // Explicitly disabled wind adds no still-air drag to the legacy simulation.
    if((settings.velocity.x==0&&settings.velocity.y==0&&settings.velocity.z==0)||settings.drag==0||settings.maxParticleForce==0)return {};
    validateGraph(graph);

    const double time=static_cast<double>(graph.tick)*graph.settings.dt;
    const double angle=tau*settings.frequency*time;
    const double modulation=settings.gust==0?1:1+settings.gust*(
        .65*std::sin(angle+phase(settings.seed))+
        .35*std::sin(angle*1.731+phase(settings.seed^UINT64_C(0xd1b54a32d192ed03))));
    const V3 air=settings.velocity*modulation;

    std::vector<size_t> order(graph.edges.size());
    std::iota(order.begin(),order.end(),0);
    std::sort(order.begin(),order.end(),[&](size_t a,size_t b){return graph.edges[a].id<graph.edges[b].id;});
    std::vector<V3> accumulated(graph.particles.size());
    for(size_t index:order){
        const auto& edge=graph.edges[index];
        if(!edge.active||edge.width==0)continue;
        const auto& a=graph.particles[edge.a];
        const auto& b=graph.particles[edge.b];
        const V3 span=b.x-a.x;
        const double spanLength=norm(span);
        require(finite(span)&&std::isfinite(spanLength),"wind edge geometry exceeds finite numeric range");
        if(spanLength<=1e-12)continue;
        const V3 tangent=span*(1/spanLength);
        const V3 relative=air-(a.v*.5+b.v*.5);
        const V3 transverse=relative-tangent*dot(relative,tangent);
        const double speed=norm(transverse);
        require(finite(transverse)&&std::isfinite(speed),"wind relative velocity exceeds finite numeric range");
        if(speed==0)continue;
        // Width is a line's projected diameter, not a solid carrier-face area.
        // Each endpoint gets half the segment force. Fixed-end shares are absorbed
        // by their support, rather than reassigned to the movable endpoint.
        const V3 halfForce=transverse*(.5*settings.drag*edge.width*spanLength*speed);
        require(finite(halfForce),"wind edge force exceeds finite numeric range");
        for(int particle:{edge.a,edge.b}){
            const auto& endpoint=graph.particles[particle];
            if(endpoint.anchor>=0||endpoint.invMass==0)continue;
            accumulated[particle]=accumulated[particle]+halfForce;
            require(finite(accumulated[particle]),"wind accumulated force exceeds finite numeric range");
        }
    }

    std::vector<Force> forces;
    forces.reserve(graph.particles.size());
    for(size_t particle=0;particle<accumulated.size();++particle){
        V3 value=accumulated[particle];
        const double magnitude=norm(value);
        require(std::isfinite(magnitude),"wind particle force exceeds finite numeric range");
        if(magnitude==0)continue;
        if(magnitude>settings.maxParticleForce)value=value*(settings.maxParticleForce/magnitude);
        forces.push_back({static_cast<int>(particle),value});
    }
    return forces;
}
}
