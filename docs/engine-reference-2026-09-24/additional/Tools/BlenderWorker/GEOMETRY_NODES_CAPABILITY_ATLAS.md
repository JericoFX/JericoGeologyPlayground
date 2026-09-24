# Atlas B0 de Geometry Nodes para JericoEngine

Fecha de investigación: 2026-09-08. Estado: **B0 RESEARCH COMPLETE; PRODUCT
ADMISSION NOT STARTED**. Este documento define la cobertura generalista exigida
para el backend Blender autoritativo. No admite addons, no cambia Engine/Editor,
no sustituye los gates B01-B18 y no convierte el catálogo privado
`blender-offline-candidate/v1` en producto.

Autorización del propietario, 2026-09-08: los addons comerciales ya adquiridos
por Jerico pueden analizarse como referencias privadas y sus técnicas pueden
reimplementarse en node groups propios. Esta autorización no cambia la licencia
de cada producto: sus ZIP/BLEND, código, texturas, materiales y assets no se
publican ni redistribuyen salvo permiso explícito de su licencia. La entrega Jerico
usa implementación, nombres, schemas, fixtures y evidencia propios.

## 1. Decisión ejecutiva

JericoEngine debe construir y poseer una biblioteca pequeña de node groups
versionados para los patrones que se repiten en todos sus géneros. Geometry Nodes
es el núcleo de evaluación geométrica; `bpy`/`bmesh`, Cycles CPU y los modificadores
nativos cubren las fases que GN no resuelve bien: importación autoritativa,
validación topológica, UV especializada, bake high-to-low, LOD, colisión, export y
auditoría. Los addons gratuitos son referencias o adaptadores aislados, nunca una
segunda autoridad ni una UI paralela.

La cobertura es exhaustiva por **familia de producción y frontera observable**,
no por afirmar que existe una lista finita de todos los addons publicados en
Internet. Cada candidato recibe una de estas decisiones:

| Decisión | Significado |
| --- | --- |
| `BUILD_OWN` | Implementar node group/operación Jerico con IDs, semántica, pruebas y licencia propias. |
| `ADAPT` | Aislar una utilidad libre detrás del protocolo Jerico después de auditoría de versión, licencia, dependencias y headless. |
| `STUDY` | Usar algoritmos, UX o casos de prueba como referencia; no distribuir ni ejecutar como dependencia. |
| `REJECT` | No sirve para el backend autoritativo por licencia, red, binarios, versión, UI/contexto, nondeterminismo o autoridad incorrecta. |

## 2. Base Blender 5.2 LTS y fronteras técnicas

- Pin objetivo: Blender 5.2.1 portable privado del worker. Blender 5.2 LTS tiene
  soporte oficial entre julio de 2026 y julio de 2028. El binario del worker se
  valida por versión, build y SHA-256; el Blender del usuario no es fallback.
- Un graph autoritativo se instala como **Geometry Nodes Modifier**. Los Node Tools
  dependen del contexto/selección del viewport, no admiten inspección completa,
  eliminan shape keys al modificar y no permiten Simulation/Viewer; no son la API
  headless del producto.
- Blender 5.2 incorpora Geometry Bundles, listas, `Collection Children`, captura de
  selección, transferencia/renombre/listado de atributos, `Mesh Bevel`, operaciones
  de conectividad, acceso a componentes, mejoras de curvas e inputs de Node Tools
  asignables desde Python. Son útiles, pero todo graph debe declarar su versión
  mínima y no degradarse silenciosamente.
- En 5.2 los inputs de un modificador GN son propiedades RNA públicas. El adapter
  debe localizar el socket por `identifier`, nunca por orden, nombre visible ni
  `Socket_N`. Descripción, tipo, subtipo, default y límites salen de la interfaz;
  enums, unidades Jerico, dependencias y semántica requieren manifest explícito.
- `--background`, factory startup, configuración privada, `--offline-mode`, rutas
  locales y red denegada forman el entorno. Assets online y resolución dinámica de
  paquetes están prohibidos en generación.
- Simulation Zones y la física experimental 5.2 sólo son candidatas para variantes
  visuales horneadas. Jolt, navegación, tráfico, gameplay, destrucción runtime y
  persistencia siguen siendo autoridad Jerico.

## 3. Biblioteca propia transversal

Estos grupos son la base compartida. Cada uno debe exponer sockets tipados y
producir geometría más atributos/provenance, sin depender de nombres de objetos.

| ID estable propuesto | Nodos/patrón principal | Salidas obligatorias | Uso |
| --- | --- | --- | --- |
| `jerico.core.selection-mask/v1` | Named Attribute, Compare, Boolean Math, Accumulate Field, Capture Attribute, Store Named Attribute | geometry, selection, inverse, stable element IDs, rejection counts | Selección, layers, decals, caminos, daño, biome masks. |
| `jerico.core.spatial-constraints/v1` | Geometry Proximity, Raycast, Sample Nearest Surface, Signed Distance/Volume, Bounding Box | allowed, protected, keep-clear, contact-band, clearance distance | No invadir espacio jugable, BSP, portales, nav, cámaras y contactos. |
| `jerico.core.curve-frame/v1` | Resample Curve, Spline Length/Parameter, Tangent, Normal, Set Curve Tilt, custom up-vector repair | curve, arc length, frame, curvature, discontinuity mask | Carreteras, tuberías, cables, rails, splines destructivos. |
| `jerico.core.profile-sweep/v1` | Curve to Mesh/Curve to Tube, profile selection, caps, Realize Instances opcional | mesh, longitudinal/lateral coordinates, cap/side class | Caminos, vigas, túneles, armas, ruedas, decals 3D. |
| `jerico.core.repeat-instance/v1` | Repeat Zone, Instance on Points, Collection Info, Pick Instance, stable sort/list | instances, instance IDs, source asset IDs, realized-on-export flag | Arquitectura modular, props, guardrails, vegetación, ruedas. |
| `jerico.core.surface-scatter/v1` | Distribute Points on Faces/Volume, Poisson, normals, rotation, density masks | points/instances, density, seed lineage, rejected point classes | Vegetación, piedras, basura, items, crowd markers offline. |
| `jerico.core.mesh-regions/v1` | Mesh Island, Face Group Boundaries, shortest paths, connected components, Split to Instances | region/fragment IDs, adjacency candidates, boundaries | Destrucción, piezas, LOD, materiales, selección loop. |
| `jerico.core.material-masks/v1` | Named attributes, normals/slope/height/curvature/proximity/noise, Map Range | named float/int masks plus class table | Layer System, PWAS/EPLS, barro/nieve/polvo/desgaste. |
| `jerico.core.provenance/v1` | Capture/Store/Transfer Attribute, domain conversion with explicit loss metrics | source/layer/generator/region/material/physical IDs | Todo producto y cada conversión. |
| `jerico.core.budget-metrics/v1` | Domain Size, statistics via accumulate/reduce, bounding boxes | counts, bounds, density, budget flags | Rechazo temprano, métricas y preview/diff. |

Reglas comunes:

1. `Geometry`, `Selection`, `Seed`, `QualityProfile`, `LayerId` y
   `SourceRevision` nunca se infieren de la escena.
2. Los IDs aleatorios derivan de `rootSeed + generatorId + layerId + stableId`;
   no dependen del orden accidental de collections ni del frame.
