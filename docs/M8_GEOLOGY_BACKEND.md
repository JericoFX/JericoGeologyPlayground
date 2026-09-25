# M8 — backend de geología seleccionado y contrato de autoría

Decisiones del owner, 2026-09-15. Documento de preparación de M8 dentro del Playground standalone. No modifica la autoridad ni el comportamiento productivo del checkout del engine.

**Estado vigente, 2026-09-24:** el owner volvió a autorizar investigación y pruebas matemáticas en este Playground durante M9, sin tocar `F:/JericoEngine`. La instrucción de cierre del 2026-09-21 queda histórica. Recetas v9; la cobertura de todas las investigaciones sigue siendo parcial. La evidencia actual está en `../Evidence/2026-09-24-geology-math-checkpoint.md`. Blender no forma parte de la generación, remesh ni bake: se reutilizan matemáticas implementadas en C++.

La última corrección del generador centra estratos, ruido y altura en los límites de la fuente, de forma que trasladar una selección BSP no cambie su forma. Dos selecciones BSP reales separadas 20 m produjeron rocas estratificadas con error superficial muestreado de 5,78×10⁻¹⁵ al compensar la traslación (`../Evidence/bsp-layered-surface-runtime.log`). También atenúa la perturbación angular hacia el centro de las tapas para evitar caras de apoyo invertidas. El preset 5 pasa 128 seeds low/32 high; el preset 11 usa ahora amplitud de perímetro **0.16** y pasa 64 seeds low/64 high, con contacto triangular completo y cero desplazamiento al reasentar. La antigua amplitud 0.28 plegó una tapa en seed 1 y queda como experimento histórico rechazado, no como valor recomendado para M8. Build Release y **16/16** tests finales: `../Evidence/geology-scale-final-build.log` y `../Evidence/geology-scale-final-ctest.log`.

## Dirección seleccionada

### Directiva para quien implemente M8

**Usar el backend del Jerico Geology Playground como base de implementación de la geología M8.** Es la dirección elegida por el owner. Partir del código existente y de sus pruebas; adaptar sus entradas/salidas a los contratos productivos de Jerico. No sustituirlo por una nueva demo ni reconstruir sus operaciones desde cero. La integración debe producir una sola ruta oficial de geología/cook; cualquier retirada de la ruta anterior se hará después de verificar sus consumidores y migración.

Ubicación de la entrega: **`F:/JericoGeologyPlayground`**. Build verificado: **`build-surface/Release`**. Estado: capacidades standalone probadas; integración productiva M8 pendiente. El calificativo de backend seleccionado expresa la decisión del owner, no que el milestone esté terminado.

### Código que debe usar M8

| Archivo del Playground | Responsabilidad para integrar |
|---|---|
| `Source/Model.hpp` y `Source/Model.cpp` | Recipe/Layer v9, compatibilidad v1–v8, contorno exterior opt-in con migración anclada, ejes separados de estrato/máscara, dirección estable de relieve abierto, pila ordenada, campos/máscaras, desgaste, DamageDecal y LayerSettle, evaluación, serialización y cancelación. Normal exterior continua entre estratos coincidentes del preset 11; entrada `evaluateSource` para una malla suministrada. |
| `Source/SurfaceGeometry.hpp` y `.cpp` | Plano abierto, Solidify y `bspSource`: selección de GUID del BspDocument real -> evaluación sólida original del engine -> subdivisión -> fuente del stack. |
| `Source/LayerFormation.hpp` y `.cpp` | Formación de estratos coherentes en marco local de la fuente, perfiles angulares compartidos de contacto con amortiguación radial interior, relieve lateral, linaje semántico y asentamiento por primer contacto geométrico con caras de apoyo etiquetadas. |
| `Source/SolidGeometry.hpp` y `.cpp` | Planos estructurales, envolventes, regiones y transferencia de atributos por triángulo cercano. |
| `Source/MeshReduction.hpp` y `.cpp` | Experimento de simplificación Manifold con límite de error muestreado y conservación de entrada cuando no logra reducir. Mantener sus límites visibles. |
| `Source/SurfaceFeatures.hpp` y `.cpp` | Curvatura firmada y vistas positiva/negativa/absoluta; distancias a crease/borde abierto, contacto y AO con radios independientes, exposición, espesor por primer impacto y confianza. Campos sobre la malla de entrada. |
| `Source/NormalBake.hpp` y `.cpp`, `Source/SurfaceRay.hpp` | Proyección high -> low por rayos o superficie compatible más cercana, UV/TBN por esquina, normal tangente y mapas AO/curvatura/espesor/ID/altura firmada; export OBJ/PNG/JSON, cancelación y métricas de misses/clipping. |
| `Source/RBrushInterop.hpp` y `.cpp` | Campo local inmutable y adaptador standalone de IR canónico RBrush al derivador 2D SDF, hash y validador de máscara originales del Engine; prueba proyección local en metros y falloff. Un test separado importa una imagen CC0 al contenedor oficial `.rbrush`; no aplica stamps en el editor. |
| `Source/RockAddonMath.hpp` y `.cpp` | Kernels C++ de proyector, mezcla de daño, cráter/labio, campos esfera/grieta, retículas y envejecimiento. Sólo el proyector de cráter está conectado como operación completa; los demás kernels están probados aisladamente. |
| `Tests/` | Contratos reproducibles de geometría, regiones, seeds, recetas anteriores, BSP, superficies y reducción. Usarlos como regresión al integrar. |
| `Source/Main.cpp` y `Source/Renderer.cpp` | Referencia del consumidor standalone, preview y diagnóstico. La UI/render productivos deben usar los sistemas existentes de JericoEditor y del engine. |

