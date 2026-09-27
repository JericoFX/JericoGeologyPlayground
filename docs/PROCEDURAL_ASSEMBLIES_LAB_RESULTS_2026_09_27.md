# Procedural Assemblies Lab — resultado 2026-09-27

## Estado

- **IMPLEMENTED:** núcleo compartido `Path/ArcLength/RMF -> Sweep/Offset/Repeat -> Semantic Assembly -> Damage -> LOD` dentro de JericoGeologyPlayground; catorce presets en la aplicación normal: tubería/uniones, vía normal/rota, baranda normal/rota, muelle normal/dañado, catwalk, beam bridge, Warren/Pratt/Howe, puente colgante y puente de cuerda/tablas. La UI edita recorrido, dimensiones, daño probabilístico/cluster, selección por lista o clic, estado manual, protección y LOD. No se modificó JericoEngine.
- **AUTOMATED VERIFIED:** Release build; 19/19 CTest; generación de las catorce familias con IDs/estados invariantes entre LOD 0 y 2; prueba de huecos reales, fit de tablas, estaciones por longitud de arco, gauge, parábola/hangers, catenaria, rechazo de parámetros inválidos y presupuestos. `--procedural-assemblies-test Evidence/procedural-assemblies` produjo 17 capturas Vulkan de 1280×960, recetas, métricas, IDs y hashes. El pick de un triángulo visible se resolvió a una pieza semántica. Segunda ejecución: 17 PNG idénticos por SHA-256 y 17 pares de hashes de malla/estado idénticos.
- **VISUALLY VERIFIED:** se inspeccionaron las capturas de tubería/uniones, vía normal/rota, baranda rota, muelle con tablas ausentes/rotas, Warren, puente colgante y puente de cuerda normal/dañado. Los huecos y miembros separados son visibles. Esta inspección es del viewport Vulkan de evidencia, no una sesión manual completa de la UI.
- **EXPERIMENTAL:** geometría estática y simplificada. Los trusses prueban tres patrones visuales, sin afirmar cálculo estructural. Los estados deformados usan transformaciones acotadas; la reconstrucción tras daño o edición de spline sigue siendo completa y sincrónica.
- **PENDING:** prueba manual de cada control y selección por clic en la ventana normal; promoción de piezas a Jolt; grafo físico de cuerdas/SoftBody; Strands & Webs; colisiones de assembly y medición de runtime móvil. El Engine expone física rígida por sus contratos, pero el Playground no enlazó Jolt directamente ni declara SoftBody integrado. La Beauty geológica original y high→low permanecen en su estado previo.

## Uso y evidencia

Abrir `JericoGeologyPlayground.exe` y elegir **Procedural Assemblies** en la barra superior. Seleccionar un preset, editar puntos XYZ/roll y parámetros; en **Semantic parts** elegir una pieza o hacer clic en la geometría del viewport y escoger su estado. Cambiar LOD para comprobar que la misma pieza mantiene ID y daño.

```powershell
cmake --build build-surface --config Release --target JericoGeologyPlayground PlaygroundProceduralAssembliesTests
ctest --test-dir build-surface -C Release --output-on-failure
.\build-surface\Release\JericoGeologyPlayground.exe --procedural-assemblies-test Evidence\procedural-assemblies
```

Artefactos retenidos: `Evidence/procedural-assemblies/procedural-assemblies-proof.json`, 17 PNG en el mismo directorio, `Evidence/2026-09-27-assemblies-full-ctest.log` y el log de build Release. El JSON incluye receta, puntos, dimensiones, hashes, estados, IDs, estaciones y métricas por caso.

| Caso | Vértices | Triángulos | Partes | CPU generación |
|---|---:|---:|---:|---:|
| Pipe + Couplings | 2.085 | 4.024 | 8 | 1,02 ms |
| Railway | 1.516 | 2.644 | 47 | 0,98 ms |
| Wooden Pier | 644 | 992 | 40 | 0,32 ms |
| Suspension Bridge | 5.782 | 10.608 | 108 | 2,57 ms |
| Rope/Plank Bridge | 8.980 | 17.224 | 54 | 3,84 ms |

Mediciones puntuales del muelle: construcción inicial **0,402 ms**, reconstrucción completa por cambiar una tabla **0,311 ms** y por mover un punto de spline **0,278 ms**. Son muestras de una ejecución, no un benchmark estadístico ni prueba de actualización incremental. Los cuerpos/constraints promovidos son **0/0**.

## Límites observados

- El chequeo exacto de autointersección de offset está acotado a 4096 frames y rechaza una entrada mayor con diagnóstico. No salta silenciosamente el chequeo.
- IDs de pieza dependen de familia, rol, índice y lado. Cambiar teselación/LOD no los altera; cambios que reordenan semánticamente estaciones o reducen el número de piezas pueden dejar un override antiguo sin pieza correspondiente.
- Las mallas se revisaron visualmente en encuadres fijos. Falta la prueba interactiva completa de clic, edición, scroll y cambios sucesivos de preset por una persona en la ventana normal.
- Los cables rotos generan huecos deterministas y la catenaria está acotada; no hay simulación de cables ni tearing físico.
