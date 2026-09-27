#include "ViewportProjection.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace Playground;

void require(bool condition,const char *message)
{if (!condition) throw std::runtime_error(message);}

int main()
try
{
    Camera camera;
    camera.yaw=0;camera.pitch=0;camera.distance=5;
    const auto center=projectToViewport({0,0,0},camera,100,100);
    require(center && std::abs(center->x-50)<1e-9 &&
            std::abs(center->y-50)<1e-9,
            "projector center must map to the rendered viewport center");
    const auto right=projectToViewport({1,0,0},camera,100,100);
    require(right && std::abs(right->x-74)<1e-9,
            "viewport projection must use the renderer aspect and scale");
    const auto hit=pointOnPlaneAtPixel(camera,100,100,50,50,
                                       {0,0,0},{0,0,1});
    require(hit && std::abs(hit->x)<1e-9 && std::abs(hit->y)<1e-9 &&
            std::abs(hit->z)<1e-9,
            "moving a damage area at screen center must hit its world plane");
    require(!pointOnPlaneAtPixel(camera,100,100,50,50,
                                 {0,0,0},{0,1,0}),
            "parallel drag plane must reject an undefined move");
    std::cout<<"PASS viewport projection and damage-area drag plane\n";
    return 0;
}
catch(const std::exception &error)
{std::cerr<<error.what()<<'\n';return 1;}