`PlaygroundModel` ya es una biblioteca C++20 sin dependencia de ImGui ni del renderer. Su `CMakeLists.txt` muestra las fuentes originales del engine que reutiliza. En la integración, enlazar los módulos Jerico correspondientes una sola vez: no duplicar `BspDocument.cpp`, `Guid.cpp`, `GeologyNoise.cpp` ni `SolidEvaluatorManifold.cpp` dentro del engine. La dependencia del header interno `TriangleBvh.hpp` y el enlace privado actual a Manifold necesitan una interfaz/target estable antes de promoverse a producto.

### Orden de integración y aceptación

1. **Entrada real:** selección BSP por GUID + revisión + transformación -> `bspSource` -> `evaluateSource`. La selección del editor sustituye al fixture `Base::BspWall`; la receta no debe depender de él. Preservar fuente, zonas protegidas y geometría no seleccionada.
2. **Núcleo:** incorporar las operaciones y contratos de datos del Playground en el módulo de autoría M8. Ejecutar los tests existentes contra ese módulo integrado y comparar con las recetas/evidencia retenidas. Mantener seed, unidades, resolución y versión explícitas.
3. **Autoría:** conectar al editor existente mediante snapshots, preview/diff, cancelación, descarte de resultados obsoletos, undo/redo y Apply transaccional. Revalidar revisión y GUID de la fuente antes de Apply. Una receta guardada no modifica por sí sola el BSP.
4. **Salida y consumidores:** transferir malla, GUID de regiones, catálogo/linaje y atributos necesarios a materiales, colisión y formatos cocinados oficiales. Verificar cook, carga runtime source-free y consumidores downstream; registrar como INTEGRATION REQUIRED lo que quede diferido. La integración requiere estas comprobaciones además del frontend.
5. **Aceptación visual y optimización:** reproducir los presets y capturas del Playground en el engine con cámara/escala/presupuesto comparables. La reducción del cliff con calidad equivalente sigue pendiente: no declararla resuelta por el simple enlace de `Reduce`.

Evidencia vigente: `../Evidence/2026-09-24-geology-math-checkpoint.md` y sus logs/capturas enlazados. La evidencia v5 bajo `../Evidence/surface-features-v4/` permanece histórica y no sustituye la validación actual. Contratos: `../BACKEND_CONTRACT.md`.

Tests que M8 debe conservar/adaptar: `Tests/SurfaceFeatureTests.cpp` (campos, daño y recetas), `Tests/RockAddonMathTests.cpp` (kernels analíticos), `Tests/NormalBakeTests.cpp` (proyecciones, TBN, mapas y entradas negativas), `Tests/RBrushInteropTests.cpp` (tip canónico, hash/validación Engine y proyección local), `Tests/RBrushImageRoundtripTests.cpp` (imagen CC0 → pack oficial → campo local), `Tests/ThicknessTests.cpp` (asentamiento), `Tests/BackendTests.cpp` (consumidor y regresión), `Tests/BspSourceTests.cpp` (selección), `Tests/ReductionTests.cpp` (presupuesto). `CMakeLists.txt` registra los 16 tests y sus dependencias.

Comandos de reproducción (PowerShell, fuentes/dependencias de Engine sólo como inputs de lectura):

