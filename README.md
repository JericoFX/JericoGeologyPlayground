# Jerico Geology Playground

Laboratorio independiente en C++20, Diligent/Vulkan e ImGui para experimentar con las operaciones identificadas en la review de addons de Blender. El [Goal local](GOAL.md) enlaza la [copia verificable de 280 documentos de referencia del Engine](docs/engine-reference-2026-09-24/README.md) y su manifiesto SHA-256; el Engine se mantiene en lectura y Blender no se ejecuta.

## Abrir

Ejecutar `build/Release/JericoGeologyPlayground.exe` o `Run.cmd`.

La compilación vigente está en **build/Release**; los otros árboles conservan pruebas históricas. Para probar el cliff, elegir `Plane cliff`, `Solid cliff` o `BSP geology` en Presets y pulsar `Fit`. El estado verificable más reciente está en `Evidence/2026-09-24-geology-math-checkpoint.md`.

1. `New` crea un cubo sin operaciones. Elegir Cube, Sphere, Cylinder, Cone, Plane/Wall o BSP wall. Esta última es un fixture construido con el BSP real del engine, no la selección de su editor productivo.
2. Elegir una operación y pulsar `Add layer`. Activar, seleccionar, duplicar, borrar o reordenar en el panel izquierdo.
3. Ajustar parámetros y seed en el derecho. Durante la edición se genera una vista previa; al terminar se reconstruye la resolución final.
4. `Inspect` permite ver el resultado final, hasta la operación seleccionada o su campo/máscara. `View` ofrece Lit, Normals, Wireframe y Field.
5. Arrastrar con botón izquierdo para orbitar, medio/derecho para desplazar, rueda para zoom. `Fit` encuadra la geometría actual.
6. `Save` escribe la versión de la receta activa en JSON. `Load` acepta v1–v16; las v1–v9 se migran a v10 y las v10–v16 conservan su versión. `Undo`/`Redo` recuperan estados de la receta; la cámara no forma parte de ella.

Los catorce presets son recetas del mismo stack y todos sus parámetros son editables. Incluyen Plane cliff, Solid cliff, BSP geology, Angular wear, Layered rock v9, Crater rock y Slate source experiment. `Reset layer` recupera los valores iniciales de esa operación; `Reset recipe` crea una receta vacía. El preset sedimentario usa resolución final 32 para respetar el presupuesto después de los cortes y la subdivisión.

## Código real del engine utilizado

Se compilan directamente estos archivos originales, sin copiarlos ni modificarlos:

- `F:/JericoEngine/Engine/Platform/Source/WindowWin32.cpp`: ventana nativa y eventos.
- `F:/JericoEngine/Tools/ProceduralAuthoring/Source/Geology/GeologyNoise.cpp`: `evaluateGeologyFbmReference`, usado por la generación CPU y por la comprobación GPU.
- `Geometry.hpp`: `TriangleMesh` y `Vec3d` reales, junto con sus headers de Core/Jobs.
- `SolidEvaluatorManifold.cpp`, `BspDocument.cpp` y `Guid.cpp`: validación de sólidos, cortes cerrados por planos, subdivisión lineal y regiones persistentes. Se enlaza Manifold **3.5.2** ya instalado para el engine.
- `TriangleBvh.hpp`: distancia a triángulos para transferencia de muestras y detección de bordes de estratos. Es un header interno del engine; debe revisarse ese límite antes de adoptar el código.
- `CancellationToken`/`CancellationSource`: la misma señal llega desde el worker a la evaluación y al adaptador sólido.

El ejecutable enlaza los binarios Diligent e ImGui ya construidos para JericoEngine y utiliza sus mismos headers. `Evidence/source-provenance.json` registra rutas, commits disponibles y hashes de los inputs principales y del ejecutable. Los binarios preexistentes tienen hashes propios; no se supone que fueron compilados en el HEAD actual del engine.

El host de rendering es pequeño y local: no utiliza el `RenderDevice` productivo, que arrastra contratos de assets/runtime. La cámara, los campos Voronoi y el stack son implementación local. No se importaron el mesher M8, Surface Nets/Hermite/QEF ni el sistema de geología completo. No hay dependencia de Blender o de sus addons.

## Operaciones y límites

