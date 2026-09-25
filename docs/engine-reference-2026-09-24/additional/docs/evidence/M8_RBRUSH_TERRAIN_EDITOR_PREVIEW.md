# M8 RBrush Terrain Editor Preview

## Estado

- Task 9, brush-first Terrain workspace: `PASS`.
- RBrush Slice 24: `PARTIAL`.
- M8, C04 global y el Goal maestro: `ACTIVE / PARTIAL`.

Este checkpoint demuestra el editor y el preview Vulkan cocinado. No reclama
todavía Delta `.epak`, runtime/Shipping source-free, equivalencia
incremental+compact contra clean cook ni el cierre completo de Slice 24.

## Comportamiento demostrado

- El mismo RBrush admitido crea Height y Material Weight sobre el único
  viewport World. No existe canvas ni renderer Terrain paralelo.
- El rayo Terrain se obtiene directamente de la cámara compartida; no requiere
  golpear primero la geometría JericoBSP.
- El candidato se evalúa y cocina fuera del hilo Qt. Qt coordina, presenta la
  generación validada y realiza el commit breve.
- Height local produce geometría adaptativa; la zona distante permanece coarse.
  Material Weight actualiza páginas Surface sin convertir todo el Terrain en
  geometría fina.
- Preview, Apply, Undo, Redo y Cancel se comprobaron mediante capturas de back
  buffer byte-exactas. Apply/Undo/Redo compensan la transacción persistente si
  el cambio visual correspondiente falla.
- El inspector vertical no necesita scroll horizontal a 460 px de ancho. Los
  controles, target, material seleccionado, layers no destructivos, estado
  `Preview` y métricas GPU permanecen legibles.
- El cursor identifica el target activo como `SCULPT` o `MATERIAL`; ya no
  presenta `HEIGHT` durante pintura de pesos.

## Métricas retenidas

Prueba integrada sobre RTX 3090, Vulkan:

```text
generation 1: 22 pages, bounds [-32, 0.000244144, -32] .. [32, 2.42704, 32]
generation 2: 28 pages, bounds [-32, 0.000244144, -32] .. [32, 2.42704, 32]
height_samples=107
weight_samples=58
triangles=3584
draws=28
height_ui_gap_ms=29
weight_ui_gap_ms=23
```

El límite de responsividad del contrato es `<100 ms`; ambas operaciones lo
cumplieron. El aumento de 22 a 28 páginas corresponde a la segunda región dirty
local, no a una subdivisión global del patch de 64 x 64 m.

## Evidencia visual inspeccionada a resolución original

- `images/m8-rbrush-terrain-sculpt.png`, 1920x1032, SHA-256
  `FA4A079B9CACF125C97E491B70ADAF7A1697E4BA851D3FB79D66B5BD1EA4AD70`.
  Muestra JericoEditor, el viewport World compartido, sculpt, mezcla material,
  cursor `MATERIAL`, dos layers, controles y `28 pages / 3584 tris / 28 draws`.
- `images/m8-rbrush-terrain-before-vulkan.bmp`, 1180x930, SHA-256
  `71EC84D3F914089AFA6A396672095B0720F786D5E167A13456419456F5673EB3`.
  Baseline World antes de admitir la primera generación Terrain.
- `images/m8-rbrush-terrain-height-vulkan.bmp`, 1180x930, SHA-256
  `80EBE2177603B9DCE39F30877A5EE80808C55BFB8CB6C8EE04B9685484790097`.
  Misma vista después del sculpt Height y antes de Material Weight.
- `images/m8-rbrush-terrain-after-vulkan.bmp`, 1180x930, SHA-256
  `E6FEBBF4F391FD8090FA305AA45642908C5D73BB96FB209FCB1AA56FF8C913BA`.
  Misma vista con el segundo material pintado y el cursor real.

Las capturas conceptuales `docs/showcase/images/editor-states/01..14` no se
usaron como evidencia.

## RED/GREEN observados

1. El test focal falló porque el workspace usaba un splitter horizontal y
   truncaba los valores en el inspector: `split->orientation() == Qt::Vertical`.
   El GREEN usa layout vertical, children no colapsables y cero scroll
   horizontal.
2. El candidato visual mostraba sólo el layer persistido. El RED exigió un
   segundo layer `Material Weight / Preview`; el GREEN presenta la receta
   candidata sin escribirla y conserva la selección estable del material.
3. La métrica GPU era invisible. El RED no encontró `TerrainRuntimeMetrics`;
   el GREEN muestra páginas, triángulos y draws de la generación visible.
4. El cursor reutilizado decía `HEIGHT` durante Material Weight. El RED añadió
   el label canónico al round-trip del overlay; el GREEN publica `SCULPT` o
   `MATERIAL` según el target.
5. Una segunda captura sobre el mismo path falló con
   `editor.viewport.capture_write_failed` / filesystem open failed. El fixture
   ahora elimina únicamente su archivo temporal antes de cada captura y retiene
   paths de evidencia distintos.
6. Un patch coarse de 64 m falló Navigation por un tile fijo de 64 celdas. El
   RED preserva páginas lejanas coarse; el GREEN deriva el tamaño de tile Recast
   por span de página, manteniendo `cellSize=0.25 m` y el refinamiento local.

## Verificación

Build focal integrado:

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target `
  JericoTerrainPaintTransactionTests JericoViewportSurfacePickingTests `
  JericoRBrushEditorWorkbenchTests JericoTerrainPaintJobTests `
  JericoTerrainPaintWorkbenchTests JericoTerrainViewportInteractionTests `
  JericoBspWorkbenchTests JericoTerrainRecipeTests `
  JericoTerrainEvaluatorTests JericoTerrainCookerTests `
  JericoTerrainMaterialRenderTests
```

Resultado: `PASS`.

Prueba integrada y captura:

```powershell
$env:JERICO_CAPTURE_EVIDENCE_DIR='F:\JericoEngine\docs\evidence\images'
& build/windows-vulkan-editor/QtRuntime/JericoRBrushEditorWorkbenchTests/Debug/JericoRBrushEditorWorkbenchTests.exe --world-surface-pick
```

Resultado: exit `0`; dos generaciones Vulkan preparadas, capturas retenidas y
sin `check failed`.

Matriz focal final:

```text
editor.terrain_paint_transaction
editor.viewport_surface_picking
editor.rbrush_editor_workbench
editor.terrain_paint_job
editor.terrain_paint_workbench
editor.terrain_viewport_interaction
editor.jericobsp_workbench
terrain.recipe
terrain.container
terrain.evaluator
terrain.cooker
render.terrain_material
```

Resultado final fresco: `12/12 PASS`, cero fallos, `107.60 s`.

## Faltantes que mantienen Slice 24 PARTIAL

- Task 10: Cook Changed + Package Changed/delta `.epak` sin full recook oculto.
- Equivalencia incremental+compact contra clean full cook.
- Mount base+overlay y replay Vulkan runtime/Shipping dos veces.
- Inventario source-free sin `.terrain`, RBrush, ABR, Qt ni autoría.
- Prueba runtime de colisión Jolt y navegación Recast/Detour sobre el tile
  cambiado.
- Captura final Shipping y métricas de CPU/GPU frame, package delta y cache.

## Siguiente acción legal

Ejecutar Task 10 del plan Terrain: comenzar por el RED de fixture incremental,
delta `.epak`, mount base+overlay, compact equivalence y Shipping source-free.
No abrir EPLS, Sparse SDF stamps, Vegetation, JericoParticles ni M9 antes de
cerrar Slice 24.