```powershell
cmake --build F:/JericoGeologyPlayground/build-surface --config Release -j 2
ctest --test-dir F:/JericoGeologyPlayground/build-surface -C Release --output-on-failure
F:/JericoGeologyPlayground/build-surface/Release/JericoGeologyPlayground.exe --surface-features-test F:/JericoGeologyPlayground/Evidence/m8-recheck-surface
F:/JericoGeologyPlayground/build-surface/Release/JericoGeologyPlayground.exe --normal-bake-test F:/JericoGeologyPlayground/Evidence/m8-recheck-bake
```

Verificar cada exit code antes de continuar. `../Evidence/2026-09-24-geology-safe-input-hashes.json` fija SHA256 de **82 inputs** vigentes, incluidas fuentes/tests, ejecutables, imagen CC0 y bibliotecas/contratos del Engine consumidos en modo lectura; se releyó con cero discrepancias. No es un commit (Playground no tiene repositorio Git). No reconstruir ni limpiar el build del Engine para repetir estas pruebas.

Para reproducir la familia estratificada v9 **vigente**: preset 11 con `perimeterAmplitude=0.16`, `shapeOuterCaps=true`, reducción low con error 0.02 y proyección high→low del mismo GUID de región a radio 0.20. El test coteja cada triángulo de contacto en 64 seeds low/high; la captura y el bake vigentes están en `../Evidence/2026-09-24-layered-safe-surface/` y `../Evidence/2026-09-24-layered-safe-six-family-bakes/`. El low tiene 13.086 triángulos frente a 27.648 high, error muestreado 0.01764 y 0 misses en 588.870 muestras. La densidad del atlas sigue insuficiente para mipmaps de producción. Las capturas v9 anteriores con amplitud 0.28 son históricas; no sustituir los parámetros probados al integrar M8 sin repetir el gate.

La continuación del Playground corrigió también un contacto que fallaba al activar deriva lateral/giro/escala en capas coherentes: ambos lados de cada horizonte usan ahora el mismo marco plano interpolado. El RED lateral, el caso combinado GREEN, el cierre de malla, 16/16 CTest y el replay de 84 archivos estáticos de seis familias están en `../Evidence/coherent-*-ctest.log` y `../Evidence/2026-09-24-coherent-horizon-six-family/`; detalle en el checkpoint. Esto preserva contacto geométrico, no implementa gravedad ni rompe la silueta repetitiva. Cliff, angular y cráter siguen requiriendo macroforma propia según la review de addons.

El Playground es la dirección elegida para el backend de geología M8. El flujo de producto será:

**Seleccionar BSP -> elegir/crear receta -> editar operaciones -> preview y diff -> Apply explícito mediante transacción -> cook.**

El BSP fuente, los assets importados y las zonas no seleccionadas permanecen intactos. La selección se referencia mediante GUID y revisión; una selección obsoleta debe rechazarse, no aplicarse a otro objeto por índice. El contrato técnico y las integraciones pendientes están en `../BACKEND_CONTRACT.md`.

## Recetas: requisito explícito de M8

Se podrán **crear, guardar, cargar, duplicar, renombrar, compartir y reutilizar recetas** de geología. Una receta contendrá la pila ordenada de operaciones habilitadas/deshabilitadas, sus parámetros y seeds, máscaras, espacios de proyección, referencias a assets por GUID/hash y versión del esquema/evaluador. Los presets serán recetas editables del mismo sistema.

Una receta deberá poder aplicarse a otro BSP o selección compatible; no quedará ligada accidentalmente a índices temporales de vértices ni a un objeto de prueba. El sistema debe distinguir los parámetros de la receta del binding de la selección y su transformación. Compatibilidad de entrada (Surface/Solid), unidades, resolución y presupuestos deben validarse al aplicarla.

Guardar una receta no hornea ni destruye el BSP. Cargar, duplicar, reordenar o modificar operaciones participa en undo/redo; preview y generación pueden cancelarse y los resultados obsoletos no reemplazan el estado nuevo. El resultado aceptado se cocina por la ruta oficial del engine. La receta editable y el resultado cocinado son representaciones distintas.

Pruebas de aceptación futuras: misma receta+inputs+versiones -> mismo resultado; save/load exacto; reutilización en al menos dos BSP distintos; máscaras y assets correctamente resueltos; edición reversible; rechazo visible de inputs/versiones incompatibles; Apply transaccional que preserve lo no seleccionado.

## Importación de heightmap y aplicación de RBrush — integración M8 pendiente