3. Se preservan instancias hasta que export/colisión/bake exijan realizarlas.
4. Cada transferencia de dominio declara pérdida y métrica; nearest-surface no es
   prueba de identidad semántica.
5. `Geometry Bundles` puede agrupar canales internos, pero el contrato de salida
   Jerico sigue siendo explícito y serializable.

### 3.1 Inventario nativo 5.2 que debe evaluarse

La tabla es el inventario funcional para Jerico, no una promesa de usar cada nodo
en cada graph. La admisión verifica el `bl_idname` real bajo el pin 5.2.1 y bloquea
un asset si falta un tipo; el nombre visible de esta investigación no es ABI.

| Categoría | Nodos/patrones nativos útiles | Aplicación Jerico | Precaución |
| --- | --- | --- | --- |
| Geometry | Join Geometry, Separate Components, Delete Geometry, Duplicate Elements, Split to Instances, Geometry to Instance, Transform Geometry, Set Position, Bounding Box | composición de layers, separación de piezas, instancing y bounds | Join no fusiona topología ni resuelve semántica por sí solo. |
| Componentes 5.2 | Get Geometry Component, Geometry Bundles y sockets de bundle | transportar conjuntos internos de componentes/campos | exteriorizar siempre outputs serializables; bundle no sustituye schema. |
| Identidad/atributos | Index, ID, Set ID, Named Attribute, Capture Attribute, Store Named Attribute, Remove Named Attribute, Attribute Domain Size, Evaluate on Domain | stable IDs, masks, provenance y métricas | Index cambia con topología; sólo ID capturado antes de mutar puede ser linaje. |
| Atributos 5.2 | captura con selection, Rename Attribute, Get Attribute Names, Transfer Attribute(s) | preservar/inspeccionar semántica y medir conversiones | transferencia espacial puede ser ambigua; devolver misses/distancia/pérdida. |
| Muestreo espacial | Raycast, Geometry Proximity, Sample Nearest, Sample Nearest Surface, Sample Index | BSP contact, terrain conform, decals, máscaras y transfer | fijar dominio, modo de interpolación, max distance y receiver ID. |
| Malla: primitivas | Cube, Cone, Cylinder, Grid, Icosphere, UV Sphere, Mesh Circle, Mesh Line | blockout, props, boulders base, terrain grids | perfiles de calidad limitan resolución antes de crear. |
| Malla: generación | Extrude Mesh, Scale Elements, Subdivide Mesh, Triangulate, Dual Mesh, Flip Faces, Mesh Boolean, Mesh to Curve, Curve to Mesh | arquitectura, hard-surface, caves, profiles y cortes | booleans/remesh requieren validación manifold/self-intersection. |
| Malla 5.2 | Mesh Bevel, Merge Points by Distance, Merge Points by Connected, Cluster by Distance | bordes, weld y clusters/fragments | no soldar across source/material/chunk/keep-clear boundaries. |
| Lectura topológica | Edge Vertices, Edge Angle, Edge Neighbors, Face Neighbors, Vertex Neighbors, Mesh Island, Face Area, Is Face Planar, Face Group Boundaries | loops/regions, hard edges, material y fragment classification | polos/non-manifold se diagnostican; no inventar continuidad. |
| Paths de malla | Shortest Edge Paths, Edge Paths to Selection, Edge Paths to Curves | roads sobre mesh, grietas, selection paths, vines | pesos no negativos, ties canónicos y endpoint IDs explícitos. |
| Curvas: primitivas | Curve Line, Curve Circle, Arc, Quadratic Bézier, Bézier Segment, Spiral, Star | profiles, roads, cables, shapes y detalles | primitives son formas auxiliares, no source spline authority. |
| Curvas: lectura | Curve Length, Spline Length, Spline Parameter, Curve Tangent, Curve Normal, Sample Curve | coordenadas métricas, frame, placement y UV longitudinal | reparar normal/tilt en inflexiones y loops cerrados. |
| Curvas: edición | Resample Curve, Trim Curve, Fillet Curve, Subdivide Curve, Reverse Curve, Set Curve Radius, Set Curve Tilt, Set Spline Cyclic/Type/Resolution | carreteras, tubos, ramas, bevel, decal/spline destruction | preservar endpoints, knots semánticos y arc-length lineage. |
| Curvas: generación | Curve to Mesh, Fill Curve, Interpolate Curves, Curve to Tube 5.2 | perfiles, muros, túneles, vegetación y cables | caps/orientation/material IDs se validan después. |
| NURBS 5.2 | acceso/asignación de order y weight | carrocerías y arquitectura curva | no garantiza continuidad/retopo; medir superficie. |
| Puntos | Points, Mesh to Points, Points to Vertices, Points to Curves, Set Point Radius | scatter, reconstruction, paths y markers | no materializar millones de puntos sin budget previo. |
| Distribución | Distribute Points on Faces, Distribute Points in Volume | vegetación, rocks, debris, interiors, caves | Poisson/random debe derivar de stable IDs; collection order fijado. |
| Instancias | Instance on Points, Instances to Points, Rotate/Scale/Translate Instances, Realize Instances, Instance on Points, Instance Bounds/Transform | kits, props, vegetation, modules y LOD | Realize tarde; conservar asset/instance IDs antes. |
| Instancias 5.2 | Instance Reference, Collection Children | elección de assets y jerarquías locales | ordenar candidatos por GUID/hash, no por orden accidental de collection. |
| Volúmenes | Volume Cube, Mesh to Volume, Volume to Mesh, Distribute Points in Volume, SDF/Fog grids | caves, boulders, overhangs, clouds auxiliares y constraints | voxel size domina memoria/error; volumen Blender no va a Shipping. |
| Materiales | Set Material, Set Material Index, Replace Material, Material Selection | preview, slots y separación para bake | material Blender es preview; Jerico GUID/class es autoridad. |
| UV | UV Unwrap, Pack UV Islands, Store Named Attribute y coordenadas de curve/profile | unwrap general, bands, roads, trims y bake | packing/quality se revalida con bpy; no asumir calidad por existir UV. |
| Texturas/campos | Noise, Voronoi, Wave, White Noise, Gradient, Gabor donde esté disponible; Map Range/Color Ramp/Float Curve | geology, weathering, scatter, masks y detail | seeds/escalas/unidades fijadas; evitar ruido dependiente del frame. |
| Matemática | Math, Vector Math, Mix, Clamp, Map Range, Compare, Boolean Math, Switch, Menu Switch, Index Switch | parámetros, selección, frames y decisiones discretas | enums Jerico se manifiestan; no inferirlos de un Switch anónimo. |
| Reducción/campos | Accumulate Field, Field at Index, Attribute Statistic, Domain Size | chain ordering, métricas, densidad y budgets | ordenar con stable keys antes de reducción si afecta bytes finales. |
| Listas 5.2 | Field to List, Closure to List, Filter List, Sort Elements/List, Get Item y List Length | candidatos, orden determinista, filtros y agregación | listas grandes requieren límite; closure no cruza el protocolo. |
| Control flow | Repeat Zone, Simulation Zone, Bake node/zone según contexto | arrays iterativos, growth offline y estados horneados | Simulation no es física/runtime; frame/cache son inputs explícitos o se rechaza. |
| Assets 5.2 | assets oficiales de proyección y PCA | frames/proyección y análisis de orientación | vendorizar localmente con hash/licencia; nunca resolver online al generar. |
| Strings 5.2 | operaciones string y nombres de atributos | diagnóstico e introspección local | IDs de producto no se construyen desde labels localizados. |
| Sound 5.2 | sampleo de sonido en GN | geometría visual offline dirigida por audio | audio gameplay/Sequencer no se evalúa en el worker autoritativo. |

