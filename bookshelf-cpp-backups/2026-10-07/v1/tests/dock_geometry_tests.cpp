#include "jerico/dock_geometry.hpp"
#include "jerico/evaluate.hpp"
#include <iostream>
#include <stdexcept>
using namespace jerico;
int main(){try{Builder a(Json{{"seed",17}});a.node("frame",{0,0,0},"Dock",nullptr);
 auto&p=dock_timber(a,"test.board",{0,0,1.5},{2.4,0,1.5},{0,1,0},.18,.075,"DeckBoard",17,false,true,.8);
 if(p["vertices"].size()<80)throw std::runtime_error("insufficient timber profile detail");
 auto errors=validate_scene(a.scene);if(!errors.empty())throw std::runtime_error(errors.dump());
 auto e=evaluate_scene(a.scene);errors=validate_scene(e);if(!errors.empty())throw std::runtime_error(errors.dump());
 Builder b(Json{{"seed",17}});b.node("frame",{0,0,0},"Dock",nullptr);dock_timber(b,"test.board",{0,0,1.5},{2.4,0,1.5},{0,1,0},.18,.075,"DeckBoard",17,false,true,.8);
 if(a.scene!=b.scene)throw std::runtime_error("timber not deterministic");
 // A split must have a visibly elongated grain tear, not a uniform short sawtooth.
 double endMin=1e9,endMax=-1e9;for(size_t i=p["vertices"].size()-13;i<p["vertices"].size()-2;++i){double x=p["vertices"][i][0];endMin=std::min(endMin,x);endMax=std::max(endMax,x);}if(endMax-endMin<.12)throw std::runtime_error("fracture grain tear too short");
 std::cout<<"PASS timber closed solid, normals, finite geometry, deterministic variation\n";
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
