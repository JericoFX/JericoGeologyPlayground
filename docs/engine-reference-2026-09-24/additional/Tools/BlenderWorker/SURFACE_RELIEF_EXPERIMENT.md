# Relieve propio sobre soporte — experimento del 2026-09-10

Estado posterior: [mezcla regional y nuevas familias](FAMILY_MIX_STATUS.md), con
[galería local](FAMILY_GALLERY.html). Esta sección conserva la evidencia histórica
de la segunda iteración; sus hashes/cuotas no describen la ventana posterior.

## Estado histórico: segunda iteración, ventana de una hora

**PARTIAL: biblioteca experimental propia operativa, no admisión de Geology ni
de cook.** El usuario aprobó continuar después de ver el primer relieve. El owner
concedió 2026-09-10 12:24:28–13:24:28 UTC, inicialmente 12 procesos geométricos y
12 renders, y autorizó explícitamente dos pruebas geométricas y un render extra.
Cuota final consumida: **14/14 procesos geométricos y 13/13 renders**. CPU de 512²,
30 segundos por hijo, dos threads BELOW_NORMAL y 12 minutos de pared agregada.
No es un presupuesto de rendimiento de producto ni autorización futura. El cierre
de consumo se comunicó al owner a las 13:00:26 UTC; la última comprobación de
procesos a las 12:56:22 UTC encontró cero hijos Blender/BlenderWorker activos.
La continuación solicitada por el usuario después de las capturas es documental
y de investigación; no amplía esta cuota ni mantiene procesos ejecutándose.

Código vigente: `surface_relief_nodes.py`, fábrica
`build_surface_relief(mask_namespace='jrc_relief')`, grupo
`JRC_SurfaceRelief_v2`, property `jerico_surface_relief_version=2`. Recetas propias
en `surface_relief_presets.py`; prueba/evidencia en `tests/probe_surface_relief.py`.
La sección v1 al final se conserva como historia, **no describe el código vigente**.

### Qué cambió y qué no

- La variación de espesor se obtiene deformando la coordenada vertical con un
  campo de baja frecuencia. No es un simulador de deposición geológica.
- White Noise sobre ID del lecho y fase de semilla produce variación discreta de
  resistencia; ésta altera la amplitud y pendiente del perfil entre lechos.
- Un fBM espacial reduce el relieve en áreas débiles; juntas Voronoi alargadas
  cruzan las capas sin separar vértices ni crear componentes.
- `NoiseStretch` modifica el frame de muestreo del ruido por eje, no la malla ni
  la escala nominal de bedding. Su influencia sobre warp puede alterar localmente
  las capas, aunque `Spacing` sigue expresado en metros del frame del soporte.
- Se mantienen una entrada y una salida Geometry, un único Set Position, cero
  scatter/remesh/extrusión/booleans. El soporte ya debe estar teselado.
- Cuatro paneles nativos organizan 14 controles de datos/campos. No hay UI Qt nueva.
- La fixture usa falloff de selección de medio metro en su chart, incluido el
  límite interior de la selección derecha. Se corrigió su dependencia anterior
  del número de subdivisiones con RED/GREEN.

Los sonidos, animación, gameplay, navegación y demás consumidores nativos siguen
fuera de este experimento. BSP/keep-clear de triángulos completos, Layer System
completo, UV/bake/LOD, PWAS/EPLS, collision, export/cook/cache y Shipping mantienen
el estado **INTEGRATION REQUIRED** de la tabla histórica inferior. No se omiten
ni se declaran implementados por estas pruebas geométricas.

### Recetas incluidas

| Receta | Forma implementada | Límite de la afirmación |
|---|---|---|
| `weathered` | Masas celulares erosionadas, sin bedding dominante | Relieve de superficie, no boulder volumétrico |
| `stratified` | Lechos variables, erosión espacial y juntas | Acantilado/pared de soporte; no simulación estratigráfica |
| `fractured` | Juntas más profundas sobre masas grandes | No fractura dinámica ni fragmentos de destrucción |
| `ice_wall` | Ruido estirado verticalmente y juntas alargadas | Geometría para pared de hielo; no hielo físico volumétrico |

