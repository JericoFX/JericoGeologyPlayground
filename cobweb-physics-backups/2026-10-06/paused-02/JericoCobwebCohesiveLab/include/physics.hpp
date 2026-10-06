#pragma once
#include "cobweb.hpp"
#include <string>
#include <vector>
namespace web::physics {
constexpr const char* solverVersion="xpbd-tensile-v3";
struct Profile { std::string name; double width, massPerLength, compliance, damping, strength; };
std::array<Profile,3> profiles(double width=1,double stiffness=1,double strength=1);
struct Particle { int id=0,sourceNode=-1,anchor=-1; V3 rest{},x{},v{}; V2 uv{}; double invMass=1; int component=0; std::vector<int> sourceNodes{}; };
struct Edge { int id=0,a=0,b=0,kind=1,strand=-1; double rest=0,compliance=1e-5,strength=1,lambda=0,load=0,overload=0,width=.002; bool active=true; };
struct BreakEvent { int tick=0,substep=0,edge=0; V2 uv{}; V3 position{}; double load=0; std::string cause; };
struct Settings { int substeps=4,iterations=2; double dt=1./120.,gravity=-9.81,damping=2,breakDelay=.008333333333,extremeStrain=2.5; int maxParticles=2048,maxConstraints=4096; };
struct Sphere { bool enabled=false; V3 previous{},center{}; double radius=.12; };
struct Force { int particle=0; V3 value{}; };
// Exact material chart survives physical junction clustering. Contracted segments have edge=-1.
struct MaterialVertex { V2 uv{}; V3 rest{}; int particle=-1; };
struct MaterialSegment { int a=-1,b=-1,edge=-1,parentEdge=-1; };
struct CohesiveInfo { bool enabled=false; int patch=-1,layer=-1,junctions=0,depthRejected=0,anchorRejected=0; double depthTolerance=0,maxRestShapeError=0,maxDepthGap=0; std::vector<MaterialVertex> vertices; std::vector<MaterialSegment> segments; };
// Dynamic local-tear chart and lineage; immutable cohesive metadata is rebuilt from the recipe.
struct MaterialCutRegion { int id=0,primaryEdge=-1,tick=0,substep=0; std::vector<V2> polygon; };
struct MaterialEdgeLineage { int edge=-1,parentEdge=-1,rootEdge=-1,cut=-1; V2 uvA{},uvB{}; double parentT0=0,parentT1=1; };
enum class MaterialChartSource { ExactAuthoringUV=0, ParticleUV=1 };
struct MaterialTearState {
 MaterialChartSource chartSource=MaterialChartSource::ExactAuthoringUV;
 bool enabled=false; double radius=.0075; int particleReserve=128,initialParticles=0,nextCutId=0,nextParticleId=0,nextEdgeId=0,primaryCuts=0,secondaryCuts=0;
 double transferredMass=0,anchorSuppliedMass=0;
 std::vector<MaterialCutRegion> cuts; std::vector<MaterialEdgeLineage> lineage;
};
struct Graph { std::vector<Particle> particles; std::vector<Edge> edges; std::vector<Anchor> anchors; std::vector<BreakEvent> events; int tick=0; Settings settings{}; CohesiveInfo cohesive{}; MaterialTearState materialTears{}; };
void validateGraph(const Graph& g);
void components(Graph& g);
void cut(Graph& g,int edge,const std::string& cause="manual",int substep=0);
void step(Graph& g,const Sphere& sphere={},const std::vector<Force>& forces={});
double energy(const Graph& g);
std::string saveState(const Graph& g);
void restoreState(Graph& g,const std::string& text);
Graph strandBench(int kind,double strengthScale=1,int segments=16);
struct BuildOptions { int patch=0; double sampleLength=.22,width=1,density=.9,stiffness=1,strength=1; bool initialDamage=true; double massScale=.01; double junctionTolerance=.005; bool cohesive=false; double cohesiveDepthTolerance=.005; };
Graph buildGraph(const Lab& lab,const BuildOptions& opt={});
struct TearView { Mesh mesh; std::vector<unsigned char> coverage; int atlas=0; std::vector<int> sourceFaces; std::vector<int> supportParticle; };
V3 deformationAt(const Graph& g,V2 uv);
bool segmentCrossesCut(const Graph& g,V2 a,V2 b);
TearView bindView(const Lab& lab,const Graph& g,int patch,int atlas=256);
void exportFrame(const Lab& lab,const Graph& g,int patch,const std::string& dir,int atlas=512,const Sphere& sphere={});
}