El owner autorizó pruebas matemáticas en el Playground el 2026-09-24. Aquí se probaron un mapa de altura firmado high→low, la derivación/proyección local de RBrush y la conversión de una imagen CC0 al contenedor oficial `.rbrush` con roundtrip exacto. No se implementaron la importación editable de heightmaps, la ingestión de un pack artístico externo preexistente ni la aplicación de stamps RBrush en el editor.

- **Importar heightmaps** como inputs referenciados y editables. Definir formatos, profundidad de bits, unidades, escala de altura, proyección y remuestreo; preservar el archivo fuente.
- **Aplicar RBrush** usando el formato, los GUID, hashes, presets y contratos de máscara/proyección existentes en M8. No crear un formato alternativo que también se llame RBrush.
- Ambas deben funcionar dentro de recetas, de forma no destructiva e interactiva, con preview/diff, cancelación, undo/redo, selección/lote explícitos y respeto de zonas protegidas.

Estado: **INTEGRATION REQUIRED**, owners Editor/Asset Pipeline/Geometry, implementación en M8 con milestone concreto aún por asignar.

## Calidad y reducción

La reducción de polígonos debe informar triángulos antes/después, coste y error geométrico, y preservar silueta, regiones, estratos y caras de corte. El remallado uniforme/adaptativo y la simplificación son operaciones distintas; no presentar una como prueba de la otra. La aceptación requiere capturas comparables y verificación geométrica. Los algoritmos, métricas y límites exactos probados deben quedar registrados.

## Detalle de alta resolución y bake de normales — implementado standalone; integrar en M8

Directiva del owner, 2026-09-15: la geología se desarrolla en `F:/JericoGeologyPlayground`, separada del engine. **Blender queda fuera del flujo de generación, remesh y bake.** El backend seleccionado es el código C++ del Playground; su adopción productiva sigue pendiente.

Debe conservarse el detalle de la superficie de alta resolución antes de reducir o remallar, para transferirlo a un normal map de la malla final. El flujo requerido es:

1. Evaluar y conservar una fuente de alta resolución reproducible, con receta, seed, versiones, regiones y transformaciones.
2. Generar la malla de destino mediante reducción o remallado, preservando la fuente. La simplificación `Reduce` existente no demuestra remallado isotrópico.
3. Establecer las UV, normales y tangentes definitivas del destino; proyectar sobre ese destino el detalle de la fuente conservada. El bake final debe corresponder a esa topología y a esa base tangente.
4. Verificar el resultado en el renderer/material consumidor y, durante la adopción, cocinar malla y textura por la ruta oficial. Invalidar el bake si cambian sus inputs o la parametrización del destino.

Estado vigente v9: `Source/NormalBake.cpp` implementa el bake real del high conservado hacia el low reducido y clasifica misses por otra región, backface o radio; `Tests/NormalBakeTests.cpp` verifica proyección, TBN, cobertura, mapas y diagnósticos. `Main.cpp::buildBakePreview` coordina generación/reducción/bake y `Renderer.cpp` muestra High, Low y Low+normal. El mapa normal, AO, curvatura, espesor, regiones y altura firmada se exportan junto al OBJ, receta high y métricas. `Tests/BakePaddingAudit.py` verificó que todos los tiles usados de los seis bakes finales y tres seeds sedimentarios tienen alfa completo, incluso en cuatro texels de padding. `Tests/BakeMipAudit.py` encontró en el preset 5 error angular p95 de aproximadamente 10°/18° al aplicar mips de caja 2/3 sobre el atlas actual; es diagnóstico CPU a UV fijo, no filtrado GPU real. El atlas por triángulo y sin mipmaps sigue siendo experimental: el problema medido es la pequeña área útil por triángulo al reducir de mip, no falta de dilatación base; debe haber unwrap/charts y filtrado productivos.

Prueba de escala con el mismo high/low y cero misses: 2048²/3072²/4096² dieron p95 de error angular en mip 2 de **10,12°/4,76°/1,82°** en el filtro CPU de caja. Las seis salidas RGBA sin comprimir requieren **96/216/384 MiB** y el bake observó **8,25/16,04/28,37 s** en una ejecución por tamaño. Capturas y JSON: `../Evidence/2026-09-24-layered-safe-six-family-bakes/`, `../Evidence/2026-09-24-sedimentary-3072-bake/`, `../Evidence/2026-09-24-sedimentary-4096-bake/`; detalle en `../Evidence/2026-09-24-geology-math-checkpoint.md`. No declarar un presupuesto de rendimiento ni un material con mips de producción a partir de esta simulación.

