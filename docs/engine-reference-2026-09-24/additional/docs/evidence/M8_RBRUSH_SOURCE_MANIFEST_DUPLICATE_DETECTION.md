# M8 RBrush Source Manifest / Duplicate Detection Evidence

Fecha: `2026-08-12`

Estado del slice: `PASS`

Estado superior: M8 `ACTIVE / PARTIAL`; C04 Jobs/Async `PARTIAL`;
JericoEngine 1.0 `ACTIVE / NOT COMPLETE`.

## Alcance demostrado

RBrush Slice 13 demuestra una sola capacidad: el hash canónico de la fuente
queda proyectado desde el manifiesto de cada pack al catálogo del proyecto y un
segundo drop de los mismos bytes se clasifica como ya importado antes de crear
Preview u operaciones de publicación.

No implementa Update/Reimport, UI, painter, runtime RBrush, Terrain,
PWAS/EPLS 2.0, partículas, virtual texturing ni Animation/Rigging.

## Producto y contratos reutilizados

- `RBrushPackIR` y su manifiesto canónico son la única autoridad de fuente.
- `Jerico::Guid`, `RBrushDigest`, `Result<EngineError>` y el catálogo existente
  se reutilizan; no se agregó registro o base de datos paralela.
- `discoverProjectRBrushAssets` y la candidatura transaccional proyectan el
  resumen desde el pack ya decodificado. No releen el `.rbrush`.
- La consulta devuelve todas las coincidencias exactas en orden determinista,
  rechaza un hash cero y limita el resultado a 4096 entradas.
- Un manifiesto individual rechaza dos fuentes con el mismo hash.
- `RBrushImportTransaction::prepare` consulta el baseline después de anclar y
  hashear la fuente, y antes de serializar el contenedor o crear operaciones.

Commits de producto:

- `9000e05 fix(asset-pipeline): reject duplicate rbrush source hashes`
- `fa9fc05 feat(editor): query canonical rbrush source manifests`
- `8ddd56c feat(editor): prevent duplicate rbrush source imports`

Diseño y plan:

- `702969e docs(rbrush): define source duplicate detection`
- `9cd3264 docs(rbrush): plan source duplicate detection`

## RED auténticos

1. Invariante del manifiesto: `asset_pipeline.rbrush_ir` falló en
   `RBrushIRTests.cpp:470` porque el mismo hash todavía era admitido dos veces.
2. Proyección/query: la compilación falló porque `ProjectRBrushAsset` no tenía
   `sourceManifest` y no existía `findProjectRBrushSourceMatches`.
3. Segundo drop: `editor.rbrush_import_transaction` falló en
   `RBrushImportTransactionTests.cpp:505` porque el duplicado alcanzó Preview.

El primer intento GREEN usó `std::ranges::fold_left`; MSVC 14.44 lo rechazó
porque esa API no está disponible. Se reemplazó por un bucle acotado simple y
se repitió la compilación.

## Matriz de comportamiento

| Caso | Resultado observado |
|---|---|
| Primera importación | Publica un pack, un source summary y un preset; consume la fuente según retention. |
| Segundo drop, bytes exactos | Error tipado `editor.rbrush_import_transaction.source_already_imported`; fuente retenida; no crea `Duplicate.rbrush`; catálogo sigue con un pack/un preset. |
| Fuente con un byte distinto | No es duplicado exacto; llega a Preview y Cancel no escribe `Changed.rbrush`. |
| Hash de consulta cero | Rechazado como identidad inválida. |
| Hash distinto | Resultado vacío. |

Slice 14 mantiene ownership de Update/Reimport y de la supervivencia de
`PresetGUID` frente a una fuente cambiada.

## Verificación fresca

### Debug

- Build focal de los cinco ejecutables: `PASS`.
- Matriz integrada exacta: `5/5 PASS`, `157.40 s`.
  - publicación ProjectAsset: `141.68 s`
  - coordinador: `14.10 s`
  - import transaction: `1.50 s`
  - catálogo RBrush: `0.06 s`
  - IR RBrush: `0.01 s`
- El focal de segundo drop pasó tres veces: `1.46 s`, `1.46 s`, `1.60 s`.

### Release

- Build de los cinco ejecutables: `PASS`, `75.1 s`.
- Matriz integrada exacta: `5/5 PASS`, `91.66 s`.
  - publicación ProjectAsset: `80.01 s`
  - coordinador: `10.21 s`
  - import transaction: `1.19 s`
  - catálogo RBrush: `0.07 s`
  - IR RBrush: `0.03 s`

### Repository, Shipping y runtime-only

Matriz: `4/4 PASS`.

- scheduling contract: `0.02 s`, 253 tests, 53 programados, 13 seriales;
- Shipping source exclusion: `2.42 s`, cero authoring roots;
- editor boundary audit: `309.33 s`, siete runtime proofs;
- runtime-only target discovery: `37.19 s`, 43 proyectos activos y cero
  targets de autoría.

El wrapper inicial agotó 244 s mientras el boundary audit seguía construyendo.
Se conservó el proceso existente y `LastTest.log` registró finalmente los cuatro
tests como `Passed`; el timeout no se cuenta como PASS por sí mismo.

`windows-vulkan-runtime` configuró (`37.0 s`) y compiló Release (`66.9 s`).
El escaneo de proyectos/configuración generados y `.exe/.dll/.lib` runtime dio
cero coincidencias para:

- `ProjectRBrushSourceManifestEntry`
- `findProjectRBrushSourceMatches`
- `RBrushImportTransaction`
- `.abr`
- `.rbrush`
- `EditorAssets/Brushes`
- `Import/Brushes`

`git diff --check`: `PASS`. El staging quedó vacío tras los commits de producto.
Los once archivos versionados ajenos y los dieciséis grupos/rutas no
versionados del usuario permanecen fuera de los commits.

## C04 y responsividad

La consulta de duplicados es in-memory, determinista y acotada. La lectura y el
hash del source pueden ser trabajo pesado cuando exista un owner Qt/UI y siguen
obligados al contrato C04: worker/proceso aislado, presupuesto Interactive,
progreso, cancelación, límites, descarte de callback tardío y commit breve en el
hilo dueño.

Este slice no abrió JericoEditor ni midió frame time. No aporta evidencia de que
el síntoma reportado de aproximadamente 1 FPS esté corregido; esa aceptación
requiere el vertical de responsividad/cancelación del owner C04 correspondiente.

## Próxima acción legal

RBrush Slice 14 — Update/Reimport: escribir el RED que modifica una fuente ya
conocida y exige actualizar el pack sin cambiar los `PresetGUID` supervivientes,
sin adelantar UI, runtime o subsistemas posteriores.
