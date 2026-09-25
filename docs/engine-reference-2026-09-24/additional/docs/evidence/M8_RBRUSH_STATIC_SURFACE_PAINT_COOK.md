# M8 RBrush Slice 34 — Static Surface Paint Cook

**Fecha:** 2026-08-14
**Estado:** `PASS`
**Producto:** `db57908 feat(rbrush): publish static surface paint changes`
**Goal maestro:** `ACTIVE / NOT COMPLETE`

## Resultado demostrado

`SurfacePaintStaticCookPlan` conecta el cooker page-aware existente con las
autoridades productivas de World, catálogo, Automation Bridge y transacciones.
Un `Cook Changed` aceptado:

```text
accepted World + SurfacePaint + catalog + bridge snapshot
-> SurfacePaintCooker::cookStatic
-> stable .rtext replacement overlays
-> WorldRenderDependencySnapshot before/after
-> atomic transaction: N .rtext Replace + World descriptor Replace
-> Package Changed / delta .epak
-> base + delta Shipping replay
```

El plan exige World `Current`, identidades y revisiones admitidas, generación
vigente, paths canónicos, GUID estable y contenido nuevo no-cero. Revalida el
catálogo y las closures antes de aplicar. Planning no muta archivos; Apply,
Undo y Redo son transacciones byte-exactas.

La razón exacta demostrada continúa siendo:

```text
Dirty Surface Paint page intersects authored change
```

Un dirty set de una página produce exactamente:

```text
overlays=.rtext: 1
operations=.rtext Replace + WorldDescriptor Replace: 2
operations=.rlevel: 0
```

El `.rlevel` permanece byte-idéntico. RBrush, strokes, receta, ABR, presets y
tips son autoría exclusiva del editor; Shipping consume la textura `.rtext`
cocinada y el World actualizado, nunca un asset RBrush.

## RED / GREEN

- RED backend page-aware: `a187c57`; faltaban request y `cookStatic`.
- GREEN backend: `77e2542`.
- RED World runtime: el proof rechazó `--level-guid` y luego se añadió el guard
  durable de World baked surface.
- GREEN World/Shipping inicial: `9a5c4c9`.
- RED plan productivo: compilación falló por ausencia de
  `SurfacePaintStaticCookPlan`; después quedaron expuestos los seams faltantes
  de catálogo y transacción.
- RED Package Changed: el proof rechazó un segundo `--package` con
  `render.proof.invalid_arguments: Duplicate option: --package`.
- GREEN final: `db57908` conecta plan, transacción, delta y replay base+overlay.

## Incremental cook y Package Changed

El fixture final no inyecta una textura pintada manualmente. Construye un
proyecto admitido, ejecuta `planSurfacePaintStaticCook`, aplica la transacción y
usa esos bytes exactos para publicar el delta. El resultado retenido es:

```text
delta entries = 1 Texture
delta levels = 0
plan overlays = 1
plan rlevels = 0
plan content hash = 0x9B6833E76C00BE91
plan cook key = 0xC965B2F8275704A2
compact inventory == clean inventory
```

El VFS monta `base.epak` con prioridad 100 y `delta.epak` con prioridad 200.
AssetRegistry carga el Level desde base y la generación nueva de Texture desde
el overlay. PackageTool compacta base+delta y su inventario es equivalente a un
clean package.

## Replay World source-free

Tras borrar `SourceAssets`, `Assets`, `Cache`, `Cooked`, los staging dirs, el
proyecto del plan, `Procedural` y `EditorAssets`, el proof renderiza:

1. dos replays Vulkan independientes desde `base.epak + delta.epak`;
2. un replay Vulkan desde `compact.epak`; y
3. exige igualdad entre los tres resultados pintados y diferencia contra el
   baseline.

SHA-256 final:

```text
baseline capture = D9C648405CEDB9769AB43DB7ED07D16694811A1A3AAB4A7862BA70F8B34FCE69
painted base+delta A = 58504B6B70AC59C9D8A9287E83C7B6EBBC698E8E7E7EDF8C9C1FFFC883BA8088
painted base+delta B = 58504B6B70AC59C9D8A9287E83C7B6EBBC698E8E7E7EDF8C9C1FFFC883BA8088
painted compact = 58504B6B70AC59C9D8A9287E83C7B6EBBC698E8E7E7EDF8C9C1FFFC883BA8088
compact package = 0866F0A3B12361832BC978BF0BC11D2E6653876043F522FB2B3BFAB6CDD20C85
```

El paquete se inspecciona como bytes y rechaza nombres de autoría y GUIDs de
brush, preset, tip y material source.

## Pruebas ejecutadas

```text
Debug focal/regresión: 9/9 PASS, 18.82 s
Release focal/regresión: 9/9 PASS, 13.70 s
Debug scheduling contract: 1/1 PASS
Release scheduling contract: 1/1 PASS
Debug focal build: PASS
Release focal build: PASS
Debug JericoSurfacePaintShippingProof: PASS, 327.9 s
Release JericoSurfacePaintShippingProof: PASS, 80.6 s
git diff --check: PASS; sólo avisos CRLF del checkout
```

Los nueve tests cubren closure, fanout, plan estático, coordinador,
transacción, recipe, evaluator, cooker y overlay PackageTool. Un revisor Terra
READ_ONLY confirmó que el proof final ya no separa el plan del replay: los
bytes aplicados por el plan entran al delta, se montan base+delta dos veces y se
comparan contra compact.

## Rendimiento y límite honesto

El flujo diario demostrado cocina sólo la página `.rtext` dirty y no vuelve a
cocinar ni empaquetar el `.rlevel`. El proof completo sigue siendo pesado porque
prepara assets representativos de BoomBox/Poly Haven y comprime texturas de
prueba; no representa la latencia interactiva del plan incremental.

Durante una matriz genérica, `render.cooked_asset_proof` lanzó
`JericoGltfCook --profile full-speed --workers 1` y superó cuatro minutos. Ese
fixture redundante se detuvo y no se usó como evidencia de Slice 34. Confirma
que C04 Jobs/Async y la optimización de cookers generales permanecen `PARTIAL`;
Slice 34 no declara que todos los cooks del engine sean instantáneos.

## Siguiente slice legal

RBrush Slice 35 — Static Sparse SDF Cook. Debe reutilizar las mismas fronteras
de dirty regions, transacción, delta package y Shipping source-free sin
reintroducir RBrush en runtime. M8 y el Goal maestro continúan activos.
