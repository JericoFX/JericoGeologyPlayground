#include "rocks/field.hpp"
#include <iostream>
#include <limits>
#include <cmath>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do { if(!(x)) throw std::runtime_error(#x); } while(0)
int main(){try{
 Recipe r; auto g=makeGrid(r,64); CHECK(validateRecipe(r,g).empty());
 for(int k=0;k<9;k++){auto q=r;auto gg=g; if(k==0)q.size=0;if(k==1)q.size=NAN;if(k==2)q.wear=INFINITY;if(k==3)q.schemaVersion=2;if(k==4)q.units="ft";if(k==5)q.generatorVersion="x";if(k==6)q.contrast=1.01;if(k==7)gg.cells=65;if(k==8)q.layerThickness=std::nextafter(4*1.5*r.size/64,0.);CHECK(!validateRecipe(q,gg).empty());}
 auto f=makeField(r); bool oblique=false;for(auto pl:f.planes)if(std::abs(pl.normal.y)>.3&&std::abs(pl.normal.y)<.95)oblique=true;CHECK(oblique);CHECK(resistance(f,{0,.12,0})<.12);CHECK(resistance(f,{0,-.08,0})>.85); CHECK(evaluate(f,{0,0,0},Stage::Base).value<0);for(auto p:{Vec3{2,0,0},{-2,0,0},{0,2,0},{0,-2,0},{0,0,2},{0,0,-2}})CHECK(evaluate(f,p,Stage::Base).value>0);
 CHECK(planeValue({{0,1,0},0},{0,-1,0})<0);CHECK(planeValue({{0,1,0},0},{0,1,0})>0);
 auto z=r;z.wear=0;auto fz=makeField(z);auto one=r;one.wear=1;auto fo=makeField(one);
 CHECK(f.planes==fo.planes);CHECK(f.cutters==fo.cutters);
 for(int x=-8;x<=8;x++)for(int y=-8;y<=8;y++)for(int zz=-8;zz<=8;zz++){
 Vec3 p{x*.17,y*.17,zz*.17};auto s=evaluate(f,p,Stage::Eroded);CHECK(std::isfinite(s.value));CHECK(std::isfinite(s.gradient.x));CHECK(s.resistance>=0&&s.resistance<=1);CHECK(s.value==evaluate(f,p,Stage::Eroded).value);CHECK(evaluate(fz,p,Stage::Eroded).value==evaluate(f,p,Stage::Stratified).value);
 if(erosionMask(f,p)==0)CHECK(s.value==evaluate(f,p,Stage::Stratified).value);
 double prev=-INFINITY;for(double w:{0.,.25,.5,.75,1.}){auto rr=r;rr.wear=w;double v=evaluate(makeField(rr),p,Stage::Eroded).value;CHECK(v>=prev);prev=v;}
 }
 for(double a:{-2.,0.,.2,2.})for(double b:{-1.,0.,1.})CHECK(subtractFields(a,b)==std::max(a,-b));
 for(double request:{-10.,-.1,0.,.1,10.})for(double feature:{.1,1.,10.}) CHECK(std::abs(boundedAmplitude(request,feature,.4)-std::clamp(request,-std::min(.2*feature,.08),std::min(.2*feature,.08)))<1e-12);
 std::cout<<"field invariants: PASS\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
