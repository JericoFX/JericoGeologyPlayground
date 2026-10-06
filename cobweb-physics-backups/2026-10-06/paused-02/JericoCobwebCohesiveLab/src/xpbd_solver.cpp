#include "physics.hpp"
#include "material_tears.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>

namespace web::physics {
namespace {
constexpr int particleLimit=2048, constraintLimit=4096, contactLimit=512;
constexpr double minimumRadius=.01, maximumRadius=2, maximumSphereSpeed=12;
double dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
double norm(V3 a){return std::sqrt(dot(a,a));}
bool finite(double v){return std::isfinite(v);}
bool finite(V3 p){return finite(p.x)&&finite(p.y)&&finite(p.z);}
bool finite(V2 p){return finite(p.x)&&finite(p.y);}
bool equal(V3 a,V3 b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
void require(bool ok,const char* message){if(!ok)throw std::invalid_argument(message);}
std::vector<size_t> edgeOrder(const Graph& g){std::vector<size_t> order(g.edges.size());std::iota(order.begin(),order.end(),0);std::sort(order.begin(),order.end(),[&](size_t a,size_t b){return g.edges[a].id<g.edges[b].id;});return order;}
V3 lerp(V3 a,V3 b,double t){return a+(b-a)*t;}
struct Nearest {double u,distance;V3 point,normal;};
Nearest nearest(V3 a,V3 b,V3 center,V3 preferred={}){
    const V3 ab=b-a;
    const double l2=dot(ab,ab);
    const double u=l2>1e-24?std::clamp(dot(center-a,ab)/l2,0.,1.):.5;
    const V3 p=lerp(a,b,u), offset=p-center;
    const double distance=norm(offset);
    V3 n;
    if(distance>1e-12)n=offset*(1/distance);
    else {
        // A deterministic separating normal, including a centered stationary sphere.
        V3 perpendicular=preferred;
        if(l2>1e-24)perpendicular=perpendicular-ab*(dot(perpendicular,ab)/l2);
        if(norm(perpendicular)<1e-12){
            const V3 axis=std::abs(ab.y)<.9*std::sqrt(l2)?V3{0,1,0}:V3{0,0,1};
            perpendicular=axis;
            if(l2>1e-24)perpendicular=axis-ab*(dot(axis,ab)/l2);
        }
        n=perpendicular*(1/std::max(1e-12,norm(perpendicular)));
    }
    return {u,distance,p,n};
}
void projectContact(Graph& g,const Edge& edge,double u,V3 n,double correction){
    auto& a=g.particles[edge.a];auto& b=g.particles[edge.b];
    const double ga=1-u,gb=u,den=a.invMass*ga*ga+b.invMass*gb*gb;
    if(den<=0||correction<=0)return;
    a.x=a.x+n*(correction*a.invMass*ga/den);
    b.x=b.x+n*(correction*b.invMass*gb/den);
}
// Closest distance between two finite segments, including point degeneracies.
double segmentDistance(V3 p,V3 q,V3 a,V3 b){
    const V3 d1=q-p,d2=b-a,r=p-a;
    const double aa=dot(d1,d1),ee=dot(d2,d2),f=dot(d2,r);
    double u=0,v=0;
    if(aa<=1e-24&&ee<=1e-24)return norm(p-a);
    if(aa<=1e-24)v=std::clamp(f/ee,0.,1.);
    else {
        const double c=dot(d1,r);
        if(ee<=1e-24)u=std::clamp(-c/aa,0.,1.);
        else {
            const double crossTerm=dot(d1,d2),den=aa*ee-crossTerm*crossTerm;
            if(den>1e-24)u=std::clamp((crossTerm*f-c*ee)/den,0.,1.);
            v=(crossTerm*u+f)/ee;
            if(v<0){v=0;u=std::clamp(-c/aa,0.,1.);}
            else if(v>1){v=1;u=std::clamp((crossTerm-c)/aa,0.,1.);}
        }
    }
    return norm(lerp(p,q,u)-lerp(a,b,v));
}
struct Contact {size_t edge;double u;V3 normal;double radius;};
bool sweptContact(const Graph& g,size_t ei,const std::vector<V3>& old,V3 begin,V3 end,double radius,Contact& result){
    const auto& e=g.edges[ei];const auto&a=g.particles[e.a];const auto&b=g.particles[e.b];
    const V3 sphereMove=end-begin, da=a.x-old[e.a],db=b.x-old[e.b];
    // Distance to a moving segment is Lipschitz-bounded by this relative speed.
    // One hundred-thousandth of the collision radius is the explicit conservative
    // TOI tolerance (2.21 micrometres for a 0.22 m sphere and 0.002 m strand).
    // Only sampled physical-radius hits create response. A leaf with positive
    // samples may miss only penetration bounded by this explicit tolerance.
    const double tolerance=std::max(1e-9,radius*1e-5);
    const V3 centroidMove=(da+db)*.5;
    const double shapeMotion=.5*norm(da-db);
    // Remove common translation before bounding the swept capsule. This exactly
    // rejects co-moving tangential grazes that defeat a world-space speed bound.
    if(segmentDistance(begin,end-centroidMove,old[e.a],old[e.b])>radius+shapeMotion+1e-12)return false;
    const double bound=std::max(norm(da-sphereMove),norm(db-sphereMove));
    double t=0;
    for(int iteration=0;iteration<80;++iteration){
        const auto q=nearest(lerp(old[e.a],a.x,t),lerp(old[e.b],b.x,t),lerp(begin,end,t),sphereMove);
        const double gap=q.distance-radius;
        if(gap<=0){result={ei,q.u,q.normal,radius};return true;}
        if(gap<=tolerance)break;
        if(bound<1e-14)return false;
        const double advance=gap/bound;
        if(t+advance>1)return false;
        const double next=t+advance*.9;
        if(!(next>t))break;
        t=next;
    }
    // Near-tangent motion can make conservative advancement arbitrarily slow.
    // Refine its remaining interval instead of guessing that a timeout is a miss.
    // The centroid-frame capsule is a conservative lower distance bound on each
    // interval: each endpoint deviates from common translation by <= shapeMotion.
    int visits=0;
    auto refine=[&](auto&& self,double lo,double hi,int depth)->bool{
        if(++visits>16384||depth>32){
            std::ostringstream details;details<<"sphere CCD refinement budget exceeded at edge "<<e.id;
            throw std::runtime_error(details.str());
        }
        const V3 a0=lerp(old[e.a],a.x,lo),b0=lerp(old[e.b],b.x,lo);
        const V3 a1=lerp(old[e.a],a.x,hi),b1=lerp(old[e.b],b.x,hi);
        const V3 s0=lerp(begin,end,lo),s1=lerp(begin,end,hi);
        const V3 intervalA=a1-a0,intervalB=b1-b0,translation=(intervalA+intervalB)*.5;
        const double deformation=.5*norm(intervalA-intervalB);
        if(segmentDistance(s0,s1-translation,a0,b0)>radius+deformation+1e-12)return false;
        const auto first=nearest(a0,b0,s0,sphereMove);
        if(first.distance<=radius){result={ei,first.u,first.normal,radius};return true;}
        const double mid=(lo+hi)*.5;
        if(bound*(hi-lo)<=tolerance){
            // With positive endpoint/midpoint samples any unresolved penetration
            // is <= tolerance/4 by the Lipschitz bound. Do not turn a speculative
            // near miss into a potentially large swept-plane impulse.
            const auto q=nearest(lerp(old[e.a],a.x,mid),lerp(old[e.b],b.x,mid),lerp(begin,end,mid),sphereMove);
            if(q.distance<=radius){result={ei,q.u,q.normal,radius};return true;}
            const auto last=nearest(a1,b1,s1,sphereMove);
            if(last.distance<=radius){result={ei,last.u,last.normal,radius};return true;}
            return false;
        }
        if(self(self,lo,mid,depth+1))return true;
        return self(self,mid,hi,depth+1);
    };
    return refine(refine,t,1.,0);
}
void addBreak(Graph& g,Edge& e,const std::string& cause,int substep){
    if(!e.active)return;
    const auto&a=g.particles[e.a];const auto&b=g.particles[e.b];
    g.events.push_back({g.tick,substep,e.id,(a.uv+b.uv)*.5,(a.x+b.x)*.5,e.load,cause});
    e.active=false;e.lambda=0;e.load=0;e.overload=0;
}
void vectorOut(std::ostream& out,V3 p){out<<p.x<<' '<<p.y<<' '<<p.z<<' ';}
void vectorIn(std::istream& in,V3& p){in>>p.x>>p.y>>p.z;}
}

std::array<Profile,3> profiles(double width,double stiffness,double strength){
    require(finite(width)&&width>0&&width<=100,"profile width must be finite in (0,100]");
    require(finite(stiffness)&&stiffness>0&&stiffness<=1e6,"profile stiffness must be finite in (0,1e6]");
    require(finite(strength)&&strength>0&&strength<=1e6,"profile strength must be finite in (0,1e6]");
    // Artistic bundle profiles, not measured biological silk properties.
    return {{{"tenue",.001*width,.02,.02/stiffness,2,.25*strength},
             {"normal",.002*width,.04,.01/stiffness,2,1*strength},
             {"monstruosa",.006*width,.08,.0025/stiffness,2,6*strength}}};
}

void validateGraph(const Graph& g){
    const auto& s=g.settings;
    require(s.maxParticles>0&&s.maxParticles<=particleLimit&&s.maxConstraints>0&&s.maxConstraints<=constraintLimit,"graph budget exceeds hard limits (2048 particles, 4096 constraints)");
    require(!g.particles.empty()&&g.particles.size()<=static_cast<size_t>(s.maxParticles),"particle budget exceeded or graph is empty");
    require(g.edges.size()<=static_cast<size_t>(s.maxConstraints),"constraint budget exceeded");
    require(s.substeps>=1&&s.substeps<=32&&s.iterations>=1&&s.iterations<=16,"invalid substep or iteration count");
    require(finite(s.dt)&&s.dt>=1e-6&&s.dt<=1./30.,"invalid fixed time step");
    require(finite(s.gravity)&&std::abs(s.gravity)<=100&&finite(s.damping)&&s.damping>=0&&s.damping<=1000,"invalid gravity or damping");
    require(finite(s.breakDelay)&&s.breakDelay>=0&&s.breakDelay<=60&&finite(s.extremeStrain)&&s.extremeStrain>0&&s.extremeStrain<=1e6,"invalid breaking settings");
    require(g.tick>=0&&g.tick<std::numeric_limits<int>::max(),"invalid or exhausted tick counter");
    std::map<int,V3> anchors;
    for(const auto& a:g.anchors){require(a.id>=0&&finite(a.position),"invalid anchor");require(anchors.emplace(a.id,a.position).second,"duplicate anchor ID");}
    std::set<int> ids;
    size_t sourceIds=0;
    for(const auto&p:g.particles){
        require(p.id>=0&&ids.insert(p.id).second,"invalid or duplicate particle ID");
        require(finite(p.rest)&&finite(p.x)&&finite(p.v)&&finite(p.uv),"non-finite particle state");
        require(finite(p.invMass)&&p.invMass>=0&&p.invMass<=1e12,"invalid inverse mass");
        require(p.component>=0&&p.sourceNode>=-1&&p.anchor>=-1,"invalid particle metadata");
        sourceIds+=p.sourceNodes.size();
        require(sourceIds<=65536,"particle source provenance exceeds ID budget");
        int previousSource=-1;
        for(int source:p.sourceNodes){require(source>=0&&source>previousSource,"source IDs must be sorted, unique and nonnegative");previousSource=source;}
        require(p.sourceNodes.empty()||p.sourceNode<0||std::binary_search(p.sourceNodes.begin(),p.sourceNodes.end(),p.sourceNode),"primary source ID missing from junction provenance");
        if(p.anchor>=0){auto it=anchors.find(p.anchor);require(it!=anchors.end(),"unknown particle anchor ID");require(p.invMass==0&&equal(p.rest,it->second)&&equal(p.x,it->second)&&equal(p.v,{}),"anchor particle must remain exactly fixed at its source anchor");}
    }
    ids.clear();
    for(const auto&e:g.edges){
        require(e.id>=0&&ids.insert(e.id).second,"invalid or duplicate edge ID");
        require(e.a>=0&&e.b>=0&&e.a!=e.b&&static_cast<size_t>(e.a)<g.particles.size()&&static_cast<size_t>(e.b)<g.particles.size(),"invalid edge particle indices");
        require(e.kind>=0&&e.kind<3&&e.strand>=-1,"invalid edge metadata");
        require(finite(e.rest)&&e.rest>1e-10&&finite(e.compliance)&&e.compliance>0,"edge rest length and compliance must be positive");
        require(finite(e.strength)&&e.strength>0&&finite(e.width)&&e.width>=0&&e.width<=2,"invalid edge strength or width");
        require(finite(e.lambda)&&e.lambda<=1e-12&&finite(e.load)&&e.load>=0&&finite(e.overload)&&e.overload>=0,"invalid tensile edge state");
        require(e.active||(e.lambda==0&&e.load==0&&e.overload==0),"cut edge retains solver state");
    }
    require(g.events.size()<=g.edges.size(),"more break events than edges");
    std::set<int> eventIds;
    for(const auto&e:g.events){
        require(e.tick>=0&&e.tick<=g.tick&&e.substep>=0&&e.substep<s.substeps&&finite(e.uv)&&finite(e.position)&&finite(e.load)&&e.load>=0&&!e.cause.empty()&&e.cause.size()<=128,"invalid break event");
        require(ids.count(e.edge)&&eventIds.insert(e.edge).second,"unknown or duplicate broken edge event");
        auto found=std::find_if(g.edges.begin(),g.edges.end(),[&](const Edge& edge){return edge.id==e.edge;});
        require(!found->active,"break event references active edge");
    }
    validateMaterialTears(g);
}

void components(Graph& g){
    std::vector<int> parent(g.particles.size());std::iota(parent.begin(),parent.end(),0);
    auto find=[&](int x){while(parent[x]!=x){parent[x]=parent[parent[x]];x=parent[x];}return x;};
    for(const auto& e:g.edges)if(e.active){require(e.a>=0&&e.b>=0&&static_cast<size_t>(e.a)<parent.size()&&static_cast<size_t>(e.b)<parent.size(),"component edge index out of range");int a=find(e.a),b=find(e.b);if(g.particles[a].id>g.particles[b].id)std::swap(a,b);parent[b]=a;}
    for(size_t i=0;i<g.particles.size();++i)g.particles[i].component=g.particles[find(static_cast<int>(i))].id;
}

void cut(Graph& g,int edge,const std::string& cause,int substep){
    if(g.materialTears.enabled){applyMaterialTear(g,edge,cause,substep);return;}
    require(!cause.empty()&&cause.size()<=128&&substep>=0&&substep<g.settings.substeps,"invalid cut event metadata");
    auto found=std::find_if(g.edges.begin(),g.edges.end(),[&](const Edge&e){return e.id==edge;});
    require(found!=g.edges.end(),"cut edge ID does not exist");
    if(found->active){addBreak(g,*found,cause,substep);components(g);}
}

void step(Graph& g,const Sphere& sphere,const std::vector<Force>& forces){
    validateGraph(g);
    const auto& settings=g.settings;
    if(sphere.enabled){require(finite(sphere.previous)&&finite(sphere.center)&&finite(sphere.radius)&&sphere.radius>=minimumRadius&&sphere.radius<=maximumRadius,"sphere radius outside validated range [0.01,2] or position non-finite");require(norm(sphere.center-sphere.previous)<=maximumSphereSpeed*settings.dt+1e-12,"sphere speed exceeds validated limit of 12 units/s");}
    std::vector<V3> acceleration(g.particles.size(),{0,0,settings.gravity});
    // Sum supplied forces in a canonical order, independent of caller container order.
    auto sortedForces=forces;
    for(const auto& f:sortedForces)require(f.particle>=0&&static_cast<size_t>(f.particle)<g.particles.size()&&finite(f.value)&&norm(f.value)<=1e9,"invalid force target or value");
    std::sort(sortedForces.begin(),sortedForces.end(),[](const Force&a,const Force&b){if(a.particle!=b.particle)return a.particle<b.particle;if(a.value.x!=b.value.x)return a.value.x<b.value.x;if(a.value.y!=b.value.y)return a.value.y<b.value.y;return a.value.z<b.value.z;});
    for(const auto& f:sortedForces)acceleration[f.particle]=acceleration[f.particle]+f.value*g.particles[f.particle].invMass;
    auto order=edgeOrder(g);
    const double h=settings.dt/settings.substeps,h2=h*h,damping=std::exp(-settings.damping*h);
    std::vector<V3> old(g.particles.size());
    for(int sub=0;sub<settings.substeps;++sub){
        const V3 sphereBegin=lerp(sphere.previous,sphere.center,double(sub)/settings.substeps),sphereEnd=lerp(sphere.previous,sphere.center,double(sub+1)/settings.substeps);
        for(auto& e:g.edges){e.lambda=0;e.load=0;}
        if(sphere.enabled){
            // Position-only overlap repair precedes the velocity reference capture.
            // It cannot create an artificial correction/h impulse at initialization.
            for(size_t ei:order){const auto&e=g.edges[ei];if(!e.active)continue;const auto q=nearest(g.particles[e.a].x,g.particles[e.b].x,sphereBegin,sphereEnd-sphereBegin);const double radius=sphere.radius+e.width*.5;if(q.distance<radius)projectContact(g,e,q.u,q.normal,radius-q.distance);}
        }
        for(auto&p:g.particles)if(p.invMass>0&&p.x.z<0)p.x.z=0;
        for(size_t i=0;i<g.particles.size();++i){auto&p=g.particles[i];old[i]=p.x;if(p.invMass>0){p.v=(p.v+acceleration[i]*h)*damping;p.x=p.x+p.v*h;}}
        std::vector<Contact> contacts;
        std::vector<bool> contacted(g.edges.size(),false),floorActive(g.particles.size(),false);
        size_t floorContacts=0;
        auto checkContactBudget=[&](){
            // Each pair uses a swept-side plane and a current radial constraint.
            const size_t scalars=2*contacts.size()+floorContacts;
            if(scalars>contactLimit||g.edges.size()+scalars>static_cast<size_t>(settings.maxConstraints))
                throw std::runtime_error("contact budget exceeded (512 contact scalars, 4096 total constraints)");
        };
        auto addContact=[&](size_t ei){
            if(contacted[ei])return;
            const auto&e=g.edges[ei];Contact c;
            if(sweptContact(g,ei,old,sphereBegin,sphereEnd,sphere.radius+e.width*.5,c)){
                contacts.push_back(c);contacted[ei]=true;checkContactBudget();
            }
        };
        auto gatherOverlaps=[&](){
            for(size_t ei:order){const auto&e=g.edges[ei];if(!e.active||contacted[ei])continue;
                const double radius=sphere.radius+e.width*.5;
                const auto q=nearest(g.particles[e.a].x,g.particles[e.b].x,sphereEnd);
                if(q.distance<radius-std::max(1e-9,radius*1e-5))addContact(ei);
            }
            std::sort(contacts.begin(),contacts.end(),[&](const Contact&a,const Contact&b){return g.edges[a.edge].id<g.edges[b.edge].id;});
        };
        auto projectContacts=[&](){
            for(const auto&c:contacts){const auto&e=g.edges[c.edge];
                auto sweptPlane=[&](){const V3 p=lerp(g.particles[e.a].x,g.particles[e.b].x,c.u);projectContact(g,e,c.u,c.normal,c.radius-dot(p-sphereEnd,c.normal));};
                sweptPlane();
                const auto q=nearest(g.particles[e.a].x,g.particles[e.b].x,sphereEnd,c.normal);
                projectContact(g,e,q.u,q.normal,c.radius-q.distance);
                sweptPlane();
            }
        };
        auto clampFloor=[&](){
            for(size_t i=0;i<g.particles.size();++i){auto&p=g.particles[i];if(p.invMass>0&&p.x.z<0){p.x.z=0;if(!floorActive[i]){floorActive[i]=true;++floorContacts;}}}
            checkContactBudget();
        };
        auto penetratingEdge=[&](){
            for(size_t ei:order){const auto&e=g.edges[ei];if(!e.active)continue;
                const double radius=sphere.radius+e.width*.5;
                const auto q=nearest(g.particles[e.a].x,g.particles[e.b].x,sphereEnd);
                if(q.distance<radius-std::max(1e-9,radius*1e-5))return e.id;
            }
            for(const auto&c:contacts){const auto&e=g.edges[c.edge];
                const V3 p=lerp(g.particles[e.a].x,g.particles[e.b].x,c.u);
                if(dot(p-sphereEnd,c.normal)<c.radius-std::max(1e-9,c.radius*1e-5))return e.id;
            }
            return -1;
        };
        if(sphere.enabled)for(size_t ei:order)if(g.edges[ei].active)addContact(ei);
        for(int pass=0;pass<settings.iterations;++pass){
            for(size_t ei:order){auto&e=g.edges[ei];if(!e.active)continue;auto&a=g.particles[e.a];auto&b=g.particles[e.b];const V3 delta=b.x-a.x;const double len=norm(delta);if(len<=1e-14)continue;const double c=len-e.rest,alpha=e.compliance/h2;const double next=std::min(0.,e.lambda+(-c-alpha*e.lambda)/(a.invMass+b.invMass+alpha));const double change=next-e.lambda;e.lambda=next;const V3 direction=delta*(1/len);a.x=a.x-direction*(a.invMass*change);b.x=b.x+direction*(b.invMass*change);}
            // Constraint projection can create contacts absent from prediction.
            if(sphere.enabled){gatherOverlaps();projectContacts();}
        }
        clampFloor();
        if(sphere.enabled){
            // Shared vertices, rotating segments, and the floor can invalidate an
            // earlier contact. Finish with bounded contact-only alternating solves
            // and a full-segment postcondition, including the final floor clamp.
            for(int cleanup=0;cleanup<16&&penetratingEdge()>=0;++cleanup){gatherOverlaps();projectContacts();clampFloor();}
            const int unresolved=penetratingEdge();
            if(unresolved>=0)throw std::runtime_error("sphere/floor separation did not converge at edge "+std::to_string(unresolved));
        }
        for(size_t i=0;i<g.particles.size();++i){auto&p=g.particles[i];if(p.invMass>0)p.v=(p.x-old[i])*(1/h);require(finite(p.x)&&finite(p.v),"non-finite solver state; simulation stopped");}
        bool changed=false;
        std::vector<MaterialFailure> materialFailures;
        for(size_t ei:order){auto&e=g.edges[ei];if(!e.active)continue;e.load=std::max(0.,-e.lambda/h2);e.overload=e.load>e.strength?e.overload+h:0;const double strain=norm(g.particles[e.b].x-g.particles[e.a].x)/e.rest-1;const bool extreme=strain>settings.extremeStrain;const bool sustained=e.load>e.strength&&e.overload+1e-12>=settings.breakDelay;if(extreme||sustained){if(g.materialTears.enabled)materialFailures.push_back({e.id,extreme?"extreme_strain":"overload"});else addBreak(g,e,extreme?"extreme_strain":"overload",sub);changed=true;}}
        if(changed&&g.materialTears.enabled){
            // Commit only after all contact/velocity work. The temporary graph
            // makes the entire batch transactional on a topology-budget failure.
            applyMaterialTearBatch(g,materialFailures,sub,2*contacts.size()+floorContacts);
            order=edgeOrder(g);old.resize(g.particles.size());
            acceleration.assign(g.particles.size(),{0,0,settings.gravity});
            for(const auto& f:sortedForces)acceleration[f.particle]=acceleration[f.particle]+f.value*g.particles[f.particle].invMass;
        }else if(changed)components(g);
    }
    ++g.tick;
}

double energy(const Graph& g){
    double sum=0;
    for(const auto&p:g.particles)if(p.invMass>0)sum+=.5*dot(p.v,p.v)/p.invMass-g.settings.gravity*p.x.z/p.invMass;
    for(const auto&e:g.edges)if(e.active){const double extension=std::max(0.,norm(g.particles[e.b].x-g.particles[e.a].x)-e.rest);sum+=.5*extension*extension/e.compliance;}
    return sum;
}

std::string saveState(const Graph& g){
    validateGraph(g);
    std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(std::numeric_limits<double>::max_digits10);
    out<<"COBWEB_STATE "<<solverVersion<<'\n'<<g.tick<<'\n';
    const auto&s=g.settings;out<<s.substeps<<' '<<s.iterations<<' '<<s.dt<<' '<<s.gravity<<' '<<s.damping<<' '<<s.breakDelay<<' '<<s.extremeStrain<<' '<<s.maxParticles<<' '<<s.maxConstraints<<'\n';
    out<<g.particles.size()<<' '<<g.edges.size()<<' '<<g.anchors.size()<<' '<<g.events.size()<<'\n';
    for(const auto&p:g.particles){out<<p.id<<' '<<p.sourceNode<<' '<<p.anchor<<' ';vectorOut(out,p.rest);vectorOut(out,p.x);vectorOut(out,p.v);out<<p.uv.x<<' '<<p.uv.y<<' '<<p.invMass<<' '<<p.component<<' '<<p.sourceNodes.size();for(int source:p.sourceNodes)out<<' '<<source;out<<'\n';}
    for(const auto&e:g.edges)out<<e.id<<' '<<e.a<<' '<<e.b<<' '<<e.kind<<' '<<e.strand<<' '<<e.rest<<' '<<e.compliance<<' '<<e.strength<<' '<<e.lambda<<' '<<e.load<<' '<<e.overload<<' '<<e.width<<' '<<e.active<<'\n';
    for(const auto&a:g.anchors){out<<a.id<<' ';vectorOut(out,a.position);out<<'\n';}
    for(const auto&e:g.events){out<<e.tick<<' '<<e.substep<<' '<<e.edge<<' '<<e.uv.x<<' '<<e.uv.y<<' ';vectorOut(out,e.position);out<<e.load<<' '<<std::quoted(e.cause)<<'\n';}
    out<<"END\n";if(g.materialTears.enabled)writeMaterialTearState(out,g);return out.str();
}

void restoreState(Graph& g,const std::string& text){
    require(text.size()<=8*1024*1024,"checkpoint exceeds size budget");
    std::istringstream in(text);in.imbue(std::locale::classic());Graph next;std::string magic,version;in>>magic>>version;require(magic=="COBWEB_STATE"&&version==solverVersion,"checkpoint format or solver version mismatch");
    in>>next.tick;auto&s=next.settings;in>>s.substeps>>s.iterations>>s.dt>>s.gravity>>s.damping>>s.breakDelay>>s.extremeStrain>>s.maxParticles>>s.maxConstraints;
    long long particles=-1,edges=-1,anchors=-1,events=-1;in>>particles>>edges>>anchors>>events;
    require(in.good()&&particles>0&&particles<=particleLimit&&edges>=0&&edges<=constraintLimit&&anchors>=0&&anchors<=particleLimit&&events>=0&&events<=edges,"invalid checkpoint counts");
    next.particles.resize(static_cast<size_t>(particles));next.edges.resize(static_cast<size_t>(edges));next.anchors.resize(static_cast<size_t>(anchors));next.events.resize(static_cast<size_t>(events));
    size_t sourceIds=0;
    for(auto&p:next.particles){in>>p.id>>p.sourceNode>>p.anchor;vectorIn(in,p.rest);vectorIn(in,p.x);vectorIn(in,p.v);in>>p.uv.x>>p.uv.y>>p.invMass>>p.component;
        long long count=-1;in>>count;require(!in.fail()&&count>=0&&count<=65536&&sourceIds+static_cast<size_t>(count)<=65536,"invalid checkpoint source provenance count");
        sourceIds+=static_cast<size_t>(count);p.sourceNodes.resize(static_cast<size_t>(count));for(int&source:p.sourceNodes)in>>source;
    }
    for(auto&e:next.edges){int active=-1;in>>e.id>>e.a>>e.b>>e.kind>>e.strand>>e.rest>>e.compliance>>e.strength>>e.lambda>>e.load>>e.overload>>e.width>>active;require(active==0||active==1,"invalid checkpoint active flag");e.active=active==1;}
    for(auto&a:next.anchors){in>>a.id;vectorIn(in,a.position);}
    for(auto&e:next.events){in>>e.tick>>e.substep>>e.edge>>e.uv.x>>e.uv.y;vectorIn(in,e.position);in>>e.load>>std::quoted(e.cause);}
    std::string end;in>>end;require(!in.fail()&&end=="END","truncated or invalid checkpoint");in>>std::ws;if(!in.eof())readMaterialTearState(in,next);in>>std::ws;require(in.eof(),"trailing checkpoint data");
    validateGraph(next);g=std::move(next);
}

Graph strandBench(int kind,double strengthScale,int segments){
    require(kind>=0&&kind<3&&segments>=1&&segments<=128,"strand class or segment count outside supported range");
    const auto profile=profiles(1,1,strengthScale)[kind];Graph g;g.settings.gravity=0;g.settings.damping=profile.damping;g.anchors.push_back({0,{0,0,1}});
    const double rest=1./segments;
    for(int i=0;i<=segments;++i){const V3 p{double(i)/segments,0,1};const double mass=profile.massPerLength*rest*(i==segments?.5:1.);g.particles.push_back({i,-1,i==0?0:-1,p,p,{}, {double(i)/segments,0},i==0?0:1/mass,0});}
    for(int i=0;i<segments;++i)g.edges.push_back({i,i,i+1,kind,0,rest,profile.compliance*rest,profile.strength,0,0,0,profile.width,true});
    components(g);validateGraph(g);return g;
}
}