| Operación | Semántica |
|---|---|
| Transform | Escala, rota y traslada posiciones; transforma también el vector de warp acumulado. |
| Normal / FBM | Desplaza sobre la normal calculada al llegar a la operación. Usa el FBM real del engine. |
| Vector / FBM | Tres campos con seeds distintas desplazan XYZ. Amplitud por componente, no límite de longitud del vector. |
| Voronoi | F1, F2 o distancia a bisectrices; threshold y width remapean el modo edge. Búsqueda local de vecinos, no diagrama global exacto. |
| Layer formation | Copias orientadas del volumen superficial de entrada, comprimidas en slabs; espesor, separación, escala y desplazamiento lateral variables. Sin unión Boolean. |
| Domain warp | Modifica coordenadas de muestreo para los campos siguientes. Solo, no cambia posiciones. El offset sigue las transformaciones posteriores del objeto. |
| Surface / field fracture | Desplazamiento negativo por campo Voronoi. No corta sólidos ni produce fragmentos físicos. |
| Mesh neighbor relax | Promedio iterativo de posiciones vecinas. Recalcula normales por iteración; puede reducir volumen. |
| Shared macro warp | Deforma la geometría mediante un campo vectorial común de baja frecuencia, con escalas de coordenadas y amplitud XYZ independientes. |
| Coherent layer formation | Opción de Layers: espesores espaciales acotados, correlación entre estratos y drift correlacionado de posición, rotación y escala. |
| Structural planes | Cortes geométricos cerrados mediante el adaptador real del engine, con regiones de corte y subdivisión opcional posterior. |
| Macroform envelope | Intersección con planos de soporte. Box exacto; ellipsoid/rounded box aproximados por 26 planos; cylinder por 8 planos laterales y 2 tapas. |
| Layer edge breakup | Ruido condicionado por distancia geométrica a las caras laterales del mismo estrato. Protege el centro fuera de la franja; requiere Layers previo. |
| Strata relief | Bandas direccionales con fase, distorsión, perfil seno/escalón/triángulo y remapeo de potencia. Deforma una pared continua; no crea volúmenes estratificados independientes. |
| Ridged noise | Suma normalizada de octavas del FBM real, remapeadas mediante `(1-abs(noise))^sharpness`. |
| Solidify surface | Dos copias desplazadas y cierre de bordes, con GUID distintos para dorso y laterales. El preset cliff usa dirección fija; grandes offsets sobre normales pueden plegarse. |
| Reduce mesh | Simplificación nativa Manifold con regiones protegidas y comprobación de error bidireccional muestreado. Si no obtiene reducción dentro del límite, conserva la entrada. No es remallado isotrópico. |

Máscaras por operación: altura, pendiente, ruido, dirección, distancia radial y gradiente, con rango e inversión. El campo mostrado es la máscara cuando existe; si no, la señal escalar de la operación. Vector muestra su componente X; Warp muestra magnitud; Layers muestra índice relativo; Relax muestra magnitud del último paso.

La resolución significa subdivisiones por cara de la primitiva, **no vóxeles**. Límites: 24 operaciones, resolución 4–64, 240.000 vértices al multiplicar capas y presupuesto de 240.000 triángulos para las operaciones sólidas. Los cortes y la subdivisión rechazan operaciones que excedan su presupuesto; no bajan calidad silenciosamente. Las deformaciones artísticas pueden plegarse o solaparse con amplitudes grandes o capas finas. La validación de malla cerrada no demuestra ausencia de todas las auto-intersecciones ni propiedades físicas. No hay remallado isotrópico ni simulación geológica. Los presets son puntos de partida, no assets finales.

Los desplazamientos de campo mantienen unidos los contactos de estratos etiquetados sin fusionar sus vértices de material o región. En esos contactos, y también cuando `abs(amplitude) * frequency >= 1` o `Relax` acumula una fuerza extrema, el Playground reduce el desplazamiento completo hasta conservar la longitud local de aristas y la orientación de caras. Las recetas históricas sin contactos etiquetados y de baja intensidad quedan iguales. Este control evita agujas por ruido exagerado; no une cortes intencionales ni cubre todas las operaciones sólidas o auto-intersecciones. [Prueba y capturas Vulkan](Evidence/2026-09-26-extreme-deformation-review.md).

El contrato, las matemáticas, la semántica de regiones y los límites para un consumidor sin UI están en `BACKEND_CONTRACT.md`. La evidencia v3 vive en `Evidence/surface-v3`; v2 y v1 permanecen en `Evidence/backend-v2` y `Evidence/baseline-v1`. La investigación acotada está en `docs/NODE_MATH_RESEARCH_2026_09_15.md`. Recetas reutilizables, importación de heightmap y aplicación de RBrush para M8 están especificados en `docs/M8_GEOLOGY_BACKEND.md`; la matemática de bake de altura, la proyección RBrush local y un pack derivado de imagen CC0 se probaron en el Playground.

## Arco y túnel de prueba

El cliff tiene además el perfil opt-in v18 `build/Release/JericoGeologyPlayground.exe --cliff-metric-relief-bakes-test Evidence/cliff-metric`: distancia al contorno en metros y relieve interior sobre la misma malla, con selección GUID, tres LOD y diez mapas high→low. La prueba y sus límites visuales figuran en `Evidence/2026-09-25-cliff-metric-relief.md`.

`build/Release/JericoGeologyPlayground.exe --cave-selected-relief-bakes-test Evidence/cave-selected` prueba relieve nativo seleccionable por GUID en la misma malla BSP: techo y dos paredes del túnel cuadrado, o todas las caras del arco. Exporta tres LOD por forma, UV y mapas high→low, más bake de AO high y capturas PBR comparables con/sin AO. `Tests/CaveBakeAudit.py`, `Tests/CaveSelectedPbrAudit.py` y `Tests/CaveDeterminismAudit.py` auditan esa prueba; los resultados y sus límites visuales están en `Evidence/2026-09-25-cave-selected-pbr.md`.