### 3.2 Combinaciones que resuelven casos repetidos

| Patrón | Cadena mínima |
| --- | --- |
| Scatter condicionado | source → slope/height/proximity/biome masks → Distribute Points → stable pick → Instance on Points → provenance. |
| Camino/carretera/tubo | Jerico spline → Resample → tangent/normal/tilt repair → profile → Curve to Mesh/Tube → longitudinal UV → instances. |
| Edificio modular | footprint loops → floors/walls/extrude → bays → collection instances → openings/booleans → rooms/façade attrs. |
| Roca/cliff | base mesh/SDF → multiscale anisotropic fields → strata/fracture/weather masks → Volume to Mesh → region/material attrs. |
| Cave | allowed SDF − tunnel/cavity fields − expanded keep-clear → Volume to Mesh → floor/wall/ceiling/contact classification. |
| Damage | selected field/curve → cut volume → Mesh Boolean/SDF subtract → regions/split → fresh-surface/provenance. |
| Decal 3D | receiver selection → raycast/projection → local frame → clip/occlusion → geometry/UV/mask → receiver IDs. |
| Selection loop/path | stable topology IDs → CPU loop/ring or Shortest Edge Paths → ordered chain → Store Named Attribute. |
| LOD preparation | lock masks → triangulate → decimate/remesh candidate → transfer attrs → measure error → reject or emit lineage. |

No combinación sustituye el validador: el graph produce un candidato y métricas;
el worker y luego el host nativo deciden si puede avanzar.

## 4. Matriz exhaustiva por género y sistema

### 4.1 FPS, TPS y acción

| Capacidad | Construcción | Decisión | Límite de autoridad |
| --- | --- | --- | --- |
| Blockout y dressing BSP-aware | Kits por collection, repeat/instance, raycast, proximity, keep-clear | `BUILD_OWN` | BSP/portales/nav/cámara los envía Jerico. |
| Cobertura, barricadas y fortificaciones | Ensamble modular con sockets, footprint y clearance | `BUILD_OWN` | Clasificación de cover y gameplay se valida en Engine. |
| Interiores procedurales | Room/door/window graph desde loops semánticos; kits por tags | `BUILD_OWN` | La conectividad jugable y puertas autoritativas no nacen de GN. |
| Armas, accesorios e items | Primitivas, bevel, booleans, arrays, profile sweep, attachment sockets | `BUILD_OWN` | Rig, animación, inventory y daño son consumidores Jerico. |
| Sangre, impactos, grietas y suciedad | Proyección/raycast, decal geometry o mask fields | `BUILD_OWN` y reuso Decal3D | Runtime decal batching/shader queda en renderer. |
| Daño autorable | Cut masks, split regions, fragment provenance, fresh-surface masks | `BUILD_OWN` y reuso Spline Destruction | Fractura/constraints/physics runtime quedan en Jolt. |

### 4.2 Autos, conducción y vehículos

| Capacidad | Construcción | Decisión | Límite de autoridad |
| --- | --- | --- | --- |
| Carreteras/carriles/banquinas | Jerico spline → frame estable → perfiles → sweep → markings/curbs | `BUILD_OWN` | Grafo vial, reglas, tráfico y navegación son Jerico. |
| Intersecciones | CPU resuelve topología y prioridades; GN genera superficies/props desde solución | `BUILD_OWN` híbrido | No resolver una red vial compleja sólo con nombres/booleans GN. |
| Guardrails, postes, señales, luces | Resample + instances por distancia/curvatura/máscara | `BUILD_OWN` | IDs y reglas de colocación se conservan para gameplay. |
| Vías férreas | Dos rails, sleepers, ballast masks, switches desde topología CPU | `BUILD_OWN` híbrido | Señalización y switching runtime quedan en Engine. |
| Neumáticos, llantas, cables, escapes | Curvas/perfiles/arrays radiales/bevel/material masks | `BUILD_OWN` | Física de rueda, suspensión y handling no se hornean en GN. |
| Carrocería configurable | Curvas guía, superficies Coons estudiadas, simetría y detalles modulares | `BUILD_OWN`; `STUDY` Surface Mesh | Retopo/UV/bake requieren pipeline híbrido y validación. |
| Variantes y daño visual | Parámetros discretos, pieces/attachments, deformation masks offline | `BUILD_OWN` | Deformación/colisión runtime es una representación distinta. |

### 4.3 Isométrico, estrategia, builder y roguelike

| Capacidad | Construcción | Decisión | Límite de autoridad |
| --- | --- | --- | --- |
| Tiles y módulos | Grid, kit collections, sockets compatibles, rotaciones canónicas | `BUILD_OWN` | Grid lógico/ocupación/placement authority queda en Jerico. |
| WFC/constraint placement | Solver CPU determinista emite elecciones; GN ensambla/decora | `BUILD_OWN`; `STUDY` WFC 3D | No depender de operadores UI ni orden aleatorio de collection. |
| Dungeons/cities | Parcelas/rooms/corridors en CPU, edificios/calles/dressing en GN | `BUILD_OWN`; `STUDY` Bene ProgGen/Infinigen | Accesibilidad y conectividad se prueban en Jerico. |
| Niveles apilados/cutaway | Layer/level IDs y visibility masks | `BUILD_OWN` | Cámara, roof hiding y navegación son presentación/runtime. |
| Props legibles a distancia | Silhouette presets, budgets por cámara/zoom, material classes | `BUILD_OWN` | No sacrificar collision/nav por un LOD visual. |

### 4.4 Arquitectura, ciudades e interiores

- `jerico.arch.building-kit/v1`: footprint/parcel loops, floors, walls, slabs,
  roofs, doors/windows, façade bays, cornices, stairs/railings, sockets, room and
  façade IDs. Usa Extrude Mesh, Scale Elements, Inset/Bevel, curves, booleans e
  instancias. `BUILD_OWN`.
- `jerico.arch.interior-kit/v1`: rooms/portals/corridors authored por Jerico;
  genera trim, zócalos, marcos, paneles, techo, fixtures y dressing condicionado.
  `BUILD_OWN`.
- `jerico.arch.infrastructure/v1`: puentes, túneles, muros, cercas, utilities y
  módulos repetidos sobre splines. `BUILD_OWN`.
- Archimesh, Arch Generator y French Houses se usan como casos de estudio de
  parámetros y construcción; no como autoridad ni requisito de UI.

### 4.5 Terreno, geología, acantilados y cuevas

Familias propias obligatorias, además de los siete candidatos ya existentes:

