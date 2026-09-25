# M8 RBrush Tip, Shape Dynamics y Scatter Editor

Estado: `PARTIAL` — checkpoint verificable de RBrush Slice 17 Editor. Demuestra
el primer grupo productivo de propiedades, pero no cierra Slice 17, Slice 18,
M8, Virtualized World, JSVGI ni el Goal maestro.

## Autoridad y alcance

- Spec maestro: `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`.
- Diseño: `docs/superpowers/specs/2026-08-12-m8-rbrush-tip-shape-scatter-editor-design.md`.
- Plan: `docs/superpowers/plans/2026-08-12-m8-rbrush-tip-shape-scatter-editor.md`.
- Diseño y plan: commits `451a0db` y `88b609d`.
- Producto y pruebas: commits `cbf6503`, `039f5d0`, `46a16a4`, `cd0b57b`,
  `ce69527`, `0ca8da9`, `c39428e` y `a231d50`.

El slice reutiliza el GUID estable de pack y preset, el IR/contenedor RBrush
canónico, el catálogo de assets, la transacción `ProjectAsset Replace` y el
historial Undo/Redo existentes. No crea un segundo formato, base de assets ni
camino de persistencia.

## Comportamiento demostrado

- Asset Browser publica una fila estable por preset RBrush y conserva los GUID
  de pack y preset como roles del modelo.
- Doble click abre el preset admitido en el workspace RBrush; no abre Smart
  Import ni vuelve a importar el archivo.
- El inspector ofrece controles tipados de Tip, Shape Dynamics y Scatter.
- Las unidades visibles se convierten de manera explícita al IR canónico.
- `Preview` calcula el diff sin modificar bytes persistidos.
- `Cancel` descarta el preview.
- `Apply` reemplaza el asset mediante la transacción canónica y conserva
  ownership de secciones durante reimport.
- Undo/Redo restauran exactamente los bytes anteriores y posteriores.
- El shell espera correctamente la finalización asíncrona del preview de Smart
  Import; la prueba ya no presupone una sola iteración del event loop.

## TDD

RED observados:

1. C1083 por ausencia de `RBrushEditorWorkbench.hpp`.
2. La prueba de apertura desde Asset Browser no compilaba porque no existía el
   rol estable `RBrushPresetGuid`.
3. La matriz Release falló `editor.shell` en la línea que buscaba el botón
   Apply después de una sola bomba de eventos; el preview ya era asíncrono.

GREEN mínimo:

- esquema 6/contenedor 1.5 con ownership explícito de secciones;
- operación pura y transacción real para editar shape properties;
- workbench Qt conectado al asset admitido;
- acceso desde Asset Browser;
- espera por condición observable en la prueba del shell.

## Verificación

- Matriz focal Debug fresca: `10/10 PASS`, 25.37 s.
- Matriz focal Release fresca: `10/10 PASS`, 12.87 s.
- Test final del workbench Debug: `1/1 PASS`, 2.08 s.
- Test final del workbench Release: `1/1 PASS`, 0.64 s.
- `repository.qt_admission_contract`: PASS.
- `repository.test_scheduling_contract`: PASS.
- `repository.shipping_source_exclusion`: PASS.
- `repository.runtime_only_target_discovery`: PASS.
- `editor.qt_deployment`: Debug y Release PASS después de reconstruir el staging
  canónico del editor.
- Gates finales agrupados: Debug `5/5 PASS`, 28.57 s; Release `5/5 PASS`,
  26.78 s.

## Evidencia visual retenida

- Archivo: `docs/evidence/images/m8-rbrush-tip-shape-scatter-editor.png`.
- SHA-256: `75BD0E5B22BE7E5337B749538C2A1CE5796062D3ED344E1F53001C1CC2D256C9`.
- Tamaño: 46,003 bytes.

La captura proviene del ejecutable de prueba real. Muestra el preset
`Rocks/Cliffs/Cliff Scatter`, los controles Tip/Shape Dynamics/Scatter y el diff
de Preview `Spacing 37.500 -> 125.000`, con el estado que confirma que Apply aún
no alteró los bytes canónicos. No es concept art.

## Límites honestos

Slice 17 todavía requiere Transfer, response curves, Dual Brush, Texture, Color
Dynamics, Brush Pose y otros estados conocidos. Slice 18 debe hacer visibles
spacing/scatter/jitter/pressure en un canvas independiente del World. Este
checkpoint tampoco prueba el Brush Engine, render, cook visual ni Shipping del
resultado pintado.

## Siguiente capacidad legal

Continuar Slice 17 con `Transfer + response curves`, reutilizando el mismo IR,
transacción, workbench, catálogo y pruebas Debug/Release antes de abrir Slice 18.