`Tests/NormalBakeTests.cpp` demuestra además que trasladar juntos high y low en el fixture analítico deja byte por byte iguales la normal tangente y todos los mapas auxiliares. La normal tangente también es idéntica al escalar uniformemente ambos meshes y el cage, y al girarlos 90° con las normales fuente actualizadas. El build focalizado y la suite actual **16/16** están en `../Evidence/normal-rigid-build.log` y `../Evidence/geology-playground-final-ctest.log`. Son fixtures analíticos, no una validación de transformaciones arbitrarias en M8.

Diagnóstico causal adicional: el filtro CPU confinado a cada tile triangular deja **idéntico** el error p95 de mip 2 en las tres resoluciones. En mip 3 lo reduce de 18,33°→15,47° (2048²), 10,87°→9,21° (3072²) y 6,20°→5,71° (4096²). Evidencia: `../Evidence/sedimentary-{2048,3072,4096}-mip-isolation-audit.json`. Por tanto, el déficit a mip 2 ya existe dentro de cada chart diminuto; más padding o aislar tiles no basta. M8 necesita agrupar/desplegar charts con densidad útil y validar mips en GPU. El confinamiento es una prueba CPU, no una propuesta de algoritmo de shipping.

Un cálculo de capacidad sobre los seis low OBJ agrupa de modo codicioso pares adyacentes de la misma región con diferencia de normales faciales ≤15°. En el sedimentario 5 baja de 18.432 a **10.999 charts potenciales**, y el lado interior teórico del tile a 2048² subiría de **7 a 11 texels**, todavía por debajo de los **14** medidos a 3072². Las demás familias y umbrales están en `../Evidence/layered-safe-chart-pair-{5,8,9,10,11,12}.json`. Esto no genera UV, no comprueba distorsión ni bake/mips y no justifica elegir 15° como umbral productivo; sirve para descartar que una simple pareja de triángulos resuelva por sí sola el sedimentario a 2048².

Se probó también reducir el low sedimentario regenerándolo a resolución **10**: 7.200 triángulos, error superficial 0,0493–0,0535 en seeds 7/17/123/911, y **cero misses** en 979.200 muestras por bake. El interior del chart sube de 7 a 16 texels en 2048², pero en seed 123 el p95 del mip 2 sólo baja 10,12°→9,06° y el error visual high/low+normal empeora 2,10→3,96; IoU de silueta 0,9964→0,9912. La repetición de 32 salidas estáticas fue idéntica. Mantener **low16** como baseline de calidad; low10 queda como candidato de LOD/performance que requiere evaluación de distancia y presupuesto, no como sustituto de UV/mips productivos. Evidencia completa: `../Evidence/2026-09-24-sedimentary-low-sweep/`, `../Evidence/2026-09-24-sedimentary-low10-bakes/` y `../Evidence/2026-09-24-geology-math-checkpoint.md`.

Cuatro vistas orbitales estáticas de la seed 123 confirman que low10 tiene mayor error visual que low16 en todos los ángulos: MAE low10 **3,96/2,47/1,40/2,35** frente a low16 **2,10/1,31/0,70/1,28**. Ver `../Evidence/sedimentary-low10-orbit-visual-audit.json` y las capturas en `../Evidence/2026-09-24-sedimentary-low10-orbit/`. Falta evaluar LOD a distancias variables y en movimiento.

En un diagnóstico de cuatro UV fijos por chart y cuatro seeds, el peor 10% de triángulos aporta **63,5–64,8%** del error angular cuadrático de mip 2 en low16 y **58,7–59,6%** en low10 (`../Evidence/sedimentary-low16-low10-mip-hotspots.json`, `../Evidence/sedimentary-seed-{7,17,911}-mip-hotspots.json`). Muchos de los peores charts están cerca del perímetro expuesto, aunque no todos en seed 17. Esto motivó el experimento de UV adaptativos siguiente; el diagnóstico no establece una política de asignación productiva.

