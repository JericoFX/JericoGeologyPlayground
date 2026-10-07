# Provenance

The core, mesh evaluator, random generator, PNG writer and procedural noise implementation were copied unchanged from Procedural C++ WIP03. Shared-material GLB/OBJ exporters were copied unchanged from Bookshelf C++ checkpoint 01, which inherited and verified them from Dock C++ checkpoint 01. Original checkpoints are not modified.

The shelf prop profiles, assembly geometry, atlas mapping, native material recipes, dimensions and placement logic are original C++ for this increment. Reference images were viewed only as design guidance and are not used as textures or included in source or Git backups.

Vendored nlohmann/json retains its MIT license. Inherited Blender/Cycles noise code retains the BSD-3-Clause and Apache-2.0 license files in third_party. These are the only inherited third-party implementation components.

Python tools only inspect files or set up a Blender viewing stage, camera and lights. They do not generate, modify, save or export asset geometry or asset materials.
