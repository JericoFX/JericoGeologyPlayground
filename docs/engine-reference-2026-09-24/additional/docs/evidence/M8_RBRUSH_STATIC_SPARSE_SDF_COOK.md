# M8 RBrush Slice 35 — Static Sparse SDF Cook

**Fecha:** 2026-08-14

**Estado:** `PASS`

**Producto:** `952793f feat(rbrush): publish static sparse sdf outputs`

**Worker C04:** `2c92609 feat(rbrush): run static sdf cook off ui thread`

**Proof:** `1207101 test(rbrush): prove static sdf package changed`

**Goal maestro:** `ACTIVE / NOT COMPLETE`

## Resultado demostrado

Un resultado `SdfAuthoringResult` aceptado deja de depender de RBrush en el
runtime mediante una sola ruta canónica:

```text
accepted Sparse SDF result + admitted World/catalog/bridge snapshot
-> existing SdfCook::cookSdfCandidate
-> stable GUID .rmesh + .rcoll replacement overlays
-> WorldRenderDependencySnapshot before/after
-> atomic transaction: .rmesh Replace + .rcoll Replace + World descriptor Replace
-> Package Changed delta .epak
-> base + delta Shipping replay
```

No se añadió un segundo cooker ni un formato SDF runtime. `.rbrush`, ABR,
strokes, tips, presets, máscaras, receta SDF y campos Sparse SDF permanecen en
autoría. Shipping consume exclusivamente `.rmesh`, `.rcoll`, `.rmat`, `.rlevel`
y `.epak`.

La razón exacta demostrada es:

```text
Dirty Sparse SDF region intersects accepted geometry change
```

El dirty region coincide exactamente con los bounds del replacement field y
declara Geometry Pages, Collision, Navigation, Shadow Pages, GI Bricks y World
Cell Dependencies. Un cambio aceptado produce:

```text
overlays = 1 .rmesh + 1 .rcoll
operations = .rmesh Replace + .rcoll Replace + WorldDescriptor Replace
operations .rlevel = 0
operations .rnav = 0
```

Si el World admite `PwasRecipe` o `Navigation`, el plan conserva sus productos
anteriores, cambia el World a `BakeRequired` y marca ambos facets downstream
como stale. No oculta un nav bake ni un recook PWAS dentro del commit SDF.

## RED / GREEN

- RED auténtico: el contrato focal no compiló porque
  `Jerico/Editor/SparseSdfStaticCookPlan.hpp` no existía.
- GREEN mínimo: `952793f` añadió el plan productivo, overlays de identidad
  estable, closure before/after, catálogo candidato y transacción reversible.
- GREEN C04: `2c92609` movió el cook pesado a un worker Qt cancelable. El hilo
  dueño sólo recibe el plan terminado y aplica el commit breve.
- GREEN Package Changed: `1207101` ejecuta el plan y la transacción reales,
  publica exactamente Mesh + Collision en el delta y compara base+delta contra
  compact y clean.
- Guard downstream: `76fd506` prueba `BakeRequired`, stale transitivo y cero
  `.rnav`/`.rlevel` ocultos.

## Responsividad y cancelación

El test focal observado produjo:

```text
Qt heartbeats = 27
maximum Qt event gap = 5 ms
cookRunsOnOwnerThread = false
late callback committed = false
```

La cancelación cooperativa devuelve
`editor.sparse_sdf.static_cook.cancelled` y no modifica `.rmesh`, `.rcoll` ni el
World descriptor. Destruir el owner descarta el callback tardío y tampoco
publica bytes.

## Delta, VFS, Jolt y Shipping

El fixture parte del Sparse SDF house representativo existente, modifica de
forma válida el falloff del stamp aceptado y vuelve a generar la misma
identidad runtime con contenido nuevo:

```text
mesh GUID = CBB7EAE71AB1FA3992D279CAB9BBEFD4
collision GUID = 8A8050A82D252D93451D4AEF7F434436
vertices = 118584
indices = 118584
triangles = 39528
delta entries = 2
delta levels = 0
compact inventory == clean inventory
```

El VFS monta `base.epak` con prioridad 100 y `delta.epak` con prioridad 200.
AssetRegistry resuelve el Level y material desde base, y la nueva generación
de Mesh/Collision desde el overlay. Jolt crea un body estático con las 118.584
posiciones/índices esperados.

Después se borran por completo `Project`, `PlanProject`, todos los staging
`base/delta/clean-cooked`, el material template y toda receta/autoría. Con sólo
los paquetes restantes se ejecutan:

1. dos replays Vulkan independientes desde `base.epak + delta.epak`;
2. un replay Vulkan desde `compact.epak`;
3. comparación byte-exacta de las tres capturas; y
4. validación Shipping final de cada paquete.

SHA-256 Release:

```text
base.epak = 319323A8E19D7235E34C8DB638836647B8D55ED363ECB96D6EA05B74AC3D7FDD
delta.epak = 703BB120CE1ADBDDB8BE04AE36446FDEE06812DF3B257C84359EE1FA6A044883
compact.epak = 46D0AB169ADF625C8724223BA7F7DCD42B908C48A06CBC0C2525267211D3B2D8
capture = 29553BDBA74C31CB068415529F0DC7DCB1BC8A72CE209E95AD374F177A11EBD1
```

La evidencia visual retenida está en
[`images/m8-rbrush-static-sdf-shipping.bmp`](images/m8-rbrush-static-sdf-shipping.bmp).

## Pruebas ejecutadas

```text
Debug focal/regresión: 4/4 PASS, 18.67 s
Debug focal con downstream stale: PASS, 4.35 s
Debug scheduling contract: 1/1 PASS
Debug JericoSparseSdfStaticCookShippingProof: PASS, 90.7 s
Release JericoSparseSdfStaticCookShippingProof: PASS, 66.3 s
Release Vulkan proof_frame_ms: 2.821
Debug Vulkan proof_frame_ms: 3.382
git diff --check: PASS; sólo avisos CRLF del checkout
```

## Límite honesto

Esta captura demuestra que el resultado estático cocinado se renderiza y
colisiona sin RBrush/SDF authoring. No es una captura beauty ni cierra los gates
visuales globales de BSP/SDF M8: el material usa pipeline Vulkan real, pero sus
cuatro texturas caen en fallback. Los cliffs, caves, múltiples rocas,
iluminación/PBR completa y métricas de showcase permanecen gobernados por
`docs/JERICO_M8_BSP_SDF_VISUAL_SHOWCASE_GATE.md`.

El delta contiene sólo dos assets, pero Mesh + Collision representan casi todo
el peso del ejemplo (8.064.934 bytes frente a 8.157.878 del base). Slice 35
demuestra invalidación mínima por asset, no geometry-page delta binario dentro
de `.rmesh`/`.rcoll`.

## Siguiente slice legal

RBrush Slice 36 — Runtime Dynamic Derivative. Debe introducir el recurso brush
runtime cocinado mínimo para gameplay dinámico, demostrar pintura en un sample
runtime y mantener el `.rbrush` del editor ausente de Shipping. M8 y el Goal
maestro continúan activos.
