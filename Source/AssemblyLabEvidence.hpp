#pragma once
#include <filesystem>
namespace Playground
{
class Renderer;
void captureProceduralAssemblies(Renderer &renderer,const std::filesystem::path &directory);
void captureAssemblyFragmentPhysics(Renderer &renderer,const std::filesystem::path &directory);
}
