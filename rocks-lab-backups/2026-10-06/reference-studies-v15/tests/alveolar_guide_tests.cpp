#include "../core/src/alveolar_surface.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 const unsigned n=65;std::vector<double> plane(n*n);
 for(unsigned y=0;y<n;y++)for(unsigned x=0;x<n;x++)plane[y*n+x]=.6+.3*x/(n-1.)-.2*y/(n-1.);
 auto fit=fitAlveolarPlane(plane,n,32,32,13,9);CHECK(std::abs(fit.slopeX-.3)<1e-12);CHECK(std::abs(fit.slopeY+.2)<1e-12);CHECK(fit.residual<1e-12);
 auto guide=makeAlveolarGuide(plane,n,2);CHECK(!guide.protectedNodes[32*n+32]);CHECK(guide.protectedNodes[0]);CHECK(guide.mask[32*n+32]==1);
 for(unsigned y=0;y<n;y++)for(unsigned x=32;x<n;x++)plane[y*n+x]+=.2;
 auto step=makeAlveolarGuide(plane,n,2);CHECK(step.protectedNodes[32*n+32]);CHECK(step.mask[32*n+32]==0);
 std::vector<unsigned char> p(9*7);p[1*9+2]=1;p[5*9+7]=1;
 auto distance=euclideanProtectedDistance(p,9,7,.125);
 for(unsigned y=0;y<7;y++)for(unsigned x=0;x<9;x++){
  double brute=1e9;for(unsigned j=0;j<7;j++)for(unsigned i=0;i<9;i++)if(p[j*9+i])brute=std::min(brute,std::hypot(double(x)-i,double(y)-j)*.125);
  CHECK(std::abs(distance[y*9+x]-brute)<1e-12);
 }
 CHECK(alveolarMaskAt(step,0,0)==0);
 std::cout<<"local plane, joint protection, exact Euclidean transform PASS\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