El soporte curvo de cueva es una fixture semicilíndrica abierta de radio 6 m;
demuestra deformación de una pared/techo existentes. No genera una cueva completa,
una cápsula, salas ni navegación. Sus normales apuntan hacia el interior y la
demostración usa Direction=1: consume parte del hueco **sólo en esta fixture sin
BSP**. En producción el dueño espacial debe elegir el lado sólido, margen y
permiso, y validar el resultado. No reutilizar esa dirección ciegamente para un
espacio jugable.

### Interfaz vigente para el futuro front

| Panel | Controles |
|---|---|
| Surface | Influence, Depth, Direction |
| Noise | Seed, MacroScale, DetailScale, Warp, NoiseStretch |
| Strata | StrataWeight, StrataAxis, Spacing, BedVariation |
| Weathering | Erosion, JointStrength |

Además de los sockets v1 descritos abajo, se incorporan:

| Socket | Default | Rango / significado |
|---|---:|---|
| BedVariation | 0.8 | 0–1, variación de espesor y resistencia entre lechos |
| Erosion | 0.35 | 0–1, pérdida espacial de relieve |
| JointStrength | 0.25 | 0–1, profundidad de las juntas |
| NoiseStretch | (1,1,1) | 0.05–8 por eje, multiplicador direccional de frecuencia |

`transformer.blend_probe.describe(tree)` exporta la interfaz real: panels, parent,
identifier, socketType, default, min/max y description. La prueba serializa ese
reporte como JSON, recupera Depth por su identifier, lo cambia a 0.1 y verifica
desplazamiento efectivo no vacío y menor o igual a 0.1 m. Esto comprueba un enlace
backend real, no un mock ni una búsqueda de texto en el código.

Ejemplo real de la interfaz, reducido a un socket:

```json
{
  "itemType": "SOCKET",
  "name": "Influence",
  "identifier": "Socket_2",
  "direction": "INPUT",
  "socketType": "NodeSocketFloat",
  "parent": "Surface",
  "default": 0,
  "minimum": 0,
  "maximum": 1,
  "description": "Point field: selection times boundary falloff. Zero preserves support."
}
```

`Socket_2` pertenece a esta interfaz concreta: el cliente debe leerlo, no fijarlo
en código. Tampoco debe identificar el contrato mediante el sufijo automático
`.001` de un datablock; el reporte de prueba puede llamarse `JRC_SurfaceRelief_v2.007`.
La identidad/cache futura debe incorporar fábrica/version y hashes de fuentes,
schema y receta. Este reporte **no** está registrado como GeneratorSchema de
producción; el registro actual requiere otra admisión y no se modificó.

Influence es un campo espacial: el front no debe sustituir una selección por un
slider global. El wrapper de la prueba conecta Named Attribute
`jrc_relief_influence` a Influence; la receta de la capa debe proporcionar ese
campo. La máscara debe calcularse en el chart/frame correcto y conservar los
IDs autoritativos; la fixture rectangular no implementa selección loop general.

### Máscaras y significado para materiales/physical material

Hay siete atributos FLOAT/POINT en 0–1, multiplicados por Influence:
`jrc_relief_macro`, `jrc_relief_strata`, `jrc_relief_cavity`,
`jrc_relief_height`, `jrc_relief_joint`, `jrc_relief_resistance` y
`jrc_relief_erosion`.

Son campos artísticos, no propiedades físicas ya admitidas. `joint` describe el
campo potencial de juntas antes de aplicar JointStrength; `cavity` es el campo de
cavidad fina, no la suma de toda erosión y juntas. `erosion` incluye Erosion y
resistencia, pero no Depth. `height` es la altura normalizada final afectada por
Influence, antes de Depth/Direction. El mapeo PWAS/EPLS debe conocer estas unidades
y operaciones, no inferir durabilidad o fractura activa sólo por el nombre.
La fábrica acepta un namespace por capa y lo valida antes de crear el graph:
`jrc_[a-z][a-z0-9_]{0,47}`. Por ejemplo, `jrc_layer_a` genera
`jrc_layer_a_macro` hasta `jrc_layer_a_erosion`. La prueba aplica dos modificadores
con namespaces distintos y conserva los siete atributos de la primera capa.
El caller debe asignar namespaces únicos/estables: repetir el mismo nombre sigue
sobrescribiendo sus atributos. Esta capacidad no cierra el Layer System completo,
sus reglas de mezcla, undo/apply ni la admisión del schema de producción.

