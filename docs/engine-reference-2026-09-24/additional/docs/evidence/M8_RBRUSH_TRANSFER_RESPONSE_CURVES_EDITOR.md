# M8 RBrush Transfer and Response Curves Editor

Estado: `PARTIAL` — checkpoint verificable de RBrush Slice 17 Editor. Añade
Transfer y response curves al editor productivo, pero no cierra Slice 17,
Slice 18, M8 ni el Goal maestro.

## Autoridad y alcance

- Spec maestro: `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`.
- Diseño: `docs/superpowers/specs/2026-08-12-m8-rbrush-transfer-response-curves-editor-design.md`.
- Plan: `docs/superpowers/plans/2026-08-12-m8-rbrush-transfer-response-curves-editor.md`.
- Diseño/plan: `342185f`, `d733c10`.
- Producto/evidencia: `c70d3ca`, `3e0b1be`, `ffce290`, `f27bf78`,
  `8910be8`, `f68bc6b`, `35dc65a`.

El vertical reutiliza el IR Transfer/curves ya importado desde ABR, el GUID
estable de asset/preset, el contenedor RBrush, el catálogo admitido y el único
ProjectAsset Replace. No crea otro formato, scanner, catálogo ni History.

## Comportamiento demostrado

- Opacity, Flow, Wetness y Mix exponen Value, Jitter, Minimum, Control y una
  response curve tipada en JericoEditor.
- La curva usa puntos normalizados, ordenados y validados por el IR canónico.
- Add Point inserta determinísticamente en el mayor intervalo; Remove Point
  elimina la fila seleccionada.
- Shape y Transfer se editan como una sola operación y un solo Replace.
- Preview conserva los bytes; Cancel restaura controles; Apply persiste;
  Undo/Redo alternan bytes exactos; reopen lee desde el catálogo real.
- Reimport conserva los cuatro canales Transfer y sus curvas cuando el preset
  tiene ownership Transfer. Sin ese bit, los valores nuevos de la fuente ganan.
- IR schema 7 y container 1.6 persisten el ownership. Schema 6/container 1.5
  siguen admitidos para los tres bits anteriores y rechazan Transfer en vez de
  perderlo silenciosamente.

## TDD observado

1. C2838/C2065: `RBrushPresetOverrideSection::Transfer` no existía.
2. C1083: faltaba `RBrushPresetEditableProperties.hpp`.
3. Reimport RED: cuatro checks fallaron porque opacity, flow, wetness y mix eran
   sustituidos por la fuente pese al ownership Transfer.
4. La transacción RED no compiló: faltaban `RBrushEditPropertiesRequest` y
   `prepareEditProperties`.
5. C1083: faltaba `RBrushResponseCurveEditor.hpp`.
6. Workbench RED: no existían los controles Value/Control/Curve de Opacity.

Al intentar registrar un ejecutable de prueba adicional, el gate scheduling
rechazó correctamente el inventario `258 -> 259`. La cobertura se integró en
los ejecutables focales RBrush ya admitidos; el inventario y su hash no se
debilitaron.

## Verificación fresca

- Build focal Debug: PASS.
- Matriz integrada Debug: `10/10 PASS`, 26.63 s.
- Build focal Release: PASS, 101.1 s.
- Matriz integrada Release: `10/10 PASS`, 13.27 s.
- Gates finales Debug: `5/5 PASS`, 26.84 s.
- Gates finales Release: `5/5 PASS`, 26.83 s.
- Gates incluidos: Qt admission, scheduling contract, Shipping source
  exclusion, runtime-only target discovery y Qt deployment.
- El deployment gate detectó copias stale de GltfCook en JericoEditor y
  JericoEditorShellTests; ambos targets se reconstruyeron y el gate final pasó.

## Evidencia visual retenida

- Archivo:
  `docs/evidence/images/m8-rbrush-transfer-response-curves-editor.png`.
- SHA-256:
  `6DA82026215C613A22B3FB33F9A8AF4BC4D3A8DDE42A99DD643BFC7415813872`.
- Tamaño: 45,720 bytes.

La captura proviene del ejecutable Qt real y fue inspeccionada a resolución
original. Muestra el preset `Rocks/Cliffs/Cliff Scatter`, las cuatro tabs
Transfer, Opacity 75%, la curva con P2 50/85 y un Preview legible:
`Opacity 90 -> 75`, `P2 50/70 -> 50/85`, junto al estado que confirma que los
bytes siguen sin cambios hasta Apply. No es concept art.

## Límites honestos

Slice 17 aún necesita Dual Brush, Texture, Color Dynamics, Brush Pose y los
otros estados conocidos. Slice 18 debe hacer visibles spacing/scatter/jitter/
pressure en un canvas independiente del World. Este checkpoint no demuestra
Brush Engine, pintura, runtime, cook visual o Shipping de un resultado pintado.

## Siguiente capacidad legal

Continuar RBrush Slice 17 con `Dual Brush`, preservando la misma operación
compuesta, ownership por sección, transacción y evidencia Debug/Release antes
de abrir Texture o Slice 18.