`build/Release/JericoGeologyPlayground.exe --cave-forms-bakes-test Evidence/cave-forms` genera dos cortes BSP sustractivos en roca: un arco facetado y un túnel cuadrado. Ambos atraviesan la masa en Z y conservan 0,3 unidades de piso. `Source/SurfaceGeometry.cpp` construye los brushes con la biblioteca C++ del Engine; la fuente y los GUID de los dos brushes se conservan en el resultado. El high a subdivisión 8 recibe FBM nativo con lacunaridad 1,87 (amplitud 0,12 en arco y 0,025 en cuadrado); los LOD 6/4/2 se regeneran desde los mismos brushes. Cada low recibe UV automática y proyección del high de normal, AO, contacto, curvatura, cavity, borde convexo, crease, espesor, región y altura, con mips. Las imágenes `high-material.png` y `lod-*/low-material.png` son capturas Vulkan a 1280×960 con textura CC0 de roca.

La corrida retenida está en `Evidence/2026-09-25-cave-forms-rock-material/`. Es una comparación geométrica de entradas cavernosas, no una caverna natural terminada: el bloque exterior y la repetición triplanar del diffuse todavía se leen artificiales. No se usó OpenVDB ni se modificó el Engine.

La variante posterior en `Evidence/2026-09-25-cave-faceted-relief-retained/` usa un brush exterior convexo facetado sólo para el arco, aumenta su relieve high a 0,12 y ensaya una proyección triplanar de mayor escala en las dos formas. El túnel cuadrado conserva el contorno BSP original como control. Esta variante es la referencia visual vigente; la primera corrida queda para comparar la silueta y el tiling.

`build/Release/JericoGeologyPlayground.exe --cave-geology-bakes-test Evidence/cave-geology` aplica la macrodeformación vectorial XYZ al arco y relieve geológico a las cuatro paredes interiores del túnel cuadrado. Guarda vistas con material y sin material, tres LOD y diez mapas con UV/mips. `--cave-forms-bakes-test` conserva el BSP plano como control. La corrección del camino geológico y sus capturas están en `Evidence/2026-09-25-square-default-walls.md`; `Evidence/2026-09-25-cave-geology-macro.md` describe la corrida histórica anterior.

`build/Release/JericoGeologyPlayground.exe --cave-axial-lods-bakes-test Evidence/cave-axial` añade boca/garganta/interior al arco, reduce sus LOD desde la malla macro high y conserva el relieve interior del túnel cuadrado. El bake high→low, UV y mapas pasan; `Evidence/2026-09-25-cave-axial-lods.md` conserva las capturas históricas y el límite visual pendiente.

El experimento separado `--cliff-nonharmonic-bakes-test Evidence/cliff-nonharmonic` conserva el cliff con alfa RBrush y cambia sólo la lacunaridad de su `CliffCutLoop` de 2 a 1,87. Los tres LOD y bakes se generan, pero la diferencia visual medida es mínima; no es un nuevo preset aprobado.

`--cliff-macro-warp-bakes-test Evidence/cliff-macro` ensaya el vector FBM nativo que precede al RBrush con mayor desplazamiento lateral y vertical. El candidato retenido `Evidence/2026-09-25-cliff-macro-warp035-retained/` usa amplitud 0,35 y conserva high, tres LOD, mapas y capturas Vulkan; cambia la silueta sin alterar la región del corte. Es experimental y no modifica el preset por defecto.

`--cliff-eroded-rim-bakes-test Evidence/cliff-eroded-rim` prueba un perfil v14 opt-in: el RBrush con alfa CC0 conserva el corte seleccionable y retrae el perímetro en ejes locales mediante ruido no armónico. Incluye tres LOD high→low, UV automática y los mapas/bakes habituales. El resultado verificado y su límite visual están en `Evidence/2026-09-25-cliff-eroded-rim.md`; sigue siendo un experimento, no un cliff natural aprobado.

Los campos de transformación se interpretan según la operación: Transform, envelope y DamageDecal tienen coordenadas propias; las operaciones de ruido usan Frequency y Coordinate XYZ. `meshHash` prueba posiciones e índices: no es un hash de materiales/metadatos. El stack conserva GUID por triángulo y un catálogo de regiones.

## CPU, GPU y mediciones

El stack completo se evalúa actualmente en CPU, en un worker cancelable. El render usa Vulkan. `Compare scalar FBM` ejecuta un compute shader local y contrasta sus valores con el código CPU real del engine en las posiciones mostradas. Se habilita para una capa Normal FBM cuando la regeneración termina.

Es una prueba de concordancia del **campo escalar**, no de toda la deformación, los tres canales vectoriales ni del pipeline M8. No incluye warp o máscaras previos en el probe: toma las posiciones finales como puntos de prueba. El algoritmo GPU es local; su referencia CPU proviene del engine.