### Pruebas y capturas de la segunda iteración

Raíz: `_local/surface-relief-20260910/`.

| Evidencia | Resultado y propósito |
|---|---|
| iteration2-red / iteration2-green-01 | 3 fallos esperados por controles ausentes; luego 7/7 |
| presets-red / presets-green | Presets ausentes; luego 9/9 incluyendo soporte curvo |
| metric-mask-red / metric-mask-green | Falloff dependiente de densidad reproducido; luego 11/11 |
| interface-red / interface-green | Paneles ausentes; luego 12/12 con binding por identifier |
| directional-red / directional-green | NoiseStretch ausente; luego 13/13 con preset de hielo |
| review-red / review-green | RED de namespace y guardas de captura; luego 16/16, siete máscaras y mezcla parcial comprobadas |
| repeat-contract-red / final-repeat-green | RED de comparador ausente; luego 17/17 y comparación independiente entre procesos PASS |
| final-cliff-clay / final-cliff-stone | Mismo preset de acantilado, material neutro y demostración de piedra |
| final-ice-cave-clay / final-ice-cave-blue | Misma geometría de hielo, material neutro y demostración azul |
| final-cave-clay | Cueva estratificada con el falloff métrico corregido |
| metric-weathered / metric-fractured | Variantes erosionada y fracturada sobre plano |
| final-guard-render | Render de acantilado con guardas de guardado/render y provenance ampliada; exit 0, PNG inspeccionado |

Cada render tiene 25,921 vértices / 25,600 quads, 512², 16 samples CPU. Los pares
clay/material tienen hashes de posiciones idénticos. El campo histórico
`geometryHash` del manifest de render es un hash **sólo de posiciones**, no de UV,
materiales, conectividad o un asset cocinado. No debe usarse como cache key runtime.
No hay bump ni texturas externas. Los materiales de demostración son opacos;
el de hielo no simula espesor, absorción o refracción volumétrica. Las superficies
planas a la derecha son zonas excluidas deliberadas.

La revisión detectó que un retorno `CANCELLED` de Blender podía pasar inadvertido.
El harness ahora exige `{'FINISHED'}` y un archivo existente/no vacío tanto para
guardar el BLEND como para renderizar. Hay negativos de cancelación, archivo
ausente y vacío; el gate de archivo no decodifica PNG, por lo que la inspección
visual real sigue siendo otra comprobación. `final-guard-render` ejercita estas
guardas y se inspeccionó a resolución original de 512².

En los manifests actuales se registra `assetLoadPolicy` como
`NATIVE_FACTORY_NO_EXTERNAL_ASSET_INPUTS` y se exige que la lista observada de
librerías externas esté vacía. El hash de fuente del render final incluye
posiciones, caras, influencia, UV, IDs de caras, índices de material y matriz.
También se registran valores efectivos de todos los inputs, semilla/dirección,
fixture, binding/falloff de selección y hashes de las siete dependencias declaradas.
No es una prueba general de sandbox ni una auditoría de todos los addons locales.

Se retiene `presets-wall/` como ejecución fallida: creó su PNG pero rechazó el
manifest porque StrataAxis era una tupla no JSON. Se corrigió a lista, se valida
antes del render y `presets-wall-green/` repite con salida correcta. No se borró ni
se reetiquetó el fallo como PASS. Persisten warnings de deprecación `use_nodes`
para Blender 6.0; no son errores del pin ejecutado 5.2.1.

La suite no prueba todas las semillas, entrada arbitraria, error Hausdorff LOD,
autointersección o keep-clear BSP. Cubre los contratos declarados de soporte,
selección, máscaras, variación, dirección, control y muestreo. Los fingerprints
del resultado son de posiciones con Seed17/Seed18, no un hash integral de asset.

`final-repeat-green/comparison.json` compara `review-green/result.json` con la
ejecución final y comprueba estado pasado, pin/build, fábrica, dependencias y dos
fingerprints. Resultado: `PASS_SAME_HOST_PINNED_POSITION_REPEAT`. La prueba negativa
rechaza cambios en posiciones, implementación, build, dependencias o estado.
Esto amplía la repetición en un solo proceso de v1, pero no demuestra equivalencia
entre hardware/builds ni determinismo integral de UV/materiales/GLB.

