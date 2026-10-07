#pragma once
#include "scene.hpp"
namespace jerico {
// Native C++17 shared PNG16 PBR textures and an additional texture_uvs loop layer.
// Does not change evaluated vertices, faces, normals or the original metric uvs.
// At most 16 families: six blackened timber, six muted leather/cloth covers,
// one sepia page edge, one aged pseudotext label, one worn gilt and one iron.
// Book parts require book_frame={origin,u,v,w}: world-space right-handed unit
// axes with u=thickness, v=depth, w=height; book_cover also needs book_family 0..5.
// Timber parts require the equivalent timber_frame with u following the grain.
// Dominant local planes share one affine map across coplanar triangles; local
// geometry bounds are measured, never inferred from book dimensions. Stable
// ID/recipe-seed offsets avoid per-piece atlases. Resolution range: [32,4096].
// Approved V1 cover charts normalize each axis independently: edge placement
// is preserved, but narrow spines retain the V1 aspect-ratio texture stretching.
// Validates all parts before writing files or mutating the scene.
void bake_bookshelf_materials(Json& evaluatedScene,const std::string& directory,
                             int resolution=512);
}