| ID propuesto | Construcción GN/híbrida | Máscaras/salidas |
| --- | --- | --- |
| `jerico.terrain.heightfield/v1` | grid adaptativo, displacement multiescala, terraces, slope limits | height, slope, curvature, drainage candidate, biome, walkability candidate |
| `jerico.geology.sedimentary-strata/v2` | coordenada estratigráfica, warping suave, banding, differential erosion | stratum, bedding, exposed, sediment class |
| `jerico.geology.fractured-boulder/v2` | base convexa/SDF, multi-noise anisotrópico, planar cuts, chips | fracture, fresh surface, edge wear, region |
| `jerico.geology.columnar/v2` | seeds hex/irregular, columnas orientadas, split/jitter, caps erosionadas | column, joint, top/bottom/side, fracture |
| `jerico.geology.rounded-weathered/v2` | remesh/SDF, low-frequency form, smoothing/erosion proxy, cavities | convexity, cavity, weathering, wetness candidate |
| `jerico.geology.cliff/v2` | cliff frame, shelves, strata, talus scatter, overhang SDF | cliff face, shelf, talus, overhang, material classes |
| `jerico.geology.cave-shell/v2` | BSP boundary + allowed/protected SDF, tunnels/cavities, portal locks | interior/exterior, ceiling/wall/floor, contact, keep-clear distance |

La erosión físicamente fiel, retopología y grandes cuevas con overhang son híbridas;
GN puro no debe fingir exactitud. A.N.T. Landscape y Terrain Mixer son referencias.
Infinigen se estudia por composición/constraints, no se integra tal como está.

### 4.6 Vegetación, naturaleza y scatter

- `jerico.nature.scatter/v1`: points on faces/volume, Poisson, slope/height/
  biome/proximity/visibility masks, stable IDs, density budgets, collection pick
  determinista y exclusión keep-clear.
- `jerico.nature.tree/v1`: trunk/branch curve graph, taper, branching rules,
  leaf/twig instances, wind metadata y LOD lineage. No simula viento runtime.
- `jerico.nature.vines/v1`: surface-guided paths, attachment/contact mask,
  gravity/phototropism proxy, stems/leaves; caminos largos pueden resolverse en CPU.
- `jerico.nature.ground-cover/v1`: grass, flowers, debris, rocks, fallen leaves y
  macro clumps con presupuestos por World Cell.
- Easy Tree, Modular Tree, Generate Tree y BagaPie se evalúan como referencias.
  Un asset generado sólo se admite si su licencia de redistribución es compatible;
  licencia GPL del código no concede automáticamente derechos sobre contenido de
  terceros empaquetado.

### 4.7 Props, items, hard-surface y ensamblajes

- `jerico.props.modular-assembly/v1`: sockets tipados, attachment rules, symmetry,
  radial/linear arrays, variants, material slots, collision hints y provenance.
- `jerico.props.hardsurface/v1`: primitive blocks, booleans, bevel, panels,
  fasteners, cables/pipes, damage masks; `bpy/bmesh` valida manifolds y aplica
  modificadores en orden canónico.
- `jerico.props.container/v1`: crates, barrels, shelves, pallets, furniture,
  pickups y dressing con footprints y keep-clear.
- `jerico.props.weapon-item/v1`: receiver/body, modular attachments, sockets,
  silhouette budgets y bake groups. Gameplay/rig siguen fuera.
- Hardflow se estudia por flujo boolean/curve/decal/kitbash. No se ejecuta su UI en
  headless ni se expone cada PropertyGroup automáticamente.

### 4.8 Destrucción, spline destructivo y Decal 3D

`jerico.damage.authoring/v1` consume malla/source IDs y una operación tipada
(impact, cut, crack, chip, slice, crush visual). Produce cut field, fragment/region
IDs, adjacency candidate, fresh-surface mask, anchors/bonds candidates y geometría
high/low. Cell Fracture puede evaluarse como adaptador de authoring, pero la
partición final debe conservar determinismo, límites y provenance.

`jerico.spline-destruction/v2` reutiliza `core.curve-frame` y
`core.profile-sweep`: la spline Jerico define ancho/profundidad/falloff, semántica,
revision y selección; el graph crea el campo/cinta/volumen de corte y regiones. No
crea constraints Jolt ni decide mutaciones runtime.

`jerico.decal3d/v2` reutiliza las mismas proyecciones, selección y layers: proyecta
malla/curva/imagen sobre una surface admitida, produce geometry decal opcional,
UV/local frame, clip/occlusion mask, receiver source/face IDs y material class.
El renderer decide decal runtime; el cooker decide mesh/material transport.

### 4.9 Selección: loops, rings, paths, regiones y placement

La selección es un dato versionado, no estado oculto de Blender:

| Operación | Implementación preferida |
| --- | --- |
| Edge loop/quad loop | CPU/BMesh sobre snapshot con stable edge IDs; GN recibe la máscara. |
| Edge ring | CPU/BMesh con reglas explícitas para polos/non-manifold. |
| Shortest path | `Shortest Edge Paths` + `Edge Paths to Selection/Curves` cuando pesos y topología son suficientes. |
| Boundary loop | Face Group Boundaries/Mesh Boundary, luego chains ordenadas en CPU. |
| Connected/island/region | Mesh Island, connectivity 5.2, Split to Instances. |
| Angle/curvature/material | fields GN, con threshold/unidad y dominio declarados. |
| Visibility/occlusion | raycast desde cámara/sampling contract; no viewport selection. |
| Placement | source IDs + local transforms + allowed/protected/contact masks. |

Branches, poles, non-manifold, virtual centerlines, winding y múltiples caminos se
reportan como diagnósticos; no se elige silenciosamente un loop diferente.

## 5. Layer System y material masks

La receta Jerico es source-of-truth. Cada layer tiene `layerId`, `type`, `enabled`,
`seed`, `selection`, `phase`, `dependencies`, `operator`, `parameters`,
`materialPolicy`, `physicalPolicy` y `locks`. Orden de fases:

1. `SourceAdmission`: snapshot/BSP/IDs/revisiones/unidades/transforms.
2. `SpatialConstraints`: allowed/protected/keep-clear/contact bands.
3. `PrimaryForm`: terreno, roca, edificio, carretera o prop.
4. `SecondaryForm`: strata, erosion proxy, bevel, trims, modules.
5. `Detail`: cracks, chips, scatter, vegetation, decals.
6. `Classification`: material/physical/PWAS/EPLS/navigation candidate masks.
7. `Preparation`: high/low, UV, bake targets, LOD, collision.
8. `ValidationExport`: budgets, provenance, artifacts y manifest.

Operadores permitidos: `REPLACE`, `UNION`, `SUBTRACT`, `INTERSECT`, `BLEND`,
`ATTRIBUTE_ONLY`, `SCATTER`, `DECAL_PROJECT`, `SPLIT_REGIONS`. Cada layer conserva
`beforeHash`, `afterHash`, mask hashes y rejection metrics. Locks de boundary,
portal, material, UV seam, chunk y authored region sobreviven fases posteriores.

Máscaras canónicas mínimas: `jerico_source_id`, `jerico_layer_id`,
`jerico_region_id`, `jerico_fragment_id`, `jerico_material_class`,
`jerico_physical_class`, `jerico_surface_class`, `jerico_stratum`,
`jerico_fracture`, `jerico_fresh_surface`, `jerico_weathering`, `jerico_cavity`,
`jerico_wetness`, `jerico_slope`, `jerico_contact`, `jerico_keep_clear_distance`,
`jerico_nav_candidate`, `jerico_decal_receiver` y `jerico_lod_lineage`.

