# JericoEngine — autoridad M8 de geología: backend nativo probado

## 0T. Decisión vigente: integrar el código del Playground en M8 — 2026-09-15

**Directiva explícita del owner:** el Jerico Geology Playground fue el banco de
pruebas y su resultado fue aceptado como base del backend de geología de M8.
**M8 debe usar ese código dentro de JericoEngine.** El Playground no es el destino
del producto ni una tarea pendiente de investigación que deba repetirse.

Esta sección sustituye, para el productor de geología, la obligación de usar
Blender headless de0R/0S y del corpus inferior. La prohibición histórica de un
productor nativo no impide adoptar esta implementación elegida por el owner.
No se recupera automáticamente el antiguo productor retirado: se integra el
código probado del Playground. Blender y sus addons quedan como referencias de
estudio para esta geología; no son dependencias obligatorias de su generación.
Los demás usos del worker y el alcance de assets por curvas conservan sus propios
contratos. Se mantienen el orden **M9 -> M8 -> M10**, el owner único, los criterios
de salida y los nuevos controles de UI en M10.5.

### Código existente que debe integrar M8

**Cierre del banco de pruebas, por orden del owner, 2026-09-21:** la continuación
y los faltantes pertenecen al **slice M8 de `F:/JericoEngine`**. No continuar el
desarrollo en otro hilo del Playground. Éste conserva el código/evidencia de
origen; el producto, sus integraciones y aceptación se implementan en JericoEngine.
Esta actualización documental no cambia el ACTIVE_SLICE M9 ni su ownership.

Origen: **`F:/JericoGeologyPlayground`**, biblioteca C++20 `PlaygroundModel`.
El código ya reutiliza fuentes reales de Jerico y Manifold3.5.2 de su build;
no depende de Blender, ImGui ni Diligent para evaluar la pila CPU.

| Archivos bajo `Source/` del Playground | Responsabilidad |
|---|---|
| `Model.hpp`, `Model.cpp` | Recipe/Layer v5 compatible con v1–v5; operaciones ordenadas, campos, máscaras, desgaste y DamageDecal, validación, seeds, serialización y evaluación cancelable. `evaluateSource` recibe una malla fuente real. |
| `SurfaceGeometry.hpp/.cpp` | Plano abierto, Solidify y `bspSource`: selección por GUID del BspDocument real, evaluación sólida y subdivisión. |
| `LayerFormation.hpp/.cpp` | Estratos coherentes, variación de espesores, drift y linaje de regiones. |
| `SolidGeometry.hpp/.cpp` | Cortes por planos, envolventes, GUID de regiones y transferencia de atributos. |
| `MeshReduction.hpp/.cpp` | Simplificación Manifold con control independiente del error muestreado; preserva la entrada cuando no consigue reducir dentro del límite. |
| `SurfaceFeatures.hpp/.cpp` | Curvatura firmada/convexidad/concavidad, distancia a crease/borde abierto, contacto y AO con radios independientes, exposición, espesor por primer impacto y confianza. |
| `NormalBake.hpp/.cpp`, `SurfaceRay.hpp` | High -> low: proyección explícita por rayos o superficie compatible más cercana; UV/TBN, normal tangente y mapas AO/curvatura/espesor/región; export OBJ/PNG/JSON y métricas de cobertura. |
| `RockAddonMath.hpp/.cpp` | Matemáticas C++ de proyector/cráter/labio, mezcla de daño, esfera/grieta, retículas e indicadores de envejecimiento. Sólo el proyector está integrado como operación del stack; los demás son kernels probados aisladamente. |
| `Tests/` | Regresiones de recetas anteriores, determinismo, geometría, metadata, superficies, BSP y reducción; trasladar los contratos al módulo integrado. |

El frontend y renderer del Playground son consumidores de prueba. En el engine,
usar los módulos existentes de ProceduralAuthoring, JericoEditor, jobs, renderer,
asset pipeline y transacciones. No copiar de nuevo las fuentes Jerico que el
Playground compila directamente (`BspDocument.cpp`, `Guid.cpp`, `GeologyNoise.cpp`,
`SolidEvaluatorManifold.cpp`); enlazar sus targets correspondientes. Resolver el
acceso al header interno `TriangleBvh.hpp` y a Manifold mediante la frontera Jerico
privada existente. Las rutas absolutas del experimento no serán dependencias de
build, cook o Shipping una vez integrado el código en el repositorio.

### Flujo de producto y orden de trabajo en M8

**Seleccionar BSP -> crear/elegir receta -> editar pila -> preview/diff -> Apply
transaccional -> cook -> consumidores runtime.**

1. Incorporar el núcleo probado a los módulos de autoría del engine y ejecutar
   sus contratos allí. No reconstruir las operaciones desde cero ni abrir otra
   demo. Conservar el comportamiento de las recetas aceptadas y su evidencia.
2. Conectar la selección real: GUID, revisión/hash, transformaciones, bounds y
   zonas protegidas. Usar `bspSource`/`evaluateSource` como referencia del contrato;
   el fixture `Base::BspWall` no sustituye la selección del editor. Preservar el
   BSP fuente, los objetos no seleccionados y el espacio jugable/keep-clear.
3. Usar snapshots, jobs asíncronos, cancelación, fencing y caché determinista.
   Revalidar fuente y selección antes de Apply; un resultado obsoleto no puede
   reemplazar uno nuevo. Integrar undo/redo y transacción por las rutas existentes.
4. Conservar GUID, regiones, estratos, cortes y campos útiles hasta materiales,
   colisión, LOD, navegación y demás consumidores aplicables. Validar cook y replay
   source-free por la ruta oficial; no introducir otro formato runtime ni otro
   pipeline permanente. Mapear el JSON experimental a los contratos Jerico con
   migración/versionado explícitos.
5. Reproducir en el engine las recetas y resultados aceptados, con escala,
   cámara y presupuesto comparables. El frontend debe consumir este backend;
   las integraciones funcionales y los gates de producto siguen siendo exigibles.

### Recetas, heightmap y RBrush: requisitos del M8 del engine

- **Recetas:** crear, guardar, cargar, duplicar, renombrar, compartir y reutilizar
  pilas editables. Incluir orden/activación, parámetros, seeds, máscaras, espacios
  de proyección, referencias a assets por GUID/hash y versiones de schema/evaluador.
  Los presets son recetas editables. Separar la receta del binding de selección
  para reutilizarla sobre otros BSP compatibles; guardar no hornea ni destruye la
  fuente. Probar roundtrip y reutilización sobre al menos dos selecciones reales.
- **Importar heightmaps** como entradas referenciadas de la receta: formatos y
  profundidad de bits admitidos, unidades, escala de altura, proyección y
  remuestreo explícitos; conservar el asset fuente y reimportación reproducible.
- **Aplicar RBrush** mediante sus formatos y contratos ya existentes, con máscara,
  proyección, procedencia y replay/reproyección. No crear un segundo formato RBrush.
- Todo el flujo debe ser **no destructivo e interactivo**, con preview/diff,
  cancelación, undo/redo, selección/lote y respeto de locks. Heightmap y RBrush se
  incorporan al alcance de integración M8; el owner pidió documentarlos aquí,
  no implementarlos ahora en el Playground. Datos/operaciones/transacciones M8;
  nuevos controles visuales M10.5 conforme al calendario vigente.

### Evidencia disponible y aceptación pendiente de integración

**Entrega vigente v5:** `F:/JericoGeologyPlayground/build-surface/Release/`.
En `F:/JericoGeologyPlayground/Evidence/surface-features-v4/` (nombre histórico):

- `VERIFICATION.md`: resultado, alcance y límites vigentes; separa v5 de v4.
- `v5-build.log` y `v5-tests.log`: build Release y **14/14 tests**, exit 0,
  63,06 segundos, después de corregir validación de tamaños de buffers y NaN/Inf.
- `v5-input-hashes.json`: SHA256 de 36 inputs de fuente/tests/CMake/ejecutable;
  verificados sin diferencias al cierre. No es un commit: Playground no tiene Git.
- `native-final-v5-visible/surface-features-proof.json`: nueve casos, tres familias
  por tres seeds, replay determinista y capturas before/after Lit/Normals.
- `v5-bake-runtime.log` y `native-final-v5-visible/normal-bake-{10,11,12}/`:
  ejecución Vulkan exit 0, High/Low/Low+normal, OBJ, cinco PNG, receta y bake.json.
  Cráter: 12.288 -> 1.058 triángulos y cobertura 100%; estratos 100%; angular
  99,672% con 3.420 misses explícitos. No se ocultan fallos de proyección.

Pruebas a integrar: `Tests/SurfaceFeatureTests.cpp`, `RockAddonMathTests.cpp`,
`NormalBakeTests.cpp`, `BackendTests.cpp`, `BspSourceTests.cpp`,
`ReductionTests.cpp` y el resto de los 14 targets en `CMakeLists.txt` del Playground.
Reproducción: `cmake --build F:/JericoGeologyPlayground/build-surface --config Release -j 2`
y `ctest --test-dir F:/JericoGeologyPlayground/build-surface -C Release --output-on-failure`.
El detalle técnico está en `F:/JericoGeologyPlayground/BACKEND_CONTRACT.md`;
el inventario de origen en `F:/JericoGeologyPlayground/docs/M8_GEOLOGY_BACKEND.md`
es documentación auxiliar, no un slice fuera del Engine ni una autoridad superior.

**Evidencia histórica v3:**

En `F:/JericoGeologyPlayground/Evidence/`: `surface-final-tests.log` registra
11/11 suites; `surface-focused-final-tests.log`, 3/3 tras el último ajuste;
`surface-v3/runtime-proof.json` y sus PNG contienen diez presets y comparativas
Vulkan; `source-provenance.json` identifica inputs y ejecutable. El informe
`SURFACE_V3_VERIFICATION.md` conserva resultados y límites. Al integrar, conservar
en la evidencia del engine las recetas, hashes y pruebas relevantes, sin depender
del disco del Playground para reproducir el producto. La investigación de nodos
y fórmulas está en `docs/NODE_MATH_RESEARCH_2026_09_15.md` del Playground.

La reducción probada pasa de6912 a12 triángulos en el cubo planar. En el cliff,
16896 ->10756 con presupuesto0.05 y error muestreado0.03669046 unidades; hay
cambios visuales. Con presupuesto0.01 conserva16896. **No es remallado isotrópico
ni una cota continua de error.** M8 debe terminar la reducción/remesh preservando
calidad, regiones y atributos; no declarar resuelta esa obligación por enlazar
`Reduce`. El núcleo probado usa CPU; el probe FBM GPU no demuestra toda la pila GPU.

El experimento aceptado determina qué implementación usar. La integración M8
debe verificar assets/cook, representación runtime, materiales/interiores,
colisión, LOD/streaming, transacciones y consumidores downstream. Cada integración
diferida se registra como `INTEGRATION REQUIRED` con input, interfaz, owner,
dependencia y milestone; los requisitos restantes del plan M8 se conservan.
Esta actualización es documental: no ejecuta ni declara cerrada la integración.

### Faltantes asignados al M8 de JericoEngine

No declarar completa toda la investigación por los tests del Playground.
Reutilizar su código probado y cerrar estos faltantes dentro del slice M8 legal:

| Pendiente | Punto de partida | Aceptación restante |
|---|---|---|
| Adopción C++ | Biblioteca `PlaygroundModel` y fuentes anteriores | Integración en ProceduralAuthoring y targets existentes, fronteras privadas BVH/Manifold, regresiones y una sola ruta de cook. |
| BSP/recetas/autoría | `bspSource`, `evaluateSource`, Recipe v5 | GUID/revisión/transformación reales, selección protegida, preview/diff, cancelación, fencing, caché, Apply transaccional, undo/redo y receta reutilizable en dos selecciones. Nuevos controles M10.5. |
| Interiores y daño completo | `DamageDecal`, `RockAddonMath`; investigación Mix Damage/AgedFX | Boolean/SDF local con agujeros y caras interiores reales, materiales/linaje/colisión; conectar grietas, retículas y envejecimiento. El cráter actual sólo desplaza superficie. |
| Remesh/segmentación | `MeshReduction`; investigaciones Smart Remesh/Smart Segmentation | Remallado isotrópico/quad/campo 4-RoSy y agrupación de regiones según contrato, transferencia semántica y calidad. Simplificación no demuestra remallado. |
| Materiales | `SurfaceFeatures`, `agingFields`; reconstrucción Ucupaint | MaterialLayerIR, canales/máscaras tipados, UV flow, intervalos de altura/provenance y VDM según alcance vigente; consumidores reales y unidades normalizadas. |
| Bake productivo high -> low | `NormalBake`, `SurfaceRay`; investigación SimpleBake | Conservar high reproducible; UV/TBN finales del low, unwrap/UDIM, padding/mips/AA, packing/PBR, costuras/confianza de espesor e invalidación por todos los inputs. Atlas actual por triángulo, sin mips. |
| Heightmap/RBrush | Requisitos anteriores y contrato RBrush existente | Importación/reimportación, GUID/hash, proyección/unidades, receta no destructiva, preview/cancel/undo. |
| Hielo | Propuesta registrada en `F:/JericoGeologyPlayground/DESIGN.md` | Acumulación y carámbanos no implementados; reconciliar alcance con autoridad M8 antes de implementar. |
| Consumidores y aceptación | Mesh, regiones/linaje, campos, low+UV/TBN+mapas | Materiales, colisión, LOD/streaming, destrucción/consultas y demás consumidores aplicables; cook/replay source-free y Beauty representativa con métricas comparables. Colores de región no sustituyen GUID. |

Referencias matemáticas existentes en `docs/research/`:
`M8_MIX_DAMAGE_LACES_FENCE_AGEDFX_PBG2_MATHEMATICAL_RECONSTRUCTION.md`,
`M8_UCUPAINT_2_4_9_MATHEMATICAL_RECONSTRUCTION.md`,
`M8_SIMPLEBAKE_2_9_11_TECHNICAL_INPUT.md`,
`M8_SMART_REMESH_NATIVE_REIMPLEMENTATION_INPUT.md`,
`SMART_REMESH_3_CPP_PORT_DESIGN.md`, `M8_SMART_SEGMENTATION_1_0_TECHNICAL_INPUT.md`.
Los addons aportan problemas/matemáticas para implementación C++; no ejecutar
Blender como productor de estas capacidades.

**Siguiente acción cuando corresponda M8:** verificar inputs/evidencia de origen,
incorporar el backend y sus tests al Engine, y ejecutar su aceptación productiva.
Conservar receta/high y proyectar su normal sobre el low; no reemplazar ese bake
por recalcular normales geométricas del low. Estado M8: **PARTIAL / DEFERRED**;
el cierre del Playground no cierra M8 ni el Goal 1.0.

---

## 0R. Precedencia vigente y publicación — 2026-09-08

**Precedencia:** para geología, aplicar primero0T. Las exigencias de productor
Blender de esta sección son históricas en lo que contradigan la decisión15-09.

**Estado:** directiva documental publicada por petición del owner; backend
`PARTIAL`, sujeto a sus pruebas y admisión efectiva. Publicar este documento no
demuestra que su implementación, Blender objetivo o integraciones estén admitidos.

La aclaración explícita de Jerico del08-09 exige llegar al foco M8 posterior a M9
con **todo el backend construido, integrado en sus contratos no visuales y
probado**. No basta un worker mínimo, un plan o scripts sueltos. El orden global
continúa **M9 -> M8 -> M10**. La preparación backend avanza antes de entrar en M8
mediante la excepción de carpeta ya autorizada a `Tools/BlenderWorker/`; no cambia
el ID del slice M9, no crea una task nueva y no interfiere con sus mediciones.
Sólo Astra integra contratos compartidos, publica y conserva Git/checkout.

Al retomar el foco M8, el orden es:

1. Backend completo y probado como precondición de entrada.
2. Revisar y seleccionar los addons/capacidades concretos.
3. Construir el conversor/adaptador Jerico que exponga operaciones, parámetros y
   controles mediante `GeneratorSchema` y bindings del motor.
4. Integrar el front de JericoEditor; la UI nueva sigue en M10.5 según la autoridad
   vigente. El backend debe ofrecer antes sus contratos consumibles y verificables
   sin depender de widgets ni interacción con Blender.

Los Geometry Nodes propios siguen siendo preferidos. La introspección puede
traducir interfaces de node groups; un addon con operadores requiere un adaptador
headless explícito. No existe promesa de convertir automáticamente cualquier
panel Python. El catálogo local de referencia no se vuelve una dependencia del
worker, los generadores propios o Shipping. La selección de una extensión
opcional exige la admisión normal; no autoriza copiar o portar referencias.

El usuario podrá dejar extensiones en una carpeta física y ver su catálogo,
controles y diagnósticos en JericoEditor cuando se integre el front. Layer System,
selección por loop/contorno, Decal 3D y Spline Destruction conservan sus contratos
reutilizables. La autoridad espacial sigue siendo BSP/semántica/keep-clear; la
autoridad de publicación sigue siendo el cook y la transacción Jerico.

### Alcance obligatorio del backend antes de M8

| Área | Resultado que debe estar integrado y probado |
|---|---|
| Admisión y ejecución offline | Versión/build/hash admitidos, lifecycle del proceso, límites, jobs, cancelación, fencing y recuperación; sin dependencia de red. |
| Catálogo y contratos | Carpeta física y catálogo local, manifest/schema, parámetros, diagnóstico y transporte de solicitudes/resultados con versiones explícitas. |
| Generación y semántica | Geometry Nodes/bpy propios, Layer System, selección, IDs y datos semánticos conservados hasta sus consumidores. |
| Autoridad espacial | Snapshot BSP, espacio jugable, keep-clear, bounds y validación antes de aceptar/publicar resultados. |
| Preparación de assets | Export, UV, bake high/low, LOD, collision, masks, materiales y procedencia según el corpus técnico. |
| Integración no visual | Cache/determinismo, validación y reintegración, contratos de cook/runtime y consumidores físicos/materiales/navegación/destrucción aplicables; Shipping source-free y Blender-free. |
| Evidencia | Casos positivos/negativos, errores recuperables, ejecución limpia sin addons de referencia y resultados reproducibles. Cada faltante conserva estado y prueba esperada. |

La matriz actual de cobertura y faltantes vive en la documentación propia de
[`Tools/BlenderWorker/`](../Tools/BlenderWorker/README.md). Los criterios del corpus
deben mapearse a implementación y evidencia; no se presume su cumplimiento por
existir un archivo o un catálogo. Las integraciones visuales diferidas requieren
`INTEGRATION REQUIRED` con input/interfaz/owner/dependencia/milestone M10.5.

### Reconciliación explícita de instrucciones anteriores

El corpus del06-09 se conserva íntegro a continuación por su arquitectura,
investigación, ejemplos y trazabilidad. **Sus instrucciones operativas retiradas
son históricas y no deben ejecutarse.** Esta sección0R y las autoridades actuales
[`GOAL.md`](../GOAL.md) §4.1, [`AGENTS.md`](../AGENTS.md) y
[`SESSION_PROTOCOL.md`](work/SESSION_PROTOCOL.md) prevalecen sobre ellas.

| Regla del corpus original | Estado y regla vigente |
|---|---|
| §§0,1.12,38,41: empezar todo el backend sólo después de cerrar M9 | **SUPERSEDED** por aclaración08-09: preparación completa antes de entrar M8 mediante la excepción de carpeta y ventanas coordinadas; M9 sigue activo. |
| §§0,38,41: task continua entre todos los slices | **SUPERSEDED** por política07-09: un chat nuevo por slice cerrado/publicado y transferencia única de dos fases; un solo owner del checkout y sin archivar chats. |
| §§7,38,39,41: todos los controles Qt funcionales antes de M10 | **SUPERSEDED** en calendario por UI M10.5. Backend y bindings no visuales deben estar completos antes de M8; el front se implementa después de selección/adaptación y conserva todos los controles exigidos. |
| §§4,37,38: número ADR-043 observado el06-09 | **HISTÓRICO**; verificar identificador libre al admitir el ADR. Este documento no reserva ni crea ese ADR. |
| §2: versiones y hechos de investigación del06-09 | **SNAPSHOT DE INVESTIGACIÓN**; comprobar upstream, binario real y lockfile al admitir. No se presentan como una nueva verificación del08-09. |
| §39: cierre por todos los requisitos incluidos UI | Conservar requisitos; evaluar backend previo a M8 y obligaciones visuales M10.5 por separado, sin declarar completitud global ni omitir downstream. |

### Procedencia del corpus

- Fuente entregada por el usuario: `JERICOENGINE_M8_BLENDER_HEADLESS_GEOLOGY_AUTHORITY_DIRECTIVE.md`, redactada06-09.
- SHA-256 original verificado antes de importar: `c68e949b699cbbaba8639e41705de14d7a8a9eef8e2d5adcdcc0a9dfb65e0481`.
- El corpus conserva su contenido; sólo normaliza UTF-8/LF y ocho saltos Markdown a la sintaxis explícita sin espacios finales.
- Aclaración original del owner: tarea «Diseña backend Blender para Geology», mensaje08-09, turn `01a08178-36d3-7972-a228-27d5af344b6b`.
- Publicación documental solicitada expresamente; no supone admisión automática de dependencias, código externo ni declaración de backend completo.

---

## 0S. Assets por curvas dentro de M8 — directiva del owner, 2026-09-10

**Estado:** requisito de M8, implementación e integración `NOT STARTED` para este
paquete añadido; no inferir ausencia de capacidades previas ni declarar cierre
por esta documentación. El owner pidió expresamente incorporar la decisión
híbrida al alcance autoritativo M8. Esta sección especializa0R para assets por
curvas y conserva **M9 -> M8 -> M10**, el único owner y la UI nueva en M10.5.
No reabre ventanas de Blender ni autoriza implementación paralela durante M9.

### Productores y representación exigidos

- Jerico Tools/Cooker C++: tubos, cables/cuerdas, telarañas y ensamblajes por
  splines, incluyendo cadenas, rieles, escaleras y pasarelas. Reutilizar la única
  autoridad spline/path, perfiles, placement, jobs y transacciones Jerico; no
  construir una spline serializada alternativa ni exigir Blender para cada tubo.
- Blender headless: rocas, cliffs, cuevas y generación offline compleja que
  justifique el coste, con el backend propio, límites y gates de0R. No reactivar
  productores geológicos nativos retirados bajo la etiqueta de generación híbrida.
- Telarañas: malla ligera no coplanar, con caída/pliegues y efecto3D mediante
  patrón de cobertura/transparencia y normales. No generar un cilindro por hilo
  por defecto, tampoco ocultándolo en una malla fusionada. Cuerdas gruesas sí
  conservan volumen geométrico real. Ambas requieren LOD y prueba visual en movimiento.
- Runtime: assets cocinados por la ruta Jerico, estáticos o instanciados por
  defecto. Simulación/deformación se habilita sólo donde el producto la requiere,
  mediante física Jolt y contratos admitidos; no un generador pesado por frame.
- JericoEditor es el único front de autoría. En M8 se demuestran los contratos
  mediante rutas no visuales/existentes; nuevos controles, paneles y pickers M10.5.

La [investigación publicada](../Tools/BlenderWorker/CURVE_ASSET_BACKEND_RESEARCH_20260910.md)
fundamenta la comparación, no admite sus bibliotecas. Se conserva la selección
matemática de [Spline/Geometry Math](JERICO_SPLINE_AND_GEOMETRY_MATH_BACKENDS_1_0.md),
incluidos sus gates normales antes de enlazar dependencias. Primero auditar las
capacidades existentes; cualquier brecha/admisión requiere evidencia específica.

### Integraciones requeridas

Todas las filas siguientes son `INTEGRATION REQUIRED`, pendientes de demostrar:

