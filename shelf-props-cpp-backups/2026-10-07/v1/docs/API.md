# Consumer contract

The native scene is right-handed, Z-up metres with front toward −Y. GLB exporter converts to glTF Y-up; its metadata retains the original basis. The combined preview matches the original bookshelf's transform.

Objects have stable IDs `prop.mug`, `prop.lantern`, `prop.bottle`. Each has a base pivot, full conservative OBB with unit axes, bottom_plane_z and a grab ID referencing every closed component in that object. Colliders, mass, handles, pickups and any gameplay are deliberately left to the consuming engine.

A mug and bottle mouth are open cavities in a closed wall surface. The bottom, wall and rounded lip form one oriented two-manifold shell. This is different from capping the visible mouth. Separate lip beads and attached metal tubes are physically touching assembly components, not Boolean-unioned geometry.

The lantern is unlit. Its double-wall globe is glass, the protective wire paths remain outside it and terminate in the lower cup / top hood. Metal fittings intentionally overlap their supports, as welded or seated parts. A carrying loop is modeled in one upright static pose.

Four native texture families are shared by material. No per-wire or per-fitting atlas. Native UVs and tangent normals are finite at float32 export precision. Transmission requires the consuming renderer's glTF material extensions; OBJ carries map approximations and cannot preserve full transmission behavior or grasp metadata.

The placement planner consumes existing metadata.compartments and metadata.items, computes the union of conservative item X bounds in each compartment, and only chooses a gap that meets all clearances. It gives preference to the top compartment for the mug, row 1 from the bottom for the lantern and the bottom row for the bottle. Per-item hash+seed varies the position inside the free interval without changing prior objects. A no_fit status is a valid outcome and must not be replaced by manual overlap or scaling.

The standalone source ZIP contains all production code, JSON dependency and licenses. CMake is the primary build interface; tools/build.sh supports a minimal GCC/Linux setup with zlib. Python and Blender are optional read-only verification/viewing tools, never production dependencies.