## 6. BSP y reglas para no invadir espacio jugable

Jerico envía una representación analítica autoritativa y una surface triangulada de
conveniencia, ambas ligadas por hash/revisión. El snapshot contiene solids,
half-spaces/planes, faces/loops, portal/nav/camera/playable semantics, World Cell,
selection AST y volúmenes `Allowed`, `Protected`, `KeepClear` y `ContactBand`.

Reglas:

1. Ningún vértice/triángulo/instancia admitida entra en `Protected` expandido por
   su margen semántico; `KeepClear` se evalúa también contra el footprint completo.
2. Portales, doors, spawn/interaction zones, camera corridors, nav clearance,
   vehicle envelopes y destruction escape paths son clases distintas; no un AABB
   genérico.
3. La generación puede tocar `ContactBand` sólo cuando la layer lo declara; debe
   devolver distancia y receiver/source IDs.
4. El worker valida antes, durante y después de cada layer, LOD y colisión. El host
   nativo vuelve a validar contra la revisión vigente antes de Apply/cook.
5. Discrepancia analítico/triangulado, revisión stale o semántica desconocida
   rechaza el candidato completo. No hay clip reparador silencioso.

## 7. UV, bake, LOD, collision y export

| Fase | Política productiva |
| --- | --- |
| UV | `PRESERVE_SOURCE`, `UNWRAP_ANGLE`, `UNWRAP_CONFORMAL`, `CURVE_METRIC`, `PLANAR/TRIPLANAR`, `LIGHTMAP`. Seams/locks semánticos, texel density, margin derivado de resolución/mips/compresión y métricas de overlap/stretch/occupancy. GN UV nodes ayudan; `bpy` completa y valida. |
| Bake | Cycles CPU offline, high/low/cage explícitos, tangent/object/world spaces y targets Normal/AO/Curvature/Position/Thickness/ID/material masks. Rechazar misses, backfaces, wrong-chunk hits y NaN/Inf; dilation y hashes canónicos. |
| LOD | Error de superficie/normal/UV/silhouette por perfil, boundary/chunk/material locks, region preservation y lineage. Decimate nativo es candidato, no prueba completa. |
| Collision | Static triangle, convex hull/decomposition o authored chunks según producto; budgets y coverage contra render mesh. Devuelve candidate + physical/region IDs; `.rcoll`/Jolt cook es nativo. |
| GLB | Transporte de malla, normals/tangents/UV, named attributes y extras manifest. Un sidecar canónico conserva tablas/IDs que glTF no representa sin ambigüedad. GLB nunca es Shipping. |

## 8. Catálogo gratuito investigado

Ninguna fila está admitida. “Compatible declarado” significa metadata pública del
proyecto, no conformance Jerico 5.2.1.

| Proyecto | Área útil | Licencia/compatibilidad declarada | Decisión y motivo |
| --- | --- | --- | --- |
| BagaPie | Scatter, ivy, arrays, arquitectura, GeoPack | GPL; Blender 5.0+ | `STUDY`, luego `ADAPT` sólo para kernels aislables; orientación UI y catálogo de assets requieren auditoría. |
| WFC 3D Generator | Ensamble modular constraint-based | GPL-3.0; Blender 4.2+ | `STUDY` solver/formato; preferir solver propio determinista y GN para output. |
| Archimesh | Rooms, doors, windows, roofs, cabinets | GPL-2.0-or-later; Blender 4.2+ | `STUDY`; soporte limitado y operadores UI. |
| Arch Generator | Arches y pillars por BMesh | GPL-3.0; Blender 5.0+ | `STUDY`/`ADAPT` kernel pequeño tras test headless. |
| French Houses GN | Fachadas/edificios procedurales | MIT; proyecto GN Blender 5.0 | `STUDY`; buena referencia de graph/assets, auditar redistribución de cada asset. |
| Bene ProgGen Maps | City, terrain y dungeon generation | GPL-3.0; Blender 4.2+ | `STUDY`; separar layout/constraints de generación visual. |
| City Generator | Ciudad Geometry Nodes | CC0-1.0; creado para Blender 4.2.1+ | `STUDY`; graph compacto para parcelas/edificios, sin asumir compatibilidad 5.2.1. |
| MRoadGen | Road networks, intersections, overpasses, embankments y markings | CC0-1.0; declara Blender 5.0+ | `STUDY` fuerte y posible asset `ADAPT`; comparar contra spline/BSP/provenance Jerico. |
| Driving Scenario Creator | OpenDRIVE/OpenSCENARIO, roads y escenarios automotrices | GPL-3.0; tests geométricos upstream | `STUDY` formatos/curvas/intersections; evitar dependencias pip dinámicas y autoridad de tráfico duplicada. |
| Maze Generator Pro | Mazes/dungeons, curve/mesh paths y surface variation | GPL-3.0; Blender 4.2+ | `STUDY`/`ADAPT` layout offline; accesibilidad y seeds requieren gates propios. |
| Easy Tree | Árboles Geometry Nodes | GPL | `STUDY`; validar determinismo, provenance, LOD y assets incluidos. |
| Modular Tree | Árboles y Pivot Painter | GPL-3.0; Blender 5.0+ | `STUDY`/`ADAPT`; Pivot Painter puede informar metadata runtime, no sustituye contrato Jerico. |
| Generate Tree Plugin | Árboles low-poly seeded | GPL | `STUDY`/`ADAPT` si el kernel headless pasa pin y licencia. |
| Hardflow | Boolean, curves, decals, kitbash | GPL-3.0; Blender 4.2+ | `STUDY`; UI-heavy, no convertir paneles arbitrarios en controles Jerico. |
| Surface Mesh | Superficies Coons desde curves | GPL-3.0; Blender 4.2+ | `STUDY`/`ADAPT` para carrocerías/arquitectura curva. |
| Tissue | Tessellation/computational design | GPL-2.0-or-later | `STUDY`; dependencias opcionales online/Numba bloquean admisión sin paquete offline reducido. |
| Sverchok | Sistema visual procedural amplio | GPL-3.0 | `STUDY`, `REJECT` como runtime del backend: segundo sistema de nodos, gran superficie y dependencias opcionales. |
| Infinigen | Naturaleza/interiores procedurales | BSD-3-Clause; pyproject actual fija bpy 4.2 | `STUDY`: constraints, composición y transpilation; `REJECT` como dependencia 5.2.1 sin port/audit acotado. |
| BlenderProc | Generación de escenas/datasets | GPL-3.0 | `STUDY` provenance/composición; no dependencia del worker. |
| A.N.T. Landscape | Terrain/landscape | GPL | `STUDY`/`ADAPT` algoritmo después de prueba 5.2.1. |
| Terrain Mixer | Mezcla/generación de terrenos | GPL; declara Files y Network | `STUDY`; red prohibida y pipeline de assets debe aislarse. |
| Cell Fracture | Fracture authoring | GPL | `ADAPT` candidato tras demostrar seed, bounds, chunks y provenance. |
| Remi | Remesh, UV y baking | licencia declarada por upstream; usa PyMeshLab | `ADAPT` candidato fuerte por smoke tests headless; eliminar instalación por red y fijar wheel/hash antes. |
| EasyMesh Batch Exporter | Batch export | GPL | `ADAPT`/`STUDY`; comparar con export propio y conservar validación Jerico. |
| Monty Batch Exporter | Export CLI/headless | GPL | `ADAPT`/`STUDY`; verificar URL/versión exacta y formatos antes de intake. |
| Attribute Baker | Bake de atributos | GPL | `STUDY`/`ADAPT`; validar espacios, targets, misses y determinismo. |
| Sequenced Bake | Bake queues, sprite sheets y position/sculpt maps | GPL-3.0; Blender 5.0+; permiso Files | `STUDY`; timer/modal/UI no es host headless, pero aporta casos de cancel y mapas. |
| Bake To Single Material | Consolidación de materiales/UV/bake | GPL-3.0; Blender 5.1+ | `STUDY`/`ADAPT` kernel tras test CPU/headless; no usar su detección GPU automática. |
| The Better Baker | PBR/UDIM bake | GPL-3.0; Blender 5.1+; permiso Files | `STUDY`/`ADAPT`; auditar high-to-low/cage, progreso, CPU determinista y targets Jerico. |
| LODGen | LOD generation | GPL | `STUDY`/`ADAPT`; no admitir sin métricas de error/locks/lineage. |
| Blender Game Tools | Preparación/export game assets | GPL | `STUDY`; Jerico conserva cook y schemas propios. |
| GamiFlow | Game asset workflow | GPL | `STUDY`; evaluar naming/collections/export, no importar UI. |
| ACT Game Asset Toolset | UV/LOD/export utilities | GPL | `STUDY`; extraer casos y no crear segunda pipeline. |
| Texture Mixer | Mezcla de texturas | GPL-3.0, alpha | `STUDY`; `REJECT` para producción mientras sea alpha/no cumpla bake gates. |
| Blender Rock Generator (lupin4) | Roca low-poly, UV y variación | MIT; script público | `STUDY`; limpia la escena y depende del editor, por lo que sólo interesan algoritmo/casos. |
| DragonGraph Toolset | Pack abierto de node groups/asset library | GPL-3.0 | `STUDY`; inventariar cada group por hash/licencia y adaptar sólo patrones pequeños. |
| Rock Generator Add-on (tu2463) | Rock GN + material | repositorio público sin licencia visible en la revisión | `REJECT` hasta licencia explícita, aunque el graph parezca útil. |
| TreeGen LLM | Árboles GN guiados por LLM local | MIT; GGUF, wheels y llama.cpp | `REJECT` como dependencia: gran modelo/binarios y schema libre; estudiar sólo su separación socket-schema. |
| PROTO Game Asset Tools | FBX/export y animación | GPL-3.0; versiones públicas marcan 5.1+ unsupported | `REJECT` para el pin 5.2.1 hasta que upstream declare y demuestre soporte. |
| CrashCar101 | Dents/daño vehicular con GN | paper y material académico, no addon de producto | `STUDY` para dent fields/adaptive subdivision; implementación propia y licencia independiente. |