Fingerprints finales:

```text
Seed17 sha256:25a04a755044d8345713e3ac9f82d2b4d32fc9ccac63f8dd98ae13348dd0fe34
Seed18 sha256:5928d7daa7ae94a905c4c25391fa501c96e9b192976e2def12695c8ab07f1925
```

La suite también comprueba Influence=0.5 contra desplazamiento y siete máscaras,
erosión/juntas/dirección, preservación exacta de topología/UV/face IDs, puntos
excluidos, muestreo compartido de grids, variación por semilla, presets defensivos,
soporte curvo, falloff métrico, paneles/binding por identifier y namespaces.
Entradas NaN/Inf, ejes nulos y geometría malformada siguen requiriendo preflight
antes de admisión: no se presentan como protegidas por estas 17 pruebas.

### Cierre de dependencias y publicación

La fábrica requiere `own_node_groups.py`, `protocol.py` y bpy del pin; los presets
usan sólo stdlib. La prueba añade `transformer/__init__.py`, `transformer/library.py`,
`transformer/blend_probe.py` y `blender.lock.json`, además de la distribución local
de Blender. Sus invocation JSON fijan hashes de esas dependencias.
No incluir `_local/`, binarios, addons o referencias privadas en un commit.
Este experimento no está incluido en el commit del diagnóstico
`725aed6d7276dc453eb1cf40ccdd0c6119afb6f0`.

El owner global conserva Git e integración. La excepción de escritura de esta
carpeta no se devuelve implícitamente al finalizar una captura; requiere resultados,
aceptación y recibo explícito. Las integraciones pendientes no pueden presentarse
como backend completo.

## Archivo histórico v1 — contexto de la primera prueba

Estado: **PARTIAL / EXPERIMENTAL_NOT_COOK_ADMITTED**. Implementación y cuatro
contratos geométricos comprobados en Blender 5.2.1 LTS, build `9e2066aef7ef`.
La calidad artística de strata no está aceptada: las bandas son demasiado
regulares. No representa cierre de Geology, M8, backend ni M10.5.

## Alcance y procedencia

`surface_relief_nodes.py` crea `JRC_SurfaceRelief_v1` mediante nodos nativos.
Reutiliza únicamente el constructor propio `_Group` de `own_node_groups.py`
(que importa `protocol.digest`). No carga addons, graphs, Python, materiales,
texturas ni `.blend` de proveedores. No altera Engine, Editor ni Git.
El experimento usa la licencia SPDX GPL-3.0-or-later de la biblioteca de herramientas
existente; no cambia la licencia del Engine ni otorga derechos sobre proveedores.

Se implementó después de un RED real: las cuatro pruebas fallaron con la
aserción explícita de fábrica ausente. GREEN posterior: 4/4; repetición final: 4/4.
Las imágenes son renders reales Cycles CPU, no imágenes generativas ni fotografías.

## Algoritmo implementado

1. Posición métrica del soporte; Noise 4D fBM normalizado para masa y domain warp.
2. Voronoi Distance to Edge para masa celular, combinado con el ruido amplio.
3. Coordenada proyectada sobre un eje normalizado para bedding; perfil de rampa
   cuadrática saturada, mezclado mediante `StrataWeight`.
4. Segundo fBM para cavidades de escala intermedia.
5. Clamp de altura e influencia, almacenamiento de cuatro máscaras POINT.
6. Un único Set Position sobre la geometría de entrada, a lo largo de su normal.

No hay instanciación, scatter, remesh, boolean, extrusión, solidificación ni
subdivisión dentro del grupo. No aparecen nuevas islas o caras por este grupo.
La topología se conserva; eso **no prueba** ausencia de auto-intersecciones en
soportes curvos, ni validez de collision o navegación.

## Contrato de entrada

El llamador debe proporcionar un soporte previamente teselado en coordenadas
métricas coherentes y con transformaciones normalizadas, además de selección,
falloff de borde y orientación al lado sólido. La influencia predeterminada es
cero: conectar sólo Geometry no deforma nada. El grupo no deduce selección desde
materiales, una caja, nombres de objetos ni índices de triángulos.

