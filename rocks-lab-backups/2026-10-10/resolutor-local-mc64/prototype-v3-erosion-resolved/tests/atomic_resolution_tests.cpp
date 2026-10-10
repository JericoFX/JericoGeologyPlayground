#include "../core/src/face_resolution.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;using namespace rocks::detail;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 unsigned supported=0;
 for(unsigned mask=0;mask<256;mask++){
  if(!supportedFaceMask(mask))continue;supported++;auto faces=alternatingFaces(mask);
  CHECK(faces.size()==1||(faces.size()==2&&(faces[0]^1u)==faces[1]));
  std::vector<std::pair<unsigned,bool>> wantOriginal,wantAlternate;
  for(unsigned face:faces){auto a=tableConnectsNegative(mask,face),b=tableConnectsNegative(255-mask,face);CHECK(a&&b&&*a==*b);wantOriginal.emplace_back(face,*a);wantAlternate.emplace_back(face,!*b);}
  auto original=chooseFaceAlternative(mask,wantOriginal),alternate=chooseFaceAlternative(mask,wantAlternate);
  CHECK(original.has_value()&&!*original);CHECK(alternate.has_value()&&*alternate);
  if(faces.size()==2){auto mixed=wantOriginal;mixed[0].second=!mixed[0].second;CHECK(!chooseFaceAlternative(mask,mixed).has_value());}
 }
 CHECK(supported==78);
 CHECK(!supportedFaceMask(105));CHECK(!supportedFaceMask(126));CHECK(!supportedFaceMask(129));
 CHECK(!chooseFaceAlternative(102,{{0,true}}).has_value());
 CHECK(!chooseFaceAlternative(102,{{0,true},{0,true}}).has_value());
 CHECK(!chooseFaceAlternative(102,{{0,true},{1,false}}).has_value());
 std::vector<std::optional<bool>> candidates={true,false,std::nullopt,false,true};
 std::vector<std::vector<std::size_t>> neighbors={{1},{0,2},{1},{4},{3}};
 auto accepted=atomicComponentAcceptance(candidates,neighbors);
 CHECK(!accepted[0]&&!accepted[1]&&!accepted[2]&&accepted[3]&&accepted[4]);
 candidates[2]=true;accepted=atomicComponentAcceptance(candidates,neighbors);for(bool ok:accepted)CHECK(ok);
 std::cout<<"78 supported masks, consistent complement, mixed-face rejection, downstream component atomicity PASS\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