Rechazo inmediato adicional: addons sin licencia explícita, abandonados para ramas
antiguas, binarios cerrados, login/cloud, telemetría/red obligatoria, descarga en
primer arranque, ejecución de código arbitrario, dependencia de viewport/operator
context, contenido con derechos inciertos o resultados sin seed/provenance.

### 8.1 Profundidad real de la revisión de implementaciones

La calidad visual publicada por un proyecto no demuestra cómo está construida.
Esta matriz evita confundir marketing, interfaz de sockets y graph audit:

| Referencia | Evidencia estática comprobada | Qué puede aprender Jerico ahora | Evidencia pendiente para conocer sus nodos reales |
| --- | --- | --- | --- |
| MRoadGen | Repositorio CC0, Blender 5.0+, distribución como `.blend`; declara raycast de bordes sobre terrain, embankments, ancho variable, intersections/overpasses, surfaces, projected markings y scatter por color attributes. | Descomponer el road generator propio en corridor surface, terrain projection, embankment, marking y scatter masks. | Descargar release por hash e inspeccionar todos los node groups, links, interfaces, named attrs, materials, modifiers y muestras bajo 5.2.1. El repo no serializa el graph como texto. |
| French Houses | Repo MIT con dos `.blend`; declara houses, towers, bridges, walls, gates, cliffs, vegetation, stalls, terraces, gothic structures, church/cathedral, rails y stairs; graphs documentados con Viewer nodes. | Buen corpus integral para arquitectura modular y composición de subgroups; confirma que no basta un generador de paredes. | Probe Blender del `.blend` y capturas/goldens por familia. README no revela conexiones ni explica qué parte de la calidad proviene de materials/assets. |
| Driving Scenario Creator | Código GPL-3.0 y tests; roads muestreadas en coordenadas de carretera, cross-sections, clothoid/Euler spiral, cubic Hermite, elevation, lane/junction linking, signs/vehicles/OpenDRIVE/OpenSCENARIO. También exige pip libraries y herramientas externas. | Reusar conceptos matemáticos en un solver CPU Jerico que entregue splines/cross-sections resueltas a GN. | Auditoría de módulos/tests/versiones; no debe entrar como dependencia offline mientras instale `scenariogeneration`, `pyclothoids`, OpenSceneGraph o esmini externamente. No es prueba de un graph GN impecable. |
| DragonGraph | GPL-3.0, asset library con `.blend`, versiones que declaran 5.2 LTS; documentación upstream reconoce retraso y scripts de auto-setup de Python. | Fuente amplia para descubrir pequeños utility groups y comparar convenciones de assets. | Inspección individual por group/hash/licencia; desactivar auto-setup y dependencias. Nunca admitir el pack completo por volumen ni por compatible 5.2. |
| Arch Generator | Source Python/BMesh, perfiles paramétricos de arches/pillars y licencia GPL-3.0. | Fórmulas/perfiles y fixtures de arquitectura; posible adapter CPU pequeño. | Tests headless 5.2.1, topología, transforms, unidades y eliminación de operator/UI context. No enseña Geometry Nodes. |
| Infinigen | Source BSD-3, composición procedural y node transpiler; dependencia actual declarada sobre `bpy==4.2.0` y stack Python amplio. | Diseño de factories, constraints, seed lineage, composición y generación de corpus. | Port acotado y auditoría de dependencias; no ejecutar el sistema completo en el worker 5.2.1. |
| Addons locales previamente ingeridos | Intake estático retuvo metadata Python, hashes de bibliotecas, interfaces y miles de sockets; perfiles propios sólo para operaciones expresamente revisadas. | Nombres, tipos, defaults, ranges, panels y candidatos de binding. | Socket/interface no prueba links, evaluación, resultado visual ni equivalencia. Cada capacidad requiere behavioral golden en Blender. |

Inventario privado hoy disponible en la carpeta: informes/metadatos de RockForm
1.4.1, The Nature Generator, Edge Decal Generator 1.2.1, Smart Segmentation 1.0 y
OCD 2. Sus paquetes originales no están conservados allí; sólo existe el `.blend`
de fixture usado por el intake. Por tanto, esos informes autorizan diseño y
mapping estático, pero el graph audit exacto requiere volver a montar los ZIP o
BLEND originales desde la ubicación privada del propietario. El intake registra
hash y produce derivados privados; nunca copia el producto comprado a una ruta
publicable ni al paquete Shipping.