`Tests/BakeAdaptiveAtlasBudget.py` generó dos layouts deterministas para low16 seed 123 con rectángulo por triángulo y sin solapamiento: prioridad/otros **22/14 px** ocupa 2036 de 2048 filas (98,8%) y ofrece interiores **14/6 px**; **24/13 px** ocupa 1906 filas (92,2%) e interiores **16/5 px**. Los 1.844 charts prioritarios concentran 64,7% del error cuadrático de mip 2. El bake C++ acepta ahora un layout explícito ligado por hash a sus mallas high/low. Ambos layouts se rebakearon con cero misses; la auditoría independiente confirmó rectángulos sin solapamiento, UV exportados y cero huecos alfa en charts usados. En cuatro seeds, 24/13 reduce p95 del mip 2 de caja CPU de **10,12–10,59° a 7,70–8,27°**; en seed 123, las cuatro vistas Vulkan **empeoran** MAE high/low+normal de **2,10/1,31/0,70/1,28 a 2,16/1,40/0,73/1,34**. Evidencia: `../Evidence/adaptive-bake-24-13-seed-*-export-audit.json`, `../Evidence/adaptive-orbit-visual-audit.json` y `../Evidence/2026-09-24-adaptive-bake-*/`. Mantener el atlas uniforme como referencia de calidad estática; ninguno de estos layouts está seleccionado para M8. Se requieren charts agrupados, costuras y mips GPU reales, calidad Beauty y presupuesto de material. El primer layout tiene muy poco margen de empaquetado.

Continuación standalone del preset sedimentario 5: para seis estratos, la simplificación nativa produce error superficial 0,21–0,70 y se rechaza frente al límite 0,08. El low de prueba se **regenera desde la misma receta** a resolución 16, con estrategia identificada en el JSON y error bidireccional medido 0,02549; el high a resolución 32 conserva 73.728 triángulos, el low 18.432, y el bake tiene 0 misses en 516.096 muestras. Esta ruta prueba transferencia high→low sin llamar remallado isotrópico a la regeneración; M8 debe elegir y versionar la estrategia, invalidar el bake al cambiar entradas y verificar UV/mips/materiales productivos. Los contactos completos del preset 5 se comprueban en tres seeds, pero el asentamiento no es física dinámica.

La misma ruta high→low del preset 5 se ejecutó también en seeds 17 y 911 (`Evidence/2026-09-24-sedimentary-seeded-bakes/`): ambos 73.728→18.432 triángulos, 516.096 muestras, cero misses/backfaces, errores superficiales 0,02695/0,02539. Las comparaciones Vulkan a 1280×960 reducen el error RGB de 7,64→2,09 y 7,97→2,28; diez artefactos seleccionados son idénticos en dos ejecuciones. Esto extiende la robustez standalone, pero no prueba el material ni el filtrado de M8.

La misma textura normal del preset 5 se inspeccionó desde cuatro ángulos de cámara a 1280×960 (`Evidence/2026-09-24-sedimentary-orbit-bake/`); `Evidence/sedimentary-orbit-visual-audit.json` mide mejora de error RGB high/low+normal en los cuatro, con IoU de silueta 0,9953–0,9964. Son vistas estáticas sin mips; no sustituyen Beauty/material y movimiento productivos.

Auditoría del atlas actual: `Evidence/bake-atlas-audit.json` confirma que, para el preset 5 con 18.432 triángulos low en 2048², cada chart triangular sólo tiene 7 texels por lado útil y una huella nominal de 1,53 texels en mip 2. Es una estimación de densidad, no una prueba de filtrado. M8 necesita unwrap/agrupación de charts y padding/mips productivos antes de aceptar calidad a distancia; el PNG base con cero misses no basta.

**INTEGRATION REQUIRED — adopción productiva del bake existente.** Owners Geometry/Asset Pipeline y Materials/Renderer, slice M8. Inputs: fuente de alta resolución, destino, UV/base tangente, correspondencia de regiones, parámetros de proyección y resolución de textura. Reutilizar `bakeNormals` y sus pruebas; no reimplementar desde cero. Preservar proyección explícita, correspondencia de regiones y misses visibles; agregar identidad de cook, UV/mips productivos y material consumidor.

Aceptación standalone ejecutada: seis comparaciones High/Low/Low+normal en Vulkan, pruebas de orientación, proyección y determinismo. El conjunto vigente de `../Evidence/2026-09-24-layered-safe-six-family-bakes/` tiene cero misses y backfaces en las seis familias: sedimentaria **73.728→18.432**, cliff sólido **16.896→10.316**, pared BSP **49.152→11.658**, angular **45.198→3.856**, estratos **27.648→13.086** y cráter **12.288→1.058** triángulos. La roca angular high aún carece de detalle visual convincente pese a la cobertura completa. Pendiente repetir la aceptación en el renderer/material/cook de M8, con costuras/mips productivos, invalidación y presupuestos; evaluar silueta y colisión aparte. Un normal map no recupera geometría eliminada ni declara M8 completo.