- CPU generation: evaluación de la malla, sin upload ni rendering.
- GPU dispatch only: consulta de duración del dispatch FBM; excluye preparación, upload, sincronización y readback. Si la consulta no está disponible se muestra explícitamente.
- CPU reference + check: cálculo CPU y comparación del probe. Tiene otro alcance y no permite deducir un speedup directo.
- GPU memory estimate: buffers de malla y targets del viewport; excluye swapchain, driver, ImGui y recursos temporales del probe. No es consumo total de VRAM.

## Compilar y comprobar

Requiere Visual Studio 2022 C++, CMake y el árbol de dependencias existente de JericoEngine. No descarga ni agrega bibliotecas.

```powershell
cmake -S F:/JericoGeologyPlayground -B F:/JericoGeologyPlayground/build-surface -G "Visual Studio 17 2022" -A x64
cmake --build F:/JericoGeologyPlayground/build-surface --config Release -j 2
ctest --test-dir F:/JericoGeologyPlayground/build-surface -C Release --output-on-failure
F:/JericoGeologyPlayground/build-surface/Release/JericoGeologyPlayground.exe --smoke-test F:/JericoGeologyPlayground/Evidence/surface-v3
```

Se puede configurar `JERICO_SOURCE` y `JERICO_DEPS` para otro checkout/build compatible. El proyecto y el build deben permanecer fuera del engine. Sólo Release está verificado con estas dependencias precompiladas.

`Evidence` contiene pruebas RED/verde, log de build, log de Vulkan, recetas y capturas originales del viewport. `runtime-proof.json` registra el adaptador, geometría, tiempos y error CPU/GPU de esa ejecución.

## Comparación pendiente con M8

Esta entrega demuestra reutilización de código y un laboratorio operativo. **Todavía no demuestra que sea mejor que M8 ni que sus recetas se integren en el pipeline productivo.**

Para decidirlo: definir una familia de roca, escala y semillas; igualar cámara, iluminación y presupuesto de triángulos; guardar ambas recetas/versiones; capturar Beauty y normales a resolución original; medir generación, transferencia, rendering y memoria con los mismos límites; revisar silueta, estratos, grietas, pliegues y estabilidad entre seeds. Hacer la integración sólo después de esa evaluación.

## Integraciones futuras

El audit completo está en `DESIGN.md`. Si se decide adoptar el experimento, son **INTEGRATION REQUIRED**: cook/runtime formats (owner Asset Pipeline, entrada mesh y campos semánticos), materiales/render (owner Renderer, regiones/estratos/grietas), colisión/destrucción (owner Physics, sólidos y propiedades), LOD/streaming (owner Geometry/Assets, geometría estable), editor/undo (owner Editor, receta y transacción) y consultas de gameplay (owner Engine/Gameplay, semántica conservada). Dependen de un slice de adopción aprobado y con milestone asignado; no están programadas ni implementadas en este laboratorio.

## Nuevas pruebas v4: desgaste y normal high -> low

En la prueba v4 histórica se elegía **Angular wear v4** o **Layer contact wear v4**; el segundo preset se llama ahora **Layered rock v9** y usa contacto geométrico. Las máscaras Curvature, EdgeProximity y Contact pertenecen a la ruta de desgaste. El bloque **High -> Low -> normal map** permite reducir el high y transferir su normal al low.

1. Esperar la generación final y pulsar **Build low + bake high normals**.
2. Comparar **High**, **Low** y **Low + normal map**, también en modo Normals.
3. **Export low OBJ + normal PNG** guarda low con UV/normales partidas, normal lineal, receta high y métricas bajo `Evidence` del Playground. La operación es cancelable; editar la receta invalida el bake.

La API ofrece rayos normales y superficie compatible más cercana, sin fallback oculto. El atlas histórico por triángulo es experimental, sin mipmaps; la normal no sustituye detalle de silueta. En esta ejecución v4 histórica, el ejemplo angular conservó 0,328 % de muestras sin proyección y el estratificado tuvo cobertura completa; las cifras v9 vigentes están en el checkpoint de 2026-09-24. No son assets finales ni un cierre de calidad M8.

Para repetir el estudio histórico de tamaño del atlas con el mismo high/low sedimentario seed 123: `JericoGeologyPlayground.exe --normal-bake-3072-test <directorio>` o `--normal-bake-scale-test <directorio>` para 4096². Comparar con `--normal-bake-test` a 2048² y con los informes de padding/mips en `Evidence/2026-09-24-geology-math-checkpoint.md`. Aquellas pruebas usaban sólo la textura base; la ruta UV automática posterior sí carga cuatro niveles de normal.

`--sedimentary-low-sweep-test <directorio>` compara el high sedimentario contra lows regenerados a resoluciones 8/10/12/14/16 en cuatro seeds, sin exportar texturas. `--normal-bake-low10-test <directorio>` exporta y captura el low experimental de resolución 10 para esas seeds. Los resultados actuales mantienen low16 como baseline de calidad; ver el mismo checkpoint para los errores de geometría, imagen y mip.

