# M8 RBrush SDF Defaults Editor

Estado: `PASS` para RBrush Slice 17 Editor; M8 y el Goal maestro permanecen
`ACTIVE / PARTIAL`.

## Autoridad y alcance

- Spec maestro:
  `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`.
- Diseño:
  `docs/superpowers/specs/2026-08-12-m8-rbrush-sdf-defaults-editor-design.md`.
- Plan:
  `docs/superpowers/plans/2026-08-12-m8-rbrush-sdf-defaults-editor.md`.
- Diseño/plan: `0501c20`, `f7e8157`.
- Producto/pruebas: `7274c5f`, `6cd50f4`, `1f0be0d`, `774c16d`.

El vertical reutiliza el IR y contenedor `.rbrush`, GUID estables, catálogo de
proyecto, un único ProjectAsset Replace y las transacciones existentes. No
implementa todavía derived 2D SDF, stamps, canvas, Brush Engine, Painter ni una
ruta paralela de cook/runtime.

## Comportamiento demostrado

- IR schema 13/container 1.12 persiste `Compatible`, `Threshold`, `Invert`,
  `Default Depth`, `Edge Width` y `Profile Gamma` con ownership independiente.
- Los writers schema 12/container 1.11 rechazan estado SDF real en vez de
  perderlo; los formatos anteriores siguen legibles e hidratan defaults.
- Validación canónica rechaza threshold, profundidad, edge width y gamma fuera
  de sus límites documentados.
- JericoEditor presenta seis controles tipados. `Compatible=false` deshabilita
  los otros cinco sin borrar sus valores.
- Preview conserva bytes y muestra seis filas; Cancel restaura; Apply persiste;
  reopen recupera valores; Undo/Redo alternan bytes exactos.
- Reimport conserva el bloque completo cuando está poseído, incluido
  `RBrushSdfDefaults{}` poseído; sin ownership gana la fuente entrante.
- SHA-256 canónico del contenedor vacío 1.12:
  `8F8EAAB17AFE53BBD60C4EAE468F31E4F9831FB19FA7868AF7AD7A31C36D7075`.

## TDD observado

1. IR/container RED: MSVC `C2065` para `RBrushSdfDefaults` y `C2838/C2065`
   para `RBrushPresetOverrideSection::SdfDefaults`.
2. Edición/transacción RED: el inicializador designado falló porque
   `RBrushPresetEditableProperties` no tenía `.sdfDefaults`.
3. Reimport RED: fallaron exactamente los casos owned non-default y owned
   default; el caso unowned ya era correcto.
4. Qt RED: las seis búsquedas obligatorias devolvieron `nullptr`.

## Verificación final

- Build integrado Debug de `JericoEditor` y siete targets RBrush: PASS.
- Matriz integrada Debug: `7/7 PASS` en 16.07 s.
- Build integrado Release: el primer wrapper agotó 124 s sin error; la
  reanudación incremental finalizó `PASS` en 23.9 s.
- Matriz integrada Release: `7/7 PASS` en 9.51 s.
- Gates Debug: el primer intento detectó un `JericoGltfCook.exe` staged
  obsoleto. Tras reconstruir el target propietario `JericoEditorShellTests`,
  sin editar el test del usuario, resultado final `5/5 PASS` en 35.98 s.
- Gates Release: `5/5 PASS` en 31.82 s.
- Captura focal del ejecutable Qt Windows: PASS.

La matriz contiene catálogo, transacción, workbench, IR, container, update y
propiedades editables. Los gates cubren admisión Qt, scheduling, discovery de
targets runtime-only, exclusión de fuentes en Shipping y deployment.

## Evidencia visual retenida

- Archivo: `docs/evidence/images/m8-rbrush-sdf-defaults-editor.png`.
- SHA-256:
  `D6EBD234A7ED4783FCFEEADC1C0BF7009E5DEB067E2C36ABB4AA81F0361D3959`.
- Tamaño: 60,310 bytes.
- Resolución: 1920 x 1061.

La captura fue inspeccionada a resolución original. Muestra el grupo SDF
Defaults real, valores `35%`, `Invert`, `0.2 m`, `10%`, gamma `2.0`, las seis
filas Before/After y el mensaje de que los bytes canónicos permanecen sin
cambio hasta Apply. No es concept art.

## Cierre honesto de Slice 17

RBrush Slice 17 queda `PASS`: Tip, Shape Dynamics, Scatter, Transfer, response
curves, Dual Brush, Texture, Color Dynamics, Brush Pose, Other/Misc y SDF
Defaults son editables; tags/grupos ya están demostrados por Pack Management.
El test integrado transforma desde JericoEditor un preset de máscara R8 en un
brush con scatter, jitter y control por presión, y demuestra persistencia y
replay transaccional.

Esto no demuestra el canvas, dabs, pintura, derived 2D SDF, renderer, cook
estático SDF ni M8. La siguiente capacidad legal es RBrush Slice 18 `Brush test
canvas`: hacer visibles spacing/scatter/jitter/pressure sin World.