| Subsistema / motivo | Input e interfaz esperada | Owner / dependencia / milestone |
|---|---|---|
| Spline y generación: geometría editable estable | Receta GUID/versionada, perfil, seed, anclajes y muestras -> candidato de malla/instancias | ProceduralAuthoring/Cooker; autoridad spline/path existente; M8 |
| Frames y loops: evitar giros y costuras | Orientación inicial/roll/cierre -> frame continuo probado; preservar consumidores existentes | Path/Spline; auditar frame actual antes de perfiles no circulares; M8 |
| Telaraña y cuerda: representación pedida | Anclajes + patrón/perfil -> patch con cobertura o sweep volumétrico, UV y LOD | Autoría procedural + Materials; renderer admitido tras M9; M8 |
| PWAS/BSP: conservar espacio y selección | IDs de soporte/parte/segmento, máscaras, locks, keep-clear -> colocación validada | PWAS/JericoBSP; soporte canónico y scope explícito; M8 |
| EPLS/materiales: semántica downstream | Clases físicas/materiales, máscaras y procedencia -> atributos/regiones consumibles | Materials/EPLS; misma superficie canónica; M8 |
| Cook, LOD y streaming: producto distribuible | Candidato + error medido + IDs persistentes -> formatos Jerico, chunks y dependencias | Cooker/Virtualized World; ruta única de publicación; M8 |
| Colisión/navegación/física: comportamiento útil | Proxies y anclajes/perfil físico explícito -> consumidores Jolt/nav | Physics/Navigation; proxies/queries M8, simulación opcional M10 antes de su primer consumidor; sin colisión por hebra decorativa implícita |
| Edición/recuperación: no aplicar candidatos obsoletos | Snapshot, revisión, cancelación y diff -> validación/Apply/Undo/Redo | ProceduralAuthoring + transacción; rutas existentes M8, front nuevo M10.5 |
| Front unificado: control del diseñador | Descriptores Jerico de capacidad y parámetros -> controles de JericoEditor | JericoEditor; backend probado, selección/adaptación admitida; M10.5 |

Linaje obligatorio: receta/soporte/anclajes -> generación -> malla/instancias y
metadatos -> cook/LOD/collision -> rendering, PWAS/EPLS y consumidores físicos o
gameplay aplicables. IDs y regiones no dependen del índice del LOD visible. La
auditoría de cada vertical evalúa también destrucción, animación, audio, partículas,
save/load, queries, debugging y presupuestos, declarando límites en vez de omitirlos.

### Gates M8 pendientes

1. RED/GREEN de curvas rectas/S/verticales, loops, tangentes antiparalelas,
   puntos repetidos y perfiles/codos/uniones; sin inversión de anillos o degenerados.
2. UV métricas, normales/tangentes, identidad bajo regeneración y LOD, anclajes,
   selección/locks y keep-clear sobre la envolvente completa del asset.
3. Jobs cancelables sin bloqueo del hilo principal; publicación por revisión,
   undo/redo y fallos recuperables; determinismo/cache con inputs/versiones reales.
4. Producto representativo de tubo/riel y de telaraña/cuerda, con capturas originales
   cercanas/lejos y cámara móvil. Medir aliasing, overdraw, sombras, silueta,
   transiciones LOD y CPU/GPU/memoria; un patch plano o normal bake no prueba volumen.
5. Mismo producto cocinado y replay source-free sin Blender/Python/addons/source;
   colisión/nav y modificaciones runtime cuando el uso las requiera.

Estos gates se suman al cierre M8. Las27 referencias visuales conservan su calidad
objetivo; este añadido no termina ni reduce el trabajo pendiente de rocas/cuevas.

## Corpus técnico original del06-09 — operaciones históricas subordinadas a0R/0S

# JericoEngine 1.0 — Directiva autoritativa M8 para Geology mediante Blender 5.2.1 LTS headless

**Identificador de foco:** `M8-BLENDER-HEADLESS-GEOLOGY-AUTHORITY`\
**Estado de esta directiva:** `READY FOR ADMISSION`\
**Orden de ejecución obligatorio:** terminar el M9 actualmente activo → ejecutar esta sustitución M8 → continuar M10/M10.5\
**Autoridad de front-end:** `JericoEditor` es el único front visible y editable\
**Autoridad de generación:** Blender `5.2.1 LTS` headless, fijado por versión y SHA-256, invocado como proceso externo\
**Autoridad espacial:** el documento JericoBSP y sus IDs/semántica/volúmenes protegidos\
**Autoridad de publicación:** cook, transacción y formatos runtime de JericoEngine\
**Estado del productor Geology nativo de M8:** `SUPERSEDED` para toda generación de geometría nueva cuando esta directiva sea admitida\
**Fecha de investigación y redacción:** 2026-09-06

---

## 0. Instrucción ejecutiva para Astra

Astra debe implementar esta directiva como el cambio de foco de M8 que comienza **después de cerrar y publicar M9** y **antes de abrir M10**. No debe abrir una rama, worktree, task, chat o productor de Geology paralelo. Debe continuar en el único checkout y task autoritativos de `F:\JericoEngine`, reconstruir el estado desde `AGENTS.md`, `GOAL.md`, `docs/work/ACTIVE_SLICE.md`, `docs/work/HANDOFF.md` y Git, y admitir este foco mediante el procedimiento normal del repositorio.

La misión es sustituir el productor matemático nativo de Geology por un backend Blender headless autoritativo sin perder los contratos que Jerico ya posee: selección BSP, recetas y capas, Preview/Cancel/Apply/Undo/Redo, generación asíncrona, cook `.rmesh/.rcoll/.rmat/.rtext`, physical material, fractura/destrucción, navegación, PWAS, EPLS, World Cells, dirty bounds, procedencia, determinismo, package `.epak` y Shipping source-free.

El resultado no es “abrir Blender desde Jerico” ni incrustar Blender. El resultado es un módulo profundo con una frontera estrecha:

```text
JericoEditor
  ├─ selección BSP + SelectionExpression
  ├─ Layer Stack + parámetros generados desde GeneratorSchema
  ├─ Preview / Diff / Diagnostics / Apply / Undo / Redo
  └─ Job Controller
          │ JSON/NDJSON + archivos content-addressed
          ▼
Jerico Blender Broker
  ├─ admisión, lockfile, cache, límites y cancelación
  ├─ sandbox de proceso
  └─ Blender 5.2.1 LTS --background --factory-startup --offline-mode
          │
          ▼
Jerico-owned Geometry Nodes + bpy executor
  ├─ evalúa BSP, selección y capas
  ├─ produce high/low, LOD, UV, masks, bake y collision candidates
  └─ exporta GLB de transporte + result.json autoritativo
          │
          ▼
Jerico validators + canonical cook
  ├─ valida espacio jugable, topología, atributos, límites y procedencia
  ├─ canoniza y cocina a formatos Jerico
  └─ publica atómicamente mediante ProjectTransactionCoordinator
```

No se admite ningún botón, panel o requisito de interacción dentro de Blender. Si una función de un add-on sólo se puede operar desde el `N-panel`, un modal, un gizmo de Blender o un contexto `VIEW_3D`, esa función no pertenece al producto hasta que exista un adaptador headless y un `GeneratorSchema` que la exponga en JericoEditor.

---

## 1. Decisiones no negociables

1. **JericoEditor es el único front.** Blender no aparece, no se incrusta y no conserva estado de autoría que Jerico no pueda serializar.
2. **JericoBSP es el source-of-truth espacial.** Blender recibe una instantánea explícita; nunca corrige, renombra ni toma propiedad del BSP.
3. **El Layer System es central.** La receta ordenada, sus selecciones, semillas, máscaras, operaciones, procedencia y locks pertenecen a Jerico.
4. **Nuestros Geometry Nodes son la ruta primaria.** Los add-ons gratuitos se usan sólo como investigación, referencia o dependencia admitida; ninguno se convierte por conveniencia en autoridad de diseño.
5. **Un único camino de publicación.** El GLB es transporte DCC; la autoridad runtime sigue siendo el cook Jerico a `.rmesh`, `.rcoll`, `.rmat`, `.rtext`, `.rpref`, `.rlevel`, `.rnav` y `.epak`.
6. **Shipping es source-free y Blender-free.** Shipping no contiene Blender, Python, `.blend`, node graphs, manifests de autoría, recetas, add-ons ni source GLB.
7. **Offline no significa sandbox.** `--offline-mode` se usa, pero todo Python de Blender se trata como código nativo hostil y se contiene a nivel de sistema operativo.
8. **Determinismo se prueba en la salida canónica Jerico.** No se supone que el GLB bruto sea byte-idéntico entre plataformas o builds.
9. **Ningún resultado invade espacio jugable.** La geometría que intersecte `KeepClear`, `PlayableInterior`, portales, navegación reservada o tolerancias BSP se rechaza antes de Preview aceptado y otra vez antes de Apply.
10. **Selección y ubicación son explícitas.** Cada capa declara dónde actúa; `all` implícito sólo existe cuando el schema lo publica y el usuario lo ve.
11. **La topología de destrucción pertenece a Jerico.** Blender puede proponer/prefracturar; Jerico valida chunks, bonds, anchors, physical materials y entrega a Jolt únicamente datos cocinados.
12. **No se rompe el M9 activo.** Esta directiva se admite como siguiente foco secuencial; los cambios de roadmap no autorizan trabajo simultáneo.

---

## 2. Baseline verificado

### 2.1 Blender objetivo

- Blender `5.2.1 LTS` fue publicado el 2026-08-25 dentro de la serie 5.2 LTS.
- La serie 5.2 LTS tiene soporte anunciado hasta julio de 2028.
- La distribución de producción para Windows x64 debe fijarse a un archivo oficial `blender-5.2.1-windows-x64.zip` o instalador oficial, más su SHA-256 almacenado en el lockfile de Jerico.
- La actualización a `5.2.2`, otra revisión 5.2 o una rama 5.3/6.x es un cambio de dependencia: exige investigación de release notes, regeneración de schemas, pruebas de compatibilidad, determinismo, Beauty y Shipping.

### 2.2 Instalación encontrada en esta máquina

La instalación local verificada durante esta investigación es:

```text
C:\Program Files\Blender Foundation\Blender 5.1\blender.exe
Blender: 5.1.2
build hash: ec6e62d40fa9
build date: 2026-05-19
SHA-256 blender.exe:
1E6624AF112B3C936F4B038B025EBD2BF00AE72C4B62881A6787166D71C58FA5
```

Esta instalación sirve para comprobar que Blender está disponible, pero **no satisface el pin de producción 5.2.1**. El broker debe emitir `blender.version_mismatch` y desactivar Generate/Apply autoritativos hasta que exista el binario 5.2.1 admitido. Se permite instalar 5.2.1 lado a lado; no se reemplaza ni desinstala 5.1.2 automáticamente.

### 2.3 Realidad del repositorio preservada

Se revisaron los contratos existentes de:

- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/BspDocument.hpp`
- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/BspRecipe.hpp`
- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyRecipe.hpp`
- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyLayer.hpp`
- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/GeologyCook.hpp`
- `Editor/JericoEditor/Source/GeologyGenerationJob.*`
- `Editor/JericoEditor/Include/Jerico/Editor/GeologyWorldPreparation.hpp`
- `Editor/JericoEditor/Source/GeologyWorldPreparation.cpp`
- `Editor/JericoEditor/Source/GeologyPhysicalRepairSnapshot.cpp`
- recetas/sesiones JericoBSP, Sparse SDF, RBrush, PWAS, EPLS, materials, Physical Material, destruction y navigation;
- los documentos de Geology/M8, cook/import, Material Mask Layer System, Procedural Studio, PWAS/EPLS y Shipping source-free enumerados en las fuentes internas al final.

Contratos concretos que se preservan:

- `BspDocument` ya posee `Guid`, `revision`, brushes con `Guid`, booleano y transform;
- `BspAuthoringRecipe` ya agrega nombre, `semantic` y referencia de Physical Material;
- `GeologyStackRecipe` ya posee `Guid`, quality, splines, masks y layers;
- `GeologyLayerHeader` ya posee `Guid`, nombre, enabled, strength, seed, mask y transform;
- `GeologyCookRequest` ya transporta hashes, material, physical materials, surface, topology levels y structural field;
- `GeologyCookCandidate` ya devuelve mesh/collision bytes, hashes y `remainingImpact`;
- `GeologyGenerationJob` ya define ejecución asíncrona, generación monotónica y cancelación;
- `GeologyWorldPreparation` ya define Preview/Cancel/Apply/Undo/Redo/Recover y publicación transaccional de receta, mesh, collision, level, prefab, fracture y package.

La sustitución debe adaptarse a estas fronteras antes de inventar formatos nuevos.

---

## 3. Vocabulario y autoridad de dominio

| Término | Definición autoritativa |
| --- | --- |
| `BspSnapshot` | Instantánea inmutable y hashable del BSP, sus metadatos, caras trianguladas de intercambio y volúmenes semánticos. |
| `PlayableInterior` | Volumen que debe quedar libre para jugador, cámara, interacción y traversal. |
| `KeepClear` | Volumen o banda que ninguna capa generativa puede ocupar. |
| `GeologyEnvelope` | Exterior permitido donde la geología puede existir. No concede permiso sobre `KeepClear`. |
| `SelectionExpression` | AST tipado que decide dónde actúa una capa. Se autoriza y visualiza en JericoEditor. |
| `PlacementLoop` | Ciclo de selección, preview, diagnóstico, corrección y aceptación que muestra “dónde va cada cosa”. |
| `EvaluationLoop` | Evaluación determinista y ordenada de capas; no es interacción UI. |
| `LayerStack` | Lista ordenada de capas con IDs estables, selección, operación, parámetros, máscaras y semilla derivada. |
| `GeneratorPlugin` | Paquete admitido que expone uno o más generadores headless y manifests. |
| `GeneratorSchema` | Schema UI sanitizado derivado de la interfaz real de un node group y del manifest Jerico. |
| `NodeLibrary` | `.blend` de datos con node groups, sin autoridad sobre receta o publicación. |
| `BlenderBroker` | Proceso Jerico que admite plugins, forma jobs, impone límites, mantiene cache y valida resultados. |
| `BlenderWorker` | Un proceso Blender headless contenido que ejecuta un job o inspecciona un plugin. |
| `TransportGLB` | GLB evaluado usado para trasladar malla/UV/normales/atributos; no es el asset runtime. |
| `ResultManifest` | `result.json` autoritativo con inventario, hashes, slots, procedencia, bounds, LOD, collision y máscaras. |
| `CanonicalOutput` | Bytes Jerico validados/canonizados que sí participan en determinismo y publicación. |
| `StructuralField` | Campos durabilidad/fractura/strata/cavity y regiones que consumen material, PWAS, EPLS y destrucción. |

### 3.1 Dónde va cada responsabilidad

| Necesidad | Dueño | Blender puede | Blender no puede |
| --- | --- | --- | --- |
| Forma espacial jugable | JericoBSP | leer snapshot | cambiar brush IDs, semántica o espacio jugable |
| Forma geológica | Geology Layer System | evaluar Geometry Nodes | guardar la única copia de la receta |
| Selección de superficie | `SelectionExpression` Jerico | resolver campos/atributos | aplicar implícitamente fuera de la selección |
| Material masks | Material Mask Layer System | hornear/proponer máscaras | crear un segundo material graph runtime |
| Objetos/rocas/debris | PWAS | producir campos candidatos | convertirse en sistema de placement runtime |
| Estado ambiental | EPLS | producir/consumir campos estáticos | colocar objetos físicos |
| Decal 3D | Jerico Decal/RBrush adapter | generar volumen o mesh decal | ocultar intersecciones o escribir materiales finales por su cuenta |
| Spline destructiva | Jerico spline + Structural Geometry Math | muestrear curva, cortar/prefracturar | decidir gameplay, bonds o Jolt runtime |
| Physical Material | Jerico | devolver slots candidatos | inventar identidades físicas |
| Collision | Jerico/Jolt cook | producir candidatos low-poly | publicar `.rcoll` sin validación Jerico |
| Navigation | Jerico/Recast | devolver dirty bounds/hints | cocinar navegación dentro de Blender |
| LOD/streaming | Jerico cook/World Cells | producir mallas LOD candidatas | cambiar identidad de página/world-cell |
| Cook/Apply/Undo | Jerico | escribir artefactos temporales | tocar directamente Assets/Procedural publicados |

---

## 4. Arquitectura de módulos y seams

### 4.1 Módulos nuevos

```text
F:\JericoEngine\
├─ Tools\
│  ├─ BlenderAuthoring\
│  │  ├─ CMakeLists.txt
│  │  ├─ Include\Jerico\BlenderAuthoring\
│  │  │  ├─ BlenderInstallation.hpp
│  │  │  ├─ BlenderPluginCatalog.hpp
│  │  │  ├─ BlenderPluginLock.hpp
│  │  │  ├─ GeneratorSchema.hpp
│  │  │  ├─ BlenderJobProtocol.hpp
│  │  │  ├─ BlenderJobCache.hpp
│  │  │  ├─ BlenderSandbox.hpp
│  │  │  ├─ BlenderBroker.hpp
│  │  │  └─ BlenderResultAdmission.hpp
│  │  ├─ Source\
│  │  │  ├─ BlenderInstallation.cpp
│  │  │  ├─ BlenderPluginCatalog.cpp
│  │  │  ├─ BlenderPluginLock.cpp
│  │  │  ├─ GeneratorSchema.cpp
│  │  │  ├─ BlenderJobProtocol.cpp
│  │  │  ├─ BlenderJobCache.cpp
│  │  │  ├─ BlenderSandboxWindows.cpp
│  │  │  ├─ BlenderBroker.cpp
│  │  │  └─ BlenderResultAdmission.cpp
│  │  └─ Worker\
│  │     ├─ blender_manifest.toml
│  │     ├─ __init__.py
│  │     └─ jerico_blender_worker\
│  │        ├─ cli.py
│  │        ├─ protocol.py
│  │        ├─ schema_scan.py
│  │        ├─ bsp_import.py
│  │        ├─ layer_eval.py
│  │        ├─ geometry_validate.py
│  │        ├─ uv_bake.py
│  │        ├─ lod_collision.py
│  │        ├─ glb_export.py
│  │        └─ result_manifest.py
│  └─ ProceduralAuthoring\
│     ├─ Include\Jerico\ProceduralAuthoring\BlenderGeology\
│     │  ├─ BlenderGeologyRecipe.hpp
│     │  ├─ BlenderGeologySession.hpp
│     │  ├─ BspGeologySnapshot.hpp
│     │  ├─ GeologySelectionExpression.hpp
│     │  └─ GeologyLayerStack.hpp
│     └─ Source\BlenderGeology\
│        ├─ BlenderGeologyRecipe.cpp
│        ├─ BlenderGeologySession.cpp
│        ├─ BspGeologySnapshot.cpp
│        ├─ GeologySelectionExpression.cpp
│        └─ GeologyLayerStack.cpp
├─ Editor\JericoEditor\
│  ├─ Include\Jerico\Editor\
│  │  ├─ GeneratorSchemaModel.hpp
│  │  ├─ GeneratorSchemaWidgetFactory.hpp
│  │  └─ BlenderGeologyController.hpp
│  └─ Source\
│     ├─ GeneratorSchemaModel.cpp
│     ├─ GeneratorSchemaWidgetFactory.cpp
│     └─ BlenderGeologyController.cpp
├─ Authoring\Blender\
│  ├─ blender.lock.json
│  ├─ builtin\
│  │  └─ jerico_geology_core\1.0.0\
│  │     ├─ blender_manifest.toml
│  │     ├─ jerico.generator.json
│  │     ├─ __init__.py
│  │     ├─ nodes\jerico_geology_core.blend
│  │     ├─ licenses\LICENSE-CC0-1.0.txt
│  │     ├─ licenses\LICENSE-GPL-3.0-or-later.txt
│  │     └─ tests\fixtures.json
│  ├─ approved\
│  │  └─ <plugin-id>\<version>\...
│  └─ quarantine\
│     └─ <plugin-id>\<version>\...
├─ Tests\BlenderAuthoring\
│  ├─ BlenderInstallationTests.cpp
│  ├─ BlenderPluginCatalogTests.cpp
│  ├─ GeneratorSchemaTests.cpp
│  ├─ BlenderJobProtocolTests.cpp
│  ├─ BlenderJobCacheTests.cpp
│  ├─ BlenderResultAdmissionTests.cpp
│  ├─ VerifyBlenderHeadlessSmoke.cmake
│  ├─ VerifyBlenderOfflineSandbox.cmake
│  ├─ VerifyBlenderPluginBoundary.cmake
│  ├─ VerifyBlenderGeologyDeterminism.cmake
│  ├─ VerifyBlenderGeologyShipping.cmake
│  └─ Fixtures\...
└─ docs\
   ├─ decisions\ADR-043-BLENDER-HEADLESS-GEOLOGY-AUTHORITY.md
   ├─ evidence\M8_BLENDER_HEADLESS_GEOLOGY_*.md
   └─ research\2026-09-06-blender-5-2-1-geology-admission.md
```

`ADR-043` es el siguiente identificador libre observado en el repositorio al redactar esta directiva. Astra debe volver a comprobar exclusividad inmediatamente antes de crearlo; un conflicto nuevo bloquea la admisión y obliga a renumerar el archivo sin cambiar el contenido normativo.

### 4.2 Raíces de plugins visibles para JericoEditor

El catálogo escanea, en este orden de precedencia y sin ejecutar Python:

1. `F:\JericoEngine\Authoring\Blender\builtin` — plugins Jerico versionados con el engine.
2. `F:\JericoEngine\Authoring\Blender\approved` — terceros vendorizados y admitidos.
3. `<ProjectRoot>\Procedural\BlenderPlugins` — plugins fijados por el proyecto.
4. `%LOCALAPPDATA%\JericoEngine\BlenderPlugins` — plugins globales del usuario.

`quarantine` nunca se carga. La precedencia no permite shadowing silencioso: dos paquetes con el mismo `id` y versiones/hashes diferentes generan `plugin.identity_conflict`; el lockfile elige exactamente uno.

El front muestra cada plugin con nombre, versión, origen, licencia, compatibilidad, estado (`Built-in`, `Approved`, `Project`, `User`, `Quarantined`, `Blocked`), permisos, hash, generadores y diagnóstico. “Encontrado” no significa “habilitado”.

### 4.3 Staging que Blender realmente carga

El broker no apunta Blender directamente a las raíces escaneadas. Para cada lockfile crea un snapshot inmutable/content-addressed:

```text
<JericoDDC>\Blender\ExtensionSets\<pluginSetSha256>\
└─ system\
   ├─ jerico_geology_core\...
   └─ <approved-plugin-id>\...
```

Luego inicia Blender con:

```text
BLENDER_SYSTEM_EXTENSIONS=<snapshot-root>
BLENDER_USER_RESOURCES=<job-root>\blender-user
BLENDER_SYSTEM_SCRIPTS=<jerico-worker-scripts-root>
```

El snapshot `system` es de sólo lectura. Cada job obtiene recursos de usuario y temporales nuevos. Ninguna preferencia global de Blender ni el perfil personal del usuario participa en el resultado.

---

## 5. Pin de Blender y comando headless

### 5.1 `blender.lock.json`

```json
{
  "$schema": "jerico://schemas/blender-lock/v1",
  "schemaVersion": 1,
  "platform": "windows-x64",
  "version": "5.2.1",
  "releaseChannel": "lts",
  "admissionState": "blocked-missing-5.2.1-artifact",
  "executable": "ThirdParty/Blender/5.2.1/windows-x64/blender.exe",
  "archive": "blender-5.2.1-windows-x64.zip",
  "archiveSha256": null,
  "executableSha256": null,
  "buildHash": null,
  "pythonAbi": null,
  "allowedGpuBackend": "none-for-authoritative-generation",
  "allowOnlineAccess": false,
  "workerProtocol": 1,
  "generatorSchemaVersion": 1
}
```

Este JSON representa de forma exacta el estado comprobado al 2026-09-06: el equipo tiene Blender 5.1.2, no el artefacto 5.2.1 fijado. El broker rechaza `admissionState != admitted` o cualquiera de los cuatro `null`. La herramienta de admisión lee el ZIP oficial y el ejecutable real, prueba `--version` y Python, escribe hashes hexadecimales completos y cambia el estado a `admitted`; esos campos nunca se rellenan a mano ni aceptan comodines.

### 5.2 Comando canónico

```powershell
blender.exe `
  --background `
  --factory-startup `
  --offline-mode `
  --disable-autoexec `
  --python-exit-code 73 `
  --log-level 1 `
  --python "<worker-root>\jerico_blender_worker\cli.py" `
  -- `
  --job "<job-root>\request.json"
```

