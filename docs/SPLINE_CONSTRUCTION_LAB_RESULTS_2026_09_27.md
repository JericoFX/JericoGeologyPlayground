# Spline Construction / Pipe UV Lab — resultado experimental (2026-09-27)

## Estado de verdad

- **IMPLEMENTED:** `Source/SplineConstruction.hpp/.cpp` toma `PA::ProceduralSpline` del Engine como único documento de path. Evalúa Polyline, Catmull-Rom y Bézier, subdivide según desviación/ángulo/paso, acumula distancia en metros, transporta el frame por double-reflection RMF, corrige la costura de un loop cerrado y aplica el roll después. Un solo barrido usa perfiles Circle, Ring, Rectangle, Rounded Rectangle, U/Channel e I-Beam; genera tapas, UV analíticas y piezas repetidas por longitud de arco. `Source/SplineLabUI.cpp` expone diez presets, controles de puntos/perfil/calidad/UV/repetición y seis vistas dentro del ejecutable normal.
- **AUTOMATED VERIFIED:** build Release, 18/18 CTest, prueba focal `Playground.SplineConstruction` y comando `--spline-construction-test` con replay de hashes de malla y UV. Las pruebas cubren RMF en inflexión casi vertical, vertical exacta, curva casi recta, reverse Bézier y loop cerrado; UV longitudinal proporcional a distancia, V circular igual a `2*pi*r`, costura duplicada, índices/tapas/triángulos finitos de los seis perfiles, repetición sobre control points irregulares y rechazo de entradas inválidas.
- **VISUALLY VERIFIED:** se abrió `JericoGeologyPlayground.exe` normal, se entró a **Spline Construction**, se cambió Pipe a Rectangular Duct y `Add Point` modificó la malla y las métricas en vivo. Diez capturas Vulkan originales de 1280×960 están en `Evidence/spline-construction/`, incluidos S-curve, UV checker, seam real, RMF vertical, ducto, cable tray con 13 crossbars, I-Beam y curva cerrada de alta curvatura.
- **EXPERIMENTAL:** el Smooth Bend se obtiene mediante Catmull-Rom/Bézier y calidad adaptable. En curvas cerradas hay facetas visibles a calidad Preview; Production/Hero reduce la desviación. Los crossbars son prismas geométricos de prueba, aún sin fitting mecánico final.
- **FAILED/REJECTED:** el frame `worldUp` recalculado en cada muestra invierte el perfil 180° cerca de una inflexión vertical; el RED inicial lo reprodujo. Se retuvo double-reflection RMF. El primer Seam Debug marcaba límites de tiles, no la costura; fue corregido para leer la marca de los vértices duplicados.
- **PENDING:** detección y resolución de autointersecciones en curvas extremadamente cerradas, miter/radius controlado para corners duros de Polyline, junctions Tee/Y/Cross, segmentación incremental de regeneración y evaluación en Editor/Engine. La Beauty del cliff geológico sigue parcial y high→low permanece fuera de este slice.

## Comando reproducible y medición

```powershell
cmake --build build-surface --config Release
ctest --test-dir build-surface -C Release --output-on-failure
.\build-surface\Release\JericoGeologyPlayground.exe --spline-construction-test Evidence\spline-construction
```

`Evidence/spline-construction/spline-construction-proof.json` registra por caso receta/control points, tolerancias, desviación aceptada, tiempo CPU, frames, vértices, triángulos, repeticiones, memoria geométrica aproximada, hashes de malla/UV e imagen. En esta máquina con Vulkan/RTX 3090, los diez casos midieron aproximadamente 0,06–0,28 ms de generación CPU, 17–25 frames y 125–972 vértices. Son mediciones puntuales de estos fixtures, no un presupuesto de tiempo garantizado ni una medición GPU. Las capturas se inspeccionaron a resolución original; la distorsión de UV puede volverse visible al exigir radio extremo y debe seguir investigándose.

La calidad controla tres criterios del path y dos del perfil. Los círculos derivan sus segmentos de la sagitta `e=r*(1-cos(delta/2))`, con máximo angular y límite de 256 segmentos. Si esa cota no cumple la tolerancia, la generación se rechaza con diagnóstico; tampoco se recorta silenciosamente una esquina redondeada al límite de 64 segmentos. La malla tiene presupuesto explícito de hasta 500.000 vértices y el path hasta 65.536 muestras; la UI muestra el diagnóstico cuando se excede un límite. Los identificadores del segmento se conservan en los frames y permiten investigar regeneración localizada, pero la implementación actual reconstruye el resultado al cambiar un control.

## Handoff de propiedad hacia JericoEngine

No se escribió ningún archivo de `F:\JericoEngine`. Este laboratorio consume el tipo `ProceduralSpline` existente y compila su fuente en el Playground para comparar su contrato actual sin cambiarlo. La integración futura corresponde a los owners existentes:

| Evidencia del Playground | Owner del Engine a considerar |
| --- | --- |
| Identidad de puntos/segmentos, evaluación y arc length | `ProceduralSpline` / Path Math |
| Double-reflection RMF, roll y seam cerrado | Path Math y Procedural Studio Sweep |
| Perfil 2D, barrido genérico, tapas y smooth bend | Procedural Studio / Geometry IR |
| UV analítica métrica y costura duplicada | GenerateUV |
| Crossbars repetidos por distancia real | RepeatAlongPath / ConstructionDefinition |
| Presets y controles probados en vivo | ConstructionDefinition / Asset Browser |
| Malla validada y salida futura | Geometry IR / rmesh cook / prefab parts |

No se propone otra autoridad de spline, prefab ni cook. Las juntas de una red futura necesitarían un nodo de construcción explícito, perfiles de corte compatibles y una resolución de unión/UV independiente del barrido de tramos; este experimento deja tramos con posición, orientación, perímetro y UV métricos para probar esa arquitectura después.