| Socket | Default | Rango declarado / semántica |
|---|---:|---|
| Geometry | — | Malla de soporte existente |
| Influence | 0 | Campo POINT 0–1, selección × falloff × permiso espacial |
| Seed | 17 | Entero 0–65535; fase de ruido acotada |
| Depth | 0.7 | 0–10 metros de desplazamiento máximo local |
| Direction | 1 | -1–1, signo respecto de la normal del soporte |
| MacroScale | 0.65 | 0.01–10 ciclos por metro de coordenada |
| DetailScale | 3 | 0.01–40; el caller debe respetar el límite de muestreo |
| StrataWeight | 0.65 | 0–1; cero selecciona relieve erosionado |
| StrataAxis | (0,1,0) | Eje métrico no nulo; pared de prueba usa (0,0,1) |
| Spacing | 0.65 | 0.05–10 metros |
| Warp | 0.22 | 0–2 metros |

El código limita valores numéricos de campo dentro del graph. No es un validador
de entradas no finitas, geometría malformada, eje cero, dimensiones o presupuesto.
Esa admisión sigue siendo responsabilidad del llamador; esta fábrica no se
registró como generador público ejecutable sin dicho control.

## Conexión de controles y Layer System

La interfaz usa sockets con tipos, defaults, límites y descripciones de bpy.
El futuro adaptador Qt debe leer los identificadores de la instancia del grupo;
no hardcodear Socket_N. Este experimento no modifica GeneratorSchema ni implementa
Qt. `tests/probe_surface_relief.py:attach` demuestra la conexión efectiva mediante
un grupo envolvente nativo: Geometry entra al grupo, un Named Attribute FLOAT
`jrc_relief_influence` entra a Influence, y Geometry sale al modificador.

Cada capa Jerico debe calcular su influencia a partir de la región/selección
estable, bordes y exclusiones. Las bandas geológicas no son las capas de autoría.
No se implementó composición multicapa aquí. Las máscaras llevan nombres
reservados `jrc_relief_macro`, `jrc_relief_strata`, `jrc_relief_cavity` y
`jrc_relief_height`, FLOAT/POINT 0–1, multiplicadas por influencia. Encadenar
instancias sobrescribiría estos nombres: el adaptador de capas debe renombrarlos
o extraerlos por capa antes de componer. No se presenta ese encadenamiento como
resuelto.

UV de esquina y `source_face_id` FACE sobreviven en las pruebas porque no se
cambia topología. No se usa nearest-point para reconstruir lineage. Los valores
de máscaras son campos artísticos, no certificados de dureza o physical material.
PWAS/EPLS deben mapearlos mediante las reglas autoritativas del Engine.

## Evidencia reproducible conservada

Raíz local: `Tools/BlenderWorker/_local/surface-relief-20260910/`.

- `red/`: 4 fallos esperados, fábrica ausente; logs originales e invocation JSON.
- `green-01/`: 4/4, sin fallos ni errores.
- `green-final/`: repetición final 4/4, sin fallos ni errores.
- `render-support/`: plano fuente sin modificador.
- `render-weathered/`: plano erosionado, StrataWeight=0.
- `render-strata/`: plano estratificado, StrataWeight=0.85.
- `render-wall/`: pared estratificada, eje Z.

Cada render contiene `beauty.png`, `scene.blend`, `render.json`, invocation y logs.
Capturas 512×512, 16 samples Cycles CPU, dos threads. Cada soporte de render tiene
25,921 vértices / 25,600 quads antes y después; no son medidas de rendimiento de
Jerico. El área plana a la derecha es deliberadamente no seleccionada. La máscara
de la fixture tiene falloff en el perímetro exterior y un corte en la selección
derecha; no se debe confundir ese borde con una fractura geológica natural.

Los tests comprueban:

- 1,681 vértices / 1,600 quads antes y después, mismos índices de caras, UV e IDs;
- influencia cero inmóvil y zona activa no vacía; desplazamiento normal entre
  cero y 0.7 m en el plano de prueba;
- máscaras variables dentro de 0–1; repetición exacta en la misma ejecución y
  cambio geométrico no vacuo al cambiar Seed;
