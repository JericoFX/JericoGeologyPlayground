# M8 RBrush Slice 10 — ABR Semantic Mapper

Estado: `PASS` para RBrush Slice 10. M8 y el Goal maestro JericoEngine 1.0
permanecen `ACTIVE / PARTIAL`.

## Alcance demostrado

El mapper de autoría `mapAbrSemantics` recorre el árbol tipado producido por el
parser genérico de Slice 9 y genera semántica RBrush sin resolver todavía los
IDs `sampledData` contra samples físicos. Esa resolución pertenece a Slice 11.

Se demostraron:

- Brush Tip Shape: diámetro, dureza, ángulo, roundness, spacing, flips,
  interpolation, antialias y `sampledData`.
- Shape Dynamics: size/angle/roundness, mínimos, jitter, controles numéricos y
  enumerados y curvas de respuesta normalizadas.
- Scatter y count; Transfer para opacity, flow, wetness y mix.
- Dual Brush conservando el ID secundario sin convertirlo en GUID.
- Texture, Color Dynamics, Brush Pose y flags misc.
- aliases `charID`/`stringID`, tipos/unidades exactos, duplicados, valores no
  finitos y límites.
- cancelación cooperativa y recorrido determinista.
- `UNKNOWN_KEY`, `UNKNOWN_TYPE` y `PARSED_UNSUPPORTED` con path, offsets y raw
  encoding retenido para las siguientes fronteras de importación.

Los valores admitidos persisten en RBrush schema 3 / container 1.2. Los readers
siguen admitiendo schema 1/2 y container 1.0/1.1, mientras writers antiguos
rechazan semántica extendida en vez de descartarla silenciosamente.

## RED / GREEN

RED inicial:

```text
fatal error C1083: No se puede abrir el archivo incluir:
'Jerico/AssetPipeline/AbrSemanticMapper.hpp'
```

RED de matriz:

```text
27 checks fallaron con defaults en shape/scatter/transfer.
dualBrush, texture, colorDynamics y pose estaban ausentes.
Crv y UNKNOWN_KEY no estaban mapeados.
aliases diameter/hardness/angle/roundness/spacing y wtVr/mxVr/Mnm fallaron.
```

GREEN final:

```text
asset_pipeline.abr_semantic_mapper PASS
asset_pipeline.rbrush_ir PASS
asset_pipeline.rbrush_container PASS
asset_pipeline.abr_descriptor_parser PASS
```

## Fixture ABR real retenida

```text
F:\JericoEngine\build\local-fixtures\abr\Free Cracks Photoshop Brushes 10.abr
bytes: 23,435,296
SHA-256: 0D8875E5E19228A828F3A259348B8B94DB4584849F33BFC66DD8287FB65FEE9D
document_version: 16
presets_mapped: 15
```

El test local condicionado por existencia leyó realmente el archivo y comprobó
en orden fuente:

- nombres `121`..`135`;
- los 15 UUID `sampledData` literales registrados por Slice 9;
- diámetro `2500`, roundness `1`, spacing `0.25`, ángulo `0`;
- interpolation habilitado y flips X/Y deshabilitados.

El fixture sigue fuera de Git y no fue modificado ni staged.

## Gates

```text
Debug focal parser/mapper/IR/container: 4/4 PASS (0.38–0.40 s)
Debug AssetPipeline completo: 10/10 PASS (1.29 s)
Debug repository/Shipping + mapper: 6/6 PASS (73.38 s)
Fresh final Debug AssetPipeline + repository/Shipping: 15/15 PASS (71.09 s)
Release mapper/IR/container: 3/3 PASS (0.32 s)
repository.test_scheduling_contract: PASS; 250 tests, 51 scheduled
windows-vulkan-runtime configure/runtime-only discovery: PASS
windows-vulkan-runtime Release build: PASS (38.5 s)
runtime-only project hits: 0
runtime-only binary hits: 0
git diff --check sobre producto: PASS
```

La configuración runtime-only mantiene `JERICO_ENABLE_ASSET_IMPORT=OFF`; no hay
target, fuente, proyecto, símbolo ni artefacto de `AbrSemanticMapper`,
`AbrDescriptorParser`, `AbrInspector`, `abr_inspect` o `8BIMdesc` en Shipping.

## Commits de producto

```text
c98bf76 docs(rbrush): plan semantic mapper
7dd3731 feat(asset-pipeline): persist complete rbrush semantics
686bdfd feat(asset-pipeline): map abr tip semantics
190f975 feat(asset-pipeline): map complete abr brush semantics
dee6248 test(asset-pipeline): prove real abr semantics
```

## Límites y continuidad

- No se agregó UI, publicación, cook completo, ejecución runtime ni resolución
  de `sampledData`.
- No se agregó ninguna dependencia.
- C04 Jobs/Async continúa `PARTIAL` para los cookers/bakes pesados restantes.
- Virtual Texturing/páginas MegaTexture continúa diferido a JericoEngine 1.1.
- La autoridad de Animation/Rigging in-editor se conserva para su milestone y
  no fue adelantada.
- Próxima acción única legal: RBrush Slice 11 — ABR ID Resolution.

Estado: `PASS` (Slice 10 solamente).

Resultado: Semántica ABR completa admitida, preservada y verificada con fixture
real; Shipping permanece source-free.

Pruebas ejecutadas: focal Debug/Release, AssetPipeline completo, scheduling,
admission, Shipping exclusion, runtime-only discovery y build Release.

Evidencia: este documento y los commits listados.

Faltantes: Slice 11 y slices posteriores de M8; M8/Goal 1.0 no están completos.

Riesgos: ABR es propietario y extensible; las claves no admitidas deben seguir
preservándose como raw y nunca descartarse silenciosamente.

Siguiente slice: RBrush Slice 11 — ABR ID Resolution.
