# M8 RBrush Update / Reimport Evidence

Fecha: `2026-08-12`

Estado del slice: `PASS`

Estado superior: M8 `ACTIVE / PARTIAL`; C04 Jobs/Async `PARTIAL`;
JericoEngine 1.0 `ACTIVE / NOT COMPLETE`.

## Alcance demostrado

RBrush Slice 14 demuestra la actualización transaccional de una única fuente ya
admitida. Una fuente cambiada reemplaza el `.rbrush` existente en la misma ruta,
conserva la pareja estable `RBrushAssetGUID + PresetGUID` de los presets que
sobreviven, retiene como `sourceMissing=true` los presets retirados y admite los
presets realmente nuevos con sus GUID propuestos.

Preview y Cancel no modifican el proyecto. Apply publica un único `Replace`,
reabre el resultado antes de consumir la fuente y Undo/Redo alternan bytes y
catálogos exactos. Un fallo de la reapertura final restaura el pack anterior y
retiene la fuente.

No implementa Brush Library, UI, Painter, runtime RBrush, Terrain, SDF,
PWAS/EPLS 2.0, partículas, vegetation, virtual texturing ni Animation/Rigging.

## Producto y contratos reutilizados

- `RBrushPackIR`, su manifiesto y `RBrushDigest` siguen siendo la autoridad
  canónica; no se agregó un registro de reimportación paralelo.
- El schema IR avanza a `4` y el contenedor `.rbrush` a `1.3`. Schemas 1-3
  decodifican `sourceIndex=unavailable` y `sourceMissing=false`; schema 3 rechaza
  estado exclusivo de schema 4.
- `reconcileRBrushSourceUpdate` recibe el pack completo existente, un único pack
  de fuente entrante y el hash exacto de la fuente previa.
- El matching único y cerrado se ejecuta en este orden: internal ID ABR,
  internal ID no-ABR, nombre dentro del mismo source lógico, hash canónico del
  tip y, por último, índice de fuente. Una ambigüedad falla sin adivinar.
- El catálogo `ProjectAsset`, `ProjectTransactionCoordinator`, recovery journal
  y snapshots autenticados existentes publican un único `Replace`; no existe un
  servicio, journal o formato alternativo.
- La ruta y `AssetGUID` no cambian; la revisión continúa en `1` y el content hash
  cambia. History admite dos lados RBrush presentes con la misma revisión y
  hashes diferentes.

Commits de producto:

- `bd1c070 feat(asset-pipeline): version rbrush source update state`
- `6159111 feat(asset-pipeline): reconcile rbrush source updates`
- `5d6827c feat(editor): replace admitted rbrush assets`
- `59a7965 feat(editor): update rbrush sources transactionally`

Diseño y plan:

- `650cc93 docs(rbrush): define update reimport`
- `ba2468f docs(rbrush): plan update reimport`
- `f6c64b1 docs(rbrush): correct update container version`

## RED auténticos

1. Schema de actualización: el build focal falló porque no existían
   `sourceIndex`, `sourceMissing` ni `rbrushSourceIndexUnavailable`.
2. Reconciliador: configure/build falló porque no existían
   `RBrushUpdate.hpp`, `reconcileRBrushSourceUpdate` ni su target de prueba.
3. Replace/history: el coordinador rechazó `Replace` dentro del conjunto cerrado
   de operaciones RBrush y History asumía que el lado inverso debía estar
   ausente.
4. Vertical de producto: el build falló porque no existían `RBrushImportMode`,
   `priorSourceHash`, `undo()` ni `redo()`.
5. El primer rollback GREEN de fallo de reapertura chocó con residuo autenticado
   del `Replace` retenido:
   `editor.smart_import.filesystem_rename_failed: Cannot create a file when the
   file already exists`. Se corrigió derivando un command ID de History nuevo y
   la prueba se repitió.

## Matriz de comportamiento

