# M8 RBrush Brush Pose Editor

Estado: `PARTIAL` — checkpoint verificable de RBrush Slice 17 Editor. Añade
autoría Brush Pose al editor productivo, pero no cierra Slice 17, Slice 18, M8
ni el Goal maestro.

## Autoridad y alcance

- Spec maestro:
  `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`.
- Diseño:
  `docs/superpowers/specs/2026-08-12-m8-rbrush-brush-pose-editor-design.md`.
- Plan:
  `docs/superpowers/plans/2026-08-12-m8-rbrush-brush-pose-editor.md`.
- Diseño/plan: `5887fe7`.
- Producto/pruebas: `87057e9`, `e6967b3`, `85945ec`, `a9f1cd9`.

El vertical reutiliza `RBrushPoseIR`, el contenedor RBrush admitido, GUID
estables, el catálogo real y el único ProjectAsset Replace. No introduce otro
formato, catálogo, scanner, History, importer ni referencia a `SourceAssets`.

## Comportamiento demostrado

- `Configured` distingue ausencia canónica de Pose configurado.
- Tilt X, Tilt Y y Rotation se presentan en grados y persisten en radianes.
- Pressure se presenta como porcentaje y persiste normalizado.
- Los cuatro override flags son independientes; desmarcar uno conserva el valor.
- Preview mantiene bytes; Cancel restaura; Apply persiste; Undo/Redo alternan
  bytes exactos y reopen restaura valores y flags.
- El diff muestra Configured y cada valor/override realmente cambiado.
- Brush Pose posee ownership independiente. Reimport conserva el valor completo
  o la ausencia poseída; sin ownership, la fuente nueva gana.
- IR schema 11/container 1.10 persisten ese ownership. Schema 10/container 1.9
  siguen legibles y rechazan el bit al escribir en vez de perderlo.

## TDD observado

1. IR/container RED: MSVC `C2838/C2065` por la ausencia de
   `RBrushPresetOverrideSection::BrushPose`.
2. Edición/transaction RED: MSVC `C2440` porque
   `RBrushPresetEditableProperties` no contenía `.pose`.
3. Reimport RED: las assertions reprodujeron la pérdida del valor poseído y de
   la ausencia poseída.
4. Qt RED: nueve búsquedas obligatorias devolvieron `nullptr` porque el panel no
   existía.
5. Revisión visual/TDD del diff: siete filas Pose faltaban al configurar desde
   ausencia; la prueba las exigió antes de ampliar el diff.
6. El writer real fijó el SHA canónico del pack vacío en
   `3767130519E103A5C36214B5E534DB215D5006765B271CB8004E3E887D7FA00B`.

## Verificación fresca

- IR/container Debug: `2/2 PASS`.
- Edición pura + transacción Debug: `2/2 PASS`.
- Reimport + edición + transacción Debug: `3/3 PASS`.
- Workbench focal Debug: `1/1 PASS`, 3.88 s en la repetición previa a captura.
- Build integrado Debug: PASS.
- Matriz integrada Debug: `7/7 PASS`, 28.61 s.
- Gates finales Debug: `5/5 PASS`, 36.20 s.
- Build integrado Release: PASS. La primera envoltura agotó 124 s sin error;
  no quedaron procesos hijos y la repetición incremental terminó con exit 0.
- Matriz integrada Release: `7/7 PASS`, 10.75 s.
- Gates finales Release: `5/5 PASS`, 31.19 s.
- Gates: Qt admission, scheduling, Shipping source exclusion, runtime-only
  target discovery y Qt deployment.

## Evidencia visual retenida

- Archivo: `docs/evidence/images/m8-rbrush-brush-pose-editor.png`.
- SHA-256:
  `98496E843C64F8414AC95B7CEE236ADB2420833810FA4A9BBD8CBC5A355C40FC`.
- Tamaño: 66,373 bytes.

La captura proviene del ejecutable Qt real con backend Windows y fue
inspeccionada a resolución original. Muestra Brush Pose configurado, Tilt X
15 grados con override, Tilt Y -20 grados sin override, Rotation 30 grados con
override, Pressure 65% con override y las ocho diferencias reales antes de
Apply. No es concept art.

## Límites honestos y siguiente capacidad

No se demuestra canvas, Brush Engine, pintura, Misc/Other known state, SDF
defaults, renderer, Virtualized World runtime, JSVGI ni Shipping pintado.
Continuar Slice 17 con `Other known state / Misc`, la siguiente sección canónica
no demostrada, usando ownership separado, la misma transacción y evidencia
Debug/Release; no abrir Slice 18, Terrain/EPLS 2.0, Painter ni M9.