`--normal-bake-low10-orbit-test <directorio>` captura cuatro ángulos del mismo low10 seed 123 para comparar con `--normal-bake-orbit-test`; las vistas son estáticas, sin mips ni aceptación de LOD runtime.

`Tests/BakeAdaptiveAtlasBudget.py` calcula layouts experimentales de dos densidades a partir del error de mip medido. `--normal-bake-adaptive-test <directorio> <layout.json> <seed>` rebakea high→low con UV explícitos y valida la identidad de ambas mallas; `--normal-bake-adaptive-orbit-test` captura cuatro vistas de la seed indicada. Los manifiestos y los rebakes de 22/14 y 24/13 px están en `Evidence/2026-09-24-adaptive-atlas-budget/` y `Evidence/2026-09-24-adaptive-bake-*/`. El candidato 24/13 mejora el p95 de mip 2 simulado por CPU en cuatro seeds, pero empeora levemente el error visual estático en las cuatro vistas medidas. Sigue siendo un atlas experimental sin mips GPU ni aceptación de material; ver el checkpoint.

La investigación visual incluye cuatro fotos reales de USGS en dominio público (`Evidence/2026-09-24-usgs-geology-references/manifest.json`) para estratos cruzados, cliff de arenisca, cliff basáltico y bloques de talud. Sus observaciones se contrastan con los presets existentes en el checkpoint. `Tests/AutoUvGapAudit.py` mide los UV de los 17 lows bakeados históricamente: el atlas triangular usaba 10,8–36,8 % de área UV activa en LOD0 y sus 4 px de margen no cubrían el objetivo diagnóstico de cuatro mips (8 px). La nueva ruta C++ guiada por ZenUV aún es experimental; sus costuras, densidad, LOD y calidad visual no tienen aceptación productiva.

La validación v4 histórica pasó 13/13 tests. La extensión v5 añade otro test, nueve casos de superficie y tres comparaciones high/low. Ver resultados y límites vigentes en `Evidence/surface-features-v4/VERIFICATION.md`. Para repetir las capturas, ejecutar el mismo binario con `--surface-features-test <directorio>` o `--normal-bake-test <directorio>`.

## Extensión nativa v5

Elegir **Crater rock v5** para probar el proyector elíptico de daño, profundidad y labio sobre geometría. Las máscaras nuevas son convexidad, concavidad, AO, exposición, proximidad al borde abierto y espesor. Contacto y AO tienen radios independientes.

El bake genera el high primero y proyecta sobre el low: normal tangente, AO, oclusión de contacto, curvatura firmada, cavity, borde convexo, máscara de distancia a pliegues, espesor, IDs de región y altura firmada. Exporta diez PNG, OBJ, receta y métricas. El espesor usa el primer impacto hacia dentro; cero también puede significar ausencia de impacto. El atlas sigue siendo experimental.

`RockAddonMath` contiene además kernels probados de mezcla de daño, esfera/grieta, retículas y envejecimiento. Salvo el proyector de cráter, esos kernels todavía no constituyen operaciones completas del stack ni materiales finales. No se afirma haber completado todos los addons: faltan cortes con nuevos interiores, remallado/segmentación avanzados, UV/UDIM/mips productivos y el pipeline de materiales. Blender y sus addons son referencias matemáticas; la implementación y las pruebas son C++.

El código reutilizable está en `PlaygroundModel`; la adopción M8 y sus gates están descritos en `BACKEND_CONTRACT.md`. No se modificó el Engine ni se instaló ningún addon de Blender: las investigaciones del repo sirvieron como referencia técnica.

## Continuación del Playground — 2026-09-24

Para reproducir la prueba más reciente de UV/bakes nativos: `build-surface/Release/JericoGeologyPlayground.exe --auto-uv-bake-test <directorio> [familia]`. Sin filtro recorre 5/8/9/10/11/12; exporta un low con UV automática, diez mapas high→low, cuatro niveles de normal y capturas Vulkan por familia. `Evidence/2026-09-24-auto-uv-soft-chart-six-family/` conserva el resultado con bases normales/tangentes continuas por isla; `Tests/AutoUvBakeAudit.py` verifica cobertura/alpha y `Tests/BakeVisualAudit.py` compara High, Low y Low+normal. Build Release y CTest 17/17 pasaron. El cliff todavía parece una escalera y la roca estratificada conserva algunas líneas oscuras finas; esto es un laboratorio, no un bake M8 aceptado.

La instrucción vigente del owner reabre la investigación matemática en este Playground mientras el Engine está en M9; el cierre v5 anterior queda como evidencia histórica. Ver `Evidence/2026-09-24-geology-math-checkpoint.md` para pruebas, capturas, referencias CC0 y faltantes. La receta v9 conserva los perfiles de contacto y el asentamiento de v7, separa el eje de estratos del eje de máscara para el cliff BSP y evita pliegues causados por encadenar normales cambiantes en superficies abiertas. El bake high→low exporta normales y altura firmada; las seis familias de prueba tienen 0 misses con sus parámetros explícitos (sedimentaria/cliff/BSP/angular/estratos/cráter). La roca angular todavía parece un bloque facetado y no se declara resuelta su calidad visual. `RBrushInterop` consume el IR canónico y el derivador de máscara SDF 2D del Engine como fuentes de lectura. Los UV/mips productivos y la aplicación editable de RBrush, importación de heightmaps y adopción productiva permanecen pendientes de M8.