| Caso | Resultado observado |
|---|---|
| Survivor A | Conserva `RBrushAssetGUID + PresetGUID`, nombre manual, grupo y tags; adopta tip/settings nuevos. |
| Preset B retirado | Conserva su GUID y contenido recuperable con `sourceMissing=true`. |
| Preset D nuevo | Conserva su GUID propuesto no cero cuando no colisiona. |
| Preview | Una operación `Replace`; bytes, catálogo y fuente sin cambios. |
| Cancel | Bytes iniciales y catálogo inicial exactos; fuente presente. |
| Apply | Pack nuevo exacto; fuente consumida sólo después de reapertura final. |
| Undo | Bytes iniciales exactos y content hash anterior. |
| Redo | Bytes nuevos exactos y content hash aplicado. |
| Mismos bytes de fuente | `editor.rbrush_import_transaction.source_already_imported`. |
| Baseline/hash/ruta/prior source incorrectos | Rechazo tipado antes de publicación. |
| Fuente cambia después de Preview | `editor.rbrush_import_transaction.source_stale`. |
| Reapertura final falla | `editor.rbrush_import_transaction.final_reopen_failed`; pack anterior restaurado y fuente retenida. |
| Identidad ambigua o colisión GUID/tip | Rechazo cerrado; no produce IR parcial. |

La fixture de producto conserva literalmente la pareja estable antes de Apply y
compara los vectores completos producidos por `writeRBrushContainer` después de
Apply, Undo y Redo. Los hashes del catálogo se comparan contra el baseline y el
handoff aplicado, no contra valores recalculados tolerantes.

## Verificación fresca

### Debug

- Build de los siete ejecutables: `PASS`, `/m:2 /nr:false`.
- Matriz integrada exacta: `7/7 PASS`, `149.35 s`.
  - publicación ProjectAsset: `130.87 s`
  - coordinador: `14.06 s`
  - import/update transaction: `3.91 s`
  - catálogo RBrush: `0.09 s`
  - reconciliador RBrush: `0.02 s`
  - IR RBrush: `0.01 s`
  - contenedor RBrush: `0.30 s`
- Antes del checkpoint, el focal Update/Reimport se repitió tres veces y pasó
  `1/1` en cada ejecución.

### Release

- Build de los siete ejecutables: `PASS`, `91.4 s`, `/m:2 /nr:false`.
- Matriz integrada exacta: `7/7 PASS`, `92.99 s`.
  - publicación ProjectAsset: `79.08 s`
  - coordinador: `10.55 s`
  - import/update transaction: `3.00 s`
  - catálogo RBrush: `0.06 s`
  - reconciliador RBrush: `0.12 s`
  - IR RBrush: `0.07 s`
  - contenedor RBrush: `0.07 s`

### Repository, Shipping y runtime-only

Matriz Release: `4/4 PASS`, `338.03 s`.

- scheduling contract: `0.03 s`;
- Shipping source exclusion: `2.70 s`;
- editor boundary audit: `301.64 s`;
- runtime-only target discovery: `36.36 s`.

`windows-vulkan-runtime` configuró en `28.8 s` con
`JERICO_ENABLE_ASSET_IMPORT=OFF` y `JERICO_ENABLE_EDITOR=OFF`. El build Release
completo pasó en `51.1 s` con `/m:2 /nr:false`.

El escaneo de proyectos/configuración generados no-dependency y de todos los
`.exe/.dll/.lib` runtime no-dependency dio cero coincidencias para:

- `RBrushUpdate`
- `reconcileRBrushSourceUpdate`
- `RBrushImportTransaction`
- `sourceMissing`
- `.abr`
- `.rbrush`
- `EditorAssets/Brushes`
- `Import/Brushes`

`git diff --check`: `PASS`. El staging estaba vacío. Los doce archivos
versionados ajenos y las dieciséis rutas/grupos no versionados del usuario
permanecieron fuera de los commits de producto y del checkpoint.

## C04, editor y evidencia visual

El reconciliador es Qt-free, determinista y trabaja sobre snapshots owned; el
commit filesystem es transaccional y breve. El owner interactivo futuro aún debe
demostrar scheduling fuera del hilo Qt, progreso, cancelación, presupuesto de
concurrencia/memoria, descarte de callbacks tardíos y shutdown seguro.

Este slice no abrió JericoEditor ni midió frame time. No demuestra que el síntoma
reportado de aproximadamente 1 FPS esté corregido. Tampoco usa las imágenes
conceptuales BSP/SDF/RBrush como evidencia de producto.

## Próxima acción legal

RBrush Slice 15 — Brush Library: comenzar por el RED de catálogo/biblioteca
canónica sobre los packs RBrush admitidos. No adelantar UI, Painter, runtime,
SDF/PWAS/EPLS 2.0, Terrain, partículas ni vegetation.