- coincidencia de posiciones compartidas entre grids 20×20 y 40×40 en plano y
  pared con influencia unitaria.

Esto último comprueba independencia del campo respecto de la densidad, **no**
igualdad de silueta/triángulos interpolados ni aceptación LOD. Tampoco demuestra
reproducibilidad entre hardware, builds o procesos separados.

El runner usa el pin de `blender.lock.json`, factory startup, offline mode,
disable-autoexec, entorno de usuario aislado y el supervisor existente con timeout
de 30 segundos y prioridad BELOW_NORMAL. Cada ejecución requiere una ventana del
owner. Cuatro renders completados; aparecen advertencias de deprecación de
Material/World `use_nodes` para Blender 6.0, no errores de evaluación en el pin 5.2.1.

Invocación histórica del checkout local, con deadline acordado y directorio de
salida nuevo. El ejecutable y los outputs bajo `_local/` no se publican en Git;
esas rutas absolutas sólo describen la máquina donde se conservó esta evidencia:

```powershell
python -B tests/probe_surface_relief.py --blender F:/JericoEngine/Tools/BlenderWorker/_local/runtime/blender-5.2.1-windows-x64/blender.exe --out F:/JericoEngine/Tools/BlenderWorker/_local/surface-relief-replay --deadline 2026-09-10T12:16:36+00:00
```

El deadline del ejemplo es el de la prueba histórica; el supervisor rechaza
reutilizarlo fuera de esa ventana. No constituye autorización para una ejecución.

## Revisión visual y siguiente corrección

El relieve erosionado conserva el soporte y produce facetas/cavidades, pero no se
afirma paridad con addons comerciales. En plano y pared, strata muestra surcos
demasiado periódicos. La siguiente iteración dentro del alcance aprobado debe
variar grosor y resistencia entre lechos, interrumpir bandas por erosión espacial
y aumentar la variación de escala, conservando las pruebas del soporte. No añadir
scatter ni corregir el aspecto a costa de geometría fuera de la selección.

## End-to-end audit: INTEGRATION REQUIRED

| Subsistema / owner | Input y conexión requerida | Momento / dependencia |
|---|---|---|
| BSP / autoría | IDs, máscara estable, lado sólido y keep-clear; validar triángulos completos después del desplazamiento | M8 backend, antes de admisión |
| Generadores / worker | Preflight de malla, valores finitos, presupuesto, schema, receta y hash de este módulo + helpers | M8 backend, previo registro |
| Layer System / editor | Máscaras por capa, nombres separados, selección loop/region, undo/cancel/apply | Contrato backend antes de UI M10.5 |
| Materiales / PWAS / EPLS | Campos artísticos a clasificación/material físico mediante contratos existentes | Integración autoritativa, no inventar IDs aquí |
| UV / bake / LOD | Unwrap cuando corresponda, high→low bake, error de silueta y reproyección, no regenerar otra semilla | M8 pipeline antes de cook |
| Decal 3D / spline destruction | Reutilizar influencia y campos en un frame común; resolver nuevos bordes y lineage | Adaptador backend, front M10.5 |
| Collision / navegación | Validación espacial y collision cocinada independiente de Beauty | M8 cook, owner nativo |
| Export / cache / Shipping | Exportar máscaras + provenance explícitamente; cache con hash; Shipping sin bpy ni fuentes | Cooker y package gates existentes |

Animación, audio, scripting de gameplay y simulación no se implementan en este
experimento; sólo podrán consumir assets/materiales cocinados mediante el Engine.
No se transfirió ownership como trabajo final aceptado: falta calidad artística y
admisión de las integraciones anteriores. El owner global y Git siguen en M9.

## Referencia de API

La construcción de sockets sigue [NodeTreeInterface](https://docs.blender.org/api/current/bpy.types.NodeTreeInterface.html),
consultada mediante Context7 y comprobada ejecutando el pin local. Las URLs directas
5.2 no pudieron recuperarse en esta vuelta; no se asumió equivalencia completa con
`current`. Las técnicas geológicas se diseñaron independientemente a partir del
estudio conceptual registrado en `PRIVATE_REFERENCE_TECHNIQUE_AUDIT.md`.
