# M8 RBrush Slice 11 — ABR ID Resolution

Estado: `PASS` para RBrush Slice 11. M8 y el Goal maestro JericoEngine 1.0
permanecen `ACTIVE / PARTIAL`.

## Alcance demostrado

El reader v6.2 extrae ahora la identidad Pascal interna de cada record físico
`samp` dentro del prefijo ya acotado de 301 bytes. `AbrDecodedSample` conserva
esa identidad junto a dimensiones, polaridad, hash y pixels canónicos.

`resolveAbrPresetSamples` recibe la semántica de Slice 10 y los samples de Slice
8. Construye un índice exacto por ID y devuelve referencias descriptor-ordered
a índices físicos, sin copiar las máscaras. El contrato:

- nunca empareja `desc[i]` con `samp[i]`;
- admite varios presets que compartan un mismo tip;
- rechaza IDs físicos vacíos o duplicados y referencias sin sample;
- resuelve el tip dual cuando existe un ID;
- conserva un Dual Brush deshabilitado sin ID como sección semántica sin tip;
- rechaza un Dual Brush habilitado sin ID;
- informa samples físicos no referenciados en orden estable;
- observa cancelación cooperativa durante indexado y resolución.

No se agregó UI, publicación, consumo de fuente, update/reimport ni ejecución
runtime. No se agregó ninguna dependencia.

## RED / GREEN

RED de inventario:

```text
repository.test_scheduling.full_inventory_changed
expected_count=250
actual_count=251
actual_sha256=a3396a24e2f526fc9945de1b28290b2bdd64d0df5150326be74a4b1cfeece03c
```

RED de API:

```text
fatal error C1083: No se puede abrir el archivo incluir:
'Jerico/AssetPipeline/AbrIdResolver.hpp'
```

La primera ejecución con el ABR real produjo un RED adicional:

```text
asset.rbrush.abr_sample_id_invalid
Descriptor preset 0 dual-brush sampledData ID is empty.
```

La investigación aisló que el fixture conserva una sección Dual Brush
deshabilitada sin tip. Un test focal reprodujo el caso. El GREEN diferencia
sección deshabilitada vacía de Dual Brush habilitado sin tip, sin relajar la
validación de identidad primaria.

GREEN final:

```text
asset_pipeline.abr_sample_decoder PASS
asset_pipeline.abr_id_resolver PASS
```

## Fixture ABR real retenida

```text
F:\JericoEngine\build\local-fixtures\abr\Free Cracks Photoshop Brushes 10.abr
bytes: 23,435,296
SHA-256: 0D8875E5E19228A828F3A259348B8B94DB4584849F33BFC66DD8287FB65FEE9D
descriptor presets: 15
physical samples: 15
unreferenced samples: 0
```

El test Release y Debug exige las 15 identidades literales y los 15 hashes
canónicos de Slice 8. La relación física demostrada es:

```text
121 -> samp[14]
122 -> samp[13]
123 -> samp[12]
124 -> samp[11]
125 -> samp[10]
126 -> samp[9]
127 -> samp[8]
128 -> samp[7]
129 -> samp[6]
130 -> samp[5]
131 -> samp[4]
132 -> samp[3]
133 -> samp[2]
134 -> samp[1]
135 -> samp[0]
```

Esta inversión completa prueba observablemente que la resolución usa UUID y no
posición. El ABR continúa ignorado, inmutable y fuera de Git.

## Gates finales

```text
Debug focal decoder/resolver: 2/2 PASS (7.18 s)
Debug ABR/RBrush + scheduling: 9/9 PASS (7.60 s)
Debug AssetPipeline completo: 11/11 PASS (7.91 s)
Repository admission/scheduling/Shipping: 4/4 PASS (43.00 s)
Release focal build, 6 targets: PASS (93.9 s, incluye reconfigure)
Release focal CTest: 6/6 PASS (1.24 s)
windows-vulkan-runtime configure: PASS (28.8 s)
windows-vulkan-runtime Release completo: PASS (38.2 s)
runtime-only resolver/ABR/AssetPipeline hits: 0
editor.boundary_audit: 1/1 PASS (267.59 s)
```

El runtime-only se buscó por `AbrIdResolver`, `resolveAbrPresetSamples`,
`abr_id_resolution`, `JericoAssetPipeline`, `AbrSampleDecoder`,
`AbrSemanticMapper` y `AbrDescriptorParser`; no hubo target, proyecto, fuente,
símbolo ni artefacto.

## Commits

```text
85d866e docs(rbrush): plan abr id resolution
a4b09c9 feat(asset-pipeline): resolve abr samples by id
```

## Límites y continuidad

- C04 Jobs/Async continúa `PARTIAL` para los cookers/bakes pesados restantes.
- Virtual Texturing/páginas MegaTexture continúa diferido a JericoEngine 1.1.
- Animation/Rigging, Terrain/EPLS 2.0, JericoParticles y Vegetation no se
  adelantaron.
- Slice 12 conserva ownership de importación transaccional y consumo de fuente.

Estado: `PASS` (RBrush Slice 11 solamente).

Resultado: los presets ABR se relacionan con tips físicos exclusivamente por ID
interno, probado contra orden físico inverso real.

Pruebas ejecutadas: focales Debug/Release, AssetPipeline, contratos repository,
Shipping source-free, runtime-only Release y boundary audit.

Evidencia: este documento, los tests versionados y los commits listados.

Faltantes: Slice 12 y posteriores; M8/Goal 1.0 no están completos.

Riesgos: formatos ABR futuros pueden contener identidades distintas; deben
preservarse bounded y nunca degradarse a matching posicional.

Siguiente slice: RBrush Slice 12 — Transactional Import + Consume.