Reglas:

- el orden de argumentos es fijo porque Blender procesa opciones en orden;
- `--factory-startup` elimina `startup.blend` y preferencias del usuario;
- `--offline-mode` deshabilita acceso online declarado, pero no reemplaza el sandbox;
- `--disable-autoexec` evita drivers/startup scripts arbitrarios; el único entrypoint es el worker explícito;
- `--python-exit-code 73` convierte excepciones no manejadas en fallo inequívoco;
- `--python-use-system-env` está prohibido;
- no se carga un `.blend` de usuario como archivo inicial;
- `--addons` no es el mecanismo de producción; se usa el extension set `system` fijado y el worker registra sólo IDs del lock;
- el worker consume argumentos únicamente después de `--`;
- stdout contiene exclusivamente NDJSON del protocolo; stderr se conserva como log diagnóstico acotado.

### 5.3 CPU y GPU

La generación autoritativa de malla, UV, LOD, atributos y collision candidate usa CPU. El bake normal high→low autoritativo usa Cycles CPU con samples, bounces, device y tile policy fijados. Un preview GPU puede existir detrás de un capability check y `gpu.init()` en background, pero su resultado nunca publica ni llena el cache autoritativo hasta demostrar conformance con la ruta CPU.

---

## 6. Formato de plugin

### 6.1 Layout obligatorio

```text
<plugin-id>\<semver>\
├─ blender_manifest.toml
├─ jerico.generator.json
├─ __init__.py
├─ nodes\
│  ├─ library.blend
│  └─ catalog.txt
├─ licenses\
│  ├─ LICENSE-<SPDX>.txt
│  └─ THIRD_PARTY_NOTICES.md
├─ icons\
│  └─ <generator-id>.png
└─ tests\
   ├─ fixtures.json
   └─ golden\<fixture-id>\manifest.json
```

No se admiten binarios sueltos, wheels, DLL o acceso de red en el plugin core. Una necesidad posterior de wheel requiere plataforma explícita, SBOM, hash, licencia, análisis y sandbox gate separado.

### 6.2 `blender_manifest.toml` Jerico core

```toml
schema_version = "1.0.0"
id = "jerico_geology_core"
version = "1.0.0"
name = "Jerico Geology Core"
tagline = "Headless Geometry Nodes geology generators for JericoEngine"
maintainer = "JericoEngine Authors"
type = "add-on"
tags = ["Geometry Nodes", "Modeling"]
blender_version_min = "5.2.1"
blender_version_max = "5.3.0"
license = ["SPDX:GPL-3.0-or-later"]
platforms = ["windows-x64"]

[permissions]
files = "Read admitted job inputs and write outputs inside the assigned job directory"

[build]
paths_exclude_pattern = [
  "__pycache__/",
  "/.git/",
  "/*.zip",
  "/*.blend1"
]
```

`blender_version_max = "5.3.0"` significa “menor que 5.3.0”; evita una actualización accidental. La extensión Jerico solicita `files` porque debe leer/escribir dentro del job root. No solicita `network`, `clipboard`, `camera` ni `microphone`.

### 6.3 `jerico.generator.json`

```json
{
  "$schema": "jerico://schemas/generator-plugin/v1",
  "schemaVersion": 1,
  "id": "jerico_geology_core",
  "version": "1.0.0",
  "displayName": "Jerico Geology",
  "vendor": "JericoEngine",
  "engineCompatibility": {
    "minimum": "1.0.0",
    "maximumExclusive": "1.1.0"
  },
  "blenderCompatibility": {
    "minimum": "5.2.1",
    "maximumExclusive": "5.3.0",
    "platforms": ["windows-x64"]
  },
  "headless": {
    "supported": true,
    "requiresView3D": false,
    "requiresWindowManager": false,
    "requiresGpu": false,
    "network": "forbidden",
    "writeScope": "job-root-only"
  },
  "license": {
    "python": "GPL-3.0-or-later",
    "nodeLibrary": "CC0-1.0",
    "redistribution": "allowed",
    "sourceUrl": "repo:Authoring/Blender/builtin/jerico_geology_core/1.0.0"
  },
  "libraries": [
    {
      "path": "nodes/jerico_geology_core.blend",
      "sha256": "filled-by-plugin-pack",
      "linkMode": "append-readonly"
    }
  ],
  "generators": [
    {
      "id": "jerico.rock.sedimentary_strata",
      "displayName": "Sedimentary / Strata",
      "category": "Geology/Rock",
      "nodeGroup": "JRC_GN_Rock_SedimentaryStrata",
      "schemaMode": "introspect-and-overlay",
      "overlay": "schemas/sedimentary.overlay.json",
      "capabilities": ["mesh", "lod", "uv", "masks", "collision-candidate"]
    },
    {
      "id": "jerico.rock.fractured_boulder",
      "displayName": "Fractured Boulder",
      "category": "Geology/Rock",
      "nodeGroup": "JRC_GN_Rock_FracturedBoulder",
      "schemaMode": "introspect-and-overlay",
      "overlay": "schemas/fractured.overlay.json",
      "capabilities": ["mesh", "lod", "uv", "masks", "structural", "destruction", "collision-candidate"]
    },
    {
      "id": "jerico.rock.columnar",
      "displayName": "Columnar / Basalt",
      "category": "Geology/Rock",
      "nodeGroup": "JRC_GN_Rock_Columnar",
      "schemaMode": "introspect-and-overlay",
      "overlay": "schemas/columnar.overlay.json",
      "capabilities": ["mesh", "lod", "uv", "masks", "structural", "collision-candidate"]
    },
    {
      "id": "jerico.rock.rounded_weathered",
      "displayName": "Rounded / Weathered",
      "category": "Geology/Rock",
      "nodeGroup": "JRC_GN_Rock_RoundedWeathered",
      "schemaMode": "introspect-and-overlay",
      "overlay": "schemas/rounded.overlay.json",
      "capabilities": ["mesh", "lod", "uv", "masks", "collision-candidate"]
    },
    {
      "id": "jerico.geology.cave_cliff_shell",
      "displayName": "Cave / Cliff Shell",
      "category": "Geology/World",
      "nodeGroup": "JRC_GN_CaveCliffShell",
      "schemaMode": "introspect-and-overlay",
      "overlay": "schemas/cave_cliff.overlay.json",
      "capabilities": ["bsp", "spline", "mesh", "lod", "uv", "masks", "structural", "destruction", "collision-candidate", "navigation-dirty-bounds"]
    }
  ],
  "limits": {
    "maxNodeGroups": 64,
    "maxLayers": 128,
    "maxRepeatIterations": 64,
    "maxInputObjects": 4096,
    "maxOutputObjects": 4096,
    "maxOutputVertices": 20000000,
    "maxOutputTriangles": 30000000,
    "maxTextureDimension": 8192,
    "maxTextureBytes": 1073741824
  }
}
```

Un overlay puede **restringir**, ordenar, agrupar, ocultar outputs internos y agregar semántica Jerico; nunca puede inventar un socket inexistente, ampliar min/max, cambiar tipo o sustituir el `identifier` introspectado.

---

## 7. Descubrimiento, admisión y exposición permanente en el front

### 7.1 Escaneo sin ejecutar código

El catálogo realiza primero un parse hostil y acotado de ambos manifests, normaliza rutas, prohíbe escapes/symlinks fuera del paquete, calcula SHA-256 de cada archivo y verifica:

- ID ASCII lower snake-case y SemVer válidos;
- versión/arquitectura Blender exactas;
- licencia SPDX y textos presentes;
- permisos dentro de policy;
- tamaño/número de archivos y ratio de compresión;
- ausencia de bytecode/obfuscación, ejecutables y archivos no declarados;
- hashes coincidentes con `blender.plugins.lock.json`;
- ausencia de conflicto de identidad.

### 7.2 Inspección contenida

Luego un worker de schema, un plugin y un proceso carga únicamente la biblioteca declarada y enumera node groups. No genera assets. El broker valida su NDJSON, mata el proceso al superar tiempo/memoria, y compara el node group real con el manifest.

### 7.3 Regla “siempre en el front”

Un generador sólo aparece como `Ready` cuando todos sus parámetros operables existen en `GeneratorSchema`. JericoEditor genera:

- selector de plugin/generador;
- árbol de panels;
- editor de Layer Stack;
- controles Qt por socket;
- selector visual de `SelectionExpression`;
- Preview, Compare, Diagnostics, Cancel, Apply, Undo y Redo;
- estado de cache, versión y procedencia;
- controles LOD/UV/Bake/Collision declarados.

No se duplica el `N-panel` de Blender. Se traduce la interfaz declarativa del node group a controles Jerico. Un add-on que cree botones/operadores fuera de su node-group interface debe aportar un adaptador headless declarativo; de lo contrario su estado es `ReferenceOnly`.

La entrega previa a M10 incluye funcionalidad completa mediante el inspector Qt genérico existente. M10.5 puede mejorar distribución visual, iconos, accesibilidad y paridad estética, pero no es requisito para que ningún parámetro funcione.

---

## 8. `GeneratorSchema` autogenerado

### 8.1 Fuente de verdad

Blender 5.2 cambió el acceso Python de propiedades públicas de Geometry Nodes. El scanner debe usar:

```python
node_group.interface.items_tree
modifier.properties.inputs.<identifier>.value
modifier.properties.inputs.<identifier>.type
modifier.properties.inputs.<identifier>.attribute_name
modifier.properties.outputs.<identifier>.attribute_name
```

No debe usar el patrón anterior `modifier["identifier"]`.

Por cada item de `items_tree` se capturan:

- `item_type` (`PANEL` o `SOCKET`);
- `name`, `identifier`, `description`, parent y orden;
- `in_out`, `socket_type`/`bl_socket_idname`;
- `hide_in_modifier`, `hide_value`, `default_input` y `attribute_domain`;
- default y, si la subclase los expone, `min_value`, `max_value`, `soft_min`, `soft_max`, subtype, unit y dimensiones;
- para menu socket, sus items estables y valor por defecto;
- para object/collection/material/image, el tipo de asset admitido por overlay.

Panels conservan nombre, descripción, orden y `default_closed`. El scanner rechaza identifiers vacíos/duplicados, valores no finitos, rangos invertidos, menu vacío, tipos no soportados y sockets que dependan de estado UI.

### 8.2 Mapeo Qt

| Socket/interfaz | Widget Qt | Regla |
| --- | --- | --- |
| Bool | `QCheckBox` | valor exacto |
| Int | `QSpinBox` | hard min/max del socket; sin rango ilimitado |
| Float | `QDoubleSpinBox` + slider si hay soft range | unidad y precisión explícitas |
| Float Factor | slider 0..1 + spinbox | clamp estricto |
| Distance | spinbox métrico | serializa metros |
| Angle | spinbox grados visible | serializa radianes canónicos |
| Vector | tres spinboxes | subtipo Position/Direction/Translation visible |
| Rotation | editor Euler/quat estable | serializa quaternion normalizado |
| Color | color picker lineal + campos | conversión de color declarada |
| Menu | `QComboBox` | guarda ID estable, no índice visible |
| String | `QLineEdit` | longitud y patrón del overlay |
| Object/Collection | picker de entidad/asset Jerico | jamás nombre Blender libre |
| Material | picker `.rmat` | resuelve Guid Jerico |
| Image | picker `.rtext`/source admitido | pasa copia content-addressed al job |
| Geometry input | oculto/sistema | lo conecta el worker desde BSP/source |
| Output socket | diagnóstico/binding | no es editable |
| Panel | `QGroupBox`/sección de inspector | orden/descripcion preservados |

Sockets Shader, Closure, Bundle, Matrix arbitraria, Sound, Camera, Clipboard o cualquier tipo sin serialización Jerico definida se rechazan para controles públicos. Geometry Bundles pueden usarse internamente entre nuestros node groups, pero no cruzan el protocolo ni sustituyen atributos/result manifest.

### 8.3 Schema resultante de ejemplo

```json
{
  "$schema": "jerico://schemas/generator-schema/v1",
  "schemaVersion": 1,
  "plugin": {
    "id": "jerico_geology_core",
    "version": "1.0.0",
    "packageSha256": "sha256:7a6f..."
  },
  "generator": {
    "id": "jerico.rock.sedimentary_strata",
    "nodeGroup": "JRC_GN_Rock_SedimentaryStrata",
    "nodeGroupLibrarySha256": "sha256:5d21..."
  },
  "panels": [
    {
      "id": "form",
      "label": "Form",
      "description": "Controls the admitted macro silhouette",
      "defaultClosed": false,
      "order": 0
    },
    {
      "id": "strata",
      "label": "Strata",
      "description": "Controls bedding direction, spacing and erosion",
      "defaultClosed": false,
      "order": 1
    }
  ],
  "controls": [
    {
      "id": "Socket_4",
      "binding": "modifier.properties.inputs.Socket_4.value",
      "label": "Scale",
      "description": "Overall rock dimensions in meters",
      "panel": "form",
      "order": 0,
      "type": "vec3-distance",
      "default": [3.0, 2.0, 2.5],
      "minimum": [0.1, 0.1, 0.1],
      "maximum": [200.0, 200.0, 200.0],
      "unit": "m"
    },
    {
      "id": "Socket_12",
      "binding": "modifier.properties.inputs.Socket_12.value",
      "label": "Strata Spacing",
      "description": "World-space distance between primary sediment beds",
      "panel": "strata",
      "order": 0,
      "type": "float-distance",
      "default": 0.28,
      "minimum": 0.02,
      "maximum": 5.0,
      "softMinimum": 0.05,
      "softMaximum": 1.5,
      "unit": "m"
    },
    {
      "id": "Socket_18",
      "binding": "modifier.properties.inputs.Socket_18.value",
      "label": "Weathering Profile",
      "description": "Selects the bounded erosion response",
      "panel": "strata",
      "order": 6,
      "type": "enum",
      "default": "balanced",
      "items": [
        {"id": "sharp", "label": "Sharp"},
        {"id": "balanced", "label": "Balanced"},
        {"id": "soft", "label": "Soft"}
      ]
    }
  ],
  "selection": {
    "required": true,
    "domains": ["face", "voxel"],
    "default": {"op": "all", "args": [{"predicate": "geology-envelope"}]}
  },
  "limits": {
    "estimatedWorkUnitsMaximum": 200000000,
    "repeatIterationsMaximum": 64
  },
  "schemaSha256": "sha256:0bc1..."
}
```

### 8.4 Estabilidad

La persistencia usa `identifier`, nunca label ni posición. Renombrar un label conserva recetas. Cambiar/eliminar un identifier requiere una tabla de migración declarada por `(pluginId, fromVersion, toVersion)` y golden round-trip. Si el node group y el schema hash divergen, JericoEditor deja el generador visible pero `Blocked: schema drift`, conserva valores y no ejecuta.

---

## 9. BSP source-of-truth y transporte a Blender

### 9.1 Coordenadas

La convención de intercambio fijada es:

- Jerico: right-handed, `+Y` up, `+Z` forward, metros.
- Blender: right-handed, `+Z` up, `-Y` forward, Blender Unit = 1 metro.
- Jerico → Blender: `(xB, yB, zB) = (xJ, -zJ, yJ)`.
- Blender → Jerico: `(xJ, yJ, zJ) = (xB, zB, -yB)`.
- la matriz es una rotación con determinante `+1`; no invierte winding.
- posiciones, normales, tangentes, AABB, planos y splines usan la misma conversión probada.

No se infiere up-axis del contenido. `coordinateContract` y su versión forman parte del cache key.

### 9.2 Componentes del snapshot

Cada job recibe:

```text
input/
├─ bsp.snapshot.json
├─ bsp.surface.glb
├─ masks/
│  ├─ <mask-guid>.exr
│  └─ mask-index.json
├─ splines/
│  └─ splines.json
└─ assets/
   └─ <sha256>.<ext>
```

`bsp.snapshot.json` contiene la geometría analítica/semántica autoritativa. `bsp.surface.glb` es sólo una triangulación de conveniencia para proximity/raycast/preview. Si difieren, gana el JSON y el job se rechaza como `bsp.transport_mismatch`.

### 9.3 Ejemplo de BSP

```json
{
  "$schema": "jerico://schemas/bsp-geology-snapshot/v1",
  "schemaVersion": 1,
  "document": {
    "id": "2f2c1b92-5e7f-4a55-b84a-b390d24747af",
    "revision": 1842,
    "canonicalHash": "sha256:1f53..."
  },
  "coordinateSystem": {
    "handedness": "right",
    "up": "+Y",
    "forward": "+Z",
    "unit": "m",
    "toBlender": [
      1, 0, 0, 0,
      0, 0, 1, 0,
      0, -1, 0, 0,
      0, 0, 0, 1
    ]
  },
  "brushes": [
    {
      "id": "5ac91f09-b40d-4a19-8dcf-b5ee3f46d512",
      "operation": "add",
      "name": "Tunnel_A",
      "semantic": "playable-interior",
      "tags": ["player", "camera", "navigation"],
      "shape": {
        "kind": "convex",
        "planes": [
          {"id": 0, "normal": [1, 0, 0], "distance": 4.0},
          {"id": 1, "normal": [-1, 0, 0], "distance": 4.0}
        ],
        "vertices": [],
        "faces": []
      },
      "localTransform": [
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
      ],
      "physicalMaterial": {
        "definitionGuid": "dc4d87ea-71d8-4928-a720-11b4e14c7d7d",
        "definitionHash": "sha256:a817..."
      }
    }
  ],
  "faceSlots": [
    {
      "slot": 17,
      "brushId": "5ac91f09-b40d-4a19-8dcf-b5ee3f46d512",
      "faceId": 3,
      "semantic": "cave-wall",
      "tags": ["geology-source", "material-rock"],
      "normal": [1, 0, 0],
      "materialGuid": "87d7aaca-07ba-470b-ac4f-793254573b9e",
      "physicalMaterialGuid": "dc4d87ea-71d8-4928-a720-11b4e14c7d7d"
    }
  ],
  "volumes": [
    {
      "id": "532dc6e9-153f-4b0a-b750-69b0e3a6ddef",
      "semantic": "keep-clear",
      "priority": 1000,
      "marginMeters": 0.35,
      "shape": {"kind": "bsp-brush-ref", "brushId": "5ac91f09-b40d-4a19-8dcf-b5ee3f46d512"}
    },
    {
      "id": "ed42f95f-f0f7-4dd2-9af0-329dafc1c236",
      "semantic": "geology-envelope",
      "priority": 100,
      "marginMeters": 0.0,
      "shape": {"kind": "aabb", "minimum": [-20, -4, -30], "maximum": [20, 18, 30]}
    }
  ],
  "anchors": [
    {
      "id": "fc9aed9b-b83a-4ac6-835c-af7bd56fb8e3",
      "semantic": "destruction-anchor",
      "position": [0, 8, 0],
      "radiusMeters": 1.0
    }
  ],
  "worldCells": [
    {"id": "cell:+0003:+0001:-0002", "bounds": {"minimum": [0, 0, -32], "maximum": [32, 32, 0]}}
  ]
}
```

La forma completa incluye todas las caras/planes; la abreviación anterior sólo reduce el ejemplo. El validator exige convex brush completo y finito.

### 9.4 Identidades dentro de Blender y GLB

Blender recibe atributos numéricos compactos, no GUID como float:

| Atributo | Dominio | Significado |
| --- | --- | --- |
| `jrc_source_slot` | face/point interno | índice a `faceSlots`/source table |
| `jrc_layer_slot` | face/point interno | capa que produjo la región |
| `jrc_region_slot` | face/point interno | región/chunk estable |
| `jrc_material_slot` | face | material Jerico |
| `jrc_physical_slot` | face | Physical Material Jerico |
| `jrc_keep_clear_distance` | point/voxel | distancia firmada al límite protegido |
| `jrc_provenance_weight` | point | contribución dominante |

Al exportar GLB se duplican como `_JRC_SOURCE_SLOT`, `_JRC_LAYER_SLOT`, `_JRC_REGION_SLOT`, `_JRC_MATERIAL_SLOT`, `_JRC_PHYSICAL_SLOT` y máscaras `_JRC_MASK_<NAME>` porque el exporter glTF sólo exporta atributos custom cuyo nombre comienza con `_`. Los GUID y strings viven en `result.json` y, como redundancia diagnóstica, en `extras.jerico` del objeto. Los límites por triángulo y tablas de slots del sidecar son autoritativos, pues glTF no representa de forma portable toda la semántica face-domain.

---

## 10. Regla espacial: jamás invadir espacio jugable

### 10.1 Regiones derivadas

Jerico construye tres campos antes de Blender:

```text
Protected = union(PlayableInterior, KeepClear, PortalClearance,
                  InteractionClearance, NavigationReserved,
                  CameraClearance)

Allowed = GeologyEnvelope - dilate(Protected, requiredMargin)

ContactBand = exterior_band(BspSourceFaces, minContactDepth, maxGrowthDepth)
```

La selección final de una capa de forma debe cumplir `Allowed ∩ ContactBand`, salvo un generador de roca aislada que declare un placement volume explícito y aun así respete `Allowed`.

### 10.2 Márgenes mínimos

El margin efectivo es el máximo de:

- margin del volumen semántico;
- `playerRadius + skinWidth` para traversal;
- `cameraRadius` para túneles estrechos;
- `navigationAgentRadius` en corredores;
- `collisionContactOffset`;
- error máximo de voxelización;
- error máximo de LOD/collision;
- tolerancia de cuantización/cook.

Ningún slider de plugin puede reducirlo. Un perfil puede aumentarlo.

### 10.3 Doble validación

1. **Dentro de Blender:** SDF Boolean sustrae Protected dilatado; se guarda `jrc_keep_clear_distance`; cualquier vértice con distancia negativa fuera de tolerancia aborta.
2. **Fuera de Blender:** Jerico vuelve a probar la malla, cada LOD y cada collision candidate contra el BSP analítico/volúmenes. Esta segunda prueba no confía en Python ni en los atributos exportados.

El validator externo prueba:

- triángulo–convex/volume intersection;
- distancia mínima de vértices y caras;
- ray/portal clearance;
- conectividad del corredor;
- muestras swept-sphere del player/cámara;
- diferencia de free-space antes/después;
- collision candidate, que puede ser más conservador que render mesh pero nunca invadir Protected;
- cada LOD, porque decimation puede cruzar una frontera aunque LOD0 no lo haga.

Fallo exacto:

```json
{
  "code": "geology.keep_clear_violation",
  "message": "Generated geometry intersects protected playable space",
  "details": {
    "object": "GeoChunk_000",
    "lod": 2,
    "triangle": 18431,
    "protectedVolumeId": "532dc6e9-153f-4b0a-b750-69b0e3a6ddef",
    "penetrationMeters": 0.0174,
    "requiredMarginMeters": 0.35
  }
}
```

No se recorta silenciosamente después del bake. El job falla, conserva el último Preview válido y ofrece diagnóstico/overlay rojo en JericoEditor.

---

## 11. Layer System autoritativo

### 11.1 Principio

El Layer System no es una colección de modifiers de Blender. Es una receta Jerico ordenada, serializable, versionada y transaccional. Blender recibe una copia y devuelve evidencia por capa. La lista se conserva aunque el plugin quede temporalmente ausente: el front muestra `Missing Generator`, mantiene parámetros y bloquea Generate sin destruir autoría.

Cada layer contiene:

