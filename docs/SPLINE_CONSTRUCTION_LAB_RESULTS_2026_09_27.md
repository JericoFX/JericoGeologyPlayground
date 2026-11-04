# Spline Construction / Pipe UV Lab — resultado experimental (2026-09-27)

Las cifras iniciales siguientes describen la primera línea base. La extensión final de efectos y roturas se detalla al final.

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

## Extensión final: efectos, rotura y usos procedurales

- `ConstructionSettings` expone longitud de taper, escala de puntas, amplitud/espaciado de corrugación, vueltas de torsión y fractura con posición, separación y rugosidad. Los efectos son deterministas, usan distancia real de arco y el muestreo se ajusta al espaciado de la corrugación. Una fractura inserta dos frames exactos, elimina el tramo central, produce extremos irregulares y tapa ambos cortes; las UV por longitud siguen siendo métricas. Se rechazan huecos imposibles y torsión no entera en loops cerrados.
- La UI normal tiene 13 presets, incluidos **Corrugated Hose**, **Fractured Pipe** y **Twisted Duct**, además de controles editables para cada efecto. El cable roto de Procedural Assemblies reutiliza la misma fractura del barrido en vez de quitar muestras y dejar extremos planos.
- `Playground.SplineConstruction` y `Playground.ProceduralAssemblies` pasan. El build Release y CTest completo pasan **21/21**. `Evidence/spline-effects-final` y `Evidence/spline-effects-final-b` contienen 13/13 capturas Vulkan 1280×960 idénticas y hashes de malla/UV coincidentes entre corridas; las tres capturas nuevas se inspeccionaron en tamaño original. Corrugated Hose: 329 frames, 8.883 vértices, 17.104 triángulos, 3,23 ms CPU en esta corrida. Fractured Pipe: 1.404 vértices, 2.704 triángulos, 0,55 ms. Twisted Duct: 125 vértices, 196 triángulos, 0,08 ms. Estas duraciones son casos aislados y no presupuestos de producción.
- La misma ruta ya puede ensayar aislamiento de tubería, mangueras corrugadas, cable trenzado visual, conductos retorcidos y roturas de cables/caños. Para una herramienta del Engine faltan junctions, autoría de varios perfiles/materiales por tramo, colisiones de efectos contra sí mismos, edición visual directa de cortes y regeneración incremental. Este laboratorio no crea otra autoridad de spline, cook ni prefab.

## Ajuste de grietas a la superficie real (2026-11-04)

La revisión reprodujo dos defectos del botón **Add crack / gouge**: un tubo de radio 0,04 m y escala 0,15 colocaba el centro del cortador a 0,04 m, aunque su superficie estaba a 0,006 m; un barrido sin tapa también admitía la propuesta y fallaba después de modificar la receta. El RED conserva ambas comprobaciones en `Evidence/spline-authoring-2026-11-04/red`.

`Construction::proposeSplineDamage` reutiliza `SurfaceRay` y el BVH existente para encontrar la superficie generada, orientar el cortador y medir la pared posterior. Por eso contempla perfil, escala, ancho por punto, taper, nervaduras, rotación y torsión sin duplicar sus fórmulas. La propuesta acota ancho/profundidad respecto a la pared medida y reduce la longitud cuando el relieve no cabe en el cortador planar compartido. Examina quince estaciones por intento, con ocho intentos como máximo: es una comprobación local muestreada, no una garantía contra toda autointersección o perforación entre muestras. Conserva `PA::ProceduralSpline`, `PathDamageRule`, `DamagePathCutter` y su formato guardado.

La UI calcula una candidata con los controles actuales y sólo añade el área después de construir un corte que cambie la malla. Un host abierto, una separación sin superficie, falta de clearance o un corte sin efecto muestran un diagnóstico y conservan las operaciones anteriores. Los daños ya guardados permanecen editables en sus coordenadas originales; no se reubican al cambiar el perfil.

**Verificación automática:** `green-1` registra build Release con salida 0 y las suites `Recovered.PlaygroundSplineConstructionTests` y `Recovered.PlaygroundSplineDamageIOTests`, 2/2 PASS en 0,91 s, sin deriva de los inputs registrados. Trece casos comprueban corte real, sólido cerrado, interior fresco, replay y restitución exacta al desactivar; cubren los seis perfiles, escalas 0,15/3, ancho por punto, taper, nervaduras, torsión/rotación y curva. La regresión sitúa el centro en 0,006 m y rechaza el host abierto. En el tubo hueco de pared 0,008 m la profundidad propuesta fue 0,003171 m. El caso con nervaduras redujo la longitud propuesta a 0,043493 m.

