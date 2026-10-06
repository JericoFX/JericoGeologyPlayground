// Read-only integration compile check; not part of standalone contract build.
#include "physics.hpp"
#include "uv_clip.hpp"
int main(){web::Mesh mesh;auto result=web::uvclip::subtract(mesh,{});return result.mesh.faces.empty()?0:1;}
