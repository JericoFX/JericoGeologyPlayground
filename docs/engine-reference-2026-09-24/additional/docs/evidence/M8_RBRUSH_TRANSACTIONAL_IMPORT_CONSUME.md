# M8 RBrush Slice 12 — Transactional Import + Consume

Estado: `PASS` para RBrush Slice 12 solamente. M8, C04 Jobs/Async y el Goal
maestro JericoEngine 1.0 permanecen `ACTIVE / PARTIAL`.

## Alcance demostrado

JericoEditor puede admitir un pack canónico `.rbrush` desde el inbox fijo
`Import/Brushes/Inbox`, publicarlo en `EditorAssets/Brushes` por el único
catálogo y el `ProjectTransactionCoordinator`, reabrir el archivo final con el
reader normal y recién entonces consumir la fuente exacta cuando el manifiesto
autoriza `EmbedAndConsume`.

El contrato demostrado incluye:

- identidad de contenido calculada sobre el IR canónico serializado;
- descubrimiento estricto sólo bajo `EditorAssets/Brushes/**/*.rbrush`;
- GUID, revisión, hash, path, límites de cantidad/bytes y rechazo de links;
- Preview sin escrituras y Apply por journal autenticado/crash-safe existente;
- publicación Create exclusivamente con baseline `revision=0`, `hash=0`;
- reapertura final y comparación exacta de GUID, hash e IR completo;
- fuente anclada por path inmediato, identidad persistente, bytes y SHA-256;
- consumo posterior a admisión, nunca anterior;
- rollback autenticado de la creación si falla la verificación final;
- retención segura de fuente y pack ante cancelación o fallo posterior a la
  admisión;
- cancelación previa que descarta sólo el Preview y no escribe nada.

No se creó un segundo catálogo, journal, formato o archivo temporal adyacente.
No se agregó UI, painter, update/reimport, duplicate detection ni parser de
autoría al runtime.

## RED / GREEN

RED de identidad canónica:

```text
fatal error: public Jerico/AssetPipeline/RBrushHash.hpp not found
```

RED de catálogo:

```text
fatal error: Jerico/Editor/RBrushProjectCatalog.hpp not found
```

RED de publicación:

```text
AutomationProjectAssetKind::RBrush is not a member
ProjectTransactionRoot::EditorAssets is not a member
ProjectTransactionTargetKind::EditorAsset is not a member
```

RED de importación transaccional:

```text
fatal error: RBrushImportTransactionTestAccess.hpp not found
```

El cierre source-free agregó un RED independiente: una raíz de autoría con un
asset cocinado válido todavía era aceptada por el gate anterior.

```text
repository.shipping_source_exclusion FAILED (2.41 s)
Shipping gate accepted authoring root: EditorAssets
```

El GREEN mínimo extendió la frontera común de Shipping para rechazar
`EditorAssets` e `Import`, además de probar rechazo explícito de `.abr` y
`.rbrush`.

## Prueba de estado y fallos

La prueba end-to-end exige el siguiente orden observable:

```text
Prepare: source present, destination absent, catalog unchanged
Apply:   final pack present and reopened through the normal reader
Consume: exact anchored source absent only after final verification
Catalog: one exact GUID/revision/content-hash/path row
```

Matriz negativa demostrada:

- hash de manifiesto inválido: sin publicación ni consumo;
- política de retención inconsistente: sin publicación ni consumo;
- destino fuera del root o con nombre inválido: rechazo previo;
- cancelación antes de Apply: source presente, destination ausente;
- identidad/bytes de fuente cambiados antes de Apply: sin publicación;
- colisión de destino: no sobrescribe el pack existente;
- fallo inyectado al reabrir el final: rollback autenticado, sin pack;
- cancelación inmediatamente después de admisión: pack y fuente retenidos;
- fuente cambiada inmediatamente antes del consumo: pack y fuente retenidos
  con diagnóstico.

El focal se ejecutó tres veces consecutivas como procesos CTest separados:
`1.20 s`, `1.20 s` y `1.19 s`, todas `PASS`. Una repetición final con comparación
del IR completo pasó en `1.23 s`.

## Gates finales

```text
Debug build focal, 5 targets: PASS (37.2 s)
Debug integrated CTest: 5/5 PASS (145.67 s)
  project asset publication: 130.49 s
  project transaction coordinator: 13.76 s
  RBrush import transaction: 1.22 s
  RBrush project catalog: 0.08 s
  RBrush canonical IR: 0.01 s

Release build focal, 5 targets: PASS (21.5 s)
Release integrated CTest: 5/5 PASS (91.20 s)
  project asset publication: 79.26 s
  project transaction coordinator: 10.59 s
  RBrush import transaction: 0.98 s
  RBrush project catalog: 0.07 s
  RBrush canonical IR: 0.03 s

Shipping exclusion RED: expected FAIL (2.41 s, EditorAssets accepted)
Shipping exclusion GREEN: 1/1 PASS (2.25 s)
windows-vulkan-runtime configure: PASS (31.6 s)
windows-vulkan-runtime Release completo: PASS (50.7 s)
Scheduling + Shipping + boundary + runtime discovery: 4/4 PASS (315.38 s)
  scheduling contract: 0.02 s
  Shipping source exclusion: 2.38 s
  editor boundary audit: 287.06 s
  runtime-only target discovery: 28.27 s
runtime-only generated graph RBrush/ABR hits: 0
runtime-only EXE/DLL/LIB RBrush/ABR hits: 0
```

La búsqueda independiente cubrió `RBrushImportTransaction`,
`RBrushProjectCatalog`, `EditorAssets/Brushes`, `Import/Brushes`, `.abr` y
`.rbrush` en proyectos/configuración generados y artefactos Release. No hubo
coincidencias.

## Commits

```text
c52c063 docs(rbrush): define transactional import consume
b139cc1 docs(rbrush): plan transactional import consume
201fced feat(asset-pipeline): expose canonical rbrush identity
ccf3cd8 feat(editor): catalog project rbrush assets
3b25a5c feat(editor): publish rbrush through project authority
5fb05d0 feat(editor): transact and consume rbrush imports
```

## Límites y continuidad

- La publicación genérica tarda `79-130 s` en estas matrices. Esto refuerza
  C04 Jobs/Async como `PARTIAL`; no demuestra responsividad de UI ni resuelve
  todavía todos los cookers/bakes pesados.
- Este slice es autoridad Qt-free y transaccional; no contiene panel, modal ni
  prueba visual/runtime de edición, por lo que no se reclama evidencia visual.
- Update/reimport y duplicate detection siguen fuera de Slice 12.
- Virtual Texturing/páginas MegaTexture permanece diferido a JericoEngine 1.1.
- Terrain, PWAS/EPLS 2.0, JericoParticles, Vegetation y Animation/Rigging no se
  adelantaron.

Estado: `PASS` (RBrush Slice 12 solamente).

Resultado: un `.rbrush` se publica por la autoridad canónica, se reabre y
verifica completo, y sólo entonces puede consumirse su fuente exacta.

Pruebas ejecutadas: focales Debug repetidas, integración Debug/Release,
Shipping source exclusion con RED/GREEN, runtime-only Release, scheduling,
boundary audit, target discovery y búsquedas independientes de leakage.

Evidencia: este documento, los tests versionados y los commits listados.

Faltantes: Slice 13 y posteriores; M8/C04/Goal 1.0 no están completos.

Riesgos: la ruta genérica de publicación sigue siendo demasiado costosa para
uso interactivo; debe medirse y sacarse del hilo Qt bajo el contrato C04.

Siguiente slice: RBrush Slice 13 — Source Manifests / Duplicate Detection.
