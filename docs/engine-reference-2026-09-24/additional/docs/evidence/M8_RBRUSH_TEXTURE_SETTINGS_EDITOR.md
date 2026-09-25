# M8 RBrush Texture Settings Editor

Estado: `PARTIAL` — checkpoint verificable de RBrush Slice 17 Editor. Añade
autoría Texture al editor productivo, pero no cierra Slice 17, Slice 18, M8 ni
el Goal maestro.

## Autoridad y alcance

- Spec maestro:
  `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`.
- Diseño:
  `docs/superpowers/specs/2026-08-12-m8-rbrush-texture-settings-editor-design.md`.
- Plan:
  `docs/superpowers/plans/2026-08-12-m8-rbrush-texture-settings-editor.md`.
- Diseño/plan: `2788601`.
- Producto/pruebas: `86969af`, `61ebbb6`, `9266b66`, `0e45726`,
  `86b0e22`.

El vertical reutiliza `RBrushTextureIR`, el contenedor admitido, GUID estables,
el catálogo real y el único ProjectAsset Replace. No introduce otro formato,
scanner, catálogo, History, importer o referencia a SourceAssets.

## Comportamiento demostrado

- `Configured` distingue ausencia canónica de una configuración deshabilitada.
- `Enabled`, pattern ID/name, scale, invert, brightness, contrast, mode, depth,
  minimum depth, depth jitter, control y texture-each-tip son editables.
- Modos fuente desconocidos admitidos se preservan mediante combo editable.
- Preview mantiene bytes; Cancel restaura; Apply persiste; Undo/Redo alternan
  bytes exactos y reopen restaura los controles.
- Texture posee ownership independiente. Reimport conserva el valor completo o
  la ausencia poseída; sin ownership, la fuente nueva gana.
- IR schema 9/container 1.8 persisten ese ownership. Schema 8/container 1.7
  siguen legibles y rechazan el bit al escribir en vez de perderlo.
- Se corrigió el retorno temprano de Dual Brush: las secciones posteriores se
  procesan aunque Dual Brush no esté configurado.

## TDD observado

1. C2838/C2065: faltaba `RBrushPresetOverrideSection::Texture`.
2. C2440: `RBrushPresetEditableProperties` no admitía `.texture`.
3. Reimport RED: dos assertions reprodujeron pérdida de valor y ausencia
   poseídos.
4. Qt RED: catorce controles Texture requeridos no existían.
5. El primer container GREEN mostró los dos golden cambios esperados: header
   1.7 -> 1.8 y SHA canónico del pack vacío. Se actualizaron desde el writer
   real y se retiró el diagnóstico temporal.

## Verificación fresca

- IR/container Debug: `2/2 PASS`.
- Edición pura Debug: `1/1 PASS`.
- Reimport + edición Debug: `2/2 PASS`.
- Transacción real Debug: `1/1 PASS`, 10.79 s.
- Workbench focal Debug: `1/1 PASS`, 1.93 s.
- Matriz integrada Debug: `7/7 PASS`, 23.62 s.
- Build focal Release: PASS, 102.1 s.
- Matriz integrada Release: `7/7 PASS`, 9.58 s.
- Gates finales Debug: `5/5 PASS`, 26.80 s.
- Gates finales Release: `5/5 PASS`, 26.73 s.
- Gates: Qt admission, scheduling, Shipping source exclusion, runtime-only
  target discovery y Qt deployment.

## Evidencia visual retenida

- Archivo: `docs/evidence/images/m8-rbrush-texture-settings-editor.png`.
- SHA-256:
  `40C08F35A97988B71DB38EFE4F7BED0674FE4D27E2234AE6D5DDBE07EFF55368`.
- Tamaño: 47,702 bytes.

La captura proviene del ejecutable Qt real y fue inspeccionada a resolución
original. Muestra Texture configurado y enabled, `pattern:granite`,
`Granite Veins`, scale 2.5, mode `height`, depth 80%, los restantes controles y
el diff previo a Apply. No es concept art.

## Límites honestos y siguiente capacidad

No se demuestra canvas, Brush Engine, pintura, patrón runtime, renderer, cook
visual, residency, JSVGI ni Shipping pintado. Continuar Slice 17 con
`Color Dynamics`, usando ownership separado, la misma transacción y evidencia
Debug/Release; no abrir Slice 18 ni M9.
