#include "cobweb.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
int main() {
    try {
        // A diagonal on a 3:1 UV chart must retain its world-space width.
        web::Lab lab;
        lab.recipe.atlas=512;
        lab.recipe.width=.03;
        lab.recipe.broken=0;
        web::Patch patch;
        patch.corners={web::V3{0,0,0},web::V3{3,0,0},web::V3{3,0,1},web::V3{0,0,1}};
        lab.patches.push_back(patch);
        lab.fibers.push_back({0,0,0,1,false,1,{{.2,.2},{.8,.8}}});
        auto atlas=web::bake(lab);
        double sum=0;
        for(float c:atlas.coverage)sum+=c;
        double area=sum/((512.-6)*(512.-6))*3;
        double expected=std::hypot(1.8,.6)*.03;
        if(std::abs(area/expected-1)>.12)
            throw std::runtime_error("anisotropic chart inflates physical fiber width: ratio="+std::to_string(area/expected));
        web::Lab line;
        line.recipe.atlas=256; line.recipe.width=.001; line.recipe.broken=0;
        web::Patch square;
        square.corners={web::V3{0,0,0},web::V3{1,0,0},web::V3{1,0,1},web::V3{0,0,1}};
        line.patches.push_back(square);
        line.fibers.push_back({0,0,0,1,false,1,{{.1,.501},{.9,.501}}});
        auto coarse=web::bake(line);
        line.fibers[0].uv.clear();
        for(int i=0;i<=128;++i)line.fibers[0].uv.push_back({.1+.8*i/128,.501});
        auto fine=web::bake(line);
        for(size_t i=0;i<fine.coverage.size();++i)
            if(std::abs(fine.coverage[i]-coarse.coverage[i])>1e-6)
                throw std::runtime_error("subdividing one fiber changes baked coverage");
        line.fibers.clear();
        line.fibers.push_back({0,0,0,1,false,1,{{.1,.501},{.5,.501}},0});
        line.fibers.push_back({1,0,1,2,false,1,{{.5,.501},{.9,.501}},0});
        auto split=web::bake(line);
        for(size_t i=0;i<split.coverage.size();++i)
            if(std::abs(split.coverage[i]-coarse.coverage[i])>1e-6)
                throw std::runtime_error("graph splits of one strand change baked coverage");
        std::cout<<"CORE_TESTS_OK: anisotropic chart world-width ratio "<<area/expected<<'\n';
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<e.what()<<'\n';
        return 1;
    }
}
