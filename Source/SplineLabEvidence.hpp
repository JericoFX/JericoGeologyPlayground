#pragma once
#include <filesystem>
namespace Playground
{
class Renderer;
void captureSplineConstruction(Renderer &renderer, const std::filesystem::path &directory);
void captureSpatialSplineDamage(Renderer &renderer,const std::filesystem::path &directory);
}
