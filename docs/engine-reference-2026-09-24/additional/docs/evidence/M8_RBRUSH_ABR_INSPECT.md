# M8 RBrush Slice 7 - ABR Inspect Evidence

**Estado del slice:** PASS
**Milestone:** M8 ACTIVE / PARTIAL
**Producto:** `3f1da23 feat(asset-pipeline): inspect abr brush packs`,
`6211faf test(asset-pipeline): harden abr preflight limits` y
`b5730f3 test(asset-pipeline): cover abr descriptor depth`
**Fecha:** 2026-08-11

## Resultado observable

`abr_inspect` inspecciona estructura ABR v6.2 hostil desde un snapshot de bytes
inmutable y acotado. Reporta secciones, muestras y cantidad de presets sin
decodificar pixeles, publicar assets ni exponer propiedades Photoshop.

La fixture local real produjo:

```text
abr_inspect.version|major=6|minor=2
abr_inspect.section|key=samp|offset=16|size=23425228
abr_inspect.section|key=patt|offset=23425256|size=0
abr_inspect.section|key=desc|offset=23425268|size=10028
abr_inspect.samples|count=15
abr_inspect.descriptors|count=15
abr_inspect.sample_profile|count=15|width=2500|height=2500|format=R8|compression=PackBits
```

## Fixture real local-only

- Archivo: `Free Cracks Photoshop Brushes 10.abr`.
- Tamaño: `23,435,296` bytes.
- ZIP SHA-256:
  `3D48C4042E0D075C9CCAD21E683FBEA3949D31E05132B8B45E4654C696673C1B`.
- ABR SHA-256:
  `0D8875E5E19228A828F3A259348B8B94DB4584849F33BFC66DD8287FB65FEE9D`.
- El ZIP y el ABR permanecen fuera de Git bajo la política de redistribución de
  la spec RBrush. La extracción temporal vive sólo en `build/local-fixtures`.

## RED y GREEN auténticos

1. El test público fue registrado antes del producto. El build llegó al nuevo
   target y falló con `fatal error C1083: cannot open include file
   'Jerico/AssetPipeline/AbrInspector.hpp': No such file or directory`.
2. La interfaz y el inspector mínimo hicieron GREEN el fixture sintético.
3. La primera ejecución real falló con
   `asset.rbrush.abr_sample_invalid`. La inspección acotada demostró que las 15
   muestras tenían un trailer reservado exacto de ocho bytes cero después del
   payload Raw/PackBits; el fixture sintético reprodujo el fallo antes del fix.
4. La siguiente ejecución real falló en descriptor. Un RED sintético confirmó
   versión estructural `1` antes de cada `Objc`.
5. La matriz real aisló el framing mixto ABR v6.2: class ID con longitud `u16`
   y property key con longitud `u32`. El fixture sintético actualizado falló
   antes de separar ambos lectores y luego pasó.
6. La ejecución final real devolvió exit `0` y el resultado observable anterior.

## Contrato implementado

- Una interfaz Tool-only profunda:
  `inspectAbr(span<const byte>, stop_token) -> Result<AbrInspection>`.
- Fuente inmutable, big-endian, sublectores limitados por sección y registro.
- v6.2, bloques `8BIM` en orden independiente, offsets y tamaños exactos,
  padding cero y rechazo de claves conocidas duplicadas.
- Preflight de muestras Raw R8/R16 y PackBits R8, límites de dimensión,
  aritmética comprobada, tabla por fila y trailer reservado de ocho bytes.
- Descriptor versión 16 con límites de recursión/items/strings, IDs de clase
  `u16`, claves `u32`, objetos versión 1 y conteo exclusivo de `Brsh/VlLs/Objc`.
- Diagnósticos estables `asset.rbrush.abr_*`, incluyendo cancelación, resource
  limit, estructura, muestra, descriptor y tipo no soportado.
- CLI de filesystem delgada con admisión `.abr`, lectura exacta hasta 512 MiB,
  salida determinista y códigos de salida estables.
- Sin dependencia nueva. La documentación Adobe y el source oficial de GIMP se
  consultaron únicamente como autoridades de interoperabilidad; no se copió ni
  vinculó código externo.

## Matriz hostil y cobertura

El test `asset_pipeline.abr_inspector` cubre:

- vacío, cancelación previa, versión y firma inválidas;
- duplicado conocido, sección declarada fuera de límites y byte final sobrante;
- bounds/dimensiones, depth, compression, trailer y suma PackBits inválidos;
- Raw R8, Raw R16 y PackBits R8;
- orden independiente `desc/patt/samp`;
- tipo descriptor desconocido y framing de class/property IDs.
- count y profundidad descriptor excesivos, y versión estructural de objeto
  inválida.

## Verificación fresca

- Targets Debug RBrush + inspector/tool: build PASS.
- RBrush Debug: `4/4 PASS` en `0.67 s`.
- Build Debug completo final: PASS en `33.8 s`.
- Matriz causal/Shipping Debug final: `9/9 PASS` en `70.32 s`.
- RBrush/admisión Release final: `5/5 PASS` en `0.69 s`; el test de profundidad
  añadido después pasó también focalmente en Debug y Release.
- `windows-vulkan-runtime` Release: configure PASS en `28.0 s`; build PASS en
  `7.6 s` en la repetición final.
- Descubrimiento runtime-only: `0` archivos por nombre y `0` hits de proyecto
  para `AbrInspector`, `abr_inspect`, `inspectAbr`, `8BIMsamp` o parser `.abr`.
- Scheduling: inventario `244`, SHA-256
  `d9a3ffdf126104f066ea99a2e5c71c3697a74d9f4575ce089bb8e39867dac9d1`;
  set programado `46` y su hash permanecen sin cambio.
- `git diff --check` y `git diff --cached --check`: PASS antes del commit de
  producto.

El primer rerun causal detectó correctamente el receipt externo aún fijado en
243 tests. Se actualizó junto al inventario a 244; el test focal y la matriz
causal completa se repitieron y pasaron. No se ocultó ese RED de integración.

## Shipping, C04 y evidencia visual

- `JERICO_ENABLE_ASSET_IMPORT=OFF` excluye inspector, CLI, tests y fuentes ABR
  del preset runtime-only. Shipping sigue consumiendo sólo assets cocinados.
- C04 permanece `PARTIAL`: el inspector soporta snapshot owned/inmutable,
  cancelación cooperativa, límites y resultado all-or-nothing. El futuro owner
  Qt todavía debe demostrar scheduling, progreso, concurrencia/memoria,
  descarte de callbacks tardíos y shutdown seguro.
- Slice 7 no produce imagen ni cambia UI/runtime. No se reclama concept art,
  Sponza ni evidencia visual.

## Límites y siguiente slice

No se decodificaron pixeles ABR, no se modelaron propiedades descriptor, no se
creó semantic mapper, RBrush IR, publicación, UI, painter ni target adapter.
M8 y JericoEngine 1.0 continúan activos.

La única próxima acción legal es **RBrush Slice 8 - ABR Sample Decode** con RED
de producto sobre Raw/PackBits y prueba real. No adelantar UI, SDF,
PWAS/EPLS 2.0, Terrain ni JericoParticles.
