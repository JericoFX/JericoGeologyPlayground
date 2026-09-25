# M8 RBrush ABR Sample Decode Evidence

## Estado

- Fecha: `2026-08-11`
- Milestone: `M8 Terrain and Surfaces`, `ACTIVE / PARTIAL`
- Slice: `RBrush 8 — ABR Sample Decode`
- Estado del slice: `PASS`
- Producto: `e3e6e16 feat(asset-pipeline): decode abr sampled masks`
- Diseño: `1345155 docs(rbrush): design abr sample decode`
- Plan: `534cb58 docs(rbrush): plan abr sample decode`
- Autoridad: `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`

Slice 8 decodifica los sampled tips ABR v6.2 Raw R8/R16 y PackBits R8 a
máscaras canónicas owned, conserva la polaridad `ZeroIsNoCoverage` y calcula el
mismo hash canónico usado por el importador de imágenes. No publica assets, no
interpreta propiedades de descriptor y no relaciona `desc[i]` con `samp[i]`.

## RED auténticos

1. Al registrar `asset_pipeline.abr_sample_decoder`, el contrato de inventario
   falló con esperado `244` frente a actual `245`. El nuevo SHA-256 de inventario
   fue `2f49115f093b9f7dd706cf8c61b679ffcbc2d16c345b124e42824d5231a8d48b`.
   El set programado permaneció exactamente en `46`, SHA-256
   `26fdb7a9fafc0473f8f29d04035ddd706084d7a11809215c498fd6261652dc9c`.
2. El primer build del test de producto falló sólo con:

   ```text
   fatal error C1083: cannot open include file
   'Jerico/AssetPipeline/AbrSampleDecoder.hpp': No such file or directory
   ```

3. Tras el GREEN Raw, el fixture PackBits `8x3` falló en
   `decoded.hasValue()` porque el producto todavía retornaba
   `asset.rbrush.abr_packbits_unsupported`.
4. La matriz de seis filas hostiles falló seis veces al exigir contexto físico
   `sample 0, row 0`; el error todavía no identificaba muestra y fila. El GREEN
   incorporó ese contexto sin relajar consumo ni ancho exactos.

## Contrato implementado

- `decodeAbrSamples(span, stop_token)` devuelve únicamente máscaras owned y un
  resultado all-or-nothing.
- Un parser privado compartido conserva una sola implementación de framing
  v6.2 para Inspector y Decoder: secciones `8BIM`, prefix sample de 301 bytes,
  bounds firmados, tabla de filas, trailer de ocho ceros y padding exacto.
- Raw R8/R16 se copia sin normalizar, recortar, reescalar, invertir o cambiar
  endian canónico.
- PackBits consume cada span de fila declarado de forma aislada:
  `0..127` literal, `129..255` repetición y `128` no-op. Cada fila debe producir
  exactamente su ancho.
- El hash compartido cubre
  `u8 format | u32le width | u32le height | pixels`; los tests existentes del
  importador de imágenes prueban compatibilidad.
- Límites: fuente/sección `512 MiB`, `4096` samples, dimensión `1..16384` y
  `512 MiB` agregados de máscaras decodificadas antes de asignar resultados.
- Cancelación antes del parse, durante tablas/filas y periódicamente dentro de
  streams largos.
- `abr_decode` es un adaptador read-only, Tool-only y disponible únicamente con
  `JERICO_ENABLE_ASSET_IMPORT`.
- No se agregó ninguna dependencia.

## Matriz sintética y hostil

- Raw R8 `3x2`, pixels `007FFF102030`, hash
  `88EA87128A7C298A32954E35F1D017A9528B0C2AFC4399C4B0C1CE7CACFC445C`.
- Raw R16 `2x2`, pixels `3412CDAB0000FFFF`, hash
  `1F95322AADC685E547F32347787A6BC1DF93C49F07EF6BE14547718ECCAA1FC7`.
- PackBits R8 `8x3`, literal/repeat/no-op, hash
  `8BAF25B326A58239114995BD682F069470A8A906E0CA05269D28185FA7A46983`.
- Rechazos `asset.rbrush.abr_packbits_invalid`: literal truncado, repeat sin
  valor, output 7/8, output 9/8, fila sólo no-op y underflow de primera fila que
  no puede tomar bytes de la segunda.
- Stop token pre-cancelado: `asset.rbrush.abr_cancelled`.
- Tres records comprimidos `16384x16384` exceden `512 MiB` canónicos en
  preflight: `asset.rbrush.abr_resource_limit`, sin decodificación parcial.

## Fixture real y hashes

Fixture local, inmutable y fuera de Git:

```text
F:\JericoEngine\build\local-fixtures\abr\Free Cracks Photoshop Brushes 10.abr
bytes: 23,435,296
SHA-256: 0D8875E5E19228A828F3A259348B8B94DB4584849F33BFC66DD8287FB65FEE9D
```

`abr_decode` produjo `15` máscaras, todas `2500x2500 R8 PackBits`,
`ZeroIsNoCoverage`, `6,250,000` bytes cada una y `93,750,000` bytes totales:

