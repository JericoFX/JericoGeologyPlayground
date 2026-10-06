#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace web {
struct V2 { double x=0,y=0; V2 operator+(V2 b)const{return{x+b.x,y+b.y};} V2 operator-(V2 b)const{return{x-b.x,y-b.y};} V2 operator*(double s)const{return{x*s,y*s};} };
struct V3 { double x=0,y=0,z=0; V3 operator+(V3 b)const{return{x+b.x,y+b.y,z+b.z};} V3 operator-(V3 b)const{return{x-b.x,y-b.y,z-b.z};} V3 operator*(double s)const{return{x*s,y*s,z*s};} };
double length(V2 v); double length(V3 v); V3 cross(V3 a,V3 b); V3 normalized(V3 v);
struct Recipe { std::string preset="radial"; uint64_t seed=42; double density=.55,sag=.55,tension=.4,broken=.10,width=.0032; int layers=2,atlas=2048,quality=20; };
struct Anchor { int id; V3 position; };
struct Patch { int id=0,layer=0; bool triangular=false; std::array<V3,4> corners{}; std::array<V3,4> bows{}; V3 bulge{}; V3 sample(V2 uv)const; };
struct Node { int id,patch; V2 uv; };
struct Fiber { int id,patch,nodeA,nodeB; bool structural=false; double width=1; std::vector<V2> uv; int strand=-1; };
struct Vertex { V3 p,n; V2 uv; };
struct Mesh { std::vector<Vertex> vertices; std::vector<std::array<int,3>> faces; };
struct Lab { Recipe recipe; std::vector<Anchor> anchors; std::vector<Patch> patches; std::vector<Node> nodes; std::vector<Fiber> fibers; Mesh mesh; int mainPatches=0; };
struct Atlas { int size=0; std::vector<float> coverage,momentX,momentY; };
class Random { uint64_t state; public: explicit Random(uint64_t seed):state(seed){} uint64_t next(); double unit(); int pick(int n); };
void validate(const Recipe& r); Lab generate(const Recipe& r); void addFibers(Lab& lab); void triangulate(Lab& lab); Atlas bake(const Lab& lab); void write(const Lab& lab,const Atlas& atlas,const std::string& out,bool overwrite=false); bool triangleInvadesVoid(V3 a,V3 b,V3 c); V2 atlasUV(const Lab& lab,int patch,V2 uv); int tileGrid(const Lab& lab);
}