**Verificación nativa:** el candidato integrado `1682DFF3435EC9F2B4465F07D6F3718651E8A89D104823122EEEF0CF12B1B703` ejecutó `--spline-construction-test` con salida 0 y generó 44 PNG originales de 1280×960 en `Evidence/spline-authoring-2026-11-04/native-1`. Sus 326 inputs y el ejecutable no cambiaron durante esa pasada. El comando conserva los dieciséis casos anteriores y añade siete parejas con la misma función de propuesta y roundtrip de archivo. Se inspeccionaron originales del tubo pequeño, hueco, viga, nervaduras y curva: muestran corte real, todavía con aspecto mecanizado. La toma de detalle del conducto torsionado quedó de canto; necesita otra vista antes de aceptar ese caso visualmente.

**Interacción real:** `Evidence/mesh-modifiers-2026-11-04/ui-1/18-spline-open-rejected-0.jpg` registra el rechazo del tubo sin tapa con cero áreas y 1.272 triángulos. Tras restaurar las tapas, el botón produjo una regla editable y 2.218 triángulos frente a 1.296 del tubo intacto. La edición de Profile scale a 0,15 y posterior creación de daño también se verificó en la ventana normal: 2.220 triángulos y un área. Las capturas 19, 22 y 23 conservan esos estados. Son JPEG originales del cliente, no imágenes lossless. El ejecutable terminó con salida 0 y hash intacto. Durante este recorrido cambió únicamente `Tests/RocksIntegrationTests.cpp` del manifiesto compartido; el registro conserva explícitamente esa deriva ajena, que no es input del ejecutable gráfico. No se atribuyen 326 inputs idénticos a esta segunda pasada.

**Pendiente de aquella tanda:** mejorar la forma de grieta, revisar los demás originales y la toma del conducto torsionado, teclado/DPI y regresiones finales de consumidores. La actualización siguiente corrige el estado al cambiar de preset y añade cortes/relieves espaciales. No se declara Beauty, Undo/Redo general de splines, paridad de todos los destructores ni cierre del goal.

## Curvas espaciales y relieves — 2026-11-04

Publicado en `5281f1da22ed543b9e739121bf404edf5d73124d`. La misma operación conserva una `PA::ProceduralSpline` opcional, su proyector, IDs, puntos, handles, anchura y roll. El barrido permite retirar volumen o añadir un relieve unido al receptor. `DecalSurface` identifica lo añadido sin convertirlo en material interior expuesto. El ruido se evalúa por distancia y es periódico en recorridos cerrados; el test del anillo verifica conectividad y característica de Euler cero.

Uso: añadir una grieta, elegir **Convert to 3D spline** y editar los puntos XYZ; **Surface operation** permite **Cut / groove** o **Raised decal**. Guardar/cargar conserva la elección. El formato v3 protege el nuevo modo de lectores antiguos; v1/v2 siguen admitidos. El cambio de preset limpia el mensaje de estado anterior.

Cuatro suites Release y cuatro con AddressSanitizer pasan. Se comprobó carga real del runtime, imports de cuatro ejecutables e instrumentación en DamageField, DamageGeometry, SplineConstruction, SplineDamageIO y Manifold. Las uniones de tramos se simplifican después de unificar su procedencia, sin aumentar la tolerancia del kernel. El test de cubo de ocho vértices verifica una única componente, volumen acotado, identidad, localidad y conservación del volumen entre corte y unión.

`Evidence/spline-authoring-2026-11-04/spatial-decal-native-final2` contiene 48 vistas del cubo y tubo curvo: antes/después/off/replay. Build, cuatro suites y ejecución nativa terminaron en cero, con 344 inputs sin deriva. Binario `239B678B9B07835C4975FA76A040E1656D0AF42E0B0685A0165E65BEA0307FD4`. La autoría se recorrió en la ventana mediante carga v2, cambio de modo, save/load v3 y desactivación. El corte y el relieve se distinguen geométricamente; los originales conservan las facetas y los tramos que todavía necesitan mejor ajuste a la superficie.

**Pendiente:** calidad visual natural, adaptación automática continua al receptor, otros modos del contrato Decal3D, UI a diferentes escalas/tamaños, Undo/Redo y aceptación global. Esta entrega implementa corte y unión volumétrica; no representa los modos de máscara, MeshInsert ni toda la destrucción física.