- `id` GUID estable;
- `name` visible;
- `enabled`, `mute`, `solo`, `locked` y `manualOwnership`;
- `generatorId`, `pluginId`, plugin version, schema hash y node-group hash;
- categoría y fase;
- operación espacial;
- `SelectionExpression` explícita;
- strength/opacity y blend radius en metros;
- transform right-handed en espacio World/Object/SourceFace/SplineFrame;
- seed local y seed derivada;
- parámetros tipados por identifier;
- bindings de máscaras/attributes;
- outputs publicados;
- cache identity y procedencia;
- estado `Clean`, `Dirty`, `Evaluating`, `Valid`, `Invalid`, `MissingPlugin`, `SchemaDrift` o `Frozen`.

### 11.2 Fases y orden

| Orden | Fase | Responsabilidad | Ejemplos |
| ---: | --- | --- | --- |
| 0 | `Foundation` | importa source y fija envelope/protected | BSP source, imported mesh, primitive block |
| 100 | `Macroform` | crea silueta/masa primaria | boulder form, cliff shell, cave host |
| 200 | `Structure` | geología direccional y discontinuidades | strata, joint sets, columns, faults, fracture cells |
| 300 | `Carve` | resta vacíos controlados | cave spline, directed crack, cavity, undercut |
| 400 | `Weathering` | modifica escala media | rounding, erosion, ledges, talus/contact preservation |
| 500 | `SurfaceDetail` | detalle geométrico acotado | chips, pitting, fine warp |
| 600 | `MaterialMask` | clasifica sin cambiar silueta | slope, cavity, strata class, wet/mineral mask |
| 700 | `Decal3D` | aplica volumen/mesh decal seleccionado | erosion stamp, damage, embedded strata patch |
| 800 | `Destruction` | propone cortes/chunks/structural fields | spline cut, bonds, durability |
| 900 | `Output` | cleanup, chunks, LOD, UV, bake y collision candidates | weld, triangulate, unwrap, decimate |

El usuario puede reordenar dentro de una fase. Reordenar entre fases requiere que el schema lo permita y muestra un diagnóstico de dependencia. `Output` siempre es final. `Foundation` nunca puede ir después de una capa de forma.

### 11.3 Operaciones de composición

```text
ReplaceSource
Union
Difference
Intersect
SmoothUnion
SmoothDifference
SurfaceDisplace
AttributeOnly
MaskOnly
MaterialClassify
ChunkAnnotate
```

`SmoothUnion` y `SmoothDifference` incluyen radio métrico acotado. `SurfaceDisplace` no puede cruzar `Allowed`. `AttributeOnly`, `MaskOnly` y `MaterialClassify` no alteran posiciones ni topología; el validator compara geometry hash antes/después. `ChunkAnnotate` sólo agrega clasificación/IDs.

### 11.4 Receta completa de ejemplo

```json
{
  "$schema": "jerico://schemas/blender-geology-recipe/v1",
  "schemaVersion": 1,
  "id": "2f76fca8-10a2-4385-aede-5d19e9c0987f",
  "name": "Tunnel_A_Sedimentary",
  "source": {
    "bspDocumentId": "2f2c1b92-5e7f-4a55-b84a-b390d24747af",
    "bspRevision": 1842,
    "bspCanonicalHash": "sha256:1f53..."
  },
  "qualityProfile": "authoring-high",
  "rootSeed": "0x7A6B8C9D01234567",
  "coordinateContract": "jerico-rh-y-up-z-forward-meters/v1",
  "layers": [
    {
      "id": "1802977f-3b97-4428-8522-f7bc32eae4ab",
      "name": "BSP Contact Shell",
      "phase": "Foundation",
      "enabled": true,
      "mute": false,
      "solo": false,
      "locked": true,
      "manualOwnership": "procedural",
      "plugin": {
        "id": "jerico_geology_core",
        "version": "1.0.0",
        "schemaSha256": "sha256:33c2...",
        "nodeGroupSha256": "sha256:851a..."
      },
      "generatorId": "jerico.internal.bsp_envelope",
      "operation": "ReplaceSource",
      "selection": {
        "op": "all",
        "args": [
          {"predicate": "face-semantic", "values": ["cave-wall", "cave-ceiling"]},
          {"predicate": "inside-volume-semantic", "value": "geology-envelope"},
          {"op": "not", "arg": {"predicate": "inside-volume-semantic", "value": "keep-clear"}}
        ]
      },
      "strength": 1.0,
      "blendRadiusMeters": 0.0,
      "space": "World",
      "transform": {
        "translation": [0, 0, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1]
      },
      "seed": "derive:root/layer-guid",
      "parameters": {
        "Socket_ContactDepth": 0.45,
        "Socket_MaxGrowth": 4.0,
        "Socket_VoxelSize": 0.05
      },
      "maskBindings": [],
      "outputs": ["geometry", "source-slot", "keep-clear-distance"]
    },
    {
      "id": "a2247dc7-3550-4d8f-a30d-9b2adb5da7a8",
      "name": "Primary Strata",
      "phase": "Structure",
      "enabled": true,
      "mute": false,
      "solo": false,
      "locked": false,
      "manualOwnership": "procedural",
      "plugin": {
        "id": "jerico_geology_core",
        "version": "1.0.0",
        "schemaSha256": "sha256:0bc1...",
        "nodeGroupSha256": "sha256:59be..."
      },
      "generatorId": "jerico.rock.sedimentary_strata",
      "operation": "SurfaceDisplace",
      "selection": {
        "op": "all",
        "args": [
          {"predicate": "face-semantic", "values": ["cave-wall"]},
          {"predicate": "normal-cone", "axis": [0, 1, 0], "minimumDegrees": 35, "maximumDegrees": 145},
          {"predicate": "distance-to-spline", "splineId": "a5a40cd3-89cb-49f6-8ff0-7937e1484f9e", "minimumMeters": 0.0, "maximumMeters": 22.0},
          {"op": "not", "arg": {"predicate": "mask", "maskId": "8c37e31c-6010-4e98-8fda-1a69c77e9052", "threshold": 0.5}}
        ]
      },
      "strength": 0.86,
      "blendRadiusMeters": 0.12,
      "space": "World",
      "transform": {
        "translation": [0, 0, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1]
      },
      "seed": "derive:root/layer-guid",
      "parameters": {
        "Socket_StrataAxis": [0.08, 0.98, 0.16],
        "Socket_StrataSpacing": 0.28,
        "Socket_StrataJitter": 0.18,
        "Socket_HardnessContrast": 0.62,
        "Socket_UndercutDepth": 0.11,
        "Socket_MacroWarpScale": 6.0,
        "Socket_MacroWarpStrength": 0.22,
        "Socket_WeatheringProfile": "balanced"
      },
      "maskBindings": [
        {"input": "Socket_ManualExclusion", "maskId": "8c37e31c-6010-4e98-8fda-1a69c77e9052", "channel": "R"}
      ],
      "outputs": ["geometry", "mask-strata", "mask-fresh-break", "mask-cavity", "durability"]
    },
    {
      "id": "cbffab79-f738-4665-9a09-d1dc8cbf70ab",
      "name": "Damage Spline",
      "phase": "Destruction",
      "enabled": true,
      "mute": false,
      "solo": false,
      "locked": false,
      "manualOwnership": "hybrid-frozen-after-apply",
      "plugin": {
        "id": "jerico_geology_core",
        "version": "1.0.0",
        "schemaSha256": "sha256:4419...",
        "nodeGroupSha256": "sha256:31d4..."
      },
      "generatorId": "jerico.structural.destructive_spline",
      "operation": "Difference",
      "selection": {
        "op": "all",
        "args": [
          {"predicate": "distance-to-spline", "splineId": "9ae6907a-a29f-4617-804a-fae57622b984", "maximumMeters": 1.2},
          {"predicate": "durability-range", "minimum": 0.0, "maximum": 0.58}
        ]
      },
      "strength": 1.0,
      "blendRadiusMeters": 0.025,
      "space": "SplineFrame",
      "seed": "derive:root/layer-guid",
      "parameters": {
        "Socket_CutWidth": 0.08,
        "Socket_BranchProbability": 0.22,
        "Socket_ChunkTargetMeters": 0.65,
        "Socket_AnchorFalloff": 1.5
      },
      "maskBindings": [],
      "outputs": ["geometry", "fracture-region", "bond-candidate", "fresh-break-mask"]
    }
  ],
  "outputProfile": {
    "lodCount": 4,
    "uvPolicy": "unwrap-and-pack",
    "bakeProfile": "game-pbr-2k",
    "collisionPolicy": "stable-chunk-trimesh-or-convex",
    "chunkPolicy": "world-cell-and-structural-boundary"
  }
}
```

### 11.5 Semilla y orden

La seed de una capa no depende de su índice visual. Se calcula:

```text
layerSeed128 = first128bits(
  SHA-256("JRC_LAYER_SEED_V1" || rootSeed || layerGuid || generatorId)
)
```

Cada uso interno deriva otra seed con label estable, por ejemplo `macro-warp`, `joint-set`, `chips` o `lod-jitter`. Reordenar capas cambia el resultado por composición, no cambia las secuencias aleatorias internas de cada capa. Duplicar una capa genera GUID nuevo y por tanto variación nueva; “Duplicate exact” conserva seed explícita pero asigna GUID nuevo y marca la relación de procedencia.

### 11.6 Cache por prefijo

El cache conserva snapshots después de cada layer válido. Si cambia la capa `N`, se reutiliza `0..N-1` y se recalcula `N..final`. Mute/solo es una variante de preview no publicable; Apply exige stack normal, sin solo temporal. Una capa locked puede reutilizar su snapshot si sus dependencias siguen exactas; un source/selection/schema/hash diferente invalida el lock.

---

## 12. Selection & Placement Loop — “dónde va cada cosa”

### 12.1 Dos loops que no deben confundirse

**Loop del artista:**

```text
Select target
  → compose SelectionExpression
  → visualize affected region and inverted region
  → Preview selected layer or stack prefix
  → validate KeepClear / budgets / topology
  → adjust selection or parameters
  → Compare before/after
  → Apply one transaction or Cancel
```

**Loop de evaluación:**

```text
for layer in stable ordered LayerStack:
    resolve selection on current geometry
    evaluate one admitted node group
    validate invariants
    record stage hash/provenance
    publish stage snapshot to job-local cache candidate
```

El loop de evaluación heterogéneo se controla desde Python/Jerico para preservar cache e identidades por capa. Los `Repeat Zone` de Geometry Nodes se usan sólo dentro de un generador para pasos homogéneos acotados. `For Each Geometry Element Zone` se usa para curvas/chunks grandes cuando aporta valor; se evita dividir millones de elementos en micro-geometrías porque la documentación oficial advierte el costo.

### 12.2 AST de selección

```json
{
  "$schema": "jerico://schemas/selection-expression/v1",
  "schemaVersion": 1,
  "domain": "face",
  "expression": {
    "op": "all",
    "args": [
      {"predicate": "source-brush-id", "values": ["5ac91f09-b40d-4a19-8dcf-b5ee3f46d512"]},
      {"predicate": "face-semantic", "values": ["cave-wall"]},
      {"predicate": "slope-range", "up": [0, 1, 0], "minimumDegrees": 55.0, "maximumDegrees": 110.0},
      {"predicate": "height-range", "minimumMeters": 0.4, "maximumMeters": 8.5},
      {
        "op": "any",
        "args": [
          {"predicate": "painted-mask", "maskId": "e083d650-7431-4165-b30a-4b7b9671b52e", "minimum": 0.25},
          {"predicate": "distance-to-spline", "splineId": "a5a40cd3-89cb-49f6-8ff0-7937e1484f9e", "maximumMeters": 3.0}
        ]
      },
      {"op": "not", "arg": {"predicate": "inside-volume-semantic", "value": "keep-clear"}}
    ]
  },
  "featherMeters": 0.18,
  "freezeManualEdits": true
}
```

### 12.3 Predicados admitidos

| Familia | Predicados | Uso |
| --- | --- | --- |
| Identidad | brush/face/region/layer/material/physical ID | selección exacta y estable |
| Semántica | semantic/tag/all-tags/any-tag | cave-wall, ceiling, exterior, no-generate |
| Espacio | inside/outside volume, box, sphere, oriented box | permisos y zonas |
| Superficie | slope, normal cone, curvature, concavity, convexity, thickness | ubicación geológica/material |
| Elevación | height, distance to ground/water | estratos, humedad, weathering |
| Proximidad | spline, decal, mesh, BSP face, anchor, portal | falloff localizado |
| Máscara | painted, RBrush, imported, previous-layer output | control artístico |
| Estructura | strata class, fracture influence, durability, cavity | destrucción/material/PWAS |
| Procedural | noise con seed derivada y escala métrica | ruptura de bordes, nunca permiso espacial |
| Booleano | all, any, not, threshold, feather | composición tipada |

Una máscara/noise puede reducir dónde se genera; nunca puede ampliar fuera de `Allowed`.

### 12.4 Front JericoEditor

El inspector muestra:

```text
Layer Stack                 Viewport                         Inspector
✓ BSP Contact Shell         affected = green                Selection
✓ Primary Strata            protected = red                 ├─ Source faces
✓ Weathering                inverted = dim                  ├─ Slope 55°..110°
✓ Damage Spline             invalid = magenta               ├─ Height 0.4..8.5 m
○ Material Masks                                            ├─ Near Spline_A 3 m
                                                            ├─ PaintedMask_02
Add | Duplicate | Delete                                    └─ Exclude KeepClear
Up | Down | Mute | Solo | Lock

Preview Layer | Preview Through Here | Compare | Cancel | Apply
```

Click en una cara BSP agrega `source-face-id`; selección loop/connected/edge ring de malla se convierte a una lista de IDs o una máscara congelada con source hash. Si la topología source cambia, Jerico no remapea por índice silenciosamente: intenta remap por identidad/proximidad dentro de tolerancia, muestra diff y requiere aceptación.

### 12.5 Selección como loop/contorno

Para un loop geométrico dibujado por el usuario:

1. JericoEditor captura una spline cerrada o una cadena de aristas con GUID y frame.
2. La proyección se define (`surface-geodesic`, `world-planar`, `volume-extrusion` o `spline-tube`).
3. Se rasteriza una signed-distance mask determinista usando el contrato RBrush/mask existente.
4. La máscara se envía a Blender como EXR/mesh/curve admitidos, con transform y hash.
5. Geometry Nodes usa proximity/raycast para reconstruir el field en el dominio actual.
6. El front muestra interior, feather y exterior antes de ejecutar la capa.

Así “loop selection” no depende de la selección efímera de Edit Mode de Blender y puede reutilizarse por Geology, Decal 3D, Spline Destruction, materials, PWAS y EPLS.

---

## 13. Biblioteca Geometry Nodes propia

### 13.1 Decisión

La biblioteca `jerico_geology_core.blend` contiene node groups creados por Jerico y probados en Blender 5.2.1. Son datos CC0-1.0 sin scripts embebidos. El executor Python GPL-3.0-or-later los carga, conecta inputs por identifier y evalúa el depsgraph.

Grupos públicos:

```text
JRC_GN_Rock_SedimentaryStrata
JRC_GN_Rock_FracturedBoulder
JRC_GN_Rock_Columnar
JRC_GN_Rock_RoundedWeathered
JRC_GN_CaveCliffShell
JRC_GN_DestructiveSpline
JRC_GN_Decal3D
```

Grupos internos compartidos:

```text
JRC_GN_Input_BspEnvelope
JRC_GN_Select_Compose
JRC_GN_Select_KeepClear
JRC_GN_Field_SeededNoise3D
JRC_GN_Field_Strata
JRC_GN_Field_JointSet
JRC_GN_Field_CurvatureMasks
JRC_GN_Sdf_Compose
JRC_GN_Sdf_ContactPreserve
JRC_GN_Attribute_Provenance
JRC_GN_Output_Chunk
JRC_GN_Output_UvPrep
JRC_GN_Output_LodPrep
```

### 13.2 Reglas de construcción

- Todos los parámetros artísticos públicos viven en panels/sockets con descripción, hard min/max y unidades.
- No hay valores mágicos editables dentro del graph; las constantes matemáticas se documentan en labels internos.
- Todos los Random Value/Noise reciben seed derivada; no se usa estado global ni frame/tiempo.
- Feature size se expresa en metros, no voxels.
- La resolución es un input de quality policy, no un control artístico libre.
- Selection es un field que entra a cada operación con socket Selection; no se borra/procesa todo el mesh para luego mezclar.
- Los atributos de source/layer/region/material/physical se transfieren por ID/proximity de forma explícita y se validan.
- Antes de un boolean o SDF conversion se valida manifold/finite bounds y presupuesto.
- Se usa SDF grid para operaciones volumétricas que exigen unión/resta continua; mesh operations para cleanup/UV/LOD donde SDF destruiría identidad innecesariamente.
- Geometry Bundles 5.2 pueden transportar fields internos entre grupos, pero outputs permanentes son named attributes/result manifest.
- Repeat iterations máximas = 64 y forman parte del schema/cache.
- `For Each Geometry Element` se limita a splines, source faces o chunks macro; no a cada microface de una malla densa.

### 13.3 Pipeline compartido volumétrico

```text
BSP/source mesh
  → validate + realize instances
  → Mesh to SDF Grid at quality voxel size
  → subtract Protected/KeepClear SDF
  → run structural generator fields
  → SDF Boolean / Offset / Fillet / Mean/Median only as declared
  → Grid to Mesh threshold 0, adaptivity fixed by quality profile
  → transfer stable source classification
  → material/structural masks
  → cleanup + chunk boundaries
  → external no-invasion validation
```

Blender 5.2 aporta `Mesh to SDF Grid`, `SDF Grid Boolean`, `SDF Grid Offset`, `SDF Fillet`, operaciones mean/median/curvature, `Grid to Mesh`, `Grid Dilate & Erode`, `Store Named Attribute`, `Transfer Attributes`, `Get Attribute Names`, `UV Unwrap`, `Geometry Proximity`, `Raycast`, `Repeat Zone`, `For Each Geometry Element`, Lists y Geometry Bundles. Esta directiva usa esos bloques, pero conserva autoridad y schemas en Jerico.

---

## 14. Generador `Sedimentary / Strata`

### 14.1 Objetivo visual

Rocas y paredes con capas sedimentarias direccionales, espesores irregulares, dureza alternada, undercuts entre estratos, roturas que cruzan capas y weathering diferencial. Debe rechazar el resultado “cubo redondeado con ruido uniforme”.

### 14.2 Pipeline de nodos

```text
admitted source SDF
  → transform Position to geological frame
  → macro domain warp (low-frequency seeded vector noise)
  → project onto normalized StrataAxis
  → phase = warpedHeight / StrataSpacing
  → seeded spacing jitter + fold term
  → strata bands = periodic waveform with bounded profile
  → hardness = remap(strata class)
  → displacement = band edge * hardness * selected-facing response
  → subtract thin weak-layer undercuts
  → joint/crack field independent of band periodicity
  → contact-preserving erosion near BSP
  → clamp to Allowed and preserve Protected
  → extract mesh
  → emit strata/fresh-break/cavity/durability/mineral masks
```

La periodicidad principal no usa un Texture Coordinate generado por bounds del objeto, porque cambiar el tamaño desplazaría los estratos. Usa posición world/geological frame y origen estable. El macro warp tiene frecuencia al menos cuatro veces menor que el spacing; el detalle no puede destruir lectura de capas.

### 14.3 Controles públicos

| Panel | Control | Rango |
| --- | --- | --- |
| Form | Scale | 0.1..200 m por eje |
| Form | Contact Depth | 0.02..5 m |
| Strata | Axis | vector normalizado |
| Strata | Origin | posición métrica |
| Strata | Spacing | 0.02..5 m |
| Strata | Spacing Jitter | 0..0.45 |
| Strata | Thickness Bias | -0.8..0.8 |
| Strata | Hardness Contrast | 0..1 |
| Strata | Undercut Depth | 0..min(0.45×spacing, 0.5 m) |
| Strata | Fold Amplitude | 0..10 m |
| Strata | Fold Wavelength | max(4×spacing)..100 m |
| Warp | Macro Scale | 0.5..100 m |
| Warp | Macro Strength | 0..0.45×spacing |
| Fracture | Joint Spacing | 0.1..20 m |
| Fracture | Joint Anisotropy | 1..20 |
| Weathering | Profile | Sharp/Balanced/Soft |
| Weathering | Exposure | 0..1 |

### 14.4 Outputs

`_JRC_MASK_STRATA_CLASS`, `_JRC_MASK_HARD_LAYER`, `_JRC_MASK_WEAK_LAYER`, `_JRC_MASK_FRESH_BREAK`, `_JRC_MASK_CAVITY`, `_JRC_MASK_EROSION`, `_JRC_DURABILITY`, source/layer/region slots y geometry.

### 14.5 Acceptance focal

- autocorrelation de la silueta demuestra spacing direccional sin repetición perfecta;
- normal distribution muestra ledges y paredes, no esfera ruidosa;
- mínimo tres clases de estrato visibles en Beauty neutral;
- undercut visible supera el umbral de píxeles de la cámara fixture;
- cambio de StrataAxis rota estructura sin mover BSP/contact anchors;
- misma seed produce canonical hash idéntico en diez procesos limpios;
- KeepClear clearance pasa para LOD0..N y collision.

---

## 15. Generador `Fractured Boulder`

### 15.1 Objetivo visual

Boulder con macroplanos, joint sets geológicamente coherentes, aristas rotas, zonas frescas y erosionadas, opcional prefracture compatible con destrucción. No debe ser una icosphere decimada con Voronoi superficial.

### 15.2 Pipeline

```text
ellipsoid / imported silhouette / BSP region
  → low-frequency anisotropic displacement
  → 2..4 oriented JointSets
  → seeded cells with stable cell IDs
  → plane cuts and selected cell offsets
  → SDF difference for cracks with bounded aperture
  → edge chips on high convexity selection
  → weathering weighted by exposure/durability
  → optional structural separation into chunk candidates
  → contact base preservation
  → masks + adjacency/bond candidates
```

Cell IDs se derivan de seed/centroid cuantizado antes de extracción; Blender devuelve candidatos, Jerico reidentifica/canoniza regiones y construye el bond graph. `Cell Fracture` puede estudiarse como referencia, pero la ruta propia no depende de su operador/contexto.

### 15.3 Controles

`Scale`, `Elongation`, `Flattening`, `MacroPlaneCount 4..64`, `MacroRoughness`, `JointSetCount 1..4`, orientación/spacing/persistence por set, `CrackAperture 0..0.25 m`, `ChipScale 0.01..1 m`, `Weathering 0..1`, `ContactPreservation 0..1`, `Prefracture Off/Preview/PublishCandidate`, `TargetChunkCount 2..512`.

### 15.4 Outputs

`fracture_cell_id`, `joint_set_id`, `fresh_break`, `weathered_surface`, `durability`, `anchor_weight`, chunk mesh candidates, adjacency pairs con contact area/centroid/normal y collision candidates.

### 15.5 Acceptance

- mínimo de tres macroplanos no coplanares y silhouettes diferentes en tres seeds;
- cracks tienen interior geométrico y no son sólo color;
- prefracture recompone el volumen dentro de tolerancia sin overlaps importantes;
- bond candidates son simétricos, únicos y conectados a anchors cuando corresponde;
- Jolt recibe sólo el resultado Jerico cocinado;
- Beauty original-resolution separa fresh break, weathered planes y crack cavities.

---

## 16. Generador `Columnar / Basalt`

### 16.1 Objetivo visual

Columnas poligonales con eje geológico coherente, variación de diámetro, segmentación longitudinal, juntas transversales y erosión en extremos. Debe soportar pared, acantilado, techo y boulder columnar sin asumir Y/Z global fijo.

### 16.2 Pipeline

```text
selected volume
  → project to plane perpendicular to ColumnAxis
  → seeded 2D sites with Lloyd relaxation bounded
  → Voronoi-like cells / extruded column prisms
  → clip/intersect with host SDF and Allowed
  → bend/warp axis slowly
  → transverse joints at independent spacing
  → erode exposed ends and selected edges
  → merge where render shell requires continuity
  → retain stable column and segment IDs
```

