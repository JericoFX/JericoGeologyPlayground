# M8 RBrush Misc Editor

Estado: `PARTIAL` — checkpoint verificable de RBrush Slice 17 Editor. Añade
autoría de Other/Misc al editor productivo, pero no cierra Slice 17, Slice 18,
M8 ni el Goal maestro.

## Autoridad y alcance

- Spec maestro:
  `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`.
- Diseño:
  `docs/superpowers/specs/2026-08-12-m8-rbrush-misc-editor-design.md`.
- Plan:
  `docs/superpowers/plans/2026-08-12-m8-rbrush-misc-editor.md`.
- Diseño/plan: `ed8697b`, `c9c6864`.
- Producto/pruebas: `3f77229`, `54ba4a8`, `7b4803e`, `aa1f2f6`.

El vertical reutiliza `RBrushMiscFlags`, el mapper ABR existente, el contenedor
RBrush admitido, GUID estables, el catálogo real y el único ProjectAsset
Replace. No introduce otro formato, catálogo, scanner, History, importer ni
referencia a `SourceAssets`.

## Comportamiento demostrado

- JericoEditor expone Noise, Wet Edges, Build Up, Smoothing, Airbrush y Protect
  Texture como seis checkboxes canónicos.
- Preview mantiene bytes y muestra sólo los flags cambiados; Cancel restaura;
  Apply persiste; Undo/Redo alternan bytes exactos y reopen restaura los seis.
- Misc posee ownership independiente. Reimport conserva la combinación completa,
  incluido all-false owned; sin ownership, la fuente nueva gana.
- IR schema 12/container 1.11 persisten ese ownership. Schema 11/container 1.10
  siguen legibles y rechazan el bit al escribir en vez de perderlo.
- SHA-256 canónico del contenedor vacío en 1.11:
  `27E0492F0A6DDD94A82DC5B8BF8DE73CFEC5EC7B15953C607B05F23786D30EF2`.

## TDD observado

1. IR/container RED: MSVC `C2838/C2065` por ausencia de
   `RBrushPresetOverrideSection::Misc`.
2. Edición/transaction RED: MSVC `C2440` porque
   `RBrushPresetEditableProperties` no contenía `.misc`.
3. Reimport RED: assertions exactas reprodujeron pérdida de la combinación owned
   y del all-false owned; el caso unowned siguió correcto.
4. Qt RED: seis búsquedas obligatorias devolvieron `nullptr` porque el panel no
   existía.

## Verificación fresca

- IR/container Debug: `2/2 PASS`.
- Edición pura + transacción Debug: `2/2 PASS` en 10.97 s.
- Reimport + edición pura Debug: `2/2 PASS` en 0.13 s.
- Workbench + transacción Debug: `2/2 PASS` en 14.51 s.
- Captura focal Windows: `1/1 PASS`, 5.2 s.
- Build integrado Debug, incluido `JericoEditor`: PASS, 41.6 s.
- Matriz integrada Debug: `7/7 PASS`, 15.42 s.
- Gates Debug: admisión Qt, scheduling, runtime-only discovery y Shipping
  source exclusion pasaron; deployment detectó primero un GltfCook staged
  obsoleto. Tras reconstruir `JericoEditorShellTests`, el mismo gate pasó en
  6.30 s, sin cambio de producto ni del script del usuario. Resultado final:
  `5/5 PASS`.
- Build integrado Release: PASS, 142.4 s.
- Matriz integrada Release: `7/7 PASS`, 9.14 s.
- Gates finales Release: `5/5 PASS`, 31.22 s.

Revalidación inmediatamente anterior a publicación:

- Build incremental de `JericoEditor` y los siete targets del vertical: Debug
  PASS en 27.9 s; Release PASS en 23.8 s.
- Matriz integrada: Debug `7/7 PASS` en 15.47 s; Release `7/7 PASS` en
  8.97 s.
- Gates: Debug `5/5 PASS` en 35.97 s; Release `5/5 PASS` en 31.09 s.

La matriz integrada contiene IR, container, update, propiedades compuestas,
catálogo de proyecto, transacción y workbench. Los gates prueban además que la
autoría no entra a runtime/Shipping.

## Evidencia visual retenida

- Archivo: `docs/evidence/images/m8-rbrush-misc-editor.png`.
- SHA-256:
  `A687FFE32AD72435C2300ECC1B755A87C02AF8FFDE456242AA5B9825148C468A`.
- Tamaño: 61,156 bytes.
- Resolución retenida: 1920 x 1061 (área cliente de la ventana 1920 x 1200).

La captura proviene del ejecutable Qt real con backend Windows y fue
inspeccionada a resolución original. Muestra los seis checkboxes activos y seis
filas `Off -> On` antes de Apply. La primera captura offscreen mostró glifos no
admitidos y fue reemplazada; no se retiene como evidencia final. No es concept
art.

## Límites honestos y siguiente capacidad

No se demuestra SDF Defaults, canvas, Brush Engine, pintura, renderer,
Virtualized World runtime, JSVGI ni Shipping pintado. Continuar Slice 17 con
`SDF Defaults` —Compatible, Threshold, Invert, Default Depth, Edge Width y
Profile Gamma— usando IR canónico, ownership separado y la misma transacción.
No abrir Slice 18 ni M9 antes de demostrar esa capacidad y reevaluar todos los
criterios de Slice 17.