La formación coherente ahora interpola desplazamiento lateral, giro y escala entre marcos compartidos por cada horizonte: una receta con deriva no abre huecos entre las dos caras de contacto. El test reproduce primero el fallo anterior y verifica después contacto completo, cierre y ausencia de reajuste; los seis presets por defecto conservan 84 exportaciones/capturas estáticas idénticas. Esto no simula gravedad física. El análisis visual de cliff, roca angular y cráter en el checkpoint deja pendiente la macroforma y las roturas reales de esas familias.

`--angular-profiles-test <directorio>` compara la receta angular vigente con dos variantes de cortes C++ desde dos ángulos Vulkan. `Tests/AngularProfilesAudit.py` mide su silueta y el checkpoint explica por qué ninguna variante se seleccionó: más semiespacios recortan el bloque pero no producen interiores de fractura. El experimento es diagnóstico y no reemplaza el bake ni las seis familias de referencia.

`--rock-lod-bakes-test <directorio>` prueba por separado tres low poly/LOD frente al mismo high de cada familia 5, 8, 9, 10, 11 y 12. Exporta los diez mapas por nivel, malla con UV, receta, métricas y vistas Vulkan high/low/low+normal; los LOD2 válidos incluyen comparación a doble distancia de cámara. `Tests/RockLodBakeAudit.py` comprueba cobertura de canales, hashes, triángulos y errores; registra los niveles con misses o sin reducción como parciales/rechazados. La evidencia vigente está en `Evidence/2026-09-24-angular-lod2-selected-rock-lods/` y `Tests/RockLodVisualAudit.py` cuantifica su silueta/color cercano y lejano. Este protocolo debe acompañar las próximas variantes de roca. Todavía no hay una suite equivalente para los tres perfiles angulares históricos ni UV/mips productivos.

`--auto-uv-rock-lod-bakes-test <directorio> [familia]` aplica UV automática y bases TBN continuas a cada low, genera diez mapas y cuatro niveles por mapa. La prueba integral más reciente está en `Evidence/2026-09-24-specialist-gpu-mip-final-six-family/`: 17 niveles completos sin misses, BSP LOD2 rechazado por no reducir triángulos. `Tests/RockLodBakeAudit.py <directorio> <informe.json> --require-auto-uv --require-specialist-mips`, `Tests/AutoUvBakeAudit.py <lod> --require-covered`, `Tests/RockLodVisualAudit.py` y `Tests/SpecialistPreviewAudit.py` validan bakes, mips y capturas Vulkan. La aceptación visual/productiva y los filtros/materiales del Engine siguen pendientes.

ZenUV orienta la matemática de **UV automático**; Ucupaint orienta la generación de **bakes y máscaras** en C++ nativo. El cliff sólido usa ahora `CliffCutLoop` (receta v10): un loop local ajustado a la cuadrícula, con región frontal GUID seleccionable por datos, parámetros editables y charts UV separados dentro del mismo atlas. La corrida vigente `Evidence/2026-09-24-cut-loop-six-family-lods/` verificó los diez mapas y tres LOD del cliff sin misses y los otros presets sin regresión técnica (17 completos; BSP LOD2 rechazado). La captura aún muestra un parche central artificial: la calidad visual del cliff no está aceptada, y todavía no hay selección de región por clic en el viewport.

`--cliff-drainage-bakes-test <directorio>` prueba, sólo en familia 8 y receta experimental v13, una incisión negativa sinuosa a través de varias alturas antes del cut loop; sus tres LOD pasaron bakes/UV pero la incisión casi no se distingue en Beauty. `--cliff-edge-damage-bakes-test <directorio>` agrega un perfil de borde con caída suave y un campo de altura negativo proyectado sobre el eje del cliff antes del corte. El perfil y la dirección de proyección nuevos sólo se aplican desde v13; v10–12 conservan su geometría. Auditar con `Tests/RockLodBakeAudit.py <paquete> <informe.json> --family 8 --cliff-drainage` o `--cliff-edge-damage`, según la variante. Los diez mapas high→low se exportan por LOD; el candidato retenido en `Evidence/2026-09-24-cliff-edge-damage-retained/` tiene LOD0/1 sin misses, pero LOD2 registra 41 misses y 32 muestras UV interiores sin pintar. La pared aún muestra una protuberancia ovalada artificial. Son pruebas matemáticas optativas, no presets ni aceptación M8. El campo negativo se genera analíticamente; esta prueba no importa un height map externo.

