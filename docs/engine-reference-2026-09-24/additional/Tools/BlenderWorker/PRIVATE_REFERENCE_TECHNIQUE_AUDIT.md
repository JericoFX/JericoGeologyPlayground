# Auditoría privada de técnicas y biblioteca propia de Geometry Nodes

## Revisión 2026-09-10: por qué un plano acaba en piezas o volumen

Estado de esta revisión: **ESTUDIO PARCIAL; METADATOS GN EXTRAÍDOS;
SIN MODIFICACIÓN NI EVALUACIÓN DE NODOS**.
Amplía el diagnóstico del plano de 10 × 10 documentado en
[integración privada](NEW_ADDONS_BACKEND_INTEGRATION.md). El owner M9 autorizó
inspección estática y documentación en `Tools/BlenderWorker/**`, manteniendo
Blender y tests CPU pausados durante sus adquisiciones Vulkan. La fase inicial
fue estática. Después autorizó una ventana metadata-only hasta 04:23:00 UTC;
se extrajeron graphs con Blender y se liberó a las 04:16:57 UTC, sin hijos.
No se ejecutaron addon Python, Text, drivers, remesh ni renders. No se tocaron
originales, Engine, Editor, Git ni admisión nativa. Los hallazgos finales de
conexiones de abajo sustituyen las hipótesis de la fase inicial cuando se indica.

La sección fechada 2026-09-08 que sigue es histórica. Actualmente los originales
están disponibles en `F:/JericoEngine/_local_reference/blender_addons`, en modo
de consulta para esta tarea. Existen también ejecuciones privadas posteriores;
sus resultados no convierten automáticamente las propuestas propias en nodos
implementados ni aceptados visualmente.

### Evidencia y límites de cobertura

Se revisaron las conexiones retenidas de Rockify, metadatos de Nature,
RockForm, OCD y Stair Builder, y fragmentos relevantes de las implementaciones
Python de Cracker, Edge Decal, Smart Remesh y Smart Segmentation. El inventario
incluye además Instant Clean, Zen UV, SimpleBake y los componentes Auto-Rig.
**Inventariar un paquete no equivale a estudiar todas sus ramas.** No se afirma
haber reconstruido todos los graphs ni leído todos los archivos de cada addon.

Fuentes privadas, relativas a este directorio:

- Rockify: `_local/rockify-control-20260909/probe/data.json`; contiene conexiones
  y defaults de `RockGenerator` y `Granite`. SHA-256 del informe verificado ahora:
  `14a23dab5d82b7e8e1384eb891b596c9b031d9df832d452f588eeb491cf20d4f`.
- Nature: `_local/intake-a6d99c65-dce4-4b4d-a9ef-f485e1686465/The Nature Generator/metadata.json`;
  SHA-256 `6405b0b72a036ff74b33488bef8c7e9b212b60b5d3b8aa85fd0c68b9f10a3338`.
- RockForm: mismo directorio de intake, `rockform_1.4.1/metadata.json`;
  SHA-256 `fd49694000f6ef24199ef034183f6aeb55a1ea35259464cf02b795255d60b1b0`.
- OCD: mismo directorio de intake, `ocd_2/metadata.json`.
- Stair: `_local/security-intake-20260909/stair-metadata-blender/metadata.json`.
- Resultados geométricos anteriores: `_local/plane-20260910/source-space-counts/`
  y `_local/plane-20260910/rockify-fragmentation/`.

Los hashes anteriores identifican informes, no son una revalidación de todos
los paquetes originales. Los metadatos de Nature/RockForm/OCD/Stair conservan
tipos, interfaces y dependencias, pero no conexiones completas ni la resolución
de cada switch. Una lista de nodos no demuestra qué rama produjo la salida.

### Rockify: causa estructural identificada

El graph principal trata la fuente como **soporte de distribución**, no como
superficie que conservar. La ruta conectada distribuye puntos sobre sus caras,
coloca instancias de una colección y realiza las instancias como salida. No hay
una rama que lleve la superficie original al resultado final de ese graph.

El grupo Granite, por su parte, construye su propia base cúbica. La deforma,
subdivide, extruye una selección aleatoria, vuelve a desplazar y suaviza mediante
subdivisión. Es una pieza rocosa, no un operador que reciba y deforme las caras
del plano. Corregir la dependencia de la fuente hizo funcionar el placement;
no cambió la naturaleza del producto, que sigue siendo una formación de piezas.

Además hay una dependencia conectada entre los bounds de la fuente y la escala
de las instancias. El informe guarda inputs, pero no todas las propiedades de
operación de los nodos Math/Vector Math: no reconstruir una fórmula exacta sólo
por sus nombres. El default retenido de Density es 10; el de Scale Elements,
0,7. Son defaults del snapshot, no un inventario completo de valores efectivos
de todas las ejecuciones, drivers y modos de distribución.

Esto explica estructuralmente por qué el plano no sobrevive como relieve. La
contribución cuantitativa del escalado por bounds debe aislarse con una prueba
de una sola variable. Los 399 componentes del caso Granite/quad corresponden
a conectividad topológica del resultado; no prueban 399 instancias, objetos,
draw calls ni piezas físicas.

`Realize Instances` convierte referencias en geometría editable: no suelda
superficies ni calcula una unión booleana. Por tanto, realizar antes del export
no elimina por sí mismo interiores, solapamientos ni coste geométrico. Véase
la [documentación oficial de Realize Instances](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/instances/realize_instances.html).

Aprendizaje aprovechable: separar construcción de la pieza, distribución,
orientación, escala y acabado. Para un futuro modo de relieve propio, no usar
esta cadena completa como si fuera un deformador. Para dispersión de piedras,
sí es una referencia conceptual pertinente, con un presupuesto explícito de
instancias y tamaño independiente del área total de soporte salvo elección de
autoría. Bajar densidad no convierte scatter en superficie continua.

### Nature: topología, capas y calidad requieren pruebas separadas

La ruta probada es `NG_Fractured_Rock_Formation_Generator`, no todos los
generadores Nature. Sus metadatos incluyen Dual Mesh, Duplicate Elements,
Split Edges, dos Extrude Mesh, Subdivide Mesh, dos Subdivision Surface,
Set Position y la dependencia `NG_Scatter_Effect`.

La interfaz distingue `Layer Amount`, `Layer Scale`, `Base Shape SubDiv Level`,
`Extrude in Normal Direction`, extrusión de detalles, displacement y resolución
de viewport/render. En el snapshot de interfaz, `Layer Amount` tiene default 3;
esto no demuestra por sí solo que sea el valor efectivo del modifier del test.
La política preview propia sí asigna `Socket_4 = 1` y `Socket_12 = 0`, pero esos
controles son subdivisión base y subdivisión de detalle: no eliminan todas las
etapas multiplicadoras ni fuerzan la rama viewport de un job background.

El mismo soporte XY con 64 caras produjo 79.062 vértices, frente a 1.191 con
un quad. Esto es consistente con generación dependiente de topología, pero no
identifica todavía qué nodo aporta cada incremento. Debe medirse la salida de
las etapas de duplicación, extrusión, subdivisión y scatter por separado,
conservando igual seed, controles, transform y cámara. No desactivar grupos por
nombre ni atribuir el resultado al scatter sin comprobar sus switches.

`Layer Amount` representa capas geométricas del generador; no reemplaza el
Layer System de Jerico con selección, GUID, orden, bloqueo, máscaras y revisión.
El shader `NG_Layer_Masks` tampoco equivale por sí solo a ese contrato.

Nature contiene candidatos adicionales para aprender: `NG_Basic Rock Generator`,
`NG_Cliff_Generator`, `NG_Layered_Cliff_Formation_Generator`,
`NG_Layered_Rock_Formation_Generator`, `NG_Ground_Assembly_Base`,
`NG_Terrain_Generator`, `NG_Terrain_Steps`, displacement, scatter y UV box
projection. Su presencia amplía el mapa de estudio, no la cobertura del test
del plano ni la lista de adaptadores operativos.

### RockForm, OCD y Cracker: no confundir hipótesis con causa

RockForm separa `RF_SetShape` de `RF_SetMat`/`RF_DisplaceTex`. El primero
contiene placement desde curva/superficie/volumen, instancias y conversión
mesh-volume-mesh; la interfaz expone guía, colección, método de spawn, density,
voxel size, escala y spread. La prueba produjo una masa cerrada y prácticamente
el mismo conteo para quad/rejilla. Esto es consistente con reconstrucción de
masa, no con conservar las caras de la guía. Falta trazar la rama activa para
atribuirlo a un socket concreto. La política preview asigna voxel size 0,4:
ese cambio afecta una reconstrucción volumétrica, no es un simple LOD.