Por ello, la respuesta técnica a “¿nos apoyamos en sus nodos?” tiene tres niveles:

1. **Sí, como diseño comprobable** cuando el source revela el algoritmo o la
   documentación identifica etapas concretas.
2. **Todavía no como graph exacto** cuando la implementación sólo vive dentro de
   un `.blend`; esos archivos requieren el probe 5.2.1 y evaluación visual.
3. **No se copia ciegamente**: incluso un graph excelente se convierte en patrones
   propios con BSP, layers, IDs, seeds, budgets, masks y provenance Jerico. Sólo se
   adapta completo si supera licencia, seguridad, determinismo y calidad.

### 8.2 Protocolo de graph audit y calidad visual

Cuando la ventana Blender esté autorizada, cada `.blend` priorizado sigue el mismo
protocolo, sin instalarlo globalmente:

1. Fijar URL/tag/commit, licencia y SHA-256 del paquete y de cada `.blend`.
2. Copiarlo a intake aislado, abrir con factory startup/offline y nunca registrar
   handlers, startup files ni preferencias de usuario.
3. Enumerar node trees, nested groups, interfaces/panels, nodos por `bl_idname`,
   links, zones, assets, modifiers, materials, images, drivers y scripts.
4. Exportar un grafo canónico JSON/DOT sólo para estudio, preservando IDs locales y
   hash; no convertir sockets automáticamente en GeneratorSchema.
5. Clasificar cada etapa: primary form, secondary form, micro detail, material/
   shader, scatter, lighting/render presentation o postprocess. Así se determina
   qué causa realmente el resultado “impecable”.
6. Ejecutar defaults, semillas múltiples y extremos con budgets; medir tiempo,
   memoria, vertices/triangles/instances y estabilidad de atributos.
7. Capturar vistas comparables y mallas/material masks. Un thumbnail del autor no
   es golden; Jerico conserva sus propias referencias originales.
8. Hacer ablation: desactivar por etapas noise/detail/material/scatter para separar
   forma geométrica de shading y assets empaquetados.
9. Reimplementar el patrón mínimo como group propio y comparar silhouette,
   surface/normal error, masks, UV, topology, rendimiento y Beauty.
10. Emitir `ADAPT`, `STUDY` o `REJECT` con evidencia. Sólo `ADAPT` aprobado entra en
    `plugins/admitted`; estudiar un graph no lo convierte en dependencia.

Prioridad de auditoría visual: (1) MRoadGen, (2) French Houses, (3) un árbol/scatter
representativo, (4) un pack de rocks/cliffs, (5) Surface Mesh para carrocería,
(6) bake/LOD utilities. Esta secuencia cubre primero los patrones más reutilizables
sin afirmar que todos los plugins producen la misma calidad.

## 9. Carpeta física, manifests y controles del futuro front

Convención local propuesta, pendiente de integración B05/B06 por su owner nativo:

```text
Tools/BlenderWorker/
  plugins/
    inbox/                 # sólo intake; nunca ejecutado
    quarantine/            # rechazado + diagnóstico
    admitted/<plugin-id>/<version>/
      jerico-plugin.json
      blender_manifest.toml
      addon/               # código permitido y fijado
      assets/              # .blend/.json/texturas permitidas
      licenses/
      hashes.json
  node_libraries/
    admitted/<library-id>/<version>/*.blend
  schemas/<generator-id>/<schema-version>.json
```

Estas raíces no existen como admisión por aparecer en el documento. El catálogo
Jerico debe escanear manifests/hashes sin importar ni ejecutar Python; sólo el
worker aislado carga un paquete admitido. Duplicados/conflictos bloquean el ID.

Manifest mínimo:

```json
{
  "schemaVersion": 1,
  "pluginId": "org.jerico.surface-mesh-adapter",
  "version": "1.0.0",
  "kind": "blender-addon-adapter",
  "blender": {"exact": "5.2.1", "buildHash": "PINNED_BY_HOST"},
  "entrypoint": "addon/__init__.py",
  "capabilities": ["vehicle.surface", "architecture.curved-surface"],
  "generators": ["jerico.vehicle.body-surface/v1"],
  "permissions": {"network": false, "subprocess": false, "writeOutsideJob": false},
  "licenses": [{"spdx": "GPL-3.0-or-later", "file": "licenses/LICENSE"}],
  "dependencies": [],
  "filesManifest": "hashes.json",
  "admission": "candidate"
}
```

`GeneratorSchema` combina introspección segura y semántica revisada:

```json
{
  "schemaVersion": 1,
  "generatorId": "jerico.road.corridor",
  "generatorVersion": "1.0.0",
  "implementation": {"kind": "geometry-nodes", "groupId": "jerico.road.corridor/v1"},
  "inputs": [
    {"id": "corridor", "type": "jerico-spline-snapshot", "required": true},
    {"id": "width", "socketIdentifier": "Input_Width", "type": "float", "unit": "m",
     "default": 7.0, "minimum": 0.5, "maximum": 100.0,
     "panel": "Road/Profile", "description": "Ancho total del corredor"},
    {"id": "surface", "type": "jerico-surface-snapshot", "required": true}
  ],
  "outputs": ["renderMesh", "lodSet", "collisionCandidate", "materialMasks", "provenance"],
  "constraints": ["bsp-keep-clear", "world-cell-boundaries", "stable-seed"],
  "publishable": false
}
```

El front M10.5 genera Qt desde `id/type/unit/default/min/max/options/panel/
description/dependencies`, no desde cualquier panel Blender. Inputs de Geometry,
Object, Collection, Material o Image se convierten a pickers Jerico por GUID y
tipo; callbacks Python, expressions, pointer libre, operators y contexto UI quedan
`manualAdapterRequired`. El backend y sus schemas deben estar operativos antes.

## 10. Admisión, seguridad, licencias y compatibilidad

Pipeline obligatorio: `INBOX → STATIC_SCAN → LICENSE_REVIEW → DEPENDENCY_LOCK →
BLENDER_5_2_1_PROBE → BEHAVIOR_CONFORMANCE → DETERMINISM → RESOURCE_LIMITS →
CANDIDATE → ADMITTED`. Un paso fallido va a quarantine con diagnóstico; no hay
auto-update.

- Sólo archivos declarados y hashados; path traversal, symlinks externos, DLLs,
  wheels o executables no declarados se rechazan.
- Red, subprocess, shell, ctypes, escritura fuera del job y auto-install están
  denegados salvo una excepción de producto revisada y fijada.
- GPL-compatible es condición para addons distribuidos por Blender Extensions,
  pero Jerico conserva SPDX, textos, copyright, source offer y licencia de assets.
- Cada upgrade es una nueva identidad. Presets/recetas migran mediante funciones
  versionadas y pruebas; nunca por mutar defaults históricos.
- Un graph `.blend` se inspecciona por hash y metadata en un proceso acotado; sus
  sockets no prueban comportamiento ni autorizan un binding.

## 11. Matriz de cierre B0 y próximos gates