El viewport abre en **Material** y deja elegir Auto/Rock/Cliff/Slate/Basalt; Auto elige una paleta y diffuse CC0 según la familia. `--material-gallery-test <directorio>` captura los 13 presets y las cuatro apariencias. `--cliff-rbrush-alpha-bakes-test <directorio>` ensaya en familia 8, receta v13, un empuje local de la **misma malla** con la imagen CC0 `cliff_side` importada por `RBrushImageImporter` y un contorno irregular; la región `CutLoop` sigue identificable por GUID. La ruta aún no ejecuta un stamp SDF transaccional del editor. La prueba de `Evidence/2026-09-25-cliff-strictlod-candidate/` produjo tres LOD (13.996/11.852/10.168 triángulos), UV automática y diez mapas high→low por LOD, incluidos normal, AO, contacto, cavity, convex-edge, espesor y altura; `Tests/RockLodBakeAudit.py <paquete> <informe.json> --family 8 --cliff-rbrush-alpha --require-specialist-mips` verificó cero misses y cero texeles interiores sin pintar. Los presupuestos geométricos son 0,02/0,03/0,04; un control de orientación descarta la simplificación que invierte una cara. Las capturas material high/low siguen pareciendo una pared repetitiva y **no** constituyen aceptación visual del cliff ni M8.

`NormalBakeSettings` permite ajustar `contactRadius` y `ambientRadius` de forma independiente en metros; si no se especifican, conserva respectivamente `featureRadius * 0.25` y `featureRadius`. `--layered-occlusion-bakes-test <directorio>` ensaya 0,08 m de contacto y 0,8 m de AO ambiental sólo en la familia 11, con UV automática, diez mapas y tres LOD. `Tests/LayeredOcclusionAudit.py <base> <candidato> <informe.json>` compara mallas, mapas y capturas de ambas corridas actuales; `Tests/RockLodBakeAudit.py <candidato> <informe.json> --family 11 --layered-occlusion --require-specialist-mips` comprueba los tres bakes. Esta variante no cambia el preset ni demuestra por sí sola una mejora visual de material.

`--layered-material-baseline-test <directorio>` y `--layered-material-candidate-test <directorio>` añaden capturas Vulkan 1280×960 high/low y LOD2 lejano sobre la misma roca 11. El modo material es sólo diagnóstico: diffuse CC0 `rock_06` por proyección triplanar, normal high→low, luz fija Lambert/GGX, AO sobre luz ambiente y contacto/cavity/edge como variaciones sutiles de color. Diffuse se filtra con mips calculados en luz lineal; los mapas de datos siguen en formato lineal. `--layered-material-no-mips-test <directorio>` reproduce el baseline sin mips de color para comparar exactamente el filtro. `Tests/MaterialPreviewAudit.py` compara las variantes y verifica el SHA de la textura en `Evidence/cc0-references/manifest.json`; `Tests/MaterialColorMipAudit.py` mide el efecto de los mips. No es un material PBR de producción ni aceptación visual de la roca o del Engine.

`--layered-horizon-fracture-bakes-test <directorio>` ensaya sólo la familia 11 con receta v15/profile 2, UV automática, tres LOD high→low y el **material automático** de pizarra (`cliff_side`, appearance 2). El selector `Horizon contour` permite comparar el perfil legacy y los horizontes independientes; el preset sigue en su valor original. [Resultado, métricas y Beauty parcial](Evidence/2026-09-25-layered-fractured-horizons.md).

El preset 13 y `--layered-slate-source-bakes-test <directorio>` prueban la fuente tabular v16: Cube con caps coincidentes, perfil 2 y espesor 0.20. El material automático, tres vistas high originales, UV y diez mapas por cada low LOD quedan en la [evidencia](Evidence/2026-09-25-layered-slate-source.md). Es una variante opt-in con Beauty aún parcial.

`--regenerated-lod-sweep-test <directorio>` examina candidatos low de BSP (9), angular (10) y cráter (12) regenerando la misma receta a menor resolución, con presupuestos geométricos intactos. El cráter a resolución 7 pasó como nuevo LOD2 (588 triángulos, diez bakes, cero misses). BSP y angular mantienen sus límites pendientes; ver `Evidence/2026-09-24-regenerated-lod-expanded-audit.json` y el checkpoint para la evidencia.

`--angular-lod2-sweep-test <directorio>` busca un presupuesto de simplificación nativa menor que LOD1 sin alterar el high ni la caja de proyección. Seleccionó 0,008: 3596 triángulos y cero misses en el LOD2 angular, con los diez mapas y capturas sobre la roca; ver `Evidence/2026-09-24-angular-lod2-sweep-audit.json`. No corrige la forma angular todavía demasiado cúbica ni el cavity casi vacío.

El bake ahora interpola primero la curvatura **firmada** del high y separa después concavidad/convexidad por texel; así un mismo punto no puede marcar a la vez cavity y desgaste convexo. Cada LOD válido añade cuatro vistas Vulkan del mapa sobre la roca (`low-ao.png`, `low-contact.png`, `low-cavity.png`, `low-convex-edge.png`). La auditoría exige cero superposición de signos y dimensiones/cobertura correctas. La corrida anterior `Evidence/2026-09-24-map-preview-rock-lods/` conserva el fallo de escala del borde del cliff para comparación.

