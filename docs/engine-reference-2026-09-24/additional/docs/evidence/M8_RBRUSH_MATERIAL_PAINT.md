# M8 RBrush Material Paint — Slice 23

Estado: **PASS para Slice 23 solamente**

JericoEngine 1.0, M8, C04 global, Terrain, Sparse SDF, Virtualized World,
JSVGI y M9 permanecen activos o pendientes según sus autoridades. Este
checkpoint no completa el Goal maestro.

## Criterio observable

La autoridad de RBrush exige para Slice 23:

```text
one brush mask applies selected channels from PBR material
```

La prueba vertical aplica una sola cobertura RBrush a BaseColor, Roughness,
Normal y Height de un material objetivo. Las identidades Shipping retenidas
son:

- material objetivo: `FE41007957E6CEF4FFCE6CEFA738E3B1`;
- BaseColor objetivo: `F07B79A644EFC56C4F0797CA23BCFBC1`;
- ORM objetivo: `07AF40354A6BB757A3502F59341B5572`;
- Normal objetivo: `00565E855ECDF09EC728CF1F080B989D`;
- Height objetivo: `E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF`.

El `.rmat` conserva referencias e identidad; su SHA-256 permanece
`cae1e30b7896aff5b8ba6a0ed9173983d8595efdd0af369f68e619521bdc0789`.

## Resultado de producto

- El recipe schema 4 conserva material fuente, material objetivo, channel mask,
  identidades de target y un único conjunto ordenado de strokes.
- El evaluator usa una sola cobertura determinista y produce cuatro resultados
  atómicos. BaseColor y Roughness cambian, Normal permanece normalizada y
  Height conserva su semantic `ScalarMask + Linear`.
- El cooker reutiliza `TextureCooker`, produce exactamente el dirty set de
  cuatro `.rtext` y no reescribe el `.rmat`.
- El job recibe snapshots cocinados e inmutables fuera de Qt. El owner publica
  brevemente el batch completo o descarta toda la generación.
- Apply, Cancel, Undo, Redo y reopen pasan por la transacción existente; no hay
  escrituras parciales ni painter paralelo.
- Vulkan admite generaciones editor RGBA8 para BaseColor, ORM, Normal,
  Emissive y Height con sus semantics/color spaces canónicos. El batch
  BaseColor+ORM se actualiza y restaura atómicamente; BC4/BC5/BC7 continúan
  reservados al contenido cocinado.
- El inspector mantiene el World dominante y separa biblioteca/punta activa de
  la tarjeta de material PBR. Expone ocho canales en una matriz 2x4 y mantiene
  Radius, Falloff, Strength y Flow visibles en ese orden.

## Evidencia visual inspeccionada

![RBrush Material Paint sobre el World](m8-rbrush-material-paint/m8-rbrush-world-material-paint.png)

Captura Qt/Windows/Vulkan real: `1920 x 1032`, `225420` bytes, SHA-256
`8EA209A1330CBEBC82371736C24CDCC9417BF036805F94539B14FAFC787E49B1`.

La captura fue inspeccionada a resolución original. Muestra una punta irregular
real, biblioteca de puntas, preview activo grande, material separado, cuatro
canales seleccionados, cursor superficial con radio/falloff/crosshair y el
diagnóstico `PREVIEW READY · MATERIAL · 4 CH · 4 textures · 2541 texels`.

La escena M7G cian sigue siendo un fixture técnico, no el aspecto final de
JericoEditor ni un beauty shot del renderer. La futura pasada UX/UI debe mejorar
chrome, jerarquía visual, Asset Browser y presentación PBR sin reimplementar
este backend ni convertir esta captura en referencia estética.

## RED y GREEN

RED auténticos retenidos durante el slice:

```text
render.asset.dynamic_texture_unsupported
SurfacePaintMaterialChannelGrid missing
Falloff must precede Strength
PREVIEW REJECTED (no real World stroke)
```

GREEN final:

```text
dynamic BaseColor + ORM batch update/restore: PASS
Material Paint World stroke: PREVIEW READY · MATERIAL · 4 CH
changed_targets=4
```

## C04 y responsividad

| Configuración | Preview | Qt gap máximo | Owner destroy | Callback tardío |
|---|---:|---:|---:|---|
| Debug | `768`, `69348` texels | `5 ms` | `89 ms` | `false` |
| Release | `768`, `69348` texels | `5 ms` | `32 ms` | `false` |

Cancelación cooperativa y descarte por lifetime permanecen demostrados. C04
global sigue `PARTIAL` porque los demás cookers y bakes todavía deben satisfacer
su contrato.

## Matrices finales

- Debug: Recipe/Evaluator/Cooker/Job/Transaction y Static Mesh Vulkan, todos
  exit `0`; catálogo/picking/RBrush `3/3 PASS` en `19.76 s`.
- Release: build focal RBrush + Static Mesh exit `0`; Static Mesh Vulkan exit
  `0` en `2.2 s`; RBrush Qt/Windows/Vulkan exit `0` en `6.1 s`.
- Shipping Release source-free: dos ejecuciones PASS en `149.167 s` y
  `57.931 s`, con hashes idénticos.

El clean full cook sigue siendo validación/recovery, no workflow interactivo.
El stroke no cocina: el flujo diario debe consumir dirty set mínimo, Cook
Changed y Package Changed cuando sus owners entren en orden.

## Shipping source-free

Después del cook/package se eliminan las fuentes y la autoría antes del replay
Vulkan. Los dos replays producen bytes idénticos:

- `.epak` SHA-256:
  `7e0b1a0a8498ffa321ae2796e4673d19e2ac239507daafc3a5397baf7c01cfe2`;
- captura runtime SHA-256 común:
  `9d6c19a7cf2880a06027db9ed1bbcd638cff77bc2763057c7217f3eac40d6cce`;
- `changed_targets=4` y el `.rmat` objetivo conserva su SHA-256;
- Shipping no depende de `.rbrush`, `.abr`, strokes, recipes ni SourceAssets.

## Commits del slice

- diseño/plan: `f2b4ba6`, `33bcd34`;
- schema/evaluator/cooker: `4d24037`, `2f56556`, `19bb63d`, `535c88c`;
- job/preview/editor/transacción: `89bb59c`, `fbf5ed7`, `0627024`,
  `1a2003b`, `fddc4ef`, `e70da5c`, `01a18be`, `4b3db38`, `4079158`;
- Shipping: `2c07440`.

## Siguiente capacidad legal

RBrush Slice 24, Terrain: el mismo RBrush debe pintar height de terrain y
material weights. Requiere diseño y RED propios y debe nacer sobre los contratos
M8 de dirty regions, virtualized world/residency, shadow cache y JSVGI. Este
checkpoint no implementa Terrain, SDF stamp, EPLS, M9 ni el Goal maestro.
