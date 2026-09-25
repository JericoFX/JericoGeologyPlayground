# M8 RBrush Texture Paint — Slice 22

Estado: **PASS para Slice 22 solamente**

JericoEngine 1.0, M8, C04 global, Material Paint, Terrain, Sparse SDF,
Virtualized World, JSVGI y M9 permanecen activos o pendientes según sus
autoridades. Este checkpoint no completa el Goal maestro.

## Criterio observable

La autoridad de RBrush exige para Slice 22:

```text
RBrush mask + independent payload texture
```

La prueba vertical usa una máscara RBrush admitida, un payload Normal
independiente y una textura Normal objetivo distinta. Las identidades retenidas
en la prueba Shipping son:

- RBrush: `909192939495969798999A9B9C9D9E9F`;
- payload Normal: `707172737475767778797A7B7C7D7E7F`;
- Normal objetivo: `00565E855ECDF09EC728CF1F080B989D`.

Las tres GUID son diferentes. El payload se admite y cocina como
`TangentNormal + Linear`; el recipe schema 3 conserva sólo GUID y content hash,
y el job recibe un snapshot RGBA inmutable del `.rtext` ya admitido. El runtime
y Surface Paint no importan el PNG fuente.

## Resultado de producto

- El evaluador proyecta UV, muestrea el payload y reemplaza/blendea normales en
  espacio tangente normalizado bajo la cobertura de la máscara RBrush.
- El cooker produce una Normal `.rtext` canónica, conserva la GUID del target y
  cambia su content hash de `0x2CCD382CC118FC9C` a
  `0x5BD0C77AF87A7CF1`; la prueba real modifica `111981` texels.
- El picker usa el hit real del World y resuelve el slot Normal del material.
  Preview, Cancel, Apply, Undo, Redo y reopen reutilizan el job y la transacción
  existentes; no se creó un segundo viewport ni una ruta de pintura paralela.
- El renderer Vulkan admite la generación editable/evaluada de Normal y el
  cambio se observa sobre la superficie compartida.
- El inspector es brush-first: cursor superficial circular grande con radio,
  falloff segmentado y crosshair; punta/máscara circular separada; payload Normal
  con thumbnail independiente; controles Radius, Strength, Spacing y Falloff.

## Evidencia visual inspeccionada

![RBrush Texture Paint sobre el World](m8-rbrush-texture-paint/m8-rbrush-world-surface-normal-payload.png)

Captura Windows/Vulkan real: `1920 x 1032`, `175118` bytes, SHA-256
`228A6DAB611A2CE5B7E3044A61A96C910A30745C436E4B9478ACBEC18539444F`.

La captura fue inspeccionada a resolución original. Se acepta como evidencia de
claridad operativa del pincel y de la separación máscara/payload. La fixture M7G
sigue siendo una escena técnica; no se presenta como beauty shot ni como PASS
del renderer M8.

## RED y GREEN

RED auténtico antes del wiring del payload:

```text
surface_paint.fixture.arguments_invalid
exit=2
```

GREEN focal:

```text
surface_paint.fixture.succeeded
target=normal semantic=TangentNormal color=Linear
old_hash=0x2CCD382CC118FC9C new_hash=0x5BD0C77AF87A7CF1
changed_texels=111981
```

## C04 y responsividad

El owner Surface Paint ejecuta evaluación/decode fuera del hilo Qt y aplica el
resultado sólo en el owner. Resultados finales:

| Configuración | Preview | Qt gap máximo | Owner destroy | Callback tardío |
|---|---:|---:|---:|---|
| Debug | `768`, `69348` texels | `5 ms` | `90 ms` | `false` |
| Release | `768`, `69348` texels | `5 ms` | `33 ms` | `false` |

Cancelación cooperativa y descarte por lifetime permanecen demostrados. C04
global sigue `PARTIAL` porque otros cookers/bakes todavía no cumplen el contrato.

## Matrices finales

- Debug: Recipe/Evaluator/Cooker/Job executables exit `0`; RBrush workbench,
  scheduling contract, static mesh Vulkan y shader cook `4/4 PASS` en `25.87 s`.
- Release: los cuatro executables exit `0`; los mismos gates `4/4 PASS` en
  `6.39 s`.
- Shipping Debug: PASS en `135.7 s`.
- Shipping Release: PASS en `36.7 s`.

El full clean cook Debug es deliberadamente un gate de validación y no un flujo
interactivo aceptable. El perfil es `interactive`, usa cuatro workers y no
oculta un recook dentro del stroke. La experiencia diaria pendiente debe usar el
Dependency Graph, dirty set mínimo, cache, Cook Changed y Package Changed.

## Shipping source-free

BoomBox se cocina, su Normal se reemplaza bajo la ruta/GUID originales, el
material conserva su referencia y el resultado se empaqueta. Luego se eliminan
`SourceAssets`, `Assets`, `Cache`, `Cooked`, `Procedural` y `EditorAssets` antes
de dos replays Vulkan byte-identical.

- `.epak` SHA-256:
  `0ba5a6c460bd644c97538648046b4059b95fc2bd375b278650a615a227653b7c`;
- captura runtime SHA-256 común:
  `9ebcd5c482c05639ba0731fee7cc445554ae9684a5c3b774fbdad15a367966f7`;
- no quedan `.rbrush`, `.abr`, Surface Paint recipe, brush strokes, rutas de
  SourceAssets/EditorAssets ni nombre del PNG fuente dentro del paquete.

## Commits del slice

- diseño/plan: `754c2e8`, `f50c276`;
- schema/evaluator/cooker: `f26f822`, `9ebfbbd`, `8be77d8`;
- job/World/editor: `f1b30a2`, `a6d7c11`, `35afb40`;
- Shipping: `ac5aff8`.

## Siguiente capacidad legal

RBrush Slice 23, Material Paint: un solo brush mask debe aplicar los canales
seleccionados de un material PBR. Requiere diseño y RED propios; este checkpoint
no lo implementa ni autoriza adelantar Terrain, SDF stamp o M9.
