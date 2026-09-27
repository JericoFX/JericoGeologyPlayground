#pragma once
#include <filesystem>
namespace Playground
{
class Renderer;
void captureWebSurface(Renderer &renderer,const std::filesystem::path &directory);
}