Se prefiere construir sites/IDs de manera determinista en Python y pasarlos como points con atributos cuando la estabilidad de una distribución nativa no sea suficiente. Geometry Nodes procesa points/cells y SDF composition. El número de relajaciones es fijo por quality profile y acotado.

### 16.3 Controles

`ColumnAxis`, `MeanDiameter 0.05..10 m`, `DiameterVariation 0..0.6`, `SiteDensity`, `RelaxIterations 0..8`, `AxisWarpScale`, `AxisWarpStrength`, `JointSpacing`, `JointJitter`, `JointAperture`, `EndWeathering`, `MergeShell`, `ColumnSelectionFraction`.

### 16.4 Outputs/acceptance

Column/segment IDs estables, masks `column-side`, `column-end`, `joint`, `weathered`, durability y adjacency. La sección transversal debe ser claramente poligonal; el eje debe respetar el frame; los joints no deben alinearse todos; no hay columnas dentro de KeepClear; LOD conserva la lectura de columnas y los límites materiales.

---

## 17. Generador `Rounded / Weathered`

### 17.1 Objetivo visual

Cantos rodados, river rocks, chalk/pumice suavizados y boulders erosionados con silueta no esférica, base/contacto creíble y detalle guiado por curvatura/exposición.

### 17.2 Pipeline

```text
ellipsoid / selected source
  → asymmetric squash + principal-axis macro form
  → sparse plane cuts to break sphere signature
  → low-frequency surface displacement
  → curvature-guided SDF mean/median/fillet with bounded iterations
  → protect concave contact and underside
  → directional flow/weather streak field
  → pits only on selected exposed regions
  → output curvature/cavity/flow masks
```

`SDF Fillet` se limita a concavidades y no sustituye un redondeo general; smoothing mean/median se usa con iteraciones y voxel size fijados. El Contact Preserve mask evita una roca flotante o una base derretida.

### 17.3 Controles

`Scale`, `AxisRatios`, `Asymmetry`, `PlaneCutCount`, `Roundness`, `RoundIterations 0..24`, `MacroNoiseScale`, `MacroNoiseStrength`, `FlowAxis`, `FlowStrength`, `PitDensity`, `PitRadius`, `ContactPreserveRadius`, `UndersideFlattening`.

### 17.4 Acceptance

- tres ejes principales no colapsan a esfera salvo preset explícito;
- base contacta support sin penetración;
- roundness reduce curvature extrema de forma monótona dentro del fixture;
- pits quedan en Selection y no perforan thin regions;
- Beauty diferencia rounded river, chalk y weathered boulder mediante presets que sólo cambian parámetros declarados.

---

## 18. Generador `Cave / Cliff Shell`

### 18.1 Objetivo y fuente

El diseñador crea el espacio con BSP y splines. Jerico genera masa natural alrededor, jamás el corredor inverso. Para caves, la spline canónica soporta width left/right, floor, ceiling, roll y profile independientes por key; no se acepta una cadena de cápsulas como producto final.

### 18.2 Pipeline

```text
BSP group / selected faces / authored cave spline
  → analytical Protected and Allowed fields from Jerico
  → host mass in GeologyEnvelope
  → contact shell around selected BSP faces
  → spline frame evaluation with independent profile channels
  → cave void SDF Difference
  → macro cliff shelves / overhang fields
  → strata or columnar structural sub-group
  → anisotropic domain warp
  → contact-preserving weathering
  → subtract Protected again
  → chunk at World Cell + portal + structural boundaries
  → LOD/UV/masks/collision candidates
```

### 18.3 Controles

`MaxGrowthDepth`, `ContactDepth`, `VoxelSize` por quality, `CliffAxis`, `OverhangStrength`, `ShelfSpacing`, `UndercutDepth`, `CaveProfileSource`, `WallRoughness`, `CeilingRoughness`, `FloorPolicy`, `EntranceBlendLength`, `PortalClearance`, `StructuralStyle None/Strata/Columnar/Fractured`, `Weathering`, `ChunkTargetMeters`.

### 18.4 Chunking

El chunker respeta, en prioridad:

1. portales/entradas y boundaries BSP semánticos;
2. World Cell boundaries;
3. fracture/column/strata regions;
4. material/physical boundaries;
5. target size.

Se generan seam records entre chunks con posiciones cuantizadas, normals/tangents y adjacency. LODs vecinos deben mantener boundary vertices o skirts internos no visibles, nunca abrir agujeros al espacio jugable.

### 18.5 Acceptance

- el espacio libre BSP se conserva topológica y métricamente;
- existe overhang/undercut no representable como heightfield para cliff/cave fixtures;
- entradas se mezclan sin bloquear portales;
- ninguna pared cruza la swept sphere del jugador/cámara;
- chunks recook localmente dentro de dirty bounds;
- navegación se invalida por regiones exactas, no todo el mundo;
- Beauty original-resolution incluye exterior/interior, topology, collision y Shipping replay.

---

## 19. Investigación de add-ons, node libraries y assets gratuitos

### 19.1 Conclusión

No se encontró un paquete gratuito que, por sí solo, cubra Blender 5.2.1 headless, BSP autoritativo, keep-clear, Layer System, Selection Loop, cinco estilos, masks, high→low, UV, LOD, collision, provenance y front Qt. Por eso la decisión es:

- **Producción:** nuestros Geometry Nodes y worker.
- **Adopción externa inicial:** ninguna dependencia obligatoria.
- **Evaluación:** ejecutar candidatos en cuarentena y extraer sólo capacidades que superen la matriz completa.
- **Referencia:** estudiar ideas/nodes; no copiar sin licencia/procedencia.
- **Contenido:** no empaquetar assets de marketplace sin permiso de redistribución concreto.

La lista siguiente permite al usuario buscarlos manualmente; Jerico no los descarga ni instala automáticamente.

### 19.2 Candidatos oficiales Blender Extensions

