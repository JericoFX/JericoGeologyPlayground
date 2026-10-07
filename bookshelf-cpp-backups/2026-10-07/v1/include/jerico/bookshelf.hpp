#pragma once
#include "jerico/scene.hpp"
namespace jerico {
Json default_bookshelf_recipe();
Json build_bookshelf(const Json& recipe);
Json validate_bookshelf(const Json& scene);
}