**INTEGRATION REQUIRED — umbral de RBrush en geología.** El importador/preset SDF admite umbral propio, pero el consumidor geológico M8 actual deriva R8 con 0,5 fijo. En el fixture CC0, 0,8 deja 31.009 texels interiores frente a 803.853 con 0,5 (`Evidence/rbrush-threshold-contract-final-runtime.log`). M8 debe decidir y versionar el umbral geológico y mostrar un preview coherente; cambiar `sdfDefaults.threshold` del preset no cambia hoy la máscara geológica. No adoptar la apariencia del preset como prueba del stamp hasta resolver esa diferencia.

Prueba acotada RBrush→rockfall: `Tests/RBrushInteropTests.cpp` pasa la máscara local al `evaluateGeologyRockfallReference` original del Engine. Un fixture de dos islas desconectadas produce 1 desprendimiento enmascarado, 2 sin máscara y 1 en la isla opuesta al invertirla. Comprueba interoperabilidad CPU de campo regional/soporte; no existe todavía aplicación GPU, física de caída ni stamp/cook productivo.

## Referencias técnicas nuevas para el cliff

- `E:/este/mods/CascadesDemoSecrets/CascadesDemoSecrets.pdf`, pp. 9, 12–18 y 26–27: campo firmado con pilares, canal negativo, giro y estantes periódicos; ruido multioctava con entradas rotadas y frecuencias no exactamente dobles. Son ideas para medir contra el cliff actual, no código ni aceptación. Pp. 58 y 63–64 describen mezcla triplanar; el material diagnóstico del Playground ya usa pesos triplanares, pero falta cuantificar estiramiento y comparar escalas. Pp. 83 y 89–95 usan parallax/height-map ray marching: modifica sombreado, no la malla ni su colisión.
- [GPU Gems 3, capítulo 1](https://developer.nvidia.com/gpugems/gpugems3/part-i-geometry/chapter-1-generating-complex-procedural-terrains-using-gpu): densidad 3D, warp de coordenadas, varias octavas y máscara 2D de amplitud para escarpes; adaptar sólo la matemática verificable a C++ nativo, sin portar código DirectX ni asumir que el perfil actual sea volumétrico.
- [Rock-fracturing, commit `b91965b`](https://github.com/aparis69/Rock-fracturing/tree/b91965b3011ed269cbc3a051b00c9b284aaa2e36), [licencia MIT](https://github.com/aparis69/Rock-fracturing/blob/b91965b3011ed269cbc3a051b00c9b284aaa2e36/LICENSE): `GenerateFractures`, `SDFBlock::Signed` y `ComputeBlockSDF` son fuentes C++ posibles para un prototipo de bloques/diaclasas. No se ha incorporado su código ni OpenVDB; faltan prueba visual, regiones semánticas, low/UV/bakes y rendimiento antes de seleccionarlo. `proc-rock` es GPL-3.0 y queda sólo como referencia de ideas sin copiar implementación.

El experimento v13 actual usa píxeles de un alfa CC0 importado mediante el contrato oficial RBrush como máscara para desplazar vértices de la misma malla. **No** es un stamp SDF RBrush del Engine. `Source/MeshReduction.cpp` rechaza candidatos con caras opuestas a la fuente; `Source/NormalBake.cpp` cubre charts largos y subpíxel para que un backface no quede oculto por ausencia de centros de texel. La corrida `../Evidence/2026-09-25-cliff-strictlod-candidate/` tiene tres LOD con los diez bakes y cero misses, pero la forma sigue siendo un cliff estilizado repetitivo. M8 todavía requiere un brush transaccional, material/cook oficial y Beauty representativa.

## Trabajo restante transferido al slice M8

Lo siguiente sigue pendiente de integración productiva M8, aunque la matemática standalone puede continuar por la instrucción vigente del owner. No confundir kernels aislados con sistemas completos. Las referencias viven en `F:/JericoEngine/docs/research/`; localizar por los nombres indicados y resolver alcance/autoridad en el único slice activo antes de implementar.

| Pendiente | Código o referencia de partida | Entrega/prueba que falta en M8 |
|---|---|---|
| Integrar backend y dependencias | Biblioteca `PlaygroundModel`, tabla de fuentes anterior, `CMakeLists.txt` | Una ruta oficial; target/API estable para BVH y Manifold; evitar fuentes duplicadas; ejecutar regresiones contra el módulo integrado. |
| BSP/recetas/autoría productiva | `bspSource`, `evaluateSource`, Recipe v9, marco local de `LayerFormation`, `Main.cpp` como ejemplo | Binding real GUID/revisión/transformación, preview/diff, Apply transaccional, undo/redo, cancelación, lote y zonas protegidas; reutilizar receta en dos selecciones. |
| Daño con interiores nuevos | `RockAddonMath`, DamageDecal; `M8_MIX_DAMAGE_LACES_FENCE_AGEDFX_PBG2_MATHEMATICAL_RECONSTRUCTION.md` | Cortes Boolean/SDF, agujeros/cavidades reales, caras interiores y linaje/material/colisión; integrar grietas/retículas/envejecimiento más allá de pruebas de kernels. |
| Remallado y segmentación | `MeshReduction`; `M8_SMART_REMESH_NATIVE_REIMPLEMENTATION_INPUT.md`, `SMART_REMESH_3_CPP_PORT_DESIGN.md`, `M8_SMART_SEGMENTATION_1_0_TECHNICAL_INPUT.md` | Remallado isotrópico/quad y campo 4-RoSy donde corresponda, grafo/agregación de regiones; transferencia semántica, silueta/calidad/presupuestos. Reduce no implementa estas capacidades. |
| Materiales de superficie | `SurfaceFeatures`, `RockAddonMath::agingFields`; `M8_UCUPAINT_2_4_9_MATHEMATICAL_RECONSTRUCTION.md` | MaterialLayerIR, máscaras/canales tipados, UV flow, intervalos de altura/provenance y VDM según autoridad; normalizar magnitudes y conectar a materiales reales. |
| Bake/UV productivo | `NormalBake`, `SurfaceRay`; `M8_SIMPLEBAKE_2_9_11_TECHNICAL_INPUT.md` | Unwrap/UDIM, padding/mips/AA, packing de canales/PBR, costuras y confianza de espesor; conservar high y vincular bake a todos sus inputs/versiones. |
| Heightmap y RBrush | Bake firmado y `RBrushInterop` como pruebas standalone; contrato RBrush existente de M8 | Import/editabilidad/preview/undo, resolución y proyección explícitas, GUID/hash y aplicación de stamp; no crear formato RBrush paralelo. |
| Ice/acumulación | Propuesta de superficie/Manifold/ice registrada en `DESIGN.md` | Todavía no hay acumulación de hielo ni crecimiento de carámbanos; decidir alcance con la autoridad M8 y verificar geometría/materiales si se incluye. |
| Cook y consumidores | Malla, corner normals, UV/TBN, texturas, catálogo GUID y campos | Formatos oficiales y replay source-free; materiales, colisión, LOD/streaming, destrucción/consultas y debugging según audit. Conservar semántica; los colores de region-id.png no reemplazan GUID. |
| Aceptación y rendimiento | Capturas/JSON vigentes en `../Evidence/2026-09-24-layered-safe-surface/` y `../Evidence/2026-09-24-layered-safe-six-family-bakes/` | Beauty representativa en Engine, comparación con mismas entradas/cámara/escala, timings de generación/upload/render/memoria separados; no considerar el resultado una calidad final fotorealista. |

Siguiente acción para M8 cuando ese slice tome ownership: leer este documento y `BACKEND_CONTRACT.md`, verificar el checkpoint vigente y las autoridades activas, e integrar el backend probado antes de extender los faltantes. Mientras M9 sigue activo, este Playground continúa pruebas matemáticas independientes. No ejecutar Blender. No se transfiere aquí ownership del checkout del Engine ni se modifica su ACTIVE_SLICE.

Siguiente experimento independiente del Playground: mantener idénticos el high y low16 de la seed 123 y hacer un rebake C++ que consuma los dos layouts de `../Evidence/2026-09-24-adaptive-atlas-budget/` como candidatos de atlas de 2048². Medir el mismo error angular de mip 2/3, costuras, cobertura, determinismo, bytes y tiempo frente al atlas triangular vigente; repetir cualquier candidato válido en seeds 7/17/911 y comparar capturas originales desde varios ángulos antes de promoverlo. Conservar GUID/regiones, UV y base tangente coherentes. El ranking de hotspots sólo prioriza dónde investigar, no es aceptación ni prescribe el algoritmo.

## Completitud

La dirección del backend está seleccionada; M8 no está declarado completo. Quedan por demostrar los contratos de asset/cook, representación runtime, materiales y caras interiores, colisión, LOD/streaming, transacciones del editor y replay productivo. Mantener el linaje semántico de regiones, estratos, cortes y campos a través de esas conversiones.