```text
00 699604C92034EA251762A6EE6E70E82197BD8A81ABDE0A9D5AD87BDDF06E856C
01 B5060C34195CA0A3A5AF30CE143A41543F0D8597B599C2D3727B7875001523B9
02 EC768EB3D5402A3AF98F4CA3898B24AE66468F8D95E28D723DF3F98D9DCFADE5
03 894E332FAB5FDF83074BF14FFDEBEB6029FA997E1194F4AADE54438564ECEDAC
04 E298D73E3BB70FB66CA786DCAC409E0F526E5E2CD98019853EA18BBBD6C53080
05 1EE670DA68E8B09A35A586334B292CBE7F39A28FD28F7E4FE7CE1835997DE111
06 F7A8F58B5B22FE4FEA3E77AE77EFC0821429BF3EE9343E0E79B971B04517787A
07 9A24253A884F00584E140DAB6C702C440791AD30F597CC15EC53EB0A88651A15
08 BB8A31C7B4BBE63A52A2AF7E7F5EDAF64EAE4F382181F581F1D546E1A5EB818F
09 5202F52EFAA3CF1CB83B1F2079171380594AFA2AA35CC9C2D4EE27FE6D08F029
10 A871336E3791127372ED19F753004DCE89C3C32D3B8C06C9EAB2DF8586A3BD62
11 04EFDDC5597BDDCB87FC176628649353623BEAA4B214D0657B2C1885FAD598E1
12 1B8AB98337AC340672E3EC84AA2B973C2857EBF81354E7E35D07EDC3DE696837
13 7DD196D4EFEE634ED5F25A1D51DA5B30E087EE3D33DA4EF548137B5DACA4C4FE
14 439CE5467C3374A85E655258E3BA124D0CADABC7D449538191C9F11C52A598BB
```

Los 15 hashes coinciden byte a byte con el oráculo Python independiente escrito
antes del producto. SHA-256 de los 15 digests binarios concatenados:

```text
547BF3EEFEBBF57039276F9795E7FB665D4394D538EAF145BECC027E7DC53B27
```

El índice anterior sólo expresa orden físico para el reporte. No es identidad
de preset ni autoriza emparejar descriptor y sample; Slice 11 resolverá por
`sampledData` ID después de que Slices 9 y 10 existan.

## Evidencia visual real

La hoja de contacto fue generada por el oráculo independiente desde los bytes
PackBits, no por el producto y no desde concept art:

```text
docs/showcase/images/editor-states/m8-rbrush-abr-sample-decode.png
dimensions: 2600x1620
SHA-256: 8E85A980A53BCE5B0D3247807946042E5E493E9D9AB1222F8A9B6CCF75556C80
```

Inspección visual: `PASS`. Los 15 tiles son distintos, completos y legibles;
las grietas conservan gradientes de cobertura, no hay máscaras vacías, filas
truncadas, repetición accidental de tile ni corrupción visible.

## Gates finales

```text
Debug focal build, 7 targets: PASS (18.0 s)
Debug focal CTest: 5/5 PASS (1.07 s)
Debug completo: PASS (33.9 s)
Debug causal/Shipping: 10/10 PASS (71.52 s)
Release focal build, 7 targets: PASS (26.0 s)
Release focal CTest/admission: 6/6 PASS (0.76 s)
windows-vulkan-runtime configure: PASS (27.9 s)
runtime-only Release completo: PASS (6.2 s)
runtime-only ABR decoder/inspector/CLI/source scan: 0 hits
git diff --check: PASS
```

La prueba causal incluye admisión de Asset Import, recibo de scheduling,
boundary de codecs Shipping, exclusión de fuentes y descubrimiento runtime-only.
El runtime-only no contiene `AbrInspector`, `AbrSampleDecoder`, `AbrV62Parser`,
`abr_inspect`, `abr_decode`, `inspectAbr`, `decodeAbrSamples` ni parser `.abr`.

## Límites y continuidad

- M8 y JericoEngine 1.0 siguen `ACTIVE / PARTIAL`; este PASS cierra sólo Slice 8.
- C04 Jobs/Async sigue `PARTIAL` bajo GAP-10/PB-06. Este slice aporta snapshot
  inmutable, cancelación, límites y resultado all-or-nothing; todavía faltan el
  scheduling fuera de Qt, progreso, budgets globales, commit en hilo dueño,
  descarte de callbacks tardíos y shutdown seguro del owner editor.
- No se implementaron descriptor IR, semantic mapper, publicación, UI, painter,
  SDF/PWAS/EPLS 2.0, Terrain ni JericoParticles.
- Próxima acción única: RBrush Slice 9 Generic Descriptor Parser. Su PASS exige
  nombres `121..135`, `sampledData` IDs, spacing/diameter/angle/roundness,
  toggles conocidos y reporte de unknowns. No resuelve aún `desc -> samp`.

Estado: `PASS` (RBrush Slice 8 solamente)

Resultado: 15 máscaras canónicas lossless reales con hashes estables.

Pruebas ejecutadas: Debug completo, causal/Shipping 10/10, Release 6/6,
runtime-only Release y oráculo/inspección visual independientes.

Evidencia: este documento y la hoja de contacto real versionada.

Faltantes: RBrush Slice 9 y slices posteriores; M8/Goal 1.0 no están completos.

Riesgos: C04 editor-owner permanece parcial; los índices físicos no son IDs.

Siguiente slice: RBrush Slice 9 Generic Descriptor Parser.

## Referencias de interoperabilidad

- Adobe Photoshop File Formats Specification:
  `https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/`
- GIMP `gimpbrush-load.c` oficial:
  `https://github.com/GNOME/gimp/blob/master/app/core/gimpbrush-load.c`

Se usaron como oráculos de formato. No se copió, vinculó ni redistribuyó código
externo.
