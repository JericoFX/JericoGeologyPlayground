# M8 RBrush Test Canvas

Estado: `PASS` para RBrush Slice 18; M8 y el Goal maestro permanecen
`ACTIVE / PARTIAL`.

## Autoridad y alcance

- Spec maestro:
  `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`.
- Diseño:
  `docs/superpowers/specs/2026-08-12-m8-rbrush-test-canvas-design.md`.
- Plan:
  `docs/superpowers/plans/2026-08-12-m8-rbrush-test-canvas.md`.
- Diseño/plan: `746cf1e`.
- Producto/pruebas: `12a2471`, `24d55ae`, `22d298f`.

El vertical reutiliza el catálogo, container decode y GUID/hashes canónicos de
RBrush. Añade un evaluador puro y acotado de preview y un QWidget editor-only.
No persiste strokes, no escribe el asset mientras se dibuja, no entra en un
World, no pinta superficies ni introduce todavía el Brush Engine compartido.

## Comportamiento demostrado

- El preset editable carga en una sola lectura anclada la cobertura R8 exacta
  de cada punta referenciada dentro del pack.
- GUID, formato, dimensiones y SHA-256 se comparan contra la admisión del
  catálogo; punta secundaria ausente o hash incorrecto fallan visiblemente.
- El modelo puro rasteriza una secuencia 2D con semilla fija y hace observables
  spacing, scatter, size/angle/roundness jitter, pressure, opacity, flow y dual
  brush.
- El demo stroke aparece al abrir el preset y se vuelve a evaluar con los
  valores candidatos de los controles sin requerir Preview/Apply.
- Mouse real, presión/tilt/rotation/direction compatibles con tableta,
  `Demo Stroke` y `Clear` están conectados al canvas.
- El trabajo está limitado a `4096` muestras, `16384` dabs y canvas máximo
  `1024 x 1024`; el canvas usado por JericoEditor es `768 x 320`.
- La prueba Qt conserva byte-exacto el `.rbrush` al cambiar controles, dibujar
  con mouse y suministrar estado de tableta. El refresh representativo medido
  queda por debajo del límite de prueba de `1000 ms`.

## TDD observado

1. Library RED: MSVC `C2039` porque `RBrushLibraryEditablePreset` no exponía
   `previewTips` y `RBrushLibraryTipCoverage` no existía.
2. Model RED: MSVC `C1083` porque `RBrushTestCanvasModel.hpp` no existía.
3. Qt RED: MSVC `C1083` porque `RBrushTestCanvas.hpp` no existía.
4. Integración Qt detectó correctamente que el widget no usa `Q_OBJECT`; el
   test pasó a resolverlo como `QWidget` y conservar el patrón real del shell.

## Verificación final

- Build integrado Debug de `JericoEditor` y siete targets RBrush: PASS en
  `33.5 s`.
- Matriz RBrush Debug: `7/7 PASS` en `16.65 s`.
- Build integrado Release: PASS en `76.7 s`.
- Matriz RBrush Release: `7/7 PASS` en `9.64 s`.
- Gates Debug: `5/5 PASS` en `37.95 s`.
- Gates Release: `5/5 PASS` en `31.75 s`.
- Captura focal del ejecutable Qt con plataforma Windows: PASS.

La matriz contiene catálogo, transacción, workbench/modelo, IR, container,
update y propiedades editables. Los gates cubren admisión Qt, scheduling,
discovery de targets runtime-only, exclusión de fuentes en Shipping y
deployment.

## Evidencia visual retenida

- Archivo: `docs/evidence/images/m8-rbrush-test-canvas.png`.
- SHA-256:
  `2911007E471D0694859FD1194EC416FF7DA224DF8396934C6B65B5370A86C101`.
- Tamaño: `97,471` bytes.
- Resolución: `1920 x 1061`.

La captura fue inspeccionada a resolución original. Muestra el canvas grande
dentro del Inspector real, el trail con variación de tamaño/opacidad/scatter,
`48 samples`, `132 dabs`, semilla `0x4A455249`, botones Demo/Clear y los
controles reales de Tip/Shape. No es concept art.

## Cierre honesto de Slice 18

RBrush Slice 18 queda `PASS`: spacing, scatter, jitter y pressure son visibles
sin World; el canvas también demuestra opacity, flow y dual brush exigidos por
la sección de Test Canvas del spec.

El evaluador actual es deliberadamente editor-only. Sus dabs no son todavía el
contrato canónico compartido `Engine/Painting/BrushEngine`, no se consumen por
Surface Painter y no prueban pintura 3D. La siguiente capacidad legal es Slice
19: definir el `BrushDab` común y promover la matemática mediante RED
determinista sin mantener dos evaluadores paralelos.

Esto no demuestra Surface Painter, SDF stamp, renderer, Virtualized World
runtime, JSVGI, M8 ni el Goal maestro.