`--edge-radius-sweep-test <directorio>` ensaya radios físicos de 0,02/0,10/0,20 m sobre el mismo high y low en sedimento, cliff, angular y cráter. El cliff usa ahora **0,10 m sólo en el paquete de LOD de prueba**: resalta cornisas en sus tres LOD sin cambiar normal, AO, cavity, malla ni Beauty. La vista de 0,20 m era demasiado ancha; la escala adecuada para BSP, cráter y otros materiales sigue abierta. Ver `Evidence/2026-09-24-edge-radius-sweep-audit.json` y `Evidence/2026-09-24-cliff-edge-selected-rock-lods-audit.json`.

En el estratificado, el test vigente coteja todos los triángulos de cada horizonte de contacto. Las aparentes bandas de aire de la captura eran normales de sombreado discontinuas: ahora se calculan a partir de las caras exteriores y se comparten sólo si las interfaces completas coinciden. En esa prueba la geometría conservó sus hashes; las capturas Vulkan y el bake high→low están en `Evidence/2026-09-24-layer-seam-v3-surface/` y `Evidence/2026-09-24-layer-seam-v3-bake/`. Aún falta mejorar la silueta repetitiva, las fracturas reales y la calidad de material; el asentamiento no es una simulación dinámica de gravedad.

Una iteración histórica del preset 11 usó `perimeterAmplitude=0.28`; sus capturas están en `Evidence/2026-09-24-layer-perimeter028-final-surface/` y su bake en `Evidence/2026-09-24-layer-perimeter028-final-bake/`. El barrido posterior de seeds encontró tapas invertidas con esa amplitud, así que no es el valor vigente. La captura anterior con error low 0.04 y 62 misses también es un límite histórico medido.

El preset estratificado v9 añade `shapeOuterCaps`: retrae de forma determinista los bordes superior e inferior respecto al horizonte de contacto vecino, sin modificar las interfaces internas. Las recetas v8 migran con esta opción desactivada y conservan su geometría anterior. La captura y el bake **vigentes** están en `Evidence/2026-09-24-layered-safe-surface/` y `Evidence/2026-09-24-layered-safe-six-family-bakes/`; el preset 11 usa amplitud 0.16 y produce high 27.648 → low 13.086 triángulos, 0 misses en 588.870 muestras. Sus 64 seeds low y high pasan contacto y cierre. La forma aún no representa piezas fracturadas independientes ni gravedad dinámica.

El antiguo preset 5 sí tenía separaciones visibles entre seis losas. Ahora usa horizontes de contacto completos, sin espacio ni deriva lateral inicial, y su asentamiento se verifica en tres seeds. Su high de 73.728 triángulos se proyecta a un low de 18.432 generado por la misma receta a resolución 16; el simplificador nativo de ese caso no pasó el límite geométrico y queda identificado como tal. El bake de seis familias y la captura Vulkan están en `Evidence/2026-09-24-sedimentary-final-bake/` y `Evidence/2026-09-24-sedimentary-contact-v10/`; todos los casos tienen cero misses. Ver métricas y límites en `Evidence/2026-09-24-geology-math-checkpoint.md`.

RBrush también tiene ahora una prueba standalone de proyección local: verifica hash canónico y máscara del Engine, transformaciones, dimensiones en metros, caída y entradas inválidas. Una imagen de desplazamiento CC0 de `cliff_side` pasa por el importador y contenedor `.rbrush` oficiales y vuelve al mismo IR; el pack derivado y dos vistas de su peso están en `Evidence/2026-09-24-rbrush-cc0-local-field/`. La suite tiene 16/16 tests. Esta prueba no equivale a paridad GPU, aplicación en JericoEditor ni cook M8.

La corrida base `Evidence/2026-09-24-specialist-mips-six-family-lods/` exportó cuatro niveles de los diez mapas high→low, incluidos AO, cavity, convex-edge e ID de región con filtro categórico. La auditoría pasó en 17 LOD válidos sin misses ni IDs inventados; BSP LOD2 sigue rechazado. `Tests/RockLodBakeAudit.py --require-auto-uv --require-specialist-mips` reproduce la comprobación. Los mips no arreglan la macroforma del cliff ni la repetición de los estratos. El checkpoint vigente detalla las pruebas, límites y referencias M8/USGS/CC0.

La prueba Vulkan más reciente está en `Evidence/2026-09-24-specialist-gpu-mip-final-six-family/`: el preview de AO, contacto, cavity y convex-edge usa ahora los mips exportados, con vistas lejanas base/mips para las seis familias. `Tests/SpecialistPreviewAudit.py` comprobó cuatro niveles y diferencias en pantalla para cada mapa no plano; los 17 bakes y 17/17 tests siguen pasando. El cliff aún tiene un parche liso y las familias estratificadas aún repiten bandas: la siguiente prueba debe construir juntas, bloques y repisas reales con linaje y LOD, no sólo variar ruido.