| Candidato | Versión/compatibilidad publicada | Licencia/permisos | Valor para Jerico | Decisión |
| --- | --- | --- | --- | --- |
| [Extra Mesh Objects](https://extensions.blender.org/add-ons/extra-mesh-objects/) | 0.4.1, Blender 4.2+ | GPL-3.0-or-later, sin permisos especiales | incluye `add_mesh_rocks`; referencia de rock básico | `REFERENCE`; insuficiente para geología y UI/ops antiguos |
| [A.N.T. Landscape](https://extensions.blender.org/add-ons/antlandscape/) | 0.2.0, Blender 4.2+ | GPL-2.0-or-later, sin permisos | noise Rock/Stone/Crackle/Cracks, strata y terrain | `EVALUATE/REFERENCE` para formulas de strata; no autoridad volumétrica |
| [Cell Fracture](https://extensions.blender.org/add-ons/cell-fracture/) | 0.2.1, Blender 4.2+ | GPL-3.0-or-later, sin permisos | fractura celular/prefracture | `EVALUATE`; operador/contexto, IDs y determinismo deben probarse |
| [Erosion terrain generator](https://extensions.blender.org/add-ons/erosion-terrain-extension/) | 1.0.0, Blender 5.2+ | GPL-3.0-or-later, sin permisos | capas de ruido/erosión, release reciente | `EVALUATE`; sólo referencia hasta prueba headless/determinista |
| [Terrain Mixer](https://extensions.blender.org/add-ons/terrainmixer/) | 3.1.3, Blender 4.2+ | GPL-3.0-or-later, Files + Network | Terramel Lite, 9 heightmaps, masks y bake | `REFERENCE`; no core por red, peso, heightmaps externos y N-panel |
| [PTerrain](https://extensions.blender.org/add-ons/pterrain/) | 1.0.4, Blender 4.2.21+ | GPL-3.0-or-later, Files + Network, wheels | DEM/map tiles y progressive LOD | `REFERENCE/REJECT` para ruta offline; estudiar LOD espacial |
| [LODGen](https://extensions.blender.org/add-ons/lod-gen/) | 0.0.5, Blender 4.5+ | GPL-3.0-or-later, sin permisos | evaluated depsgraph, `new_from_object()`, LOD progresivo | `EVALUATE/REFERENCE`; nuestro LOD adapter sigue preferido |
| [LODify](https://extensions.blender.org/add-ons/lodify-optimizer/) | 2.9.0, Blender 4.2+ | GPL-3.0-or-later, Files, Windows | LOD + texture resize | `REFERENCE`; binario/tamaño y procesamiento extra innecesarios |
| [EasyMesh Batch Exporter](https://extensions.blender.org/add-ons/easymesh-batch-exporter/) | 1.6.2, Blender 4.2+, desarrollado con 4.5/5.2 | GPL-3.0-or-later, Files | GLB, modifiers/GN, hasta 4 LOD y collision naming | `EVALUATE`; comprobar headless y custom attributes |
| [Monty Batch Exporter](https://extensions.blender.org/add-ons/batch-exporter/) | 1.0.0, 2026 | GPL-3.0-or-later, Files | CLI headless, GLB y presets JSON | `EVALUATE/REFERENCE`; built-in glTF exporter evita dependencia inicial |
| [Attribute Baker](https://extensions.blender.org/add-ons/attribute-baker/) | 1.0.2, Blender 5.0+ | GPL-3.0-or-later, Files | GN attribute → PNG/EXR mediante UV | `EVALUATE`; útil como referencia para masks bake |
| [The Better Baker](https://extensions.blender.org/add-ons/thebetterbaker/) | 5.0.0, Blender 5.1+ | GPL-3.0-or-later, Files | PBR multi-material, normal/AO/UDIM | `EVALUATE`; headless y determinismo Cycles no demostrados |
| [Bake To Single Material](https://extensions.blender.org/add-ons/bake-to-single-material/) | 1.0.2, Blender 5.1+ | GPL-3.0-or-later | join, Smart UV y diffuse bake | `REFERENCE`; no normal high→low completo |
| [Game Tools](https://extensions.blender.org/add-ons/blender-game-tools/) | 0.3.2, Blender 4.2+ | GPL-3.0-or-later | SDF/Data Baker a UV, colors/normals | `EVALUATE/REFERENCE` |
| [Ucupaint](https://extensions.blender.org/add-ons/ucupaint/) | 2.4.9, Blender 4.2+ | GPL-3.0-or-later, Files + Network | capas, AO/normal/curvature/thickness, UDIM/bake | `REFERENCE`; demasiado acoplado a UI/Shader Editor |
| [ACT Game Asset Creation Toolset](https://extensions.blender.org/add-ons/act-game-asset-creation-toolset/) | 2025.2.1, Blender 4.2+ | GPL-3.0-or-later, Files | UV2, seams, LOD naming, normals/export | `EVALUATE/REFERENCE` |
| [Source Engine Collision Tools](https://extensions.blender.org/add-ons/sourceenginecollisiontools/) | 3.2.0, Blender 4.2+ | GPL-3.0-or-later | convex cleanup/fracture | `REJECT` como dependencia: semántica Source Engine y deps innecesarias |

Las versiones son un snapshot de investigación del 2026-09-06. La admisión consulta otra vez la página/release y fija el archivo/hash exacto.

### 19.3 Repositorios de autores

| Candidato | Evidencia | Aporte | Decisión |
| --- | --- | --- | --- |
| [Remi](https://github.com/shaderko/remi-blender-addon) | GPL-3.0-or-later; README declara Blender 5.1+ y pruebas `--background --factory-startup` | repair/remesh/decimate/retopo, xatlas UV, PBR y high-low | `EVALUATE` prioritario; excluir instalación online de PyMeshLab y optional CGAL/AutoRemesher hasta vendor/hash/SBOM |
| [DRH Rock Studio Support](https://github.com/pacosalasv/DRH_Rock_Studio-Support) | GPL-3.0-or-later; mínimo 4.2; desarrollo pausado; repo de soporte, no release público completo | presets basalt, boulder, chalk, cliff, conglomerate, marble, obsidian, pumice, river, sandstone, slate | `REFERENCE`; excelente taxonomía, no dependencia disponible/admitida |
| [Procedural Cliff Generator](https://github.com/marcueberall/blender.cliffgenerator) | GPL-3.0; `.blend` asset library | emitter/container/ground, curve-painted arcs, LOD chain | `EVALUATE/REFERENCE`; probar migración 5.2 y resolver UV/identidades |
| [Stylized Rock Generator](https://github.com/mertnizamoglu/Stylized-Rock-Blender) | GPL-3.0; Blender 3.0+ | Cliff/Boulder/Pebble, displacement/decimation/bake | `REFERENCE`; antiguo y orientado a N-panel |
| [Blender-Rock-Generator](https://github.com/olafmeerbeek/Blender-Rock-Generator) | MIT; repo pequeño | seed, layered noise, squash, bevel, scatter | `REFERENCE`; no reemplaza nuestros nodes ni prueba 5.2/headless |
| [Blender-Rock-Generator de lupin4](https://github.com/lupin4/Blender-Rock-Generator) | MIT | rock low-poly, UV, Voronoi, export | `REFERENCE`; script de escena completa requiere aislamiento/adaptación |
| [rock_generator_addon](https://github.com/tu2463/rock_generator_addon) | no presenta licencia visible en el repo consultado | node group `RockGenerator` y material | `REJECT` hasta licencia, versión, manifest y test headless claros |
| [Hydra](https://github.com/ozikazina/Hydra) | MIT, release 1.1.0 (2024) | erosión hidráulica/térmica/nieve/flow | `REJECT` para core headless: ModernGL/dependencias/contexts y descarga dinámica |
| [Blendpeaks](https://github.com/oormicreations/Blendpeaks) | MIT, Blender antiguo | mountains/outcrops/snow/materials | `REJECT` por antigüedad/API |
| [Crack It](https://github.com/squarednob/crack_it) | GPL-3.0 | cracks, presets Rock/Mud/Tree, Cell Fracture | `REFERENCE`; antiguo/contextual |

### 19.4 Blender Studio

Fuentes oficiales útiles para estudiar y migrar, no para incorporar sin revisión:

- [Example — Rock Generator](https://studio.blender.org/training/geometry-nodes-from-scratch/example-rock-generator/) — `.blend` CC-BY, Simon Thommes.
- [Simple Rock Scattering](https://studio.blender.org/training/geometry-nodes-from-scratch/example-simple-rock-scattering/) — scattering CC-BY.
- [Advanced Rock Scattering](https://studio.blender.org/training/geometry-nodes-from-scratch/example-advanced-rock-scattering/) — selection/distribution más completa.
- [CC0 Rock Textures](https://studio.blender.org/blog/new-cc-0-textures-rocks/) — texturas explícitamente CC0.

Se pueden estudiar graphs y reimplementar principios con procedencia. Los ejemplos de versiones anteriores se migran node por node a 5.2.1; no se arrastra un `.blend` opaco directamente a producción.

### 19.5 BlenderKit/Blendkit

La plataforma es un catálogo comunitario y cada asset requiere licencia concreta. Candidatos visuales/manuales:

- [Procedural Realistic Rock by Geo Nodes](https://www.blenderkit.com/asset-gallery-detail/d6405ec5-bf59-4cee-9da2-fc2cc4e53315/)
- [StonedV2 Mesh To Rock](https://www.blenderkit.com/asset-gallery-detail/d3279ee4-bd72-45e8-b9e2-f26b017baf44/)
- [Rock Generator Addon](https://www.blenderkit.com/asset-gallery-detail/2e130295-41f0-4506-be10-0a488ab5de6d/)
- [Procedural Desert Sandstone](https://www.blenderkit.com/asset-gallery-detail/3ff9c348-5f64-465d-a70f-7a279e2c3b24/)
- [Natural Sandstone](https://www.blenderkit.com/asset-gallery-detail/95a89d81-ba28-46b3-9557-ee45fdfce7ca/)
- [Procedural Cave Rock](https://www.blenderkit.com/asset-gallery-detail/c57f0e93-c330-416b-a4d0-91949b68290b/)
- [Cracked Granite Rock](https://www.blenderkit.com/asset-gallery-detail/619702af-26ba-4fb5-a715-0363e2688417/)
- [Cliff Wall](https://www.blenderkit.com/asset-gallery-detail/24dd8542-0a86-41af-93ae-7361ca03d191/)

Estado: `REFERENCE/EVALUATE`. No se descarga durante un job, no requiere cuenta/red en producción, no se redistribuye dentro de Jerico y no se marca CC0 si la página concreta no lo prueba. Un asset adquirido por el usuario puede entrar como source content del proyecto mediante el importador hostil normal y su license record.

### 19.6 Gumroad/Superhive y otros marketplaces

- [S4C Rock Generator](https://shift4cube.gumroad.com/l/RockGenerator) — Blender 4.0+, GN, auto UV; pay-what-you-want, licencia de redistribución no visible: `REFERENCE`.
- [World Blender](https://lancephan.gumroad.com/l/wb2025) — Basics gratuito, terraces/erosion/masks/11 rock nodes; licencia free tier no suficientemente explícita: `REFERENCE`.
- [WallDraw](https://pleebs.gumroad.com/l/WallDraw) — curve-driven stone walls y borrado por otra curva; personal/commercial pero no reventa: `REFERENCE` para Spline Destruction, no vendor.
- [Surface Blend](https://miladkambari.gumroad.com/p/surface-blend-free-blender-addon) — proximity-based layered surface blend, Blender 4.2+/5.0; licencia, Blender 5.2 y headless no están demostrados: `REFERENCE`, no admitido.
- [Rock Generator — Geometry Nodes](https://superhivemarket.com/products/rock-generator) — producto pago y no 5.2 declarado: fuera de la lista free de admisión; referencia visual opcional.

Un precio `$0` no concede derecho de vendor/redistribución. El usuario puede evaluarlos manualmente, pero sólo un paquete con licencia exacta, source, hashes, manifest y test suite entra a `approved`.

### 19.7 Matriz estilo → fuentes → decisión propia

| Estilo | Fuentes estudiables | Implementación Jerico elegida |
| --- | --- | --- |
| Sedimentary/strata | A.N.T., Blender Studio, DRH taxonomy, BlenderKit sandstone, World Blender terraces | `JRC_GN_Rock_SedimentaryStrata` propio |
| Fractured boulder | Cell Fracture, Crack It, DRH, cracked granite assets | `JRC_GN_Rock_FracturedBoulder` + Structural Geometry Math propio |
| Columnar basalt | DRH taxonomy; no se encontró generador free 5.2 dedicado/verificable | `JRC_GN_Rock_Columnar` propio |
| Rounded/weathered | Extra Mesh Objects, Blender Studio Rock Generator, S4C, DRH, BlenderKit refs | `JRC_GN_Rock_RoundedWeathered` propio |
| Cave/cliff | Procedural Cliff Generator, A.N.T., Terrain Mixer, cave/cliff assets | `JRC_GN_CaveCliffShell` propio, BSP/spline-driven |
| UV/bake | Remi, Attribute Baker, Better Baker, Ucupaint | ruta bpy/Cycles propia; candidatos como conformance/reference |
| LOD/export | LODGen, EasyMesh, Monty | ruta depsgraph/glTF/canonical Jerico propia |

---

## 20. Integración con Decal 3D

### 20.1 Un solo contrato de selección/capas

`JRC_GN_Decal3D` consume exactamente `SelectionExpression`, transform, spline/volume masks y Layer Stack. No crea un editor paralelo. Un Decal 3D puede operar como:

- `SurfaceOnly`: desplaza/recorta dentro de una profundidad acotada;
- `VolumeUnion`: añade volumen geológico dentro de Allowed;
- `VolumeDifference`: talla una huella/corte/cavidad;
- `AttributeOnly`: sólo emite mask/material/fresh-break/wetness/damage;
- `MeshInsert`: incrusta una malla admitida y la fusiona/recorta con política explícita.

### 20.2 Recipe adapter

```json
{
  "layerKind": "Decal3D",
  "decalId": "0ac35782-acde-44ba-a7cb-749f279aa536",
  "projection": "oriented-volume",
  "operation": "VolumeDifference",
  "depthMeters": 0.12,
  "featherMeters": 0.025,
  "selection": {
    "op": "all",
    "args": [
      {"predicate": "face-semantic", "values": ["cave-wall"]},
      {"predicate": "inside-decal-volume", "decalId": "0ac35782-acde-44ba-a7cb-749f279aa536"},
      {"op": "not", "arg": {"predicate": "inside-volume-semantic", "value": "keep-clear"}}
    ]
  },
  "outputs": ["geometry", "damage-mask", "fresh-break-mask", "decal-provenance"]
}
```

### 20.3 Reutilización

- RBrush/Material Mask aporta brush/texture/height mask y proyección.
- Geology aporta SDF compose/contact preserve.
- Decal system conserva identidad, placement, visibility y material contract.
- El cook final no distingue una malla “hecha por Blender”; sólo admite output Jerico validado.
- Si el decal sólo afecta material, geometry hash debe quedar idéntico.
- Si altera silueta, se recalculan LOD, UV/bake afectados, collision/nav dirty bounds y world-cell pages locales.

---

## 21. Integración con Spline Destruction

### 21.1 Autoridad

La spline y sus keyframes/handles pertenecen a Jerico. Blender evalúa un corte candidato. `Jerico Structural Geometry Math` conserva JointSet, FractureCells, anisotropic cells, DirectedCrack, durability, masks, adjacency y anchors compartidos entre Geology y destruction.

### 21.2 Pipeline

```text
Jerico spline (stable GUID + sampled arc-length table)
  → width/depth/falloff/branch profile
  → curve to points with deterministic sampling in meters
  → points/curve to SDF cutting volume
  → optional seeded branch curves bounded by selection/durability
  → Difference from selected geology/BSP-derived mesh
  → identify separated regions
  → emit chunk and contact candidates
  → Jerico validates, canonicalizes, builds bonds/anchors
  → Physical Material maps strength/toughness/friction
  → Jolt simulates only cooked chunks
```

### 21.3 Tipos de spline

| Tipo | Uso | Output |
| --- | --- | --- |
| `CrackGuide` | fractura visible superficial/profunda | crack SDF + fresh-break mask |
| `CutLoop` | corte cerrado alrededor de región | separated chunk candidate |
| `CollapsePath` | cadena estructural de fallo | ordered weak bonds |
| `CaveCarve` | vacío geológico authored | cave shell, no runtime fracture por defecto |
| `DecalPath` | daño/erosión/material lineal | decal geometry/masks |

### 21.4 Garantías

- sampling por arc-length y metros, no por número visual de control points;
- frame right-handed, finito y estable; manejo explícito de torsión/roll;
- un loop cerrado conserva winding/inside policy;
- spline que cruza KeepClear se recorta/rechaza según policy visible; nunca corta espacio no autorizado silenciosamente;
- chunk IDs derivan de source/layer/spline/region cuantizada, no índice temporal;
- anchors BSP/manuales tienen prioridad sobre fractura procedural;
- cambios locales invalidan sólo bounds/chunks afectados;
- Preview de fractura no modifica bodies Jolt ni level; Apply publica mesh/collision/prefab/fracture/level en una transacción existente.

---

## 22. Material Layer System y masks

### 22.1 Outputs mínimos

Cada producto Geology debe poder devolver, cuando sean relevantes:

```text
slope
normal
curvature
concavity
convexity
cavity
ambient/contact occlusion candidate
strata class
hard/weak layer
fracture influence
fresh break
weathering/exposure
flow
moisture potential
mineral class
durability
debris/talus potential
source/layer/region/material/physical slots
```

Se calculan una sola vez y se enrutan a consumidores Jerico. EPLS no vuelve a inventar cavity; PWAS no vuelve a inferir fracture si existe un field válido.

### 22.2 Representación

- campos discretos/IDs permanecen como accessors/sidecar, sin interpolación destructiva;
- máscaras escalares se mantienen float durante generación;
- bake final elige 8/16-bit/channel según error medido;
- normals high→low son tangent-space +Y para glTF/convención Jerico declarada;
- masks combinables se pueden empaquetar en `.rtext` sólo después de asignación semántica estable, nunca por conveniencia de orden;
- el nombre `_JRC_MASK_*` es transporte; el nombre runtime lo resuelve el material descriptor/cook.

### 22.3 Capas materiales

El sistema existente conserva tres significados diferentes:

- `WeightBlend`: mezcla espacial horizontal.
- `HeightBlend`: mezcla por altura micro/macro con pesos normalizados.
- `PhysicalLayer`: coat/thin film/fuzz/dust admitidos por el master shader.

La geología proporciona masks. No compila un shader propio ni crea un master material paralelo. El usuario enlaza outputs a presets/material instances en JericoEditor, con Preview/Diff/Apply.

### 22.4 Selección de materiales

Ejemplo:

```json
{
  "materialLayers": [
    {
      "id": "rock-base",
      "materialGuid": "87d7aaca-07ba-470b-ac4f-793254573b9e",
      "blend": "WeightBlend",
      "weight": "1 - _JRC_MASK_FRESH_BREAK"
    },
    {
      "id": "fresh-break",
      "materialGuid": "619e0d1b-dfe8-48e6-b044-3b7e292dd418",
      "blend": "HeightBlend",
      "weight": "_JRC_MASK_FRESH_BREAK",
      "heightStrength": 0.18,
      "transition": 0.07
    },
    {
      "id": "wet-cavity",
      "materialGuid": "32f25d8d-ab30-4a48-a0fe-723b0b3f29ab",
      "blend": "PhysicalLayer",
      "weight": "_JRC_MASK_CAVITY * EPLS.Wetness",
      "layer": "coat"
    }
  ]
}
```

La expresión se serializa como graph/AST tipado del Material Mask System, no como string ejecutable; el string anterior sólo hace legible el ejemplo.

---

## 23. Protocolo de job

### 23.1 Transporte

- request: un `request.json` UTF-8, canonical JSON para hashing;
- eventos: una línea JSON por evento en stdout (`NDJSON`), tamaño máximo 64 KiB/línea;
- artefactos: sólo rutas relativas dentro del job root, con SHA-256/tamaño;
- stderr: log humano, máximo conservado 8 MiB, nunca parseado como éxito;
- exit code: `0` sólo después de `completed`; `73` excepción Python; otros códigos mapeados por broker;
- un job no abre socket, pipe de red ni callback al editor;
- cancelación cooperativa por archivo atómico `control/cancel.requested`; kill duro por Job Object tras grace period.

### 23.2 Request completo

```json
{
  "$schema": "jerico://schemas/blender-job/v1",
  "protocolVersion": 1,
  "jobId": "9ce40c8b-23e3-49ea-8496-6f98db563159",
  "operation": "generate-geology",
  "mode": "authoritative",
  "createdBy": {
    "engineBuild": "JericoEngine-1.0-dev",
    "editorSession": "acbbdcbf-891d-42e1-b239-75d7dcc46a8a",
    "generationTicket": 184
  },
  "runtime": {
    "platform": "windows-x64",
    "blenderVersion": "5.2.1",
    "blenderBuildHash": "locked-build-hash",
    "blenderExecutableSha256": "sha256:locked-executable-hash",
    "workerVersion": "1.0.0",
    "workerSha256": "sha256:2fe4...",
    "pluginSetSha256": "sha256:b7c1...",
    "offline": true,
    "factoryStartup": true,
    "gpu": "disabled"
  },
  "generator": {
    "pluginId": "jerico_geology_core",
    "pluginVersion": "1.0.0",
    "pluginSha256": "sha256:7a6f...",
    "id": "jerico.geology.cave_cliff_shell",
    "nodeGroup": "JRC_GN_CaveCliffShell",
    "nodeGroupSha256": "sha256:0c01...",
    "schemaSha256": "sha256:66a0..."
  },
  "inputs": {
    "root": "input",
    "bspSnapshot": {
      "path": "input/bsp.snapshot.json",
      "sha256": "sha256:1f53...",
      "documentId": "2f2c1b92-5e7f-4a55-b84a-b390d24747af",
      "revision": 1842
    },
    "bspSurface": {
      "path": "input/bsp.surface.glb",
      "sha256": "sha256:791c..."
    },
    "recipe": {
      "path": "input/geology.recipe.json",
      "sha256": "sha256:1e4a..."
    },
    "assets": [
      {
        "logicalId": "mask:8c37e31c-6010-4e98-8fda-1a69c77e9052",
        "path": "input/masks/8c37e31c.exr",
        "sha256": "sha256:992e...",
        "mediaType": "image/exr"
      }
    ]
  },
  "quality": {
    "id": "authoring-high/v1",
    "voxelSizeMeters": 0.05,
    "gridNarrowBandVoxels": 4,
    "surfaceAdaptivity": 0.02,
    "positionQuantizationMeters": 0.00001,
    "normalQuantization": 0.000001,
    "uvQuantization": 0.000001
  },
  "outputs": {
    "root": "output",
    "transport": "glb",
    "resultManifest": "output/result.json",
    "includeHigh": true,
    "lodCount": 4,
    "includeMasks": true,
    "includeBake": true,
    "includeCollisionCandidates": true,
    "includePreviewImages": true
  },
  "budgets": {
    "wallTimeMs": 900000,
    "cancelGraceMs": 2000,
    "memoryBytes": 12884901888,
    "processCount": 1,
    "inputBytes": 2147483648,
    "outputBytes": 4294967296,
    "vertices": 20000000,
    "triangles": 30000000,
    "textures": 64,
    "textureDimension": 8192,
    "repeatIterations": 64,
    "layers": 128
  },
  "cache": {
    "policy": "read-write-canonical",
    "jobKey": "sha256:451b...",
    "reusablePrefixLayerCount": 2,
    "prefixArtifact": "cache-input/layer-prefix-2.blend",
    "prefixArtifactSha256": "sha256:91f0..."
  }
}
```

Rutas absolutas, `..`, alternate data streams, device paths, junction/symlink escape y archivos fuera del inventario se rechazan. El worker resuelve todo contra el job root abierto por el broker.

### 23.3 Eventos

```jsonl
{"protocolVersion":1,"jobId":"9ce40c8b-23e3-49ea-8496-6f98db563159","sequence":0,"type":"accepted","stage":"startup","progress":0.0}
{"protocolVersion":1,"jobId":"9ce40c8b-23e3-49ea-8496-6f98db563159","sequence":1,"type":"progress","stage":"bsp-admission","progress":0.05,"message":"Validated BSP snapshot"}
{"protocolVersion":1,"jobId":"9ce40c8b-23e3-49ea-8496-6f98db563159","sequence":2,"type":"layer-started","stage":"layer","progress":0.15,"layerId":"a2247dc7-3550-4d8f-a30d-9b2adb5da7a8"}
{"protocolVersion":1,"jobId":"9ce40c8b-23e3-49ea-8496-6f98db563159","sequence":3,"type":"metric","stage":"layer","name":"triangles","value":184224,"unit":"count"}
{"protocolVersion":1,"jobId":"9ce40c8b-23e3-49ea-8496-6f98db563159","sequence":4,"type":"diagnostic","severity":"warning","code":"geology.uv.island_density_variance","message":"One UV island exceeds the configured texel-density variance","subject":"GeoChunk_003"}
{"protocolVersion":1,"jobId":"9ce40c8b-23e3-49ea-8496-6f98db563159","sequence":5,"type":"artifact","role":"lod0-glb","path":"output/mesh/GeoChunk_000_LOD0.glb","bytes":47188234,"sha256":"sha256:3d11..."}
{"protocolVersion":1,"jobId":"9ce40c8b-23e3-49ea-8496-6f98db563159","sequence":6,"type":"completed","stage":"complete","progress":1.0,"result":"output/result.json","resultSha256":"sha256:39d3..."}
```

`sequence` comienza en 0 y aumenta exactamente 1. Evento duplicado, fuera de orden, job ID distinto, unknown required field o `completed` sin manifest válido hace fallar el job.

### 23.4 Errores normalizados

| Código | Significado | Recuperación |
| --- | --- | --- |
| `blender.not_found` | no se encontró pin | elegir/instalar artefacto admitido |
| `blender.version_mismatch` | versión/build/hash incorrecto | no ejecutar; readmitir dependency |
| `plugin.manifest_invalid` | manifest hostil/inválido | quarantine |
| `plugin.license_blocked` | licencia/redistribución no permitida | no habilitar |
| `plugin.schema_drift` | node interface cambió | regenerar/revisar/migrar |
| `plugin.headless_unsupported` | requiere UI/contexto | ReferenceOnly |
| `job.input_hash_mismatch` | input cambió | descartar/recrear snapshot |
| `job.budget_exceeded` | tiempo/memoria/tamaño/topología | cancelar, ajustar quality/selection |
| `job.cancelled` | usuario o superseded generation | no publicar |
| `job.worker_crash` | Blender terminó sin resultado | conservar crash/log, reiniciar limpio |
| `geology.keep_clear_violation` | invadió espacio protegido | rechazar y mostrar triángulo/volumen |
| `geology.non_manifold` | salida topológicamente inválida | rechazar/ajustar generador |
| `geology.attribute_missing` | falta mask/slot obligatorio | rechazar |
| `geology.provenance_invalid` | tablas/slots no cierran | rechazar |
| `geology.lod_boundary_violation` | LOD rompe clearance/seam | rechazar LOD/job |
| `geology.bake_failed` | cage/ray/UV/material inválido | conservar geometría preview, bloquear Apply |
| `geology.collision_invalid` | candidate inválido | bloquear Apply |
| `geology.stale_completion` | generation ticket ya no vigente | descartar sin tocar estado |

---

## 24. Lifecycle de workers

### 24.1 Worker autoritativo

Se usa **un proceso Blender por job**:

1. validar lock Blender y plugin set;
2. crear job root no reutilizado;
3. materializar inputs por hardlink/copy de blobs read-only;
4. crear `BLENDER_USER_RESOURCES` vacío;
5. aplicar ACL/sandbox/Job Object;
6. iniciar proceso con comando fijo;
7. validar NDJSON mientras se consume;
8. monitorear tiempo, RSS, child process, output bytes y heartbeat;
9. si cancela, crear `cancel.requested`, esperar `cancelGraceMs`, terminar el Job Object completo;
10. al completar, cerrar handles y validar todos los artifacts desde C++;
11. canonizar/cocinar; sólo entonces promover al DDC;
12. borrar job temp de manera recuperable; crash evidence se conserva según retention policy.

No se recicla un proceso autoritativo: evita estado global, leaks, handlers y plugins residuales.

### 24.2 Preview worker opcional

Un pool prewarm puede acelerar Preview, con estas restricciones:

- no llena cache autoritativo;
- no cocina/publica;
- sólo plugins Jerico builtin;
- reinicia al cambiar plugin set, tras 8 previews, 20 minutos o RSS > 80% del límite;
- cada preview resetea a factory scene y verifica data-block inventory;
- el front etiqueta `Fast Preview`; Apply siempre regenera autoritativamente.

### 24.3 Schema worker

Un proceso por plugin package, límite 60 s/2 GiB, sin asset inputs. Produce schema + inventory y termina. No usa schemas cacheados si cambia cualquier hash del paquete.

### 24.4 Heartbeat y hang

El worker escribe evento `heartbeat` al terminar cada operación acotada. No se usa un Python thread dentro de Blender. El broker detecta falta de progreso según stage budget y termina el proceso. Operaciones monolíticas de terceros sin puntos de cancelación no se admiten como core.

### 24.5 Stale generation

`GeologyGenerationJob::generation_`/ticket sigue siendo autoritativo. Completion `N` sólo puede actualizar el preview si el controller aún espera `N`, source revision/schema/job key coinciden y no hay cancelación. `N-1` se destruye aunque haya terminado correctamente.

---

## 25. Cache y determinismo

### 25.1 Job key

```text
jobKey = SHA-256(
  "JRC_BLENDER_GEOLOGY_JOB_V1" ||
  protocolVersion ||
  engineEvaluatorVersion ||
  blenderVersion || blenderBuildHash || blenderExecutableSha256 ||
  workerSha256 || pluginSetSha256 || pluginPackageSha256 ||
  generatorId || nodeGroupSha256 || generatorSchemaSha256 ||
  bspCanonicalHash || bspRevision ||
  canonicalRecipeJson || allInputBlobHashes ||
  coordinateContractVersion ||
  qualityProfileCanonicalJson ||
  outputProfileCanonicalJson ||
  gltfExportSettingsCanonicalJson ||
  uvAlgorithmVersion || bakeProfileVersion || lodAlgorithmVersion ||
  collisionCandidateAlgorithmVersion || jericoCookVersion
)
```

No participan job ID, timestamps, paths absolutos, nombre de usuario, temp directory, orden de filesystem ni locale.

### 25.2 Canonical JSON

- UTF-8 sin BOM;
- Unicode NFC;
- keys ordenadas bytewise;
- arrays conservan orden semántico;
- floats finitos en representación decimal shortest-round-trip;
- `-0` se normaliza a `0`;
- GUID lowercase canonical;
- paths con `/`, relativos y case policy de Jerico;
- enums por ID estable;
- no comentarios ni campos desconocidos.

### 25.3 Niveles de cache

```text
SourceAdmission/<source-key>
LayerPrefix/<prefix-key>/<layer-count>
EvaluatedSurface/<surface-key>
ChunkLodUv/<post-key>
Bake/<bake-key>
Transport/<transport-key>
CanonicalCook/<cook-key>
```

Cada entry tiene manifest, hashes, creator version, byte count, dependency keys y validation stamp. DDC existente es el único cache raíz; no se crea un cache paralelo del plugin.

### 25.4 Definición de determinismo

Objetivo obligatorio en la misma plataforma/pin:

- misma entrada en diez procesos limpios → misma malla Jerico canónica, atributos, LOD, masks, collision y package bytes;
- cambio de root seed → identidad diferente y resultado visualmente diferente;
- cambio de un layer sólo invalida su prefijo y dependientes;
- Preview autoritativo y Apply producen mismos hashes;
- cache hit y cold generation producen mismos hashes.

No se acepta `hash(.blend)` o `hash(GLB)` como única prueba. Antes de cook Jerico canoniza:

1. transforma a Jerico coordinates;
2. rechaza NaN/Inf/subnormal no permitido;
3. cuantiza positions/normals/tangents/UV/weights con profile fijo;
4. ordena objects/chunks por GUID/region slot;
5. separa vertices en boundaries de atributos discretos;
6. normaliza winding;
7. crea canonical vertex keys;
8. remapea índices por orden estable;
9. ordena triangles por region/material/physical y canonical index tuple sólo cuando no rompe strip/meshlet contract;
10. serializa mediante escritores RuntimeFormats existentes.

La prueba cross-machine usa primero tolerancia geométrica/semántica. Byte identity cross-machine sólo se reclama después de probarla con pins equivalentes; no se promete por Blender de forma implícita.

### 25.5 Fuentes de no determinismo prohibidas

- random global de Python sin seed;
- hash nativo de Python;
- frame/time/current scene;
- orden de `set`, dict legado o filesystem;
- GPU/CUDA/OptiX para authoritative bake sin conformance;
- simulación temporal sin pasos/timestep/cache fijados;
- downloads, URLs o latest versions;
- library overrides no lockeados;
- transforms sin aplicar/normalizar;
- operadores que dependen de active object/selection/UI;
- locale/timezone/user preferences;
- plugins que mutan otras extensiones.

---

## 26. GLB de transporte, extras y provenance

### 26.1 Export settings

El worker evalúa modifiers y convierte curves/instances antes de exportar. Comando bpy conceptual:

```python
bpy.ops.export_scene.gltf(
    filepath=output_path,
    export_format="GLB",
    use_selection=True,
    export_apply=True,
    export_texcoords=True,
    export_normals=True,
    export_tangents=True,
    export_attributes=True,
    export_extras=True,
    export_materials="EXPORT",
    export_yup=True,
    export_animations=False,
    export_cameras=False,
    export_lights=False,
)
```

La llamada real se valida contra la firma exacta de `bpy.ops.export_scene.gltf` de 5.2.1 durante implementación; las settings canonizadas se guardan en result/cache. `Geometry Nodes Instances` experimental queda deshabilitado; los outputs se realizan explícitamente para no depender de esa ruta.

### 26.2 Naming

```text
JRC_<product-guid>_<chunk-slot>_LOD<index>
JRC_<product-guid>_<chunk-slot>_HIGH
JRC_<product-guid>_<chunk-slot>_COLLISION_<index>
```

Nombres Blender no son identidades. GUID/slot/result table lo son.

### 26.3 `extras.jerico`

```json
{
  "jerico": {
    "schemaVersion": 1,
    "productId": "2f76fca8-10a2-4385-aede-5d19e9c0987f",
    "chunkSlot": 3,
    "lod": 1,
    "sourceHash": "sha256:1f53...",
    "recipeHash": "sha256:1e4a...",
    "jobKey": "sha256:451b...",
    "coordinateContract": "jerico-rh-y-up-z-forward-meters/v1"
  }
}
```

Extras se validan contra result manifest; no se confían ni ejecutan. glTF no define namespace para extras, por eso se usa un objeto único `jerico` y schemaVersion.

### 26.4 `result.json`

```json
{
  "$schema": "jerico://schemas/blender-geology-result/v1",
  "schemaVersion": 1,
  "protocolVersion": 1,
  "jobId": "9ce40c8b-23e3-49ea-8496-6f98db563159",
  "jobKey": "sha256:451b...",
  "status": "completed",
  "runtime": {
    "blenderVersion": "5.2.1",
    "blenderBuildHash": "locked-build-hash",
    "executableSha256": "sha256:locked-executable-hash",
    "workerSha256": "sha256:2fe4...",
    "pluginSetSha256": "sha256:b7c1..."
  },
  "inputs": {
    "bspCanonicalHash": "sha256:1f53...",
    "recipeSha256": "sha256:1e4a...",
    "generatorSchemaSha256": "sha256:66a0..."
  },
  "products": [
    {
      "productId": "2f76fca8-10a2-4385-aede-5d19e9c0987f",
      "chunkId": "68cb2354-b1bd-4bc0-a7d0-cc1ae8bb3b68",
      "chunkSlot": 3,
      "worldCells": ["cell:+0003:+0001:-0002"],
      "boundsJerico": {"minimum": [0.0, 0.2, -28.1], "maximum": [18.4, 14.2, -2.0]},
      "high": {
        "path": "output/mesh/JRC_2f76_3_HIGH.glb",
        "sha256": "sha256:1ff9...",
        "vertices": 8432940,
        "triangles": 16802242
      },
      "lods": [
        {
          "index": 0,
          "path": "output/mesh/JRC_2f76_3_LOD0.glb",
          "sha256": "sha256:3d11...",
          "vertices": 384220,
          "triangles": 762814,
          "maxErrorMeters": 0.006,
          "screenThresholdPixels": 2.0
        },
        {
          "index": 1,
          "path": "output/mesh/JRC_2f76_3_LOD1.glb",
          "sha256": "sha256:83e0...",
          "vertices": 161902,
          "triangles": 320104,
          "maxErrorMeters": 0.021,
          "screenThresholdPixels": 1.5
        }
      ],
      "uvSets": [
        {"name": "UV0", "purpose": "material", "overlapFraction": 0.0, "occupancy": 0.713, "maxStretch": 1.86}
      ],
      "masks": [
        {"id": "fresh-break", "attribute": "_JRC_MASK_FRESH_BREAK", "domain": "point", "type": "float", "minimum": 0.0, "maximum": 1.0},
        {"id": "cavity", "texture": "output/masks/JRC_2f76_3_cavity.exr", "sha256": "sha256:2c77...", "channels": "R"}
      ],
      "collisionCandidates": [
        {"id": "static-trimesh-lod1", "kind": "triangle-mesh", "sourceLod": 1, "path": "output/collision/JRC_2f76_3_collision.glb", "sha256": "sha256:b622..."}
      ],
      "triangleSemantics": {
        "path": "output/meta/JRC_2f76_3_triangles.bin",
        "sha256": "sha256:6cf0...",
        "recordFormat": "jrc-triangle-semantics-v1"
      },
      "dirtyBounds": {
        "geometry": {"minimum": [0, 0, -29], "maximum": [19, 15, -1]},
        "collision": {"minimum": [-0.1, -0.1, -29.1], "maximum": [19.1, 15.1, -0.9]},
        "navigation": {"minimum": [-0.5, -0.5, -29.5], "maximum": [19.5, 15.5, -0.5]}
      }
    }
  ],
  "slotTables": {
    "source": [{"slot": 17, "brushId": "5ac91f09-b40d-4a19-8dcf-b5ee3f46d512", "faceId": 3}],
    "layer": [{"slot": 4, "layerId": "a2247dc7-3550-4d8f-a30d-9b2adb5da7a8"}],
    "material": [{"slot": 2, "guid": "87d7aaca-07ba-470b-ac4f-793254573b9e"}],
    "physical": [{"slot": 1, "guid": "dc4d87ea-71d8-4928-a720-11b4e14c7d7d"}]
  },
  "provenance": {
    "layers": [
      {
        "layerId": "a2247dc7-3550-4d8f-a30d-9b2adb5da7a8",
        "generatorId": "jerico.rock.sedimentary_strata",
        "nodeGroupSha256": "sha256:59be...",
        "parametersSha256": "sha256:f330...",
        "selectionSha256": "sha256:97a3...",
        "stageInputSha256": "sha256:4a09...",
        "stageOutputSha256": "sha256:0f82..."
      }
    ]
  },
  "metrics": {
    "wallTimeMs": 183442,
    "peakRssBytes": 7283920896,
    "cachePrefixLayers": 2,
    "cacheHit": false
  },
  "warnings": []
}
```

---

## 27. UV y unwrap

### 27.1 Policies

| Policy | Uso | Reglas |
| --- | --- | --- |
| `triplanar-only` | grandes cliffs/caves con material world-space | UV0 opcional sólo si el material/cook lo permite; lightmap/decals separados |
| `unwrap-and-pack` | rocks y chunks normales | seams + UV Unwrap + Pack, no overlap |
| `hybrid` | triplanar base + baked detail/decal | UV0 único para bake; world coords para macro material |
| `preserve-source` | imported mesh admitido | valida UV; no reunwrap sin policy explícita |

### 27.2 Pipeline GN/bpy

1. seleccionar seams por boundaries de material/chunk, high curvature, hard edges y occluded underside;
2. preservar seam locks en boundaries de chunks vecinos;
3. `UV Unwrap` con Selection/Seam/Margin;
4. guardar como named attribute Face Corner, tipo 2D Vector, porque así Blender reconoce el UV map;
5. pack con margin derivado de resolución y mip count;
6. validar overlap, range, finite values, island count, stretch, occupancy y texel-density variance;
7. repetir después de LOD si el LOD no puede transferir UV de forma segura.

### 27.3 Fórmula de margin

```text
requiredMarginPixels = max(
  bakeDilationPixels,
  2^(mipCount-1) * minimumMipSafetyPixels,
  compressionBlockSafetyPixels
)

uvMargin = requiredMarginPixels / textureResolution
```

No se fija un margin UV arbitrario independiente de resolución. Para BC, `compressionBlockSafetyPixels >= 4`.

### 27.4 Validaciones

- UV0 existe cuando bake/material lo requiere;
- todos los face corners tienen valor finito 0..1 dentro de epsilon;
- overlap 0 para opaque unique-bake salvo tiles/UDIM declarados;
- no hay islands con área mesh positiva y área UV cero;
- tangent basis válida tras triangulación;
- material/physical/region boundaries no pierden identidad por weld;
- seams entre chunks no producen discontinuidad perceptible en neutral Beauty.

---

## 28. Baking high → low

### 28.1 Autoridad y engine

Cycles CPU es la ruta autoritativa. El bake requiere high mesh, low activo, UV válido, Image Texture/Color Attribute target explícito y orden de objetos fijado. Se usa Selected to Active con cage explícito para productos de aceptación; extrusion/max ray sólo para previews o fixtures donde se demuestre ausencia de misses.

### 28.2 Mapas

| Mapa | Espacio/formato intermedio | Destino |
| --- | --- | --- |
| Normal | tangent, +X/+Y/+Z, Non-Color, 16-bit o EXR | `.rtext` normal profile |
| AO | scalar linear EXR/16-bit | canal/material policy |
| Curvature | signed/remapped EXR | mask, no runtime obligatorio |
| Cavity | scalar EXR | material/EPLS |
| Fresh break | scalar ID-safe | material/destruction |
| Strata/material class | discrete nearest/ID | mask/class table |
| Position/thickness opcional | EXR | sólo authoring si consumer real |

BaseColor/roughness/metallic se hornean sólo cuando el material procedural Blender sea una fuente admitida. El camino normal es que Jerico Materials consuman máscaras y texturas Jerico; Blender no sustituye el material master.

### 28.3 Cage

- se deriva del low antes de hard split;
- conserva exactamente topología/face order del low;
- expansión inicial = max surface error + safety;
- ray miss, backface hit, cage self-intersection y projection a otro chunk son errores medidos;
- dilation cubre mip/compression margin;
- el mapa normal se valida re-renderizando low con el material neutral y comparando contra high desde vistas fijadas.

### 28.4 Bake profile

```json
{
  "id": "game-pbr-2k/v1",
  "engine": "CYCLES",
  "device": "CPU",
  "samples": 64,
  "useDenoise": false,
  "resolution": [2048, 2048],
  "marginType": "ADJACENT_FACES",
  "marginPixels": 32,
  "selectedToActive": true,
  "useCage": true,
  "normalSpace": "TANGENT",
  "normalSwizzle": ["+X", "+Y", "+Z"],
  "colorManagement": "linear-data-for-noncolor",
  "output": "OPEN_EXR"
}
```

### 28.5 Acceptance

- 0 missed texels dentro de valid selection;
- 0 NaN/Inf;
- tangent seam error bajo umbral fixture;
- silhouette/normal re-render dentro de SSIM/error angular declarado;
- dos cold bakes producen mismos canonical texture bytes bajo el pin;
- no se usa GPU si esa igualdad no fue probada;
- bake cancelado no promueve archivos parciales.

---

## 29. LOD

### 29.1 Política

Blender genera candidatos; Jerico canoniza y decide thresholds/runtime pages. Cada producto tiene LOD0..N y opcional HIGH sólo de authoring. Ratios no son la única métrica: se exige error geométrico en metros, silhouette screen error, atributo, UV, seam y keep-clear.

Perfil inicial:

| LOD | Target relativo | Max error orientativo | Uso |
| ---: | ---: | ---: | --- |
| HIGH | 100% source high | n/a | bake/authoring, no Shipping |
| LOD0 | hasta 100% game high | 0.5× voxel | cerca |
| LOD1 | 45%..65% LOD0 | 2× LOD0 error | medio |
| LOD2 | 20%..35% LOD0 | 4× LOD0 error | lejos |
| LOD3 | 8%..18% LOD0 | 8× LOD0 error | muy lejos/HLOD input |

Los valores concretos por asset se calculan por budget/error y se guardan; no se fuerza ratio que viole estructura.

### 29.2 Restricciones de decimation

Se bloquean/pesan:

- boundaries de chunk/portal/world-cell;
- material/physical/region/strata/column boundaries;
- sharp silhouette y cave mouth;
- vertices próximos a KeepClear;
- destruction anchors/contact seams;
- UV seams cuando se preserva UV;
- collision critical surfaces.

La ruta inicial usa evaluated mesh + Decimate de Blender con settings fijadas sólo si pasa conformance. Si no puede preservar constraints, Jerico implementa/conecta su simplificador admitido existente y Blender entrega únicamente HIGH/LOD0. No se acepta deformar espacio jugable para cumplir ratio.

### 29.3 Seam y morph

Boundary vertices son locks compartidos. Neighbor chunks guardan seam identity. Geomorph/skirt sólo se admite si el runtime ya lo consume; la primera ruta acepta swaps discretos con boundaries coincidentes. Cada LOD reejecuta validation espacial y normal/tangent validation.

---

## 30. Collision candidates

### 30.1 Tipos

| Caso | Candidate | Publicación final |
| --- | --- | --- |
| cave/cliff estático | triangle mesh derivado de LOD seguro | `.rcoll StaticTriangleMesh` existente |
| boulder estático simple | convex hull o low triangle mesh | Jerico elige por error/budget |
| destructible chunk | uno o más convex hulls acotados | `.rcoll` + `.rpref`/fracture contract |
| columnar cluster | per-column/merged convex según gameplay | Jerico valida count/contact |

Blender no asigna layer/mask física por nombre libre. Devuelve `physical_slot`; Jerico lo resuelve a `RuntimeFormats::CollisionPhysicalMaterial` ya admitido.

### 30.2 Validación

- finito, non-empty, winding y volume coherentes;
- triangle/hull count bajo budget;
- no self-intersection/degenerates importantes;
- coverage no deja atravesar la superficie jugable;
- no invade Protected;
- máximo error contra render mesh medido por región;
- chunk collision corresponde exactamente al chunk/bond/anchor;
- Material/Physical slots válidos;
- deterministic canonical `.rcoll` bytes;
- real Jolt create/add/remove/reload gate.

### 30.3 Separación de autoridad

`collision.glb` no se publica. Se lee con el importador glTF admitido, se valida y se pasa a `cookGeologySurface`/reemplazo adapter para producir `.rcoll`. El Physical Material definition se vuelve a resolver en Apply y su hash debe coincidir, preservando la regla existente de revalidación.

---

## 31. Contrato de resultado y reintegración en Jerico

### 31.1 `result.json` canónico

Blender entrega un candidato, nunca un asset publicado. El candidato contiene rutas relativas a su job root y hashes de bytes completos:

```json
{
  "$schema": "jerico://schemas/blender-geology-result/v1",
  "schemaVersion": 1,
  "jobId": "018f4430-4c70-7f01-9be4-287890901400",
  "requestHash": "sha256:705ff07e...a945",
  "producer": {
    "blenderVersion": "5.2.1",
    "blenderBuildHash": "sha256:a6f56fe3...0d81",
    "workerHash": "sha256:011f2f41...d667",
    "pluginSetHash": "sha256:9d118117...4d12",
    "generatorId": "jerico.rock.sedimentary.v1",
    "generatorSchemaHash": "sha256:98b20f80...bec0",
    "nodeGroupHash": "sha256:4c246ef1...3fb9"
  },
  "products": {
    "authoringHigh": { "path": "products/high.glb", "sha256": "sha256:7d55f4fc...0aca" },
    "lods": [
      { "level": 0, "path": "products/lod0.glb", "sha256": "sha256:8b273af8...8d22", "maxErrorMeters": 0.008 },
      { "level": 1, "path": "products/lod1.glb", "sha256": "sha256:8a7cb4bd...d781", "maxErrorMeters": 0.024 },
      { "level": 2, "path": "products/lod2.glb", "sha256": "sha256:bc7659a3...c156", "maxErrorMeters": 0.071 }
    ],
    "collisionCandidate": { "path": "products/collision.glb", "sha256": "sha256:90c18781...c6ae" },
    "maskTable": { "path": "products/masks.json", "sha256": "sha256:5271054d...9600" },
    "textures": [
      { "semantic": "normal", "path": "products/textures/normal.exr", "sha256": "sha256:81390f86...e08d", "colorSpace": "Non-Color" },
      { "semantic": "orm", "path": "products/textures/orm.exr", "sha256": "sha256:0756b05c...918e", "colorSpace": "Non-Color" }
    ],
    "provenance": { "path": "products/provenance.json", "sha256": "sha256:84c8b67a...a069" }
  },
  "metrics": {
    "verticesLod0": 108244,
    "trianglesLod0": 211760,
    "uvOverlapPercent": 0.0,
    "keepClearMinimumMeters": 0.184,
    "peakWorkingSetBytes": 3187660800,
    "elapsedCpuMs": 18426
  },
  "warnings": []
}
```

Los hashes abreviados con `...` existen sólo para legibilidad del ejemplo; todos los formatos de producción exigen exactamente `sha256:` más 64 dígitos hexadecimales en minúsculas. Un resultado con path absoluto, path escapando del job root, archivo no enumerado, hash repetido con contenido distinto o campo desconocido bajo modo estricto se rechaza.

### 31.2 Ingesta

`BlenderResultAdmission` ejecuta, en este orden:

1. valida JSON, versión, job/request identity y allowlist de archivos;
2. recalcula todos los hashes sin confiar en stdout ni en `result.json`;
3. importa GLB mediante la ruta glTF admitida de Jerico, no mediante estructuras `bpy` compartidas;
4. transforma Blender RH Z-up a Jerico RH Y-up una sola vez y prueba handedness/winding/tangent basis;
5. reconstruye stable GUIDs y semantic slots desde extras/atributos, sin inferirlos por nombres de objeto;
6. valida topología, bounds, atributos, LOD, UV, masks, keep-clear, materials y collision;
7. canoniza vertex/index/attribute order y serializa bytes Jerico;
8. produce un `GeologyCookCandidate` inmutable;
9. sólo `Apply` bajo `ProjectTransactionCoordinator` promueve recipe, mesh, collision, prefab/fracture, masks y nivel como una transacción;
10. después de commit actualiza el asset catalog y dispara invalidación dirigida a RenderMesh, Collision, Navigation, Scatter y EPLS.

### 31.3 Provenance por producto y por elemento

El provenance global guarda request/cache key, Blender/worker/plugin/node hashes, seed lineage, BSP revision, recipe revision, asset dependencies, material/physical hashes, tiempos y warnings. El provenance elemental usa estos atributos GLB:

| Atributo GLB | Dominio | Tipo | Destino Jerico |
| --- | --- | --- | --- |
| `_j_source_id` | FACE | `INT` | índice en tabla de GUIDs BSP/spline/decal |
| `_j_semantic` | FACE | `INT` | semantic table estable |
| `_j_layer_id` | FACE | `INT` | layer GUID table |
| `_j_material_slot` | FACE | `INT` | MaterialDefinition slot |
| `_j_physical_slot` | FACE | `INT` | PhysicalMaterial slot |
| `_j_region` | FACE | `INT` | region/strata/column/cave component |
| `_j_mask0..3` | CORNER o POINT | `FLOAT_COLOR` | hasta 16 pesos packed |
| `_j_damage` | POINT | `FLOAT` | debilidad/fractura |
| `_j_anchor` | POINT | `FLOAT` | soporte estructural |
| `_j_keepclear_distance` | POINT | `FLOAT` | auditoría, no runtime obligatorio |

Los nombres cumplen la regla del exportador glTF de Blender para custom attributes (`_` inicial). Custom Properties de objetos/meshes se exportan como `extras` cuando `export_extras=true`; nunca contienen payload binario ni rutas secretas. Jerico rechaza atributos requeridos ausentes y atributos no declarados por el `GeneratorSchema`.

---

## 32. PWAS, EPLS, materiales, destrucción, navegación y mundo

### 32.1 PWAS

El Procedural World Authoring System conserva su autoridad de placement. Geology publica una `SurfaceDescriptor` estable por LOD0/page con posiciones, normales, pendiente, curvatura, cavidad, altura, material/physical slot, region, humedad y masks. PWAS:

- consume esos campos como inputs, nunca vuelve a evaluar Geometry Nodes;
- mantiene intactos sus placement keys, seed derivation y reglas Poisson/spline/grid;
- excluye `Protected`, `Portal`, `NoBuild`, `NavCritical` y masks de exclusión;
- invalida sólo placements cuya surface page/source hash cambió;
- conserva placements manuales y overrides del usuario;
- puede usar `strata`, `crack`, `wetness`, `debris` y `cavity` para distribución de props sin convertirlos en materiales baked irreversibles.

El orden causal es: `BSP/Recipe -> Blender candidate -> admitted geology surface -> PWAS placements`. Nunca `PWAS -> deformación silenciosa de geology`; si un prop exige carve/footprint, emite una nueva input layer identificada y regenera.

### 32.2 EPLS

EPLS, Environmental Procedural Layer System, conserva sus documentos, RegionMasks, manual layers, paint intents y pages. Geology aporta environmental base fields versionados. La composición final es:

```text
GeologyBaseField
  -> EPLS procedural samples
  -> EPLS manual paint layers in explicit order
  -> final bounded mask pages
```

No se bakea una pintura EPLS de usuario dentro de la malla de Blender. Blender puede emitir valores base (`wetness`, `porosity`, `debris`, `mineral`, `cavity`); EPLS los referencia por `surfaceProductHash + fieldSchemaHash`. Al regenerar, EPLS conserva intent/strokes y reproyecta contra el nuevo surface candidate durante Preview; Apply conjunto sólo sucede si todos los pages pasan.

### 32.3 MaterialDefinition, material masks y PhysicalMaterial

- `MaterialDefinition` sigue siendo la autoridad visual de Jerico; Blender usa previews neutrales y no introduce shader graphs runtime.
- Los masks son datos lineales, nombrados, con rango/domain/interpolation declarado y packing decidido por cooker.
- `PhysicalMaterial` sigue siendo autoridad de fricción, restitución, densidad, impacto, audio, VFX, damage y fracture response.
- El plugin pide `materialSemantic`/`physicalSemantic`; el broker resuelve a GUID/hash admitido antes del job y vuelve a resolver antes de Apply.
- Cambiar sólo MaterialDefinition puede recookear material bindings/textures sin regenerar topología si el GeneratorSchema declara que no afecta geometry.
- Cambiar PhysicalMaterial invalida collision/fracture y no necesariamente render mesh; el dependency graph decide por campos, no por un invalidation global.

### 32.4 Destruction

Para geometría destructible, Blender devuelve structural fields y candidatos, pero Jerico conserva la autoridad de simulación:

- `chunk_id`, `bond_id`, `anchor`, `support`, `damage`, `hardness`, `grain_direction` y contact faces se guardan como tablas/atributos;
- los chunks se validan como closed manifold, con masa/centro/inercia finitos y hulls bajo budget;
- bonds se derivan de contactos geométricos canonizados y reglas PhysicalMaterial, no de object names;
- un Spline Destruction layer usa el mismo curve transport, selection query, falloff y keep-clear de Geology/Decal 3D;
- la preview de corte puede ser Geometry Nodes; la publicación produce los contratos `.rpref`/fracture/collision existentes;
- la ruta Shipping no ejecuta Blender, Python, Geometry Nodes ni importa `.blend`.

### 32.5 Navigation

Navigation nunca confía en que una malla visual es transitable. Después de Apply:

1. el dirty set se calcula desde old/new admitted collision bounds más agent radius;
2. tiles Recast afectados se reconstruyen offline;
3. portals BSP mantienen GUID y se comprueba conectividad esperada;
4. `NavCritical` y KeepClear se prueban con agent capsule, step, slope y headroom reales;
5. Detour runtime consume los tiles cocinados existentes;
6. una regresión de ruta requerida invalida Apply, no se publica como warning.

### 32.6 World Cells y streaming

El partitioner corta por límites deterministas con halo de evaluación. Blender puede evaluar un conjunto de celdas vecinas para continuidad, pero emite productos por owning cell. Los vertices de borde comparten quantized seam keys; una celda no posee triángulos fuera de su expanded authoring bounds. `WorldCellGuid + recipeGuid + layerGuid + sourceGuid` participa en seed y cache. Runtime pages, HLOD inputs y package dependencies siguen el contrato de virtualized world vigente.

---

## 33. Cook, package y Shipping source-free

### 33.1 Separación estricta

| Entorno | Puede contener | Prohibido |
| --- | --- | --- |
| Authoring/Editor | Blender 5.2.1, worker Python, `.blend` node libraries, manifests, HIGH, logs | publicar sin Jerico admission |
| Cook | productos admitidos Jerico, masks/textures, collision/fracture/nav builders | ejecutar plugin no fijado o acceder a red |
| Package staging | `.rmesh`, `.rcoll`, `.rmat`/textures, `.rpref`, nav/world pages, provenance mínimo | `.blend`, `.py`, addon, Blender DLL/EXE, HIGH authoring |
| Shipping runtime | sólo formatos runtime y datos admitidos | Blender, `bpy`, Python, Geometry Nodes, JSON de authoring, fuentes |

### 33.2 Negative source-free gate

El gate inspecciona recursively el package y el dependency graph/link map. Falla si encuentra:

- `blender.exe`, cualquier Blender DLL, `.blend`, `.blend1`, `.py`, `.pyc`, wheel o addon archive;
- `blender_manifest.toml`, `jerico.generator.json`, node group source o Geometry Nodes serialized source;
- HIGH mesh o bake cage no marcado como runtime;
- ruta absoluta, username, job root, temp path o plugin source path en strings;
- dependencia dinámica/transitiva hacia Blender/Python;
- fallback que intente regenerar geometry en runtime.

El replay de aceptación se realiza desde un único package source-free con el source tree renombrado/inaccesible, red deshabilitada y caché de authoring vacía. Debe cargar render, collision, PhysicalMaterial, destruction, nav y world pages sin fallback ni error.

### 33.3 Cook identity

`GeologyCookIdentityV2` incorpora `authoringCandidateHash`, `bspSnapshotHash`, `generatorSchemaHash`, `materialBindingHash`, `physicalMaterialHash`, `maskPackingHash`, `lodPolicyHash`, `collisionPolicyHash` y cooker build hash. El viejo hash de productor nativo no se reutiliza. Un recipe migrado recibe nueva identity de producto aun cuando conserva GUID de asset/document.

---

## 34. Compatibilidad, seguridad y licencias

### 34.1 Matriz de compatibilidad

| Componente | Estado admitido inicial | Regla |
| --- | --- | --- |
| Blender | exactamente 5.2.1 LTS Windows x64 | otra patch/minor es incompatibilidad hasta conformance completa |
| Python | el embebido por ese build | no usa Python del sistema ni `PYTHONPATH` externo |
| Jerico worker protocol | v1 | major distinto bloquea; minor capability se negocia |
| GeneratorSchema | v1 | unknown required feature bloquea |
| GLB | glTF 2.0 binario, exporter del pin | import estricto y canonicalization Jerico |
| plugins built-in | exact semver + package SHA-256 | forman parte del source revision |
| plugins terceros | exact semver + package SHA-256 + admission record | nunca floating/latest |
| OS | Windows x64 soportado por JericoEditor | Linux/macOS requieren lock y goldens propios |

Un plugin declara rangos, pero el lock resuelve una identidad exacta. `compatible` en manifest significa elegible para pruebas, no admitido. No hay compatibilidad implícita por poder abrir el `.blend`.

### 34.2 Modelo de amenaza

Un addon de Blender es código Python arbitrario con capacidad de leer/escribir/ejecutar bajo el usuario. `--offline-mode` no es sandbox. Por ello:

- el catálogo parsea TOML/JSON y calcula hashes sin importar el addon;
- sólo paquetes del lockfile pasan al staging snapshot de solo lectura;
- el worker corre con Windows Restricted Token, low-integrity temporal, Job Object con kill-on-close, límites de proceso/memoria/tiempo y sin child processes salvo allowlist vacía inicial;
- Windows Firewall/AppContainer/WFP bloquea salida y entrada de red para Blender y sus hijos; el gate verifica el bloqueo con un plugin de prueba hostil;
- ACL concede lectura sólo a Blender pin, worker, plugin snapshot y job inputs; escritura sólo al job output/temp;
- `%USERPROFILE%`, Documents, repo completo, credenciales, SSH, navegadores, sockets y shares no se montan/exponen;
- environment se construye desde allowlist; se eliminan proxy, cloud, token, credential, Python y package-manager variables;
- no se habilita autoexec, drivers, handlers persistentes ni startup del usuario;
- paths se canonicalizan, se rechazan symlink/junction/reparse escapes y se vuelven a verificar al abrir;
- stdout/stderr y error messages se sanitizan antes del front;
- static scan/SBOM/antivirus son defensa adicional, no sustituyen el sandbox ni la revisión humana.

La primera versión built-in no necesita terceros para producir los cinco estilos. Esto reduce la superficie de ataque y convierte addons externos en opt-in explícito, siempre visibles y revocables desde JericoEditor.

### 34.3 Admission de plugin

Estados y transiciones:

```text
Discovered -> Quarantined -> Reviewed -> ConformancePassed -> Approved -> Locked
       \---------- incompatibility/license/security ----------> Blocked
Approved/Locked -- hash or policy change --> Quarantined
```

Para avanzar a `Approved` son obligatorios: source URL/commit/tag, package hash, manifest válido, SPDX, texto completo de licencia, copyright/attribution, SBOM, file inventory, prohibición de red, no binarios no declarados, Blender 5.2.1 smoke, deterministic golden, cancellation, resource cap, malicious-path test y revisión manual. Un update es un plugin nuevo y no hereda admission por nombre.

### 34.4 Política de licencia

- El worker Python y cualquier addon Blender distribuido por Jerico se publica bajo `GPL-3.0-or-later`, compatible con la licencia/API de Blender.
- Los node groups propios y `.blend` library se publican preferentemente `CC0-1.0` para maximizar reutilización de assets; el manifest conserva ambos alcances.
- El C++ de Jerico mantiene la licencia del engine y sólo se comunica por proceso/archivos; no enlaza Blender ni incorpora su código.
- Un addon externo GPL puede admitirse como tool separado, con source/notice correspondiente; su output no se presume GPL por ser generado en Blender.
- `NC`, `ND`, licencia ausente, “free” sin derechos de redistribución, asset-only con restricción incompatible o procedencia dudosa se bloquea.
- BlenderKit/Gumroad/marketplaces no se descargan automáticamente. Cada asset/addon se revisa individualmente y el front muestra licencia y attribution.

---

## 35. Presupuestos, cancelación y recuperación de fallos

### 35.1 Resource budgets

Cada request declara y el broker reduce al mínimo entre project policy y plugin maximum:

```json
{
  "maxWallMs": 300000,
  "maxCpuMs": 900000,
  "maxWorkingSetBytes": 8589934592,
  "maxOutputBytes": 4294967296,
  "maxVerticesHigh": 8000000,
  "maxVerticesLod0": 2000000,
  "maxTextures": 16,
  "maxTextureDimension": 4096,
  "maxLogBytes": 8388608
}
```

Preview puede solicitar un perfil menor visible en UI. Authoritative Apply nunca reduce calidad, LOD count, bake samples o collision fidelity silenciosamente para entrar en budget: falla con diagnóstico y opciones concretas de partición/parámetro.

### 35.2 Cancelación

`Cancel` marca el token compartido, cierra stdin/control pipe y concede 2 segundos para salida cooperativa entre fases. Luego termina el Job Object completo. Todo output queda en staging con estado `cancelled`; no entra al cache. El broker espera process exit, elimina handles, registra fase/peak metrics y conserva sólo un resumen sanitizado. Un resultado que llega después de que cambiaron request revision, selection snapshot o project generation fence es `stale` y se descarta aunque sea válido.

### 35.3 Tabla de recuperación

| Fallo | Detección | Recuperación obligatoria |
| --- | --- | --- |
| Blender pin ausente/hash distinto | startup probe | bloquear Generate; mostrar versión encontrada y requerida; no fallback a 5.1.2 |
| plugin conflict/hash distinto | catalog/lock | cuarentena; no elegir por precedencia silenciosa |
| schema introspection falla | isolated probe | conservar último schema sólo como lectura, deshabilitar Generate, exponer diagnóstico |
| worker exception/exit 73 | process exit + final event ausente | conservar request/log acotado; no promover; reintento manual |
| crash/transient I/O | Job Object/OS code | un único reintento automático sólo si no hubo output y el código está allowlisted como transient |
| timeout/OOM/output cap | broker counters | matar árbol; sugerir split/profile; nunca bajar calidad ocultamente |
| malformed/malicious GLB | importer/admission | cuarentena del resultado y plugin; fuzz regression fixture |
| KeepClear/NavCritical violation | double validator/nav gate | rechazar candidate; informar source GUID/layer/medida |
| stale BSP/recipe/material | Apply revalidation | descartar candidate; regenerar desde snapshot nuevo |
| cache manifest/content corrupto | read-time rehash | mover entry exacta a `Corrupt`, regenerar; no borrar otras entries |
| disk full | atomic writer | abortar antes de rename; limpiar staging propio; conservar asset previo |
| editor crash durante Apply | transaction journal | rollback/recover mediante `ProjectTransactionCoordinator`; nunca medio asset |
| worker protocol major distinto | handshake | bloquear, sin best-effort parsing |

No existe recovery que vuelva al productor Geology nativo. Mientras Blender 5.2.1 no esté admitido o falle, Geology Generate queda claramente indisponible; assets ya cocinados siguen abriendo/runtime, y documentos viejos siguen pudiendo migrarse/inspeccionarse sin regeneración.

### 35.4 Atomicidad y cache hygiene

Cada job escribe en `<DDC>\Blender\Jobs\<jobId>.staging`. `result.json` es el último archivo. Admission genera un manifest Jerico separado y hace rename atómico al cache content-addressed sólo tras fsync/rehash. Cache eviction nunca toca assets publicados. Startup limpia staging huérfano por owner PID/start time y TTL, no por wildcard amplio.

---

## 36. Test plan y criterios de aceptación

### 36.1 Regla RED -> GREEN

Cada tranche comienza con un contrato ejecutable que falla por la ausencia exacta del comportamiento. No se acepta un test que sólo pruebe parser, archivo no vacío, mock, frame debug o command exit. Los gates se registran en CTest bajo preset existente `windows-vulkan-editor`; Shipping usa además `windows-vulkan-runtime`.

### 36.2 Pirámide de pruebas

**Unitarias C++**

- lock/manifest/schema parse estricto, canonical JSON y semver;
- socket/panel mapping, enum extraction, min/max/default, conditional visibility y stable control IDs;
- BSP coordinate/bounds/semantic serialization;
- selection AST y sort estable;
- seed derivation, cache key y dependency invalidation;
- path traversal, junction, unknown fields, bad hash y zip bomb limits;
- result/provenance/mask/LOD/collision validation;
- v29 -> v30 recipe migration y rejection de downgrade.

**Python bajo Blender pin**

- cargar node library desde `--factory-startup --offline-mode`;
- introspectar todas las interfaces y comparar golden GeneratorSchema;
- generar cada estilo con min/default/max y seeds extremos;
- named attributes sobreviven realize/boolean/remesh/decimate/export;
- UV/bake/LOD/collision/export con contexto headless correcto;
- cancel hooks entre etapas y cleanup de datablocks;
- no dependencia de selection, active object, viewport, user preferences ni UI operators no soportados.

**Integración broker**

- handshake, progreso, cancel, timeout, crash, OOM simulado, log cap y restart;
- offline hostil prueba DNS/TCP/HTTP/file/child-process denial;
- plugin discovery aparece en JericoEditor sin ejecutar addon;
- todos los controles del schema son editables desde el front y round-trip exactos;
- dos procesos simultáneos no pisan staging/cache;
- cache hit no inicia Blender y produce el mismo admitted product identity;
- cache corrupto se aisla y regenera.

**Integración espacial**

- fixtures room/corridor/door/stairs/portal/shaft/cave mouth/world-cell seam;
- una layer deliberadamente invasiva es rechazada dentro de Blender y otra adulterada después del export es rechazada por Jerico;
- agent capsule conserva conectividad y headroom;
- Protected/NavCritical/Portal/NoBuild y manual exclusions sobreviven todas las layers;
- Apply preserva brushes/geometry no seleccionados y Undo/Redo/Recover restaura todos los facets.

**Assets y runtime reales**

- cinco showcases: sedimentary/strata, fractured boulder, columnar, rounded/weathered y cave/cliff;
- combinación Layer System con selectors, masks, booleans, decal y spline destruction;
- material/PhysicalMaterial, collision Jolt, nav Recast/Detour, PWAS scatter, EPLS reprojection y world-cell streaming;
- GLB round-trip con extras/custom attributes y Jerico canonical product;
- one-package source-free Vulkan replay sin source/DDC/Blender/red.

### 36.3 Determinismo obligatorio

Para cada golden se ejecuta:

1. dos cold jobs secuenciales;
2. dos cold jobs en procesos nuevos y directorios temporales distintos;
3. dos jobs concurrentes;
4. un cache hit;
5. una regeneración tras eviction.

Se comparan canonical recipe/schema, manifest, topology identity, vertex/index bytes tras quantization declarada, attributes, masks, UV, texture canonical bytes, collision, provenance causal y Jerico cooked bytes. Timestamps, PID, elapsed y logs se excluyen del hash pero permanecen en telemetry. Cualquier no determinismo debe ser eliminado o el campo se declara no autoritativo y no afecta producto; no se “arregla” aumentando tolerancias sin causa demostrada.

### 36.4 Performance y capacidad

Fixtures S/M/L registran por separado wall time, CPU time, peak working set, input/output bytes, Blender startup, GN evaluation, mesh post, UV, bake, export, Jerico import/admission y cook. Se publican p50/p95 de 5 runs warmed y un cold run; se etiqueta siempre la clase de máquina. Gates iniciales:

| Fixture | LOD0 triangles | Peak RAM | Tiempo sin bake | Tiempo bake 2K CPU |
| --- | ---: | ---: | ---: | ---: |
| Small boulder | <= 150k | <= 3 GiB | <= 20 s | <= 90 s |
| Medium cliff/cave | <= 1.0M | <= 8 GiB | <= 120 s | <= 300 s |
| Large partitioned formation | <= 2.0M por cell | <= 12 GiB | <= 300 s por cell | <= 600 s por material set |

Los caps son de producto, no permiso para degradar Beauty. Si hardware de baseline no los cumple, el slice permanece PARTIAL con métrica real; no se relaja un gate para declarar PASS.

### 36.5 Beauty y prueba visual

Cada estilo necesita al menos cinco capturas PNG de resolución original desde JericoEditor/runtime real: hero, grazing light, material/mask, collision/nav overlay separado y contexto BSP/keep-clear. Deben mostrar escala, silueta, strata/fracture/weathering/columns/cave legibles, ausencia de tiling obvio y contacto creíble. Una captura de Blender, debug normals, parser, paquete o frame no vacío no reemplaza Beauty. Se conserva camera/world/package identity, hashes, settings y provenance.

### 36.6 Gates nombrados

La admisión final requiere PASS simultáneo de:

1. `VerifyBlender521Installation`
2. `VerifyBlenderOfflineSandbox`
3. `VerifyBlenderPluginCatalogAndLicenses`
4. `VerifyGeneratorSchemaRoundTrip`
5. `VerifyBlenderJobLifecycleAndCancellation`
6. `VerifyBspKeepClearDoubleValidation`
7. `VerifyGeologyLayerSystem`
8. `VerifyFiveRockStyleGoldens`
9. `VerifyDecal3DReuse`
10. `VerifySplineDestructionReuse`
11. `VerifyGlbAttributesExtrasRoundTrip`
12. `VerifyUvBakeLodCollision`
13. `VerifyPwasEplsMaterialPhysicalIntegration`
14. `VerifyNavigationAndWorldCellSeams`
15. `VerifyBlenderGeologyDeterminism`
16. `VerifyBlenderGeologyPerformance`
17. `VerifyBlenderGeologyTransactionalApply`
18. `VerifyBlenderGeologySourceFreeShipping`
19. `VerifyBlenderGeologyBeauty`
20. `VerifyLegacyProducerAbsent`

`VerifyLegacyProducerAbsent` escanea build graph, editor symbols, commands, menus, service registry, CMake targets, runtime package y source references activas: no puede quedar un fallback invocable.

---

## 37. Migración y deprecación del productor Geology viejo

### 37.1 Estado normativo

Desde la admisión de esta directiva:

> **SUPERSEDED:** el productor Geology nativo M8, incluyendo sus evaluadores CPU/GPU Diligent y cualquier ruta histórica Native/Sparse SDF, deja de ser autoridad de generación. Blender 5.2.1 LTS headless mediante `BlenderAuthoringBroker` es el único productor. Jerico conserva autoridad de input, validación, cook, publicación y runtime.

`SUPERSEDED` no autoriza borrar contratos reutilizables ni evidencia histórica antes de probar el reemplazo. Sí prohíbe agregar features, arreglos cosméticos o nuevos assets al productor viejo.

### 37.2 Matriz de disposición

| Área actual | Disposición | Destino/regla |
| --- | --- | --- |
| `BspDocument.hpp`, `BspRecipe.hpp`, BSP GUID/revision/brush semantics | **PRESERVE** | serializar a `BspSpatialSnapshot`; BSP sigue source-of-truth |
| `GeologyRecipe.hpp` schema v29 y parser | **PRESERVE AS MIGRATOR** | lectura v29; upgrade determinista a `GeologyStackRecipe` v30; no downgrade |
| `GeologyLayer.hpp` GUID/name/enabled/strength/seed/mask/transform | **PRESERVE** | base del Layer System v30 y del stable UI identity |
| payloads nativos de strata/fault/fold/fracture/erosion/weathering | **MIGRATE** | tabla explícita a built-in generator/layer params; preview informa diferencias |
| `GeologyRecipe.cpp` parsing/validation | **SPLIT/PRESERVE** | separar codec/migration de todo evaluation native |
| `GeologyRecipe.cpp` y `Source/Geology/*` math que produce superficies | **RETIRE** | borrar sólo tras gates 1-20 y fixture migration |
| `GeologyGpuSession.hpp` | **RETIRE** | sin caller tras broker cutover |
| `Backends/GpuDiligent/*`, shaders/cache y `JericoGpuGeologyDiligent` | **RETIRE** | retirar target, compile define y linkage del Editor |
| `ProceduralFieldBackend`/FastNoise2 usados sólo por Geology | **PROVE THEN RETIRE** | `rg`/link graph debe demostrar cero consumidor ajeno; si hay otro, conservarlo fuera de Geology |
| `GeologyCook.hpp/.cpp`, `GeologyCookRequest/Candidate` | **PRESERVE/ADAPT** | consume admitted Blender candidate y mantiene Jerico cook authority |
| `GeologyGenerationJob.*` | **REWRITE IN PLACE** | conserva fencing/progress/cancel API del Editor; backend pasa a broker |
| `GeologyWorldPreparation*`, Preview/Apply/Cancel/Undo/Redo/Recover | **PRESERVE/EXTEND** | transacción multi-product Blender sin perder selección ni trabajo no seleccionado |
| Material/PhysicalMaterial, mask, RBrush, spline contracts | **PRESERVE/BRIDGE** | inputs/outputs tipados del nuevo Layer System |
| PWAS/EPLS, navigation, RuntimeFormats, WorldCell | **PRESERVE** | invalidación dirigida desde product identities nuevas |
| docs/evidence históricos M8 | **PRESERVE AS HISTORY** | banner/link al ADR-043; no reescribir resultados pasados |
| `NativeSdfGeology`/`SparseSdfGeology` históricos ya ausentes | **DO NOT REINTRODUCE** | sólo referencias arqueológicas/documentales |
| cache/hashes del productor viejo | **INVALIDATE/SEGREGATE** | namespace legacy read-only; nunca cache hit para V2 |
| runtime/cooked assets existentes | **PRESERVE** | cargan hasta ser recocinados; no requieren Blender |

### 37.3 Tabla de migración de capas v29 -> v30

| v29 | v30 built-in | Observación |
| --- | --- | --- |
| StrataField/FoldDomain/FaultDisplace | `jerico.layer.strata`, `fold_warp`, `fault_displace` | conserva order/GUID/seed; ajusta unidades |
| JointSet/FractureCells/DirectedCrack | `joint_set`, `fracture_cells`, `directed_crack` | emite damage/region/bond seeds |
| CavityCarve/PlaneCut/ImplicitBlock | `cave_carve`, `plane_cut`, `base_implicit` | boolean + keep-clear obligatorio |
| HeightfieldErosion/ContactPreservingErosion/Colluvium/Rockfall | `erosion_macro`, `contact_preserve`, `debris_deposit`, `rockfall` | ninguna GPU shortcut legacy |
| MoistureTransport/MineralDissolution/Weathering | `wetness_field`, `mineral_field`, `weathering` | base fields para material/EPLS |
| Noise/Voronoi/Aperiodic/GradientDetailWarp/Microstructure | `macro_noise`, `cell_field`, `aperiodic_breakup`, `detail_warp`, `micro_detail` | frecuencia/amplitud en metros, no hashes viejos |
| Mask/RBrush/spline references | mismos GUID/assets | se resuelven por catalog y snapshot hash |

El migrador nunca promete mesh/hash idéntico. Conserva documento GUID, layer GUID/order/enabled/seed, semántica, referencias y authored intent; guarda `migrationFromSchema=29`, `migrationToolHash` y un diff de parámetros. La primera apertura ofrece Preview migrado, no Apply automático. Una vez guardado v30, el productor viejo no puede abrirse como alternativa.

### 37.4 Momento de corte y borrado

1. mientras se construye el reemplazo, el productor viejo queda congelado y test-only para comparación de contratos, no seleccionable por usuario;
2. el broker/new pipeline alcanza todos los gates salvo LegacyProducerAbsent;
3. se cambia service registration, command route y CMake al broker como única ruta;
4. se ejecutan migration, source-free, transactional y Beauty gates sin fallback;
5. se retiran producer sources/targets definidos en la matriz;
6. se ejecuta `VerifyLegacyProducerAbsent` y full presets;
7. se publica evidencia/ADR/roadmap y sólo entonces el foco queda completo.

---

## 38. Autoridad de roadmap y secuencia M9 -> M8 replacement -> M10

### 38.1 Cambio de foco solicitado

La secuencia obligatoria del task continuo de JericoEngine es:

```text
M9 actual
  -> cierre, evidencia, commit/push y ACTIVE_SLICE actualizado
  -> M8-BLENDER-HEADLESS-GEOLOGY-AUTHORITY (esta directiva)
  -> gates 1-20, corte y publicación
  -> M10
  -> M10.5 UI especializada, si sigue planificada
```

No se interrumpe el slice M9 ya activo y no se comienza trabajo M10 antes de este reemplazo. No se crea otra tarea, branch, worktree, fork, coordinador ni checkout writer. Astra continúa en la única tarea/check-out autorizada y admite este foco en `ACTIVE_SLICE.md` sólo después del cierre real de M9.

### 38.2 Cambios documentales al admitir el foco

En la transición legal Astra debe:

- crear/renumerar sin conflicto `docs/decisions/ADR-043-BLENDER-HEADLESS-GEOLOGY-AUTHORITY.md`;
- agregar banner `SUPERSEDED` y enlace al ADR en los documentos que hoy presentan al productor nativo como current authority;
- actualizar `docs/IMPLEMENTATION_ROADMAP.md` con la secuencia anterior y Definition of Done de esta directiva;
- reemplazar `docs/work/ACTIVE_SLICE.md` por un slice acotado de este foco sólo tras publicar M9;
- actualizar `docs/work/HANDOFF.md` con repo status, preservation inventory, gates y siguiente tranche;
- conservar evidencia M8 histórica sin falsificarla ni reclasificar PARTIAL como PASS.

La directiva de usuario 2026-09-06 es más nueva que el roadmap/blueprints que cancelaban Blender/VDB como productor. El ADR registra esta supersesión; contratos comprobados de esos documentos se preservan según la matriz, pero su elección de backend deja de mandar.

### 38.3 Tranches ejecutables para Astra

Cada tranche termina en evidencia y no se mezcla con la siguiente si su gate está rojo:

| Tranche | Producto | Gate de salida |
| ---: | --- | --- |
| 0 | RED contracts, ADR/roadmap/active slice, preservation inventory | tests fallan por broker/pin ausentes, no por infraestructura rota |
| 1 | Blender 5.2.1 admission, lock, probe, sandbox y offline launcher | gates 1-2 |
| 2 | plugin catalog/lock/licensing y GeneratorSchema introspection | gates 3-4; controles aparecen en JericoEditor |
| 3 | broker/job protocol/lifecycle/cache/cancel/recovery | gates 5, 15 y broker subset |
| 4 | BSP snapshot, selection/placement loop y double keep-clear | gate 6 y spatial fixtures |
| 5 | Layer System + cinco node groups built-in propios | gates 7-8 y Beauty previews |
| 6 | GLB attributes/extras, UV, bake, LOD, collision | gates 11-12 |
| 7 | Decal 3D y Spline Destruction sobre los mismos primitives | gates 9-10 |
| 8 | Material/Physical, PWAS/EPLS, nav/world/destruction/cook | gates 13-14 |
| 9 | v29->v30 migration, transaction/recovery, performance | gates 16-17 |
| 10 | source-free package/runtime y Beauty original-resolution | gates 18-19 |
| 11 | switch final, retiro productor viejo, full regression | gate 20 + presets editor/runtime |

El front funcional se integra mediante los widgets Qt/Inspector/World Preparation existentes. Rediseño visual, browser avanzado de node graphs o panel especializado nuevo se difiere a M10.5; eso no permite ocultar plugins ni parámetros durante este foco. La UI existente debe mostrar catálogo, schemas, layers, selection loop, progress, cancel, warnings, provenance y Apply.

---

## 39. Definition of Done no negociable

El foco se considera **PASS** únicamente cuando:

- Blender 5.2.1 LTS exacto está admitido por hash; la instalación local 5.1.2 no se usa como fallback;
- JericoEditor es el único front visible y puede descubrir, inspeccionar, parametrizar, ejecutar, cancelar y diagnosticar todos los plugins admitidos;
- los controles Qt se generan desde `GeneratorSchema` derivado de sockets/panels/descriptions/min/max/default/subtype/enums y round-trip sin pérdida;
- los cinco estilos propios producen Beauty distinta, controlable y determinista;
- BSP con GUID/revision/semantics/keep-clear sigue siendo source-of-truth y no hay invasión jugable;
- Layer System, selection/placement loop, masks, UV, high->low bake, LOD y collision funcionan en una recipe combinada;
- las primitives compartidas sirven a Geology, Decal 3D y Spline Destruction sin duplicar backends ni exponer Blender UI;
- GLB devuelve mesh/LOD/masks/provenance/collision con attributes/extras completos y Jerico los revalida/canoniza;
- PWAS, EPLS, MaterialDefinition, PhysicalMaterial, destruction, navigation, World Cells, cook y runtime conservan sus contratos;
- seeds, cache keys y cold/concurrent runs prueban determinismo;
- cancel/crash/OOM/stale/corruption/disk-full recuperan sin publicación parcial;
- un package source-free reproduce render/collision/nav/destruction sin Blender, Python, source ni red;
- migración v29->v30 preserva authored intent/identity y deja clara la diferencia de producto;
- el productor nativo está marcado **SUPERSEDED**, no invocable, fuera del build graph y ausente de Shipping;
- los 20 gates, full editor/runtime presets, diff check, evidencia original-resolution y auditoría de licencias están en PASS;
- cambios owned están commit/push publicados y `ACTIVE_SLICE`/`HANDOFF` registran honestamente el estado;
- M10 no comienza hasta cumplir todo lo anterior.

Un estado PARTIAL se reporta como PARTIAL con el gate rojo y la evidencia exacta. “Compila”, “abre”, “exporta un GLB”, “se ve en Blender” o “hay un package” no equivale a cierre.

---

## 40. Referencias técnicas

### 40.1 Blender, fuentes primarias

- [Blender releases y ciclo LTS](https://www.blender.org/releases/)
- [Blender 5.2 LTS release](https://www.blender.org/press/blender-5-2-lts-release/)
- [Blender 5.2 downloads oficiales](https://download.blender.org/release/Blender5.2/)
- [Blender command-line arguments 5.2](https://docs.blender.org/manual/en/5.2/advanced/command_line/arguments.html)
- [Blender directory layout y variables de entorno](https://docs.blender.org/manual/en/latest/advanced/blender_directory_layout.html)
- [Blender Extensions command-line arguments](https://docs.blender.org/manual/en/5.2/advanced/command_line/extension_arguments.html)
- [Extensions manifest schema 1.0.0](https://developer.blender.org/docs/features/extensions/schema/1.0.0/)
- [Extension/add-on guidelines](https://developer.blender.org/docs/handbook/extensions/addon_guidelines/)
- [Extension moderation y seguridad](https://developer.blender.org/docs/features/extensions/moderation/guidelines/)
- [Blender licensing](https://www.blender.org/about/license/)
- [Geometry Nodes 5.2 release notes](https://developer.blender.org/docs/release_notes/5.2/geometry_nodes/)
- [Python API 5.2 changes](https://developer.blender.org/docs/release_notes/5.2/python_api/)
- [Blender 5.2 compatibility changes](https://developer.blender.org/docs/release_notes/compatibility/)
- [Geometry Nodes manual 5.2](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/index.html)
- [For Each Geometry Element zone](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/utilities/for_each_geometry_zone.html)
- [SDF Grid Boolean](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/volume/operations/sdf_grid_boolean.html)
- [Geometry Proximity](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/geometry/sample/geometry_proximity.html)
- [Raycast](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/geometry/sample/raycast.html)
- [Store Named Attribute](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/attribute/store_named_attribute.html)
- [UV Unwrap node](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/mesh/uv/uv_unwrap.html)
- [Cycles baking](https://docs.blender.org/manual/en/latest/render/cycles/baking.html)
- [glTF 2.0 exporter](https://docs.blender.org/manual/en/latest/addons/scene_gltf2.html)

### 40.2 JericoEngine, autoridad que se preserva

- `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`
- `docs/JERICO_1_0_PRODUCTION_EXPANSION_GEOLOGY_PWAS_EPLS_ANIMATION_DESTRUCTION.md`
- `docs/PROCEDURAL_PWAS_EPLS_MASTER.md`
- `docs/JERICO_PROCEDURAL_STUDIO_1_0.md`
- `docs/JERICO_PROCEDURAL_3D_LAYOUT_GENERATOR.md`
- `docs/JERICO_PHYSICAL_MATERIAL_FRACTURE_DESTRUCTION.md`
- `docs/ASSET_IMPORT_AND_COOKING.md`
- `docs/JERICO_VIRTUALIZED_WORLD_RUNTIME_1_0.md`
- `docs/superpowers/specs/2026-07-30-material-mask-layer-authoring-design.md`
- `docs/superpowers/specs/2026-08-22-m8-world-cell-bsp-sdf-terrain-design.md`
- `docs/superpowers/specs/2026-08-28-m8-geological-material-integration-design.md`
- `docs/decisions/ADR-031-EDITOR-PROCEDURAL-AUTHORITY-AND-WORLD-PUBLICATION.md`
- `docs/evidence/M8_GEOLOGY_BLUEPRINT_EXIT_AUDIT.md`
- `docs/evidence/M8_DEFERRED_FOR_M9.md`

Estas referencias no tienen igual precedencia: esta directiva decide el nuevo productor; las referencias Jerico conservan los contratos de dominio/publicación/runtime que la matriz marca **PRESERVE**.

---

## 41. Cláusula final para Astra

Astra debe ejecutar esta directiva como cambio de foco inmediatamente después de cerrar M9 y antes de M10, en la tarea y checkout continuos de JericoEngine. Debe investigar/validar contra el Blender 5.2.1 real durante la implementación, construir primero los cinco Geometry Nodes propios, mantener cualquier addon opcional visible y controlable desde JericoEditor, y rechazar toda solución que exponga Blender como segundo front, invada BSP jugable, dependa de red, pierda provenance, publique parcialmente o lleve Blender/Python/source a Shipping.

La autoridad queda dividida de forma deliberada y definitiva:

```text
Jerico BSP/Recipe/Layers/Selection/Materials
    = autoridad de intención y espacio

Blender 5.2.1 headless + locked Geometry Nodes
    = único productor de candidatos geométricos de Geology

Jerico Admission/Cook/Transactions/Runtime
    = autoridad de validez, publicación y ejecución
```

El productor Geology nativo de M8 queda **SUPERSEDED**. No hay dual authority, fallback oculto ni excepción de Shipping.