| Criterio B0 | Resultado |
| --- | --- |
| FPS/TPS, autos, isométrico y géneros generalistas | Cubiertos con capacidades, nodos, outputs y fronteras. |
| Escenarios, arquitectura, terreno/cuevas, caminos/rails | Cubiertos con grupos propios e híbridos. |
| Vegetación/scatter, props/items, vehículos | Cubiertos con seeds, provenance y límites runtime. |
| Destrucción, Decal3D y spline destructivo | Reuso contractual definido, no duplicación. |
| Selección loop/path/region/placement | Operaciones y fallbacks CPU definidos. |
| Layer System/material masks | Fases, operadores, locks y atributos mínimos definidos. |
| UV/bake/LOD/collision/export | Políticas y métricas requeridas definidas. |
| Addons gratuitos | Lista priorizada con BUILD/ADAPT/STUDY/REJECT y riesgos. |
| Folder/manifest/schema/front | Convención y ejemplos definidos; integración B05/B06 pendiente. |

B0 termina en esta matriz, no en código generado. La siguiente producción es
B02-B04: reconciliar broker/jobs/recovery existentes, conservar el primer RED de
comportamiento realmente ausente y proponer la mínima interfaz worker/host. Luego
B05/B06 convierten este atlas en catálogo y GeneratorSchema reales; después se
implementan familias propias por tracer vertical, con golden, determinismo,
budgets, semántica, Beauty y consumidores aplicables.

## 12. Fuentes primarias

### Blender

- Manual Blender 5.2 LTS: <https://docs.blender.org/manual/en/5.2/index.html>
- Introducción Geometry Nodes y tipos de geometría: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/introduction.html>
- Índice de nodos 5.2: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/index.html>
- Release notes Geometry Nodes 5.2: <https://developer.blender.org/docs/release_notes/5.2/geometry_nodes/>
- Release notes Python API 5.2: <https://developer.blender.org/docs/release_notes/5.2/python_api/>
- Compatibilidad de Blender: <https://developer.blender.org/docs/release_notes/compatibility/>
- Node-based tools y sus límites: <https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/tools.html>
- Simulation Zone: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/simulation/simulation_zone.html>
- Física experimental 5.2: <https://developer.blender.org/docs/release_notes/5.2/physics/>
- Curves: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/curve/index.html>
- Curve to Tube: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/generate/curve_to_tube.html>
- Mesh Bevel: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/mesh/operations/mesh_bevel.html>
- Shortest Edge Paths: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/mesh/read/shortest_edge_paths.html>
- Edge Paths to Curves: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/mesh/operations/edge_paths_to_curves.html>
- UV Unwrap node: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/mesh/uv/uv_unwrap.html>
- Performance: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/performance.html>
- Distribute Points on Faces: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/point/distribute_points_on_faces.html>
- Distribute Points in Volume: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/point/distribute_points_in_volume.html>
- Collection Info y orden: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/input/scene/collection_info.html>
- Instances: <https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/instances.html>
- Interfaz Python de sockets: <https://docs.blender.org/api/main/bpy.types.NodeTreeInterfaceSocket.html>
- Socket float, default/min/max/subtype: <https://docs.blender.org/api/main/bpy.types.NodeTreeInterfaceSocketFloat.html>
- Cycles baking: <https://docs.blender.org/manual/en/5.2/render/cycles/baking.html>
- Decimate modifier: <https://docs.blender.org/manual/en/5.2/modeling/modifiers/generate/decimate.html>
- Assets 5.2 y bibliotecas online: <https://developer.blender.org/docs/release_notes/5.2/assets/>
- Términos de Blender Extensions: <https://extensions.blender.org/terms-of-service/>

### Proyectos gratuitos

- BagaPie: <https://extensions.blender.org/add-ons/bagapie/>
- WFC 3D Generator: <https://extensions.blender.org/add-ons/wfc-3d-generator/>
- Archimesh: <https://extensions.blender.org/add-ons/archimesh/>
- Arch Generator: <https://extensions.blender.org/add-ons/arch-generator/>
- French Houses GN: <https://github.com/IRCSS/Blender-Geometry-Node-French-Houses>
- Bene ProgGen Maps: <https://github.com/Beneking102/bene-proggen-maps>
- City Generator: <https://github.com/AxiDreamdragon/city-generator>
- MRoadGen: <https://github.com/MaartenNauta-Pathtrace/MRoadGen>
- Driving Scenario Creator: <https://github.com/johschmitz/blender-driving-scenario-creator>
- Maze Generator Pro: <https://extensions.blender.org/add-ons/maze-generator-pro/>
- Easy Tree: <https://extensions.blender.org/add-ons/easy-tree/>
- Modular Tree: <https://extensions.blender.org/add-ons/modular-tree/>
- Generate Tree Plugin: <https://extensions.blender.org/add-ons/generate-tree-plugin/>
- Hardflow: <https://extensions.blender.org/add-ons/hardflow/>
- Surface Mesh: <https://extensions.blender.org/add-ons/surface-mesh/>
- Tissue: <https://extensions.blender.org/add-ons/tissue/>
- Sverchok: <https://github.com/nortikin/sverchok>
- Infinigen: <https://github.com/princeton-vl/infinigen>
- Infinigen dependencies: <https://github.com/princeton-vl/infinigen/blob/main/pyproject.toml>
- Infinigen 2: <https://github.com/princeton-vl/infinigen/blob/main/docs/source/Infinigen2.md>
- BlenderProc: <https://github.com/DLR-RM/BlenderProc>
- A.N.T. Landscape: <https://extensions.blender.org/add-ons/antlandscape/>
- Terrain Mixer: <https://extensions.blender.org/add-ons/terrainmixer/>
- Cell Fracture: <https://extensions.blender.org/add-ons/cell-fracture/>
- Remi: <https://github.com/shaderko/remi-blender-addon>
- EasyMesh Batch Exporter: <https://extensions.blender.org/add-ons/easymesh-batch-exporter/>
- Attribute Baker: <https://extensions.blender.org/add-ons/attribute-baker/>
- Sequenced Bake: <https://extensions.blender.org/add-ons/sequenced-bake/>
- Bake To Single Material: <https://extensions.blender.org/add-ons/bake-to-single-material/>
- The Better Baker: <https://extensions.blender.org/add-ons/thebetterbaker/>
- LODGen: <https://extensions.blender.org/add-ons/lod-gen/>
- Blender Game Tools: <https://extensions.blender.org/add-ons/blender-game-tools/>
- GamiFlow: <https://extensions.blender.org/add-ons/gamiflow/>
- ACT Game Asset Creation Toolset: <https://extensions.blender.org/add-ons/act-game-asset-creation-toolset/>
- Texture Mixer: <https://extensions.blender.org/add-ons/texture-mixer/>
- Blender Rock Generator: <https://github.com/lupin4/Blender-Rock-Generator>
- DragonGraph Toolset: <https://github.com/Breathfang/BreathfangGeoNodes>
- Rock Generator Add-on: <https://github.com/tu2463/rock_generator_addon>
- TreeGen LLM: <https://github.com/YuutoSeki/treegen-llm>
- PROTO Game Asset Tools: <https://extensions.blender.org/add-ons/proto-game-asset-tools/>
- CrashCar101 paper: <https://openaccess.thecvf.com/content/WACV2024/papers/Parslov_CrashCar101_Procedural_Generation_for_Damage_Assessment_WACV_2024_paper.pdf>

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