OCD ofrece una separación especialmente útil entre máscaras y desplazamiento:
dirección, gradiente, ruido, material, combinación, imagen y transferencia UV.
También contiene conversión a volumen en grupos Start/End y en OCD Plus.
Sobre el plano la salida fue vacía, no demasiado densa. Una rama volumétrica,
un filtro de selección o la etapa de damage pueden explicarlo; el histograma
de nodos no permite escoger una como causa demostrada.

Cracker expone un marco controlador y operaciones locales, no sólo un seed de
roca: `constants.py:19-92` identifica referencia Empty, radio, thickness,
displacement, push/pull, lado, pin, cut-through y UV. Los lados se describen
respecto del eje Y del controlador. Antes de culpar al volumen hay que comprobar
si el marco/controlador del test intersecta y orienta correctamente el plano XY,
y si sus opciones de lado/corte seleccionan geometría. No hay en esta revisión
un dump de conexiones completo de `Cracker_GN`; su causa de salida vacía sigue
sin resolver. No convertir comentarios de compatibilidad del proveedor en una
garantía verificada para todos los sockets.

### Aprender de todos los paquetes, sin tratarlos todos como Geometry Nodes

| Familia disponible | Evidencia y enseñanza | Alcance aún no demostrado |
|---|---|---|
| Rockify | Conexiones de formación por scatter y pieza Granite; separar placement de relieve | Todos los presets y variantes como superficie |
| Nature | Inventario de 83 grupos, interfaces, familias de terreno/roca/masks/UV | Ramas activas completas y mejora visual de cada familia |
| RockForm | 15 grupos; separar construcción de masa y detalle/shading | Bypass de masa que conserve una superficie arbitraria |
| OCD | 23 grupos; máscaras separadas, displacement y UV por corners | Causa del plano vacío y conservación de semántica final |
| Cracker | Contrato de controladores, lado, pin, radio, espesor y corte en Python | Graph completo y diagnóstico aislado del plano vacío |
| Edge Decal | Python: layers, máscaras booleanas y extracción de superficie tras volumen auxiliar | Equivalencia con decal 3D/spline destruction nativos |
| Smart Remesh | Python: campo de orientación/posición y otra ruta vía Instant Meshes | LOD con tolerancias geométricas y semánticas completas |
| Smart Segmentation | Python: diámetro de forma, adyacencia y agrupación jerárquica | Etiquetado geológico, frontera espacial/BSP y piezas Jolt |
| Stair Builder | Metadatos GN: curva, peldaños, barandillas, extrusión, sides y UV | Evaluación y binding de todas las ramas en el backend |
| Instant Clean | Inventario/revisión estática anterior; limpieza de malla | Diagnóstico nuevo de sus operaciones y tolerancias |
| Zen UV | Inventario/revisión estática anterior; seams/islas/UV/texel density | Ejecución aislada admitida de cada operación |
| SimpleBake | ZIP inventariado/revisado anteriormente; etapa de baking | Inspección nueva del flujo y operación headless admitida |
| Auto-Rig Pro / Rig Tools / Proxy Picker / glTF incluido | Componentes separados en inventario anterior; rigging/export, no generador de roca | Admisión y adaptadores por operación; no ejecutar código de botones |

La carpeta y ZIPs duplicados no representan funciones nuevas automáticamente.
Los cuatro últimos renglones no recibieron una lectura completa nueva en esta
revisión. El alcance exhaustivo solicitado permanece abierto hasta cubrirlos
por operaciones relevantes, además de extraer las conexiones GN que faltan.

Hallazgos concretos fuera de los graphs geológicos, en la raíz privada de addons:

- `Edge_Decal_Generator_v1.2.1/features/layers.py:1091-1208` y `1392-1468`
  contienen preparación de volumen auxiliar, operaciones de máscara y extracción
  de la superficie original. `_extract_original_decal_surface` consulta un BVH
  de la superficie original y conserva caras cuyos vértices quedan dentro de
  tolerancia. La idea útil es distinguir geometría de cálculo de geometría final.
  Ese criterio de vértices cercanos no demuestra por sí solo que el interior de
  cada triángulo respete keep-clear, ni sirve sin cambios para una superficie
  a la que se pidió añadir relieve.
- `smart Remesh/core/remesh.py:32-194` trabaja sobre arrays, guía orientación y
  posición, reconstruye caras y permite smoothing. El propio código deja
  smoothing en cero por falta de reproyección en esa etapa; no interpretar sus
  comentarios de métricas como mediciones realizadas aquí. `core/auto.py:30-36`
  llama a Instant Meshes y puntúa el candidato: es otra ruta, no el mismo
  algoritmo ni una autorización para lanzar ese ejecutable.
- `Smart Segmentation v1.0/Smart_Segmentation.py:546` usa **Shape Diameter
  Function**. Su abreviatura SDF no significa aquí Signed Distance Field de
  BSP/keep-clear. `cluster` en línea 767 usa adyacencia y agrupación; asignar una
  misma etiqueta a shells desconectados no los suelda ni reduce su geometría.

### Orden exacto de comprobación pendiente

#### Ampliación: ruido, superficie y detalle útil

El usuario pidió explícitamente estudiar también los ruidos. No se tratarán como
un único slider de intensidad. La auditoría debe retener tipo y dimensión,
coordenadas, escalado/orientación, roughness/lacunarity/detail, normalización,
warp/distortion, ramps/remapeos y destino del campo. Un mismo ruido puede
alimentar altura, vector displacement, selección, color o roughness; esos usos
no son intercambiables.

Hechos ya observados:

- En Granite se usa la salida **Color** de Noise en una cadena vectorial que
  participa en desplazamiento. En el dump Sedimentary, Voronoi **Color** también
  llega a una cadena de desplazamiento y Seed se conecta a W. Eso no demuestra
  uso de Distance to Edge para grietas ni hace de W el seed del scatter.
- RockForm separa RF_DisplaceTex y RF_Triplanar de la forma; el primero combina
  seis Noise, dos Voronoi, cinco Wave y diecisiete ramps. La combinación,
  orientación y remapeo merecen estudio, no sólo el conteo de octavas.
- Nature NG_Layer_Masks combina controles de pendiente, altura y ruido;
  NG_Terrain_Steps expone pasos/remapeo y Noise. NG_Snow_Shader contiene tres
  Noise y un Gabor: evidencia de acabado shader, no de geometría glaciar.

Mapa conceptual para evaluar futuras implementaciones propias, no preset
implementado ni promesa de equivalencia visual:

| Función | Campo candidato | Dónde debe evaluarse |
|---|---|---|
| Volumen/relieve principal | Ruido de baja frecuencia remapeado, forma direccional y máscara de autoría | Geometría; limitado por soporte/keep-clear |
| Estratificación | Bandas orientadas y deformación suave de sus coordenadas, espesores controlados | Geometría para silueta; máscara de estrato para material |
| Juntas/fracturas | Distancia a fronteras de celdas y máscara de separación | Relieve o corte explícito; no fragmentar siempre |
| Desgaste/cavidades | Campos combinados por exposición, concavidad y selección | Geometría mesoscópica y masks; no equivale a erosión física simulada |
| Nieve, escarcha, poros | Ruido fino y señales direccionales | Principalmente bake/material; geometría sólo cuando afecta silueta |
| Variación de piezas | Valores discretos derivados de IDs estables | Instancing; no alterar todo el campo al cambiar la triangulación |

Para el mismo plano y el mismo marco métrico, el **campo continuo** no debe
depender de cuántas caras se usaron para representarlo. La aproximación de malla
sí puede diferir por muestreo. Comparar el campo en posiciones comunes separa
un cambio geológico accidental de un simple error de discretización. Evaluar
contra bordes, orientación, escalas no uniformes y superficies plegadas; evitar
coordenadas implícitas cuyo dominio cambia entre GN y shader.

No subir Detail indefinidamente: si la longitud de onda es menor que la separación
de vértices, la geometría no resuelve ese campo. No bajar su frecuencia por LOD
y llamarlo equivalente: conservar el high como referencia, medir error, y
trasladar el detalle no representable al bake/material con masks coherentes.
La normalización de amplitud y un límite de desplazamiento explícito evitan que
añadir octavas altere silenciosamente la envolvente espacial.

