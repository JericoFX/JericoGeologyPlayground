#include "../core/src/surface_families.cpp"
#include <iostream>
#include <stdexcept>
#include <iomanip>
using namespace rocks;
#define CHECK(x) do { if(!(x)) throw std::runtime_error(#x); } while(0)
// Removing the primary longitudinal envelope must fail finite_span; restoring
// equal longitudinal ends must fail unequal_ends. Real scalar path.
int main(int argc,char**argv){try{
 const std::string mode=argc>1?argv[1]:"finite_span";
 for(unsigned seed:{48U,117U,305U}){
  HeightfieldRecipe r;r.profile="slate_foliated";r.seed=seed;r.size=.64;r.cells=256;r.width=r.height=1025;r.layerSpacingMeters=.075;r.plateLengthMeters=.28;r.thicknessVariation=.4;r.fractures=.7;
  SlateStudy whole(r);double longest=0,maxJump=0,maxDepth=0;unsigned nonempty=0,asymmetric=0;double sum=0;
  for(const auto&j:whole.faces.joints){
   SlateStudy one(r);one.faces.joints={j};const double step=r.plateLengthMeters/1024.;double first=0,last=0;bool seen=false;double strongest=0;
   for(int k=-4096;k<=4096;k++){
    double along=k*step,u=-j.nx*j.offset-j.ny*along,v=-j.ny*j.offset+j.nx*along;
    double value=one.incision(u,v);CHECK(std::isfinite(value));CHECK(value>=0&&value<=.165);maxDepth=std::max(maxDepth,value);sum+=value;
    if(value>1e-7){if(!seen)first=along;last=along;seen=true;}
    if(value>strongest){strongest=value;}
    if(mode=="continuous_ends"){
     const double e=r.size*1e-8;
     double a=one.incision(u-j.ny*e,v+j.nx*e),b=one.incision(u+j.ny*e,v-j.nx*e);
     maxJump=std::max(maxJump,std::abs(a-b));
    }
   }
   if(seen){nonempty++;longest=std::max(longest,last-first);}
   if(strongest>.01){
    const double center=r.size*(random(r.seed,j.key+23700)-.5),d=r.plateLengthMeters*.30;
    auto normalized=[&](double side){double along=center+side*d,x=-j.nx*j.offset-j.ny*along,y=-j.ny*j.offset+j.nx*along;auto q=one.faces.query(x,y);double relevance=one.faces.relevance(j,q);return relevance>.01?one.incision(x,y)/relevance:-1.;};
    double left=normalized(-1),right=normalized(1);if(left>=0&&right>=0&&std::abs(left-right)>.0005)asymmetric++;
   }
  }
  std::cout<<std::setprecision(12)<<"seed="<<seed<<" mode="<<mode<<" nonempty="<<nonempty<<" longest_primary_span_m="<<longest<<" max_depth="<<maxDepth<<" unequal_end_joints="<<asymmetric<<" max_epsilon_jump="<<maxJump<<" signature="<<sum<<std::endl;
  CHECK(nonempty>=5);CHECK(maxDepth>.025);
  if(mode=="finite_span")CHECK(longest<r.plateLengthMeters);
  else if(mode=="unequal_ends")CHECK(asymmetric>=3);
  else if(mode=="continuous_ends")CHECK(maxJump<1e-6);
  else throw std::runtime_error("unknown test mode");
 }
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
