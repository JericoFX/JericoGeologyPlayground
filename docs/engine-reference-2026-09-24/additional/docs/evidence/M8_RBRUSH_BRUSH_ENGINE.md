# M8 RBrush Shared Brush Engine

Estado: `PASS` para RBrush Slice 19; M8 y el Goal maestro permanecen
`ACTIVE / PARTIAL`.

## Autoridad y alcance

- Spec maestro:
  `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`.
- Diseño:
  `docs/superpowers/specs/2026-08-12-m8-rbrush-brush-engine-design.md`.
- Plan:
  `docs/superpowers/plans/2026-08-12-m8-rbrush-brush-engine.md`.
- Diseño/plan: `df1cd92`, `4d17a38`, `2d729bb`.
- Contrato/dynamics: `638765b`.
- Sampler/dabs deterministas: `ca36599`.
- Adapter y consumidor Qt: `836117e`.

El vertical crea `JericoPaintingBrush` como módulo Engine source-format-free que
depende sólo de `Jerico::Core`. No conoce Qt, AssetPipeline, RBrush, filesystem,
renderer ni formato fuente. JericoEditor conserva un único adapter explícito
desde RBrush IR y el canvas conserva únicamente tip lookup y rasterización.

## Comportamiento demostrado

- `BrushDab` canónico 3D contiene posición, normal, tangente, radio, aspect,
  rotación, opacity, flow, TipGUID y seed de 64 bits.
- El sampler recorre longitud de arco 3D y emite streams primario/dual
  independientes, ordenando el primario primero cuando coinciden.
- Normal/tangente se normalizan y ortogonalizan; scatter desplaza sobre
  binormal y opcionalmente tangente.
- PresetGUID, hash de contenido del pack y seed raíz forman la jerarquía
  determinista SplitMix64. Evento, dab y propiedad usan slots fijos; repetir una
  request produce igualdad exacta y otra seed cambia propiedades aleatorias.
- Spacing, size/angle/aspect dynamics, pressure/curves, opacity, flow, scatter,
  count/count-control/count-jitter, dual brush y los ocho tool overrides están
  cubiertos por casos literales.
- El preset de entrada permanece byte/value-inmutable durante overrides.
- Se rechazan stroke vacío o mayor a `4096`, GUID cero, frames degenerados,
  valores no finitos, dual incompleto, controles desconocidos, cancelación y el
  intento de emitir el dab `16385`.
- RBrush IR se adapta exhaustivamente; `roundness` es el aspect neutral y el
  content hash exacto del pack es la revisión determinista.
- El canvas expone los `Painting::BrushDab` reales y muestra los seeds canónicos
  primero/último. El source audit exige que no existan `PreviewRandom`, curve,
  control, dynamic factor ni scatter evaluator paralelos.
- Mouse, estado tablet, Demo/Clear y cero escrituras sobre `.rbrush` continúan
  demostrados por el mismo test Qt real.

## TDD observado

1. Task 1 RED: MSVC `C1083`, faltaba
   `Jerico/Painting/Brush/BrushDynamics.hpp`.
2. Task 2 RED: MSVC `C1083`, faltaba
   `Jerico/Painting/Brush/BrushEngine.hpp`.
3. Task 3 adapter RED: MSVC `C1083`, faltaba `RBrushBrushAdapter.hpp`.
4. Task 3 migration RED: el ejecutable Qt reportó cinco fallos exactos porque
   el canvas aún contenía `PreviewRandom`, `evaluateCurve`, `controlValue`,
   `dynamicFactor` y `scatterPosition`.
5. El caso de integración RBrush detectó que scatter canónico admite `0..10`,
   no sólo `0..1`; el backend fue alineado al IR antes de la migración final.
6. RED angular: la prueba literal detectó que `minimum` no se aplicaba a angle;
   GREEN lo aplica antes del remap a `2*pi`.

## Verificación final

- Build integrado Debug de `JericoEditor`, Brush Engine y siete targets RBrush:
  PASS en `47.81 s`.
- Brush Engine focal Debug: exit `0`.
- Matriz RBrush Debug: `7/7 PASS` en `22.42 s`.
- Gates Debug: `5/5 PASS` en `58.92 s`.
- Arquitectura Debug: `20/20 PASS` en `3.00 s`.
- Build integrado Release: PASS; confirmación incremental completa en
  `36.17 s` después de que el wrapper de 60 s dejara terminar el primer build.
- Brush Engine focal Release: exit `0`.
- Matriz RBrush Release: `7/7 PASS` en `11.45 s`.
- Gates Release: `5/5 PASS` en `50.09 s`.
- Arquitectura Release: `20/20 PASS` en `3.14 s`.
- Inventario CTest: exactamente `258`; el test Engine focal se ejecuta directo
  y no altera el inventario admitido.
- Escaneo del target Engine: cero coincidencias de Qt, AssetPipeline, RBrush,
  filesystem o Editor.

Las matrices RBrush cubren catálogo, pack management, workbench/modelo, IR,
container, update y shape properties. Los gates cubren admisión Qt, scheduling,
runtime-only discovery, Shipping source exclusion y deployment. El Brush Engine
no carga `.rbrush` ni ninguna fuente en Shipping.

## Evidencia visual retenida

- Archivo: `docs/evidence/images/m8-rbrush-brush-engine.png`.
- SHA-256:
  `F823635BFCC602FA0D98C9432E357BF5F1524B3E1DAC12229EF18D62CECBB609`.
- Tamaño: `103,184` bytes.
- Resolución: `1920 x 1061`.

La captura fue inspeccionada a resolución original. Muestra la ventana real de
JericoEditor, el canvas grande con variación visible de size/opacity/scatter,
`48 samples`, `801 dabs`, seeds canónicos primero/último, controles de Tip y
Shape, y Demo/Clear. No es concept art.

## Cierre honesto de Slice 19

RBrush Slice 19 queda `PASS`: existe un único Brush Engine neutral y
determinista consumido por el canvas real. Esto todavía no pinta un canal de una
superficie, no crea paint tiles, no persiste strokes en World, no implementa
dirty regions/VT pages ni prueba SDF stamping.

La próxima capacidad legal es RBrush Slice 20: Surface Painter scalar
`Roughness`, empezando por una operación no destructiva con preview/diff,
transacción undo/redo, región sucia y salida cocinable. No se inicia dentro de
este checkpoint.

Esto no demuestra Surface Painter completo, SDF stamp, renderer M8,
Virtualized World runtime, JSVGI, M8 ni JericoEngine 1.0 completo.
