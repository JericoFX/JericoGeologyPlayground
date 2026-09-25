# M8 RBrush Color Dynamics Editor

Estado: `PARTIAL` — checkpoint verificable de RBrush Slice 17 Editor. Añade
autoría Color Dynamics al editor productivo, pero no cierra Slice 17, Slice 18,
M8 ni el Goal maestro.

## Autoridad y alcance

- Spec maestro:
  `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`.
- Diseño:
  `docs/superpowers/specs/2026-08-12-m8-rbrush-color-dynamics-editor-design.md`.
- Plan:
  `docs/superpowers/plans/2026-08-12-m8-rbrush-color-dynamics-editor.md`.
- Diseño/plan: `7ee9d4e`, `8675c50`.
- Producto/pruebas: `e4fa6cc`, `846ea9f`, `ce4c242`, `73113f2`.

El vertical reutiliza `RBrushColorDynamicsIR`, el contenedor RBrush admitido,
GUID estables, el catálogo real y el único ProjectAsset Replace. No introduce
otro formato, catálogo, scanner, History, importer ni referencia a
`SourceAssets`.

## Comportamiento demostrado

- `Configured` distingue ausencia canónica de una configuración deshabilitada.
- `Enabled`, foreground/background jitter, hue, saturation, brightness, purity,
  control y apply-per-tip son editables en porcentajes normalizados.
- Preview mantiene bytes; Cancel restaura; Apply persiste; Undo/Redo alternan
  bytes exactos y reopen restaura los controles.
- Color Dynamics posee ownership independiente. Reimport conserva el valor
  completo o la ausencia poseída; sin ownership, la fuente nueva gana.
- IR schema 10/container 1.9 persisten ese ownership. Schema 9/container 1.8
  siguen legibles y rechazan el bit al escribir en vez de perderlo.
- La composición corrigió el retorno temprano de Texture: una sección Texture
  ausente ya no impide procesar Color Dynamics.

## TDD observado

1. IR/container RED: MSVC `C2838/C2065` por la ausencia de
   `RBrushPresetOverrideSection::ColorDynamics`.
2. Edición/transaction RED: `C2440/C2039` porque
   `RBrushPresetEditableProperties` no contenía `.colorDynamics`.
3. Reimport RED: dos assertions reprodujeron la pérdida del valor y de la
   ausencia poseídos.
4. Qt RED: nueve búsquedas obligatorias devolvieron `nullptr` porque el panel no
   existía.
5. El primer container GREEN expuso los golden changes esperados. El writer
   real fijó el SHA canónico del pack vacío en
   `C4DFB46AB646254A51DE1E0D2632C662C388B40980134D35CA55F2D4AAFB79BC`.

## Verificación fresca

- IR/container Debug: `2/2 PASS`.
- Edición pura + transacción Debug: `2/2 PASS`.
- Reimport + edición + transacción Debug: `3/3 PASS`.
- Workbench focal Debug: `1/1 PASS`, 3.85 s en la repetición final.
- Matriz integrada Debug: `7/7 PASS`, 28.13 s.
- Build focal/integrado Release: PASS.
- Matriz integrada Release: `7/7 PASS`, 10.63 s.
- Gates finales Debug: `5/5 PASS`, 34.70 s.
- Gates finales Release: `5/5 PASS`, 30.53 s.
- Gates: Qt admission, scheduling, Shipping source exclusion, runtime-only
  target discovery y Qt deployment.

El primer gate Debug detectó correctamente una copia stale de
`JericoGltfCook.exe` en el staging de JericoEditor. Se reconstruyeron
`JericoEditor` y `JericoEditorShellTests`; la repetición completa pasó. Un
timeout de la envoltura dejó además un MSBuild Release todavía activo; se
esperó su finalización y el rebuild incremental definitivo terminó con exit 0.

## Evidencia visual retenida

- Archivo: `docs/evidence/images/m8-rbrush-color-dynamics-editor.png`.
- SHA-256:
  `39E480F979D2E55F5C0004D047249F359111AED2556D77E36B32CEBB81FE9EB6`.
- Tamaño: 60,401 bytes.

La captura proviene del mismo ejecutable Qt real verificado. Se ejecutó con el
backend Windows para conservar glifos legibles, porque el backend offscreen de
la prueba funcional sustituyó el texto por cuadros. Fue inspeccionada a
resolución original y muestra Color Dynamics configurado y enabled,
foreground/background 25%, hue 15%, saturation 35%, brightness 45%, purity
60%, control `Pen Pressure`, apply-per-tip y el diff antes de Apply. No es
concept art.

## Límites honestos y siguiente capacidad

No se demuestra canvas, Brush Engine, pintura, renderer, cook visual,
residency, JSVGI ni Shipping pintado. Continuar Slice 17 con `Brush Pose`, la
siguiente sección canónica no demostrada, usando ownership separado, la misma
transacción y evidencia Debug/Release; no abrir Slice 18 ni M9.