Fuentes oficiales 5.2 consultadas para este apartado:
[Noise](https://docs.blender.org/manual/en/5.2/render/shader_nodes/textures/noise.html),
[Noise en GN](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/texture/noise.html),
[Voronoi](https://docs.blender.org/manual/en/5.2/render/shader_nodes/textures/voronoi.html),
[Wave](https://docs.blender.org/manual/en/5.2/render/shader_nodes/textures/wave.html),
[Gabor](https://docs.blender.org/manual/en/5.2/render/shader_nodes/textures/gabor.html) y
[White Noise](https://docs.blender.org/manual/en/5.2/render/shader_nodes/textures/white_noise.html).
Noise admite familias fractales; Voronoi F1 mide distancia al sitio, no al borde;
Wave permite bandas/anillos y warp de coordenadas; Gabor aporta estructura
direccional. El manual advierte artefactos al muestrear Noise en coordenadas
alineadas con su retícula. White Noise es discreto, no un reemplazo del campo
suave, y cambios diminutos en su entrada pueden cambiar por completo su salida.

Otro hallazgo útil es `Edge_Decal_Generator_v1.2.1/features/surface_voronoi.py`:
`surface_voronoi_build_surface_charts` en línea 857 agrupa soporte coplanar;
`surface_voronoi_merge_chart_fragments` en línea 1826 elimina fronteras internas
debidas a triangulación. La ruta de emisión alrededor de línea 2431 es
experimental/opt-in y reutiliza índices de vértices en fronteras compartidas.
Es referencia conceptual para no convertir cada cara del soporte en otra roca.
No se ha probado aquí su ruta ni debe propagarse automáticamente a través de
caras no seleccionadas/protegidas como permite el recorrido del addon.

Instant Clean también aporta un límite aprovechable: `utils/dissolve.py` usa
delimitadores NORMAL/MATERIAL/SEAM/SHARP/UV para disolver. Una limpieza que
reduzca caras pero borre fronteras de selección, physical material, UV o región
no es una optimización válida. Sus operadores trabajan con contexto/selección
Blender; el backend no debe heredar ese contexto como autoridad Jerico.

#### Experimentos geométricos pendientes

Estas son pruebas propuestas, no ejecutadas ni implementación autorizada en la
reserva actual. Se reutilizará el worker y la sonda del plano, sin scheduler,
dependencias ni bypass de presupuestos nuevos.

1. Rockify: medir puntos e instancias antes de realizar; variar sólo densidad,
   después sólo escalado, en experimentos separados. Predicción: cambia la
   multiplicidad/tamaño, pero la superficie fuente continúa ausente.
2. Nature: registrar conectividad activa, defaults efectivos, drivers, switches
   y conteos en cada etapa. Variar por separado capas geométricas, extrusión,
   resolución y scatter. Predicción: identificar el primer aumento de
   componentes/densidad, sin suponer que todos los mecanismos están activos.
3. RockForm: resolver el spawn method y la cadena de reconstrucción activa;
   comparar la entrada/salida de SetShape con la del acabado. Predicción: si el
   volumen ya existe antes de RF_SetMat, el detalle no es su causa inicial.
4. OCD y Cracker: localizar la primera etapa que pasa de caras a cero. En Cracker,
   verificar primero marco, intersección, lado y cut-through. En OCD, comprobar
   máscaras y ramas de remesh. Cambiar sólo la condición que distingue cada
   hipótesis; no solidificar el plano para ocultar el fallo del contrato.
5. Tras ese diagnóstico, decidir con el owner qué adaptación privada o grupo
   propio implementar, con un RED de relieve sobre soporte y luego validación
   geométrica/visual. Los originales siguen inmutables.

El candidato de superficie debe conservar soporte/selección, deformar con un
presupuesto físico explícito y separar volumen temporal, piedras independientes
y detalle de material. Un plano sin suficientes vértices requiere densificación
antes del desplazamiento; subdividir no crea relieve por sí solo. No prometer
fidelidad de roca con un único ruido ni equivalencia artística sin nuevas imágenes.

Controles futuros en Jerico deben describir la operación real: relieve, masa,
dispersión, daño, segmentación, UV o bake. Los controles geométricos de un addon
no sustituyen GUID/selección/layers de autoría. El descriptor de capacidades debe
hacer visible qué está operativo, experimental o bloqueado; la UI Qt sigue en
M10.5 y no se modifica aquí. PWAS/EPLS, BSP/keep-clear, physical material,
LOD/masks/provenance/collision conservan las fronteras de los contratos existentes.

### Fuentes primarias para interpretar los nodos

La semántica general se contrastó con el manual oficial **4.5 LTS**; estas páginas
no certifican interfaces ni comportamiento específico de Blender 5.2.1. El pin
instalado y las pruebas locales anteriores son la evidencia de esa ejecución;
cualquier adaptación debe volver a verificar sockets y resultados en ese pin.

- [Distribute Points on Faces](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/point/distribute_points_on_faces.html): muestreo de caras, densidad y modos de distribución.
- [Instance on Points](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/instances/instance_on_points.html): colocación de referencias de geometría en puntos.
- [Set Position](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/geometry/write/set_position.html): desplazamiento de puntos; sobre instancias mueve sus orígenes, no sus vértices internos.
- [Subdivide Mesh](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/mesh/operations/subdivide_mesh.html): densificación por interpolación simple.
- [Mesh to Volume](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/mesh/operations/mesh_to_volume.html) y [Volume to Mesh](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/volume/operations/volume_to_mesh.html): volumen de densidad e isosuperficie; no equivalen a desplazar un soporte abierto.
- [Sample Nearest Surface](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/mesh/sample/sample_nearest_surface.html) y [Raycast](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/geometry/sample/raycast.html): transferencia por superficie/rayo; controlar validez e Is Hit antes de usar muestras.
- [Store Named Attribute](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/attribute/store_named_attribute.html): elegir dominio/tipo; un atributo nuevo asigna cero fuera de selección. Por eso escribir sólo la región no garantiza conservar un atributo previo de todo el producto.

**Cierre de esta revisión:** causa estructural del scatter Rockify identificada;
hipótesis localizadas para Nature/RockForm/OCD/Cracker; diferencias útiles de
decal, remesh y segmentación registradas. Sin nueva prueba Blender ni corrección
de nodos. No hay PASS de calidad, de integración ni de cobertura exhaustiva.

### Extracción posterior: conexiones, ramas y ruido verificados

Este apartado actualiza el cierre de la fase estática anterior. La herramienta
diagnóstica `tests/probe_private_graphs.py` leyó doce bibliotecas auditadas con
Blender 5.2.1 fijado, usando el runner existente: background/factory/offline,
autoexec desactivado, dos threads, BELOW_NORMAL, timeout de 30 s por hijo,
recursos de usuario aislados y hashes de biblioteca antes/después. Cargó datos
sin enlazar los objetos a la escena ni solicitar un depsgraph evaluado. No
registró addons ni evaluó expresiones de drivers; sólo leyó las expresiones.

La primera tanda `extraction/` falló en el inspector por asumir que todo socket
del modifier tiene `.value`: el socket Geometry no lo tiene. Se corrigió la
lectura a un registro `hasValue/value`; ese fallo no pertenece a los addons.
La segunda tanda `extraction-b/` terminó doce hijos con exit code 0. El padre
ahora devuelve error si una tanda queda PARTIAL. No se modificó código de
producción ni se presentó esto como un ciclo RED/GREEN de un generador.

Evidencia privada: `_local/graph-audit-20260910/manifest.json` y
`_local/graph-audit-20260910/extraction-b/summary.json`. Cada subdirectorio
numérico conserva invocación, hash de inspector, logs y `graph.json`.

| Carpeta | Biblioteca | Registros de grupos | Nodos |
|---|---|---:|---:|
| 0 | Nature | 83 | 4.256 |
| 1 | RockForm | 15 | 490 |
| 2 | OCD | 23 | 883 |
| 3 | Cracker | 9 | 1.889 |
| 4 | Stair | 34 | 2.029 |
| 5 | Rockify Granite | 2 | 42 |
| 6 | Rockify Igneous | 2 | 40 |
| 7 | Rockify Limestone | 3 | 56 |
| 8 | Rockify Sedimentary | 3 | 79 |
| 9 | Rockify Stalactite | 2 | 45 |
| 10 | Rockify Stalagmite | 2 | 44 |
| 11 | Rockify Volcanic | 2 | 43 |
| Total | Doce bibliotecas | 180 | 9.896 |

Son registros por biblioteca, no 180 implementaciones únicas. Por ejemplo,
RockGenerator se repite; Cracker conserva versiones antiguas y grupos de vidrio
que no alimentan necesariamente al generador activo. Extraer 9.896 nodos no
significa haber explicado manualmente cada uno ni haber demostrado sus efectos.
Los transforms de objetos no enlazados son metadatos, no matrices evaluadas para
repetir las mediciones de espacio fuente del test del plano.

#### 1. Nature: el scatter sospechoso está desconectado

En `NG_Fractured_Rock_Formation_Generator` existe un nodo `NG_Scatter_Effect`,
pero tiene **cero enlaces**. Su presencia en los metadatos no explica ni las
piezas ni la densidad de este preset. Queda descartada esa hipótesis para este
graph, no para toda la familia Nature.

La entrada Geometry sí alimenta Duplicate Elements en dominio FACE; Amount
recibe `Socket_2` (Layer Amount), cuyo valor guardado en el modifier original
es 3. Siguen transformación/escala de capas, merge, subdivisión, triangulación,
dual mesh, extrusión de aristas, desplazamiento, Split Edges, otra densificación,
dual mesh, merge, extrusión individual de caras, subdivisión y acabado. Es una
construcción topológica por capas, no sólo desplazamiento de una superficie.
Esto localiza mecanismos concretos; aún hace falta una ablación geométrica
para asignar a cada etapa una parte exacta de los conteos observados.

La subdivisión final recibe un Switch INT controlado por Is Viewport:
True toma `Socket_20`, False toma `Socket_21`, a través de Clamp. El modifier
guarda 1 y 3 respectivamente. **Background no significa automáticamente False**:
Is Viewport depende del modo del depsgraph, no de la ausencia de UI. La sonda
anterior congela geometría usando el depsgraph de contexto; no registró su modo.
No atribuirle factor de subdivisión 3 sin medir ese modo/valor efectivo. Fuente:
[Is Viewport 5.2](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/input/scene/is_viewport.html).

#### 2. Rockify: escala global convertida implícitamente en escalar

El graph está en distribución POISSON. Scale Elements opera por FACE; un plano
con distintas caras no es necesariamente el mismo dominio de muestreo después
de esa operación. Además, la cadena conectada obtiene Max−Min de los bounds,
lo escala por 0,14 y lo conecta a un socket **Scale de tipo float** que escala
un vector aleatorio. Luego aplica otros escalados antes de Instance on Points.

La conversión implícita Vector→Float promedia XYZ, no toma sólo Z ni calcula
longitud. Por tanto, que el plano tenga espesor cero no hace cero la escala Z
de las piezas. Con bounds 10×10×0, esa etapa produce aproximadamente 0,9333 como
escalar. Este cálculo explica el mecanismo; no pretende reconstruir todas las
transformaciones finales o métricas de la prueba a partir de un único número.
Fuente: [conversión de atributos 5.2](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/attributes_reference.html).

En esta rama, Density no es subdivisión ni desplazamiento. Tampoco el Clamp
visible en el árbol limita automáticamente la escala final: hay que seguir su
salida, no confiar en el nombre de un nodo que puede no contribuir al resultado.

#### 3. RockForm: reconstrucción de masa confirmada en el preset

Los comparadores de RF_SetShape distinguen Spawn Method 0/1/2. El modifier
original guarda 0; esa comparación habilita la distribución en volumen de la
guía. Las formas provienen de la colección de boulders. Tras placement,
rotación y realización, la rama final convierte a volumen y extrae malla.
`Debug Instances` decide entre esa malla y las instancias realizadas; el valor
guardado es false, por lo que selecciona la malla reconstruida. La política
preview del adaptador no cambia Spawn Method ni Debug Instances.

Este preset construye masa antes del acabado RF_SetMat. Cambiar sólo el ruido
del acabado no lo transforma en un generador que conserve un plano abierto.
Se identificó también un modifier Decimate deshabilitado tanto en viewport
como en render: que exista en la pila no prueba que el LOD esté funcionando.

#### 4. Cracker: descartar la hipótesis de volumen; seguir corte/selección

`Cracker_GN` tiene 355 nodos y una dependencia directa,
`Crckr_InputCircleGroup` (37 nodos). Ninguno contiene nodos de volumen.
Sí hay un Boolean DIFFERENCE, separación/borrado por distintos dominios,
selección de lado y ramas de cut-through; la salida realiza un Join Geometry.
Por eso la sospecha anterior de un remesh volumétrico no es una explicación
válida de la ruta activa. Su primera etapa vacía aún requiere instrumentación
geométrica, que esta ventana no autorizó.

OCD sí tiene una conversión mesh→volume→mesh dentro de OCD Plus y un Bake
entre ese grupo y la salida de OCD_GN. Sus condiciones/ramas y selecciones deben
resolverse antes de concluir dónde desaparece el plano. El nodo Bake tampoco
prueba que exista una caché horneada ni una salida válida para esa entrada.

#### 5. Ruidos de los siete presets: diferencias efectivamente almacenadas

| Preset | Base/topología inspeccionada | Campos de ruido observados |
|---|---|---|
| Granite | Cubo, extrusión, subdivisión y desplazamiento | Dos Noise 3D fBM normalizados |
| Igneous | Cubo, subdivisión y desplazamiento | Noise 3D fBM normalizado; Voronoi 3D Smooth F1 con Chebyshev, sin normalizar |
| Limestone | Cubo, extrusión, varias subdivisiones y grupo auxiliar | Dos Noise 3D fBM normalizados |
| Sedimentary | Conos deformados y grupo auxiliar; capas/ensamblaje | Dos Voronoi 4D F1 Manhattan sin normalizar; Noise fBM 3D en auxiliar |
| Stalactite | Cono, extrusión, subdivisión y desplazamiento | Dos Noise 3D fBM normalizados |
| Stalagmite | Cono, extrusión, subdivisión y desplazamiento | Dos Noise 3D fBM normalizados |
| Volcanic | Dos cubos, Boolean y desplazamiento | Noise 3D fBM normalizado |

Estos datos no convierten F1 Manhattan en un campo de grietas Distance to Edge.
Hay que distinguir Color, Distance y Position de Voronoi, la orientación de sus
coordenadas y el lugar donde entra el resultado. La formación sedimentaria
inspeccionada no se reduce a un Wave de bandas; su estructura también viene
de la geometría base y de cómo se ensambla. Para una implementación propia,
estudiar separadamente esa estructura y el aspecto que aporta cada campo.

El dump nuevo registra dimensiones/tipo de ruido, normalización, distancia y
feature Voronoi, propiedades Wave/Gabor disponibles, sockets y Color Ramps.
No es un serializador universal de todos los datos internos de Blender: por
ejemplo, no conserva íntegramente todo Curve Mapping ni evalúa drivers o frames.
La transferencia a nodos propios debe partir de conceptos y criterios, no
traducir mecánicamente el dump privado a código redistribuible.

**Resultado útil y límite:** ya hay causas estructurales localizadas, hipótesis
descartadas y señales/operaciones diferenciadas por estilo. No hay aún una nueva
roca mejorada, un relieve propio aceptado ni una auditoría de cada rama de todos
los addons. Continúa pendiente la prueba de una sola variable y la implementación
propia bajo el owner/slice correspondiente. CPU/Blender quedaron pausados tras
liberar la ventana; el resto del estudio fue lectura de los dumps.

## Auditoría histórica del 2026-09-08

Fecha: 2026-09-08. Propietario: JericoEngine. Alcance de escritura:
`Tools/BlenderWorker/**`. Estado: **DISEÑO TÉCNICO DERIVADO DE METADATOS;
IMPLEMENTACIÓN Y VALIDACIÓN VISUAL AÚN NO ADMITIDAS**.

Este documento convierte el permiso explícito del propietario en una frontera de
trabajo verificable. Jerico puede leer sus addons comerciales adquiridos y estudiar
su comportamiento, organización, interfaces y técnicas. El producto resultante
debe usar node groups, nombres, código, schemas, fixtures y evidencia creados por
Jerico. Ningún ZIP, BLEND, script, material, textura, asset, conexión exacta o
identidad privada se redistribuye, se incorpora a Shipping ni se vuelve dependencia
del backend salvo que la licencia individual lo autorice expresamente.

El objetivo no es clonar un producto. Es identificar patrones técnicos generales,
reconstruirlos detrás de contratos propios y superar sus límites para un engine
multigénero, headless, determinista y gobernado por BSP, selección, layers,
keep-clear, cook y runtime Jerico.

## 1. Autoridad, evidencia disponible y límite de la inferencia

La autorización fue comunicada al hilo principal M9
`01a08009-7e3f-7722-a6f7-cdf17f61da0e`, que la reconoció el 2026-09-08. No
amplía la propiedad de carpetas: este trabajo no modifica Engine, Editor, cook,
renderer, Jolt, navegación, Git ni documentos compartidos.

Los paquetes originales completos no están montados actualmente dentro de
`Tools/BlenderWorker`. Sí están retenidos los informes privados generados por una
inspección anterior con Blender 5.2.1. Esos informes conservan SHA-256 del BLEND,
lista de datablocks, nombres e interfaces de grupos, tipo y cantidad de nodos,
cantidad de enlaces y dependencias entre grupos. No conservan la conexión exacta
de cada socket, posiciones de nodos, valores internos completos ni imágenes de
resultado.

| Referencia privada | Biblioteca inspeccionada | Identidad retenida | Cobertura |
| --- | --- | --- | --- |
| RockForm 1.4.1 | `rockform/assets/RockForm_v140_B5.blend`, 70,303,281 bytes | `eaf896aaf335fc8e824bbfb8d9f55e8109fb8e79684d19466c984177bf08086c` | 15 grupos; forma, placement, volumen, displacement, triplanar y shader. |
| The Nature Generator 1.2 | `The_Nature_Generator/asset_library/Nature_Generator_Assets.blend`, 314,399,436 bytes | `234724d0de9f10aeac4674a69801fc659899ef3c753af1409a9564811cc8da47` | 83 grupos; terrain, cliff, rocks, fracture, scatter, masks y materiales. |
| OCD 2 | `data.blend`, 4,940,422 bytes | `f2fa002014967736558e888e7bb78e2f9bfd35e17d7e745af0fa175b8376fde0` | 23 grupos; selección, máscaras, displacement, scatter y UV transfer. |
| Edge Decal Generator 1.2.1 | `assets/materials/edge_decal_materials.blend`, 121,002 bytes | `558f92fb1c1d5e7db30ef67b329dc92faacb6ab37c3edef1ee00cb4185d8bd23` | Materiales; no node group geométrico retenido. |
| Smart Segmentation 1.0 | sin biblioteca binaria detectada | hash de informe/source retenido en JSON | Flujo Python de segmentación; sin graph binario que pueda auditarse aquí. |

Fuentes locales de esta auditoría:

- `_local/intake-a6d99c65-dce4-4b4d-a9ef-f485e1686465/reports-final/rockform_1.4.1.json`
- `_local/intake-a6d99c65-dce4-4b4d-a9ef-f485e1686465/reports-final/The Nature Generator.json`
- `_local/intake-a6d99c65-dce4-4b4d-a9ef-f485e1686465/reports-final/ocd_2.json`
- `_local/intake-a6d99c65-dce4-4b4d-a9ef-f485e1686465/reports-final/Edge_Decal_Generator_v1.2.1.json`
- `_local/intake-a6d99c65-dce4-4b4d-a9ef-f485e1686465/reports-final/Smart Segmentation v1.0.json`

Cada afirmación usa uno de estos niveles:

| Nivel | Qué permite afirmar |
| --- | --- |
| `M0` | Nombre de archivo, hash, datablock o texto estático. |
| `M1` | Interfaz, dependencias, tipos y conteos de nodos/enlaces inspeccionados. |
| `M2` | Técnica probable respaldada por una combinación inequívoca de nodos. |
| `G1` | Conexiones y defaults exactos, sólo después de volver a montar el BLEND. |
| `V1` | Efecto geométrico/visual demostrado con fixture, captura original y métricas. |

La evidencia actual llega a `M2`. Este documento no afirma `G1` ni `V1`, ni que
una reconstrucción produzca equivalencia visual hasta superar sus gates.

## 2. Hallazgo principal: la calidad es una cadena, no un generador único

Los tres catálogos de nodos coinciden en una arquitectura útil:

1. Construir o elegir masa/base.
2. Posicionar, rotar y escalar por superficie, volumen, curva o puntos.
3. Crear estructura grande con capas, celdas, extrusión, dual mesh o curvas.
4. Fusionar/remallar cuando la silueta necesita continuidad.
5. Aplicar displacement multiescala con orientación controlada.
6. Calcular máscaras geométricas separadas de materiales.
7. Transferir/producir UV y atributos de procedencia.
8. Añadir materiales con AO, variación, edge wear, bump/normal y coberturas.

Por eso Jerico no debe construir un `RockGenerator` monolítico. Debe componer
grupos profundos y pequeños, y hacer que cada etapa exponga selección, IDs,
rechazos, máscaras y provenance. La geometría define masa y lectura; el shader y
el bake aportan microdetalle. Confundir ambos causa rocas con buen material pero
silueta pobre, o mallas densas que intentan modelar detalle que pertenece al bake.

## 3. Evidencia técnica recuperada

### 3.1 RockForm: forma multifuente, fusión y detalle multiescala

`RF_SetShape` contiene 35 nodos y 63 enlaces. Combina `Collection Info`,
`Object Info`, `Curve to Points`, `Resample Curve`, distribución en caras y
volumen, `Mesh to Points`, `Instance on Points`, rotación, separación de
componentes, dos `Mesh to Volume`, `Volume to Mesh` y `Realize Instances`.
Depende de grupos separados de rotación base/aleatoria, spread, escala aleatoria
y normalización por volumen. La inferencia `M2` es clara: un mismo stage acepta
fuentes de superficie, volumen, curva y puntos; normaliza placement e instancias;
y usa volumen para convertir agregados en una masa continua.

`RF_SetMat` contiene 51 nodos y 94 enlaces. Separa subdivisión, shade smooth,
displacement, set position y material de la construcción de forma. `RF_DisplaceTex`
contiene 98 nodos y 163 enlaces: seis Noise, dos Voronoi, cinco Wave, diecisiete
Color Ramp, dieciséis Mix y catorce Math. La técnica no es “Noise sobre normal”,
sino bandas y frecuencias combinadas, remapeadas e invertibles, con control
orientacional y triplanar.

`RF_Triplanar` contiene 42 nodos y 67 enlaces, tres texturas de imagen, normal,
rotaciones vectoriales y mezcla por eje. `RF_Master` es un shader de 144 nodos y
200 enlaces con AO, Bevel, tres Bump, tres Normal Map, cinco imágenes, variación
de color y coberturas superior/inferior. Parte sustancial del acabado de referencia
está en shading; no debe atribuirse al graph geométrico.

Patrones admitidos para reimplementación propia:

- source modes explícitos y mutuamente verificables;
- randomización separada de construcción de masa;
- agregados de instancias fusionados mediante volumen;
- displacement por bandas/frecuencias y no un solo ruido;
- proyección triplanar desacoplada de material;
- top/bottom cover como máscaras geométricas, no slots de material autoritativos.

### 3.2 Nature Generator: familia de generadores, fractura y calidad shader

`NG_Terrain_Generator` contiene 201 nodos y 363 enlaces. Usa cuatro
`Curve to Mesh`, dos `Grease Pencil to Curves`, dos `Shortest Edge Paths`, tres
`Geometry Proximity`, dos grids, cinco `Set Position`, tres `Store Named Attribute`,
trece `Map Range`, treinta y tres `Vector Math` y subgrupos de displacement,
scatter, steps y water. `NG_Terrain Desert Generator` agrega tres Wave y campos
de camino. La inferencia es un terrain compuesto por base, caminos/curvas,
proximidad/agua, terrazas, displacement y atributos almacenados.

`NG_Cliff_Generator` contiene 167 nodos y 261 enlaces: distribución en caras,
Dual Mesh, instancias, `Mesh to Volume`, `Volume to Mesh`, transferencia por
superficie cercana, blur, Voronoi, transformaciones y merge. La masa del cliff
proviene de instanciar/formar bloques y volver a fusionarlos, no sólo de desplazar
un plano.

`NG_Layered_Cliff_Formation_Generator` contiene 198 nodos y 270 enlaces. Usa
handle/tangent de curva, tres `Curve to Points`, dos repeat zones, cuatro
`Instance on Points`, extrusión, traducción/escala de instancias y dos ciclos de
volumen. La técnica probable es apilar estratos a lo largo de un marco de curva,
variar espesor/offset y fusionar el conjunto.

`NG_Fractured_Rock_Formation_Generator` contiene 71 nodos y 96 enlaces. Dos
Dual Mesh, Duplicate Elements, dos Extrude, edge angle/neighbors, Split Edges,
cuatro Triangulate y dos Subdivision Surface demuestran una fractura derivada de
topología/celdas, no sólo grietas pintadas.

`NG_Layered_Fracture_Boulder_Generator` contiene 74 nodos y 97 enlaces. Dos
distribuciones, dos Dual Mesh, instancias cúbicas y fusión por volumen forman un
boulder de fragmentos/celdas. `NG_Layered_Rock_Formation_Generator` duplica,
extruye, invierte caras, instancia capas y fusiona por volumen con cuatro Noise y
Voronoi.

`NG_Scatter_Effect` contiene 227 nodos y 327 enlaces, pero mezcla responsabilidades:
colección, distribución, once Named Attributes, once Sample Index, proximidad,
curvas, volumen, materiales y controles de viewport. Jerico debe dividirlo en
mask, sampling, placement, instancing y provenance.

`NG_Layer_Masks` y `NG_Layer_Effects` confirman nuevamente la contribución visual:
AO, variación cromática, PBR, nieve, Bump/Normal y mezclas. `Uv Box Project`
clasifica seis orientaciones, separa geometría y almacena atributos; `random scale`
opera por Mesh Island. Ambos son patrones reutilizables.

### 3.3 OCD: máscaras ortogonales, image displacement y UV por corners

Los grupos pequeños hacen visible una buena frontera de composición:

- Attribute Mask: Compare + Delete Geometry + Proximity + Map Range.
- Direction Mask: normal, producto vectorial/dot, Math y Switch.
- Gradient Mask: position, rotate, separate XYZ, tres Map Range y Noise.
- Noise Mask: posición/espacio de referencia, rotación, Noise y remapeos.
- Material Mask: selección de material, bounds, posición y proximidad.
- Mask Combine: dos Switch y cuatro Math.
- Displace: normal, Set Position, remapeos y switches.

`HERO - Image Displace 2.0` contiene 118 nodos y 167 enlaces: cuatro Image
Texture, tres Named Attribute, tres Normal, dos Position/Object Info, proximidad,
Sample Index, Store Named Attribute y transformaciones/alineación. El patrón útil
es un decal/displacement de imagen proyectado en un marco controlado, con máscara
y atributos retenidos.

`HERO - Scatter` contiene 66 nodos y 93 enlaces: distribución en caras, random
estable por IDs, alineación a normal, muestreo nearest/index, image mask,
proximidad y Store Named Attribute. `HERO - UV Transfer` contiene 25 nodos y 37
enlaces: corners de edge, face groups, cuatro `Field at Index`, dos
`Offset Corner in Face`, nearest/surface/index y Store Named Attribute. La técnica
correcta es transferencia UV consciente del dominio corner y de la topología;
nearest point por sí solo no conserva seams ni orientación.

### 3.4 Referencias que no justifican copiar dependencia

Edge Decal aporta materiales pero el informe no retiene un node group geométrico.
Smart Segmentation aporta conceptos de selección/segmentación Python, pero no un
graph binario. Ambos pueden ampliar fixtures y UX posterior; ninguno justifica
una dependencia runtime ni afirmar que sus algoritmos fueron recuperados.

## 4. Sustituciones obligatorias para backend headless

Los graphs privados usan elementos aceptables para una herramienta interactiva
pero no como autoridad offline. Cada reimplementación Jerico aplica estas
sustituciones:

| Patrón observado | Sustitución Jerico |
| --- | --- |
| `Is Viewport` decide calidad | Socket/enum `QualityProfile` explícito y parte del cache key. |
| Gizmos o “Ultimate Gizmo” | Transform/marco serializado; Qt lo editará en M10.5, el worker sólo consume datos. |
| `Object Info`, `Self Object` o scene lookup arbitrario | Input geometry resuelto desde GUID y snapshot inmutable. |
| Collection order como identidad | Lista canonizada por asset GUID y stable element ID. |
| Material slot como clasificación | `material_class`, `physical_class` y máscaras nombradas; los slots son representación de export. |
| Frame/animación como seed | `Hash(rootSeed, generatorId, layerId, sourceGuid, stableId, channel)`. |
| Viewer, UI operator o selección activa | Resultados/diagnósticos de datos; ninguna dependencia de contexto. |
| Instancias realizadas temprano | Mantener instancias hasta la etapa que necesite mesh real; retener lineage. |
| Nearest transfer sin métrica | Distancia, normal deviation, misses y ambiguity count obligatorios. |
| Blender shader como autoridad runtime | Preview/bake únicamente; el material Jerico y su cook siguen autoritativos. |

## 5. Biblioteca Jerico propia

Cada ID siguiente es propio, versionado y estable. Un grupo se genera por `bpy`
desde código Jerico, se guarda en la biblioteca local pinneada y se inspecciona
para producir `GeneratorSchema`. Los nombres privados sólo aparecen en esta
auditoría de provenance; nunca forman parte de la API.

### 5.1 Contrato común de todos los grupos

Inputs comunes:

- `Geometry`: snapshot resuelto; nunca el objeto vivo de la escena.
- `Selection`: field bool; fuera de selección la geometría y atributos quedan
  byte-semánticamente sin cambio.
- `RootSeed`, `LayerId`, `SourceGuidHash`, `GeneratorRevision`.
- `QualityProfile`: `PREVIEW`, `AUTHORING`, `COOK_HIGH`.
- `AllowedMask`, `ProtectedMask`, `KeepClearDistance`.

Outputs comunes:

- `Geometry`, `AffectedMask`, `RejectedMask`, `ProtectedViolationMask`.
- `source_id`, `layer_id`, `generator_id`, `stable_element_id`, `region_id`.
- `material_class`, `physical_class`, `surface_class`, `feature_class`.
- métricas: input/output counts, bounds, rejection counts y transfer loss.

Regla espacial innegociable:

```text
effective = Selection AND AllowedMask AND NOT ProtectedMask
clearance_ok = signed_distance_to_keep_clear >= KeepClearDistance
write_mask = effective AND clearance_ok
```

Cada `Set Position`, extrusión, scatter, fusión o remesh valida el resultado contra
keep-clear. No basta filtrar puntos de origen: una roca grande puede nacer fuera y
penetrar el espacio protegido después de escalarse. La etapa final recorta/rechaza
componentes que intersectan el volumen protegido y reporta GUID/región/distancia.

### 5.2 `jerico.form.multisource-placement/v1`

Propósito: convertir superficie, volumen, curva o puntos explícitos en placement
normalizado sin perder identidad.

Sockets específicos: `SourceMode`, `SourceGeometry`, `AssetList`, `Density`,
`CurveSpacing`, `VolumeVoxelSize`, `AlignAxis`, `RotationMin/Max`, `ScaleMin/Max`,
`PositionJitter`, `RealizePolicy`.

Receta propia:

1. Un Menu Switch elige exactamente una ruta: Distribute Points on Faces,
   Distribute Points in Volume, Resample Curve + Curve to Points, Mesh to Points o
   puntos de entrada.
2. Capture Attribute congela `placement_id` antes de cualquier filtrado.
3. Align Rotation to Vector usa normal/tangent del modo y un up-vector reparado.
4. Random Value recibe IDs hasheados por canal separado para rotación, escala y
   jitter; cambiar escala no cambia rotación.
5. Instance on Points usa assets ordenados por GUID y Pick Instance con índice
   derivado de `placement_id`.
6. Bounds transformados se prueban contra keep-clear antes de aceptar la instancia.
7. La salida conserva instancias, salvo `RealizePolicy=REQUIRED_BY_NEXT_STAGE`.

Pruebas: todos los source modes, orden de asset list permutado, selección parcial,
semilla por canal, curva abierta/cerrada, zero density, bounds tangentes y una
instancia cuyo centro pasa pero cuyo volumen invade keep-clear.

### 5.3 `jerico.form.volume-fusion/v1`

Propósito: fusionar agregados, estratos o bloques en masa continua.

Sockets: `Geometry`, `VoxelSize`, `ExteriorBand`, `InteriorBand`, `Threshold`,
`Adaptivity`, `PreserveMask`, `TransferMaxDistance`.

Receta: Realize Instances sólo aquí; Mesh to Volume; Volume to Mesh; Smooth by
Angle/Set Shade Smooth como presentación; Sample Nearest Surface para reponer IDs
y máscaras; rechazar transferencia ambigua o fuera del límite. `VoxelSize` deriva
de escala real y QualityProfile, se serializa y entra al cache key.

No es autoridad de campo OpenVDB: cuando la receta requiera operaciones SDF
booleanas, macro warp o cavidades BSP, el backend privado OpenVDB sigue siendo la
etapa de volumen. Este grupo GN sirve para fusión local de assets y prototipos
evaluables dentro de Blender.

### 5.4 `jerico.detail.multiscale-displacement/v1`

Propósito: macro/meso/microvariación sin destruir masa, clearance ni atributos.

Sockets: tres panels `Macro`, `Meso`, `Micro`, cada uno con `Enabled`, `Scale`,
`Detail`, `Roughness`, `Strength`, `Anisotropy`, `Rotation`, `Warp`, `Clamp`;
además `DirectionMode`, `CustomDirection`, `HeightBias`, `TopBottomBlend`.

Receta:

1. Position se transforma al marco estable de source/layer.
2. Macro usa Noise/Voronoi de baja frecuencia para silueta limitada.
3. Meso combina Wave orientado y Noise para estratos/sedimento.
4. Micro produce una máscara y opcional high mesh; no obliga densidad en low mesh.
5. Color Ramp/Map Range controlan perfil; Mix combina canales con clamps físicos.
6. La dirección es normal, radial, eje estratigráfico o vector custom explícito.
7. Set Position actúa sólo en `write_mask`; después se vuelve a validar clearance.
8. Se almacenan `displacement_macro`, `displacement_meso`, `micro_mask` y
   `displacement_clamped`.

Cada frecuencia usa un hash de canal independiente. Ningún detalle depende del
frame, orden de nodos o viewport.

### 5.5 `jerico.mapping.triplanar/v1`

Propósito: coordenadas y pesos triplanares reutilizables para displacement,
material masks y bake.

Sockets: `Position`, `Normal`, `Scale`, `RotationXYZ`, `OffsetXYZ`,
`BlendSharpness`, `NormalSpace`, `SeamBias`. Outputs: `CoordX/Y/Z`,
`WeightX/Y/Z`, `DominantAxis`, `SeamMask`.

Receta: normal absoluta elevada por `BlendSharpness`, normalización segura de pesos,
rotación por eje, coordenadas derivadas del marco de source, corrección de signo
para evitar espejado y seam mask donde dos pesos son cercanos. Las imágenes son
inputs del consumidor, no están embebidas en este grupo.

### 5.6 `jerico.rock.topology-fracture/v1`

Propósito: fractured boulders, piezas destructibles y superficies rotas con
regiones reales.

Sockets: `Geometry`, `CellCount`, `CellScale`, `CellJitter`, `CrackWidth`,
`ExtrudeDepth`, `EdgeAngleLimit`, `KeepShell`, `FragmentBudget`.

Receta:

1. Distribuir semillas estables dentro/sobre la masa según modo.
2. Construir celdas mediante Dual Mesh o bloques/celdas instanciadas según perfil.
3. Duplicate/Extrude/Split Edges separan caras y generan bandas de grieta.
4. Edge Neighbors/Edge Angle clasifican límites internos, silueta y caras planas.
5. Triangulate usa método fijo; Merge by Distance usa epsilon versionado.
6. Mesh Island + Accumulate Field asignan `fragment_id` canónico por semilla y
   centro cuantizado, no por orden accidental.
7. Store Named Attribute produce `fracture_face`, `interior_face`, `bond_candidate`,
   `anchor_candidate` y adjacency sidecar candidate.
8. Fragmentos que invaden keep-clear se recortan o rechazan antes de publicación.

La fractura offline no sustituye la destrucción runtime. Jerico cook convierte
fragment IDs, bonds, anchors y physical class a su representación autoritativa.

### 5.7 `jerico.cliff.layer-stack/v1`

Propósito: cliffs estratificados, sedimentary/strata y formaciones apiladas.

Sockets: `GuideCurve`, `BaseProfile`, `LayerCount`, `LayerSpacing`,
`ThicknessMin/Max`, `LateralOffset`, `Tilt`, `Erosion`, `Breakup`,
`FusionMode`, `CapPolicy`.

Receta:

1. Resample Curve fija estaciones por distancia, no por cantidad de control points.
2. Tangent + normal/up repair construyen un marco ortogonal continuo.
3. Repeat Zone crea capas con `stratum_id`; cada capa varía thickness, offset y
   tilt mediante canales de seed independientes.
4. Curve to Mesh o perfil extruido forma masa, no una sábana sin espesor.
5. Instancias pueden permanecer separadas (`BEDDED`) o pasar por volume-fusion
   (`WELDED`); ambas preservan `stratum_id` por nearest transfer validado.
6. Wave orientado + displacement meso erosiona bordes; macro displacement queda
   limitado para no convertir el cliff en membrana.
7. Outputs agregan `bedding_mask`, `exposed_edge`, `talus_candidate` y espesor
   mínimo medido.

Gate visual: falla cualquier formación que lea como lámina, tela colgante o plano
desplazado. Debe mostrar masa, espesor, apoyos, oclusión y silueta verificable desde
frente, perfil y tres cuartos.

### 5.8 `jerico.rock.layered-mass/v1`

Propósito: rocas sedimentarias sueltas y boulders con capas internas.

Receta: partir de base cerrada; Duplicate Elements/Extrude por bandas del eje
estratigráfico; alternar offsets y erosión; opcional Geometry to Instance para
capas; volume-fusion; multiscale displacement; clasificar top/bottom/cut faces.
Outputs: `stratum_id`, `bedding_mask`, `cut_face`, `weathering_mask`.

Este grupo comparte layer-stack y displacement, pero su dominio es una masa
cerrada; no necesita guide curve.

### 5.9 `jerico.rock.rounded-weathered/v1`

Propósito: cantos rodados, rocas erosionadas y variación apta para ríos, caminos,
props e isométrico.

Receta: base convexa cerrada; Subdivision Surface con nivel por quality; Scale
Elements por Mesh Island; displacement macro limitado; blur/smooth iterativo con
preservación de volumen; flatten/contact mask contra support plane; micro pitting
como mask/bake. Outputs: `contact_patch`, `weathering_mask`, `wetness_candidate`,
`up_cover`, `down_cover`.

### 5.10 `jerico.terrain.path-field/v1`

Propósito: terreno, caminos, canales, cauces, terraces y guía para cave/cliff.

Receta: grid o surface source; guide curves resampleadas; Geometry Proximity y
Shortest Edge Paths crean distancia/flujo; Map Range perfila corte/relleno;
Blur Attribute suaviza sólo el field; Wave orientado crea terrazas; Set Position
aplica altura; Store Named Attribute conserva `path_distance`, `cut_fill`,
`water_candidate`, `terrace_id`. El terreno final se recorta por BSP allowed volume
y no rellena portales, cámaras, nav corridors ni keep-clear.

### 5.11 Familia `jerico.mask.* /v1`

Grupos independientes y componibles:

| ID | Cálculo | Outputs |
| --- | --- | --- |
| `jerico.mask.attribute/v1` | Named Attribute + Compare + remapeo | mask, missing-attribute diagnostic. |
| `jerico.mask.direction/v1` | dot(normal, direction), signed/absolute y falloff | facing, backfacing, grazing. |
| `jerico.mask.gradient/v1` | Position en marco estable, eje/rotación y Map Range | gradient, band ID. |
| `jerico.mask.noise/v1` | Noise en coordenadas estables, scale/detail/warp | value, thresholded mask. |
| `jerico.mask.material-class/v1` | comparación de `material_class`, no slot visual | mask, unknown class count. |
| `jerico.mask.proximity/v1` | Geometry Proximity/Raycast contra GUID input | distance, inside/contact masks, misses. |
| `jerico.mask.combine/v1` | AND/OR/XOR/SUBTRACT/MIN/MAX/MULTIPLY con clamp | result y contribution por input. |

Todos exponen invert, clamp y falloff explícitos. No borran geometría internamente;
producen fields para que selección y Layer System decidan la escritura.

### 5.12 `jerico.scatter.surface/v1`

Propósito: vegetación, piedras, props, ítems, guardrails y marcadores creativos.

Receta: combinar density masks; Distribute Points on Faces con seed estable;
capturar point ID; samplear normal, máscara y attributes de source; alinear con
up-vector y slope limits; elegir asset por GUID/peso canónico; aplicar escala y
rotación con canales independientes; probar bounds completos contra keep-clear;
guardar `scatter_point_id`, `asset_id`, `source_region_id` y rejection reason.

Para autos/FPS/isométrico, los presets cambian densidad y budget, no el algoritmo
ni la autoridad. El resultado puede ser instances para authoring y candidatos de
spawn/cook; nunca spawnea gameplay desde Blender.

### 5.13 `jerico.uv.box-project/v1`

Propósito: UV robusto para cliffs, rocas, arquitectura procedural y meshes sin
unwrap estable.

Receta: normal en espacio declarado; elegir eje dominante con desempate fijo;
proyectar Position sobre los planos XY/XZ/YZ con corrección de signo; rotar/scale/
offset; almacenar en dominio face-corner; producir seam mask en cambios de eje.
No necesita separar/reunir geometría si fields y Store Named Attribute bastan; si
la versión pinneada lo exige, la ruta separada debe preservar stable IDs.

### 5.14 `jerico.uv.topology-transfer/v1`

Propósito: transferir UV high/source a low/evaluated preservando corners y seams.

Receta: detectar edge/face groups y corner adjacency; Sample Nearest Surface da
face y barycentric candidate; Field at Index/Offset Corner in Face recupera los
tres corners; interpolar dentro de la cara sin cruzar seam; Store Named Attribute
en CORNER. Outputs: UV, `uv_transfer_valid`, distance, normal deviation, source
face ID, seam ambiguity y miss count.

### 5.15 `jerico.material.geology-preview/v1`

Propósito: preview y bake reproducibles, nunca material runtime autoritativo.

Consume máscaras geométricas: slope/up/down, AO candidate, curvature/edge,
stratum, fracture interior, wetness/snow/dust candidates y triplanar coordinates.
Produce Principled BSDF y AOVs de diagnóstico. La variación de color y bump usa
los mismos seeds/coordinates versionados que el graph. Texturas externas se
resuelven sólo desde manifest local con hash y licencia admitida.

La equivalencia visual se juzga separando:

- Beauty low mesh + material runtime aproximado;
- high mesh/bake normal, AO y masks;
- wireframe/silueta sin shader;
- máscaras AOV individuales.

## 6. Styles de roca construidos por composición

| Estilo | Cadena Jerico |
| --- | --- |
| Sedimentaria/strata | layered-mass o cliff layer-stack -> multiscale displacement con Wave orientado -> bedding/top/down masks -> box UV/triplanar -> bake. |
| Fractured boulder | base cerrada -> topology-fracture -> opcional volume-fusion exterior conservando fragment IDs -> interior masks -> collision/adjacency candidates. |
| Columnar/basalto | puntos/celdas 2D estables -> polígonos/dual mesh -> extrusión por eje con altura/tilt por canal -> merge sólo en base -> column/edge/top IDs. |
| Rounded/weathered | base convexa -> rounded-weathered -> contact flatten -> macro displacement limitado -> pitting mask -> UV/bake. |
| Cave | BSP/spline skeleton -> backend OpenVDB con widths/floor/ceiling/roll/profile -> anisotropic noise/rebuild -> Blender detail/masks/UV/bake; nunca cadena de cápsulas. |
| Cliff | guide curve/base mass -> layer-stack o block placement -> volume-fusion -> path/erosion fields -> scatter/talus -> masks/UV/bake. |

`jerico.rock.columnar/v1` se implementa como especialización propia: un grid o
Poisson 2D produce semillas estables; Voronoi/Dual Mesh genera secciones; Extrude
Mesh crea columnas con altura, escala y tilt independientes; edge angle clasifica
juntas; la base puede fusionarse sin borrar `column_id`. El generador rechaza
columnas sin soporte, penetración de keep-clear y aspect ratios fuera del manifest.

## 7. Layer System y “selección como loop”

El Layer System no es una pila de modificadores opacos. Cada layer es una operación
ordenada e inmutable:

```text
Layer {
  layerId, generatorId, generatorRevision, enabled,
  selectionExpression, sourceRevision, rootSeed,
  qualityProfile, parameters, materialClass, physicalClass,
  allowedRegionIds[], protectedRegionIds[], keepClearPolicy,
  inputAttributeRequirements[], outputAttributeDeclarations[]
}
```

Evaluación:

1. Resolver el snapshot BSP/mesh/curve por GUID y revision.
2. Compilar `selectionExpression` a masks: source IDs, region/material/physical
   classes, face sets, named selections, proximity, height/slope y connected region.
3. Expandir selección sólo mediante operadores explícitos: edge loop, boundary
   loop, linked region, shortest path, ring, geodesic radius o curve interval.
4. Intersectar con allowed/protected/keep-clear.
5. Ejecutar el group sobre copia; escribir únicamente `AffectedMask`.
6. Medir diff geométrico, topológico, semántico y de bounds.
7. Pasar geometry + attributes a la siguiente layer sin renumerar identidades de
   áreas no afectadas.

La selección de loop se representa como datos (`seedElementIds`, modo, dirección,
stop conditions y topology hash), no como selección activa de Blender. Un cambio
de topología que invalida el hash produce `STALE_SELECTION`; nunca selecciona otra
zona silenciosamente. M10.5 sólo visualiza y edita esta expresión desde Qt.

## 8. Reutilización en Decal3D y spline destruction

### Decal3D geométrico

Cadena: projector transform por GUID -> proximity/raycast mask -> image/field
displacement -> clip por selection y keep-clear -> topology-transfer o box UV ->
store `decal_id`, `projector_id`, `blend_mask`, `material_class` -> bake/export.
Puede producir relieve real, corte o sólo mask. El material del addon privado no
es requerido; `jerico.mapping.triplanar`, `jerico.mask.*`, image displacement y UV
transfer cubren las técnicas generales.

### Spline destruction

Cadena: curve-frame -> profile-sweep o proximity field -> selection/keep-clear ->
topology-fracture -> connected regions -> interior/bond/anchor attributes ->
collision candidates. La spline define ruta y ancho/profundidad por estación; la
fractura produce piezas offline. Runtime destruction conserva autoridad sobre
bonds rotos, impulso, replicación, save/load y Jolt.

Los mismos componentes sirven para trincheras, carreteras rotas, túneles, cables,
barandas, pipes, huellas y edge decals sin crear un segundo framework.

## 9. Implementación limpia y auditabilidad

La reimplementación sigue un protocolo de clean-room práctico:

1. El informe de referencia enumera capacidad observable y evidencia `M0-M2`.
2. La especificación Jerico define sus propios sockets, nombres, rangos, unidades,
   IDs, errores y fixtures antes de abrir conexiones exactas privadas.
3. El builder `bpy` se implementa contra la especificación y documentación oficial
   de Blender, no mediante copiar scripts o datablocks.
4. Al montar de nuevo los BLEND privados, una auditoría separada `G1/V1` compara
   comportamientos y casos límite; no importa/append los grupos a producción.
5. El gate source-free busca hashes, nombres privados, dependencias, texturas,
   Python modules y datablocks en library, cache, GLB, sidecars y package.
6. Cada output declara `inspirationProvenance` sólo en evidencia privada; el asset
   cocinado contiene únicamente IDs y versiones Jerico.

No se publican capturas de nodos privados ni fragmentos de código sin permiso de
licencia. Sí se publican diagramas y graphs Jerico creados desde cero.

## 10. Orden de producción y gates

La prioridad de backend ya acordada no cambia por esta auditoría. Primero se cierra
el lifecycle nativo del worker y después se admite generación real:

| Orden | Entrega | Gate mínimo |
| --- | --- | --- |
| 1 | Lifecycle/cancel/admission/staging | `VerifyBlenderJobLifecycleAndCancellation`: árbol de procesos terminado, admission liberada, un terminal, cero promotion/cache. |
| 2 | Core contracts | version pin, schema, immutable inputs, seeds, selection, keep-clear y provenance con RED/GREEN. |
| 3 | Masks + mapping | outputs numéricos conocidos, domain correctness, UV seams y determinismo 10 procesos. |
| 4 | Placement + scatter | bounds completos, GUID ordering, no invasión y preserved instances. |
| 5 | Displacement + volume fusion | mass preservation, semantic transfer metrics y voxel quality/cache identity. |
| 6 | Rock/cliff/terrain generators | fixtures por estilo, topology/region IDs, no sheet geometry, collision candidates. |
| 7 | Layer composition | selección loop stale-safe, diff, cancellation, apply data, undo/redo/recovery integration. |
| 8 | UV/bake/LOD/collision/export | GLB + sidecars, normal high-to-low, masks, LOD lineage, watertight/convex policy. |
| 9 | Private comparative audit | originals montados, exact graph `G1`, output `V1`, sin import/redistribución. |
| 10 | Cook/runtime/Beauty | owners nativos validan material, physical, destruction, navigation, source-free Shipping y capturas originales. |

Gates específicos de cada node group:

- builder recreable desde factory startup sin UI ni archivo de usuario;
- interfaz introspectable por `identifier`, panels, description, type/subtype,
  default/min/max; enums/dependencias completados por manifest;
- no `IsViewport`, Viewer, active selection, scene frame, red ni asset online;
- mismo input/pin/seed produce mismo graph hash, mesh hash, attributes y diagnostics
  en diez procesos limpios;
- cambiar un control significativo cambia la métrica geométrica esperada;
- selección inversa y regiones no seleccionadas permanecen invariantes;
- cero penetración no declarada de protected/keep-clear;
- toda pérdida por remesh/UV/nearest transfer queda medida;
- budgets de vertices, faces, instances, fragments, voxels, tiempo y memoria se
  rechazan antes de promoción;
- failure/cancel no publica resultado parcial ni cache entry;
- GLB/sidecars no contienen dependencia, nombre, hash ni asset privado.

## 11. Decisión de dependencia

La decisión para estas cinco referencias es `STUDY`, no `ADAPT`:

- se aprovechan patrones de arquitectura y casos de uso;
- se crean node groups propios para la producción;
- los addons pueden seguir disponibles en una carpeta privada de referencia para
  auditoría del propietario, nunca como requisito del worker;
- si una utilidad gratuita posterior supera licencia, headless, versión,
  determinismo y security gates, podrá admitirse como adapter aislado; no reemplaza
  la autoridad Jerico ni aparece automáticamente en el front.

Resultado ejecutivo: existe evidencia suficiente para diseñar y construir la
biblioteca propia sin depender de los addons pagos. La evidencia actual no basta
para afirmar equivalencia de conexiones o calidad visual. Esas dos afirmaciones
quedan reservadas para los gates `G1` y `V1` cuando los BLEND originales estén
montados y el coordinador abra la ventana Blender/visual correspondiente.
