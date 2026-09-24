# Nuevos addons: integración privada del backend

Actualizado 2026-09-09 Uruguay / 2026-09-10 UTC.
Estado global: **PARTIAL**, no admisión nativa ni cierre M10.5.
Sólo `Tools/BlenderWorker/**`; Engine, Editor y fuentes originales sin cambios.

**Revisión visual posterior del usuario: REJECTED_BY_USER — «rockyfy se ve
horrible».** Los siete PASS de abajo son exclusivamente pruebas de ejecución y
exportación. No sirven como aceptación visual ni cierre de integración. El
descriptor actual de operación expone este rechazo; `components-v2.json` y
los resultados anteriores son evidencia histórica inmutable previa al rechazo.
En esa primera revisión aún no se había confirmado la causa raíz. La corrección
posterior de dependencia del control mesh está documentada inmediatamente abajo;
su prueba técnica no cambia por sí sola el rechazo artístico del usuario.

## Implementado y comprobado

### Publicación selectiva del diagnóstico — 2026-09-10

El owner de Git recibe únicamente `private_preview_policy.py`, `protocol.py`,
`private_suite_contract.py`, sus dos suites de contrato y estos informes propios.
Los cinco archivos de código/tests se verificaron copiados a un directorio aislado
con `python -E -S -B -m unittest discover -s tests -p 'test_private_*.py' -v`:
**20/20**, sin importar el checkout, site ni PYTHONPATH. El owner repitió la prueba.
La anterior prueba de cancelación se conserva en
`tests/test_private_runner_cancel.py`: **1/1**, con directorios temporales propios;
queda pendiente de publicación junto con el runner y su cierre de dependencias.
Los **21/21** de la sección histórica siguiente corresponden a la organización
anterior y no se presentan como 21 pruebas incluidas en este paquete reducido.

Este paquete permite reproducir los contratos de diagnóstico, no ejecutar los
proveedores ni el worker completo. `adapter_identity` requiere archivos del runner
privado todavía no publicados; `protocol.read_input` requiere `file_gate`, también
fuera de esta publicación. Ninguno se invoca en estas 20 pruebas. Se conservan
los avisos de licencia; no se publican addons, dumps, BLEND, ZIP, texturas ni
evidencia privada. El experimento posterior de relieve propio tiene otro alcance.

Hashes SHA-256 actuales de los cinco archivos aislados y publicados:

- `private_preview_policy.py`: `44042f3b28fb0b0238ce93238c46077ecc3143426ba425f281107ae1abf80ba9`.
- `protocol.py`: `b4df0a73d3e039f0e22114fe1981b07075e4befec07b29775177dcae6b813040`.
- `private_suite_contract.py`: `dd18ec5021c1f33fb2193020dbf4fbf0771217d4c54c36a19275b8ddbd496446`.
- `tests/test_private_preview_policy.py`: `a973f4551ef093bcb412fd0471ba2cb0b47f6b140ffceb923cd8b5bc79f2de0b`.
- `tests/test_private_suite_contract.py`: `3f60f2bfb6f495f77512c5eb19a1fa84b64013b283b815fdc5b9f8725165059b`.

La revisión READ_ONLY confirma el cierre de imports del alcance probado y ausencia
de cuerpos de código/medios de proveedores. El validador recibe la lista de conteos
construida por la ruta propia; no se anuncia como API para iteradores o contenedores
arbitrarios. Queda pendiente endurecer estructuras de resultados/bindings
malformadas y distinguir estrictamente bool de int en el readback de parámetros
antes de admitir esas interfaces como frontera general de entrada no confiable.
Un error Python de forma tampoco se considera una ejecución satisfactoria.

### Diagnóstico de preview corregido — 2026-09-10, 11:50 UTC

Paquete CPU ligero ejecutado por la tarea vigente de esta carpeta, con ventana
del owner Actor/Controller `01a08996-1ae6-7401-8500-69ea0d2ac335`,
11:48:15–12:18:15 UTC. No hubo Blender, GPU, proveedores, modificación de nodos,
Engine, Editor, build ni Git. Se separó el diagnóstico de
`private_preview_policy.validate_preview_mesh` sin modificar los presupuestos
ni el requisito de que cada malla tenga un conteo entero positivo.

| Condición | Código actual | Datos retenidos |
|---|---|---|
| Lista sin mallas o alguna malla de cero vértices | `private.empty_mesh` | Conteos, total y límite |
| Conteo bool, float, negativo u otro valor no entero | `private.vertex_counts_invalid` | Representación textual de los conteos y límite; no se calcula un total inválido |
| Conteos válidos y positivos cuya suma supera el límite | `private.budget` | Conteos, total y límite |

Se conserva el máximo agregado de **50.000** en preview y **500.000** en
reference; la igualdad exacta se acepta. Una mezcla `[8, 0]` sigue rechazada.
Si hay ceros y exceso simultáneamente, prevalece el diagnóstico de malla vacía;
los tipos/conteos inválidos se rechazan antes del total. Los datos numéricos
aparecen tanto en `JobError.details` como en su texto cuando son válidos: el
traceback que retiene la ruta privada ya permite entender el rechazo sin una
API transversal nueva. Los conteos inválidos usan `repr` en details para no
introducir, por ejemplo, un NaN numérico no serializable como diagnóstico.

Ejemplo del texto conservado:

```text
private.empty_mesh: Preview contains an empty mesh: vertexCounts=[8, 0]; totalVertices=8; maxVertices=50000
private.budget: Aggregate preview mesh exceeds its quality budget: vertexCounts=[30000, 20001]; totalVertices=50001; maxVertices=50000
```

Los dos consumidores siguen usando el mismo validador: `capture` antes de
crear/publicar el preview de la malla y `validate_result` al comprobar el
resultado con el plan. No se cambiaron esos consumidores, `validate_fixture_input`,
`plan_for`, umbrales ni reglas de admisión. Una lista de productos ausente/vacía
en el sobre de `validate_result` sigue siendo `private.result`, por precedencia
de la validación del sobre. No confundirla con un producto presente de cero
vértices.

Pruebas reales, desde `F:/JericoEngine/Tools/BlenderWorker`:

```text
python -B -m unittest discover -s tests -p test_private_preview_policy.py -v
python -B -m unittest discover -s tests -p test_private_suite_contract.py -v
```

- RED con producción anterior: 8 tests/3 fallos y 13 tests/1 fallo. Fallaron
  por `private.budget` indistinto y ausencia de datos, no por imports o entorno.
- GREEN después del cambio mínimo: **8/8 y 13/13 PASS**. Incluye agregados
  exactos y excedidos de ambos perfiles, mezcla de mallas válidas/vacías,
  tipos inválidos y propagación real por `validate_result`.
- El test de cancelación existente lanza un Python auxiliar con timeout 5 s;
  verificó que no crea resultado ni lanza Blender. No se ejecutaron proveedores.
- Las cuatro invocaciones reportaron unos 1,20 s acumulados de tiempo de
  terminal y 0,313 s acumulados dentro de unittest. No son una medición de CPU,
  benchmark del backend ni evidencia de rendimiento geométrico.

Logs privados inmutables de esta tanda:
`_local/preview-diagnostics-20260910/red-test_private_preview_policy.log`,
`red-test_private_suite_contract.log`, `green-test_private_preview_policy.log`
y `green-test_private_suite_contract.log`, todos en ese mismo directorio.
No se regeneraron logs, renders o hashes históricos de addons.

Hashes SHA-256 históricos de la tanda de 21 pruebas, antes de separar cancelación
(no son la lista de archivos de la publicación reducida actual):

- `private_preview_policy.py`: `44042f3b28fb0b0238ce93238c46077ecc3143426ba425f281107ae1abf80ba9`.
- `tests/test_private_preview_policy.py`: `a973f4551ef093bcb412fd0471ba2cb0b47f6b140ffceb923cd8b5bc79f2de0b`.
- `tests/test_private_suite_contract.py`: `ef726bbe6a3ef3cd40ff6625058ca11955e4e9ab8392171bfbb4c388088fb0f6`.

`adapter_identity` ya incluye el archivo de política; el fingerprint cambia
naturalmente. No se fijó un hash artificial ni se aceptarán comparaciones de
replay que mezclen identidades anteriores y nuevas. Los resultados históricos
del plano conservan sus mensajes anteriores como evidencia de aquella versión.

Alcance de aceptación: **PASS del diagnóstico CPU local**, no de generación,
Beauty, LOD, máscaras, collision ni backend completo. La propagación nativa/UI
de estados sigue INTEGRATION REQUIRED para su owner M10.5. No hubo nueva prueba
de `capture` dentro de Blender; su reutilización del validador se inspeccionó
estáticamente y la propagación por `validate_result` sí se ejecutó.

### Diagnóstico posterior: plano grande y coste de geometría sobrante

La revisión posterior de conexiones y ruidos está en
[auditoría de técnicas](PRIVATE_REFERENCE_TECHNIQUE_AUDIT.md), apartado
«Extracción posterior: conexiones, ramas y ruido verificados». Se leyeron doce
bibliotecas sin evaluar geometría. Hallazgos que afinan este diagnóstico:
Rockify usa scatter y escala ligada a bounds; el scatter de Nature fracturado
está desconectado; RockForm selecciona construcción volumétrica en su preset;
Cracker no contiene nodos de volumen en su grupo activo/dependencia directa.
No se implementó una corrección ni se obtuvo nueva aceptación visual.

Solicitud del usuario: comprobar un plano/cuadrado grande como fuente y que el
resultado modifique ese soporte, evitando crear multitud de piezas que después
deban gestionarse por culling. Diagnóstico ejecutado el 2026-09-10 UTC con
`tests/probe_private_plane.py`; no se modificaron productores, validadores de
producción, Engine, Editor/UI, originales ni Git.

Fixture: plano abierto de **10 × 10 unidades locales**, XY de −5 a +5, Z=0,
normal +Z; variante de un quad (4 vértices/1 cara) y rejilla 8×8
(81 vértices/64 caras). Seed 3 en las rutas que admiten seed; RockForm mantiene
su preset original sin binding de semilla. Perfil preview existente: 512²,
50.000 vértices, dos threads, BELOW_NORMAL, hijo ≤30 segundos,
Blender 5.2.1 fijado, offline/factory/no-autoexec. No se aumentó ese presupuesto
para convertir un rechazo en PASS.

Ventanas del owner: 03:49:10–03:54:10 y 03:54:32–03:57:32 UTC.
Última liberación anticipada: **03:56:40 UTC, sin procesos Blender**. Luego se
pausaron Blender y tests CPU para JIT/Vulkan. No usar los tiempos de esta tanda
como benchmark; hubo carga externa de HuntGame durante parte de la coordinación.

Evidencia privada bajo `_local/plane-20260910/`:

- `preview/`: diez ejecuciones iniciales, tres capturas de casos completados;
  sus comparaciones XY iniciales estaban en el espacio local del producto.
- `source-space-counts/`: diez ejecuciones con conteos de salidas rechazadas y
  mediciones corregidas al **espacio local de la fuente original** mediante
  `inverse(sourceMatrix) * outputMatrix`. Esta tanda sustituye los números XY
  preliminares; las matrices originales no se alteran.
- `rockify-fragmentation/`: dos ejecuciones que observan la topología evaluada
  después de su rechazo. No omiten el rechazo ni renderizan/exportan saltando
  el presupuesto. Los hashes del script diagnóstico constan en las invocaciones.
- `failure-counts/` y `failure-counts-b/`: intentos rechazados por margen de
  ventana menor de 45 segundos **antes de lanzar un hijo**. No son pruebas
  geométricas ni errores de addon.

Resultados con los presets y controles actuales, no una conclusión universal
sobre todas las configuraciones posibles del proveedor:

| Ruta / fuente | Vértices / polígonos | Observación |
|---|---:|---|
| Nature / quad | 1.191 / 1.206 | Lámina fragmentada: 3 componentes, 72 aristas con más de dos caras, sin borde abierto; espesor Z 0,147 unidades; 526 vértices bajo Z=0; ninguno fuera del soporte XY |
| Nature / rejilla | 79.062 / 79.838 | Excede el máximo preview de 50.000 vértices; no hay captura aprobada |
| Rockify Granite / quad | 58.578 / 58.152 | Excede preview; 399 componentes topológicos, 372 aristas con más de dos caras; espesor Z 6,796; 29.517 vértices bajo Z=0, 12 fuera de XY |
| Rockify Granite / rejilla | 57.004 / 56.568 | Excede preview; 390 componentes topológicos, 344 aristas con más de dos caras; espesor Z 6,924; 28.752 vértices bajo Z=0, 7 fuera de XY |
| RockForm / quad y rejilla | 17.572 / 17.570 en ambos | Un volumen cerrado, no simple relieve: espesor Z 6,688; 8.682 vértices bajo Z=0 y 10.279 fuera del soporte XY de la guía |
| OCD / ambas | 0 / 0 | Salida evaluada vacía; no exceso de presupuesto |
| Cracker / ambas | 0 / 0 | Salida evaluada vacía; no exceso de presupuesto |

En aquella tanda, el mensaje de producción `private.budget` englobaba tanto salida
vacía como exceso de vértices. Por eso el diagnóstico añade
`rejectedOutputCounts` sin cambiar entonces el validador. La corrección CPU
posterior está documentada arriba. No atribuir automáticamente a
un addon un exceso de polígonos sólo por ese texto.

`surfaceComponents` cuenta conectividad topológica por índices/aristas; no es
un conteo de objetos, draw calls, meshlets, chunks de culling ni piezas Jolt.
Una sola malla puede contener cientos de componentes. La existencia de piezas
que se tocan/intersectan espacialmente tampoco las convierte en conectadas
topológicamente. No se midieron frustum/occlusion culling, GPU ni física nativa.

**Conclusión: ninguna de estas configuraciones queda aceptada todavía como
generador de relieve rocoso controlado sobre el plano.** La prueba previa de
dependencia de la fuente/extrusión no demostraba este requisito: una ruta puede
reaccionar al tamaño de la fuente y aun así producir volumen o dispersión.
Rockify/Granite, en concreto, produce centenares de componentes en esta prueba;
su GLB de rocas no debe confundirse con una superficie continua de pared/suelo.

Criterio requerido para la siguiente adaptación de superficie: conservar la
selección de soporte y zonas protegidas; controlar desplazamiento normal,
espesor y transición lateral; evitar volumen interior y piezas sobrantes no
solicitadas; tratar fracturas/separaciones intencionales como decisiones de
autoría; verificar continuidad/topología y presupuesto antes del cook. No basta
con que el resultado se vea rocoso ni con ocultar geometría adicional mediante
culling. La granularidad real de culling/draws/collision corresponde a los
contratos nativos de empaquetado, no al número de componentes de esta sonda.

INTEGRATION REQUIRED: distinguir y admitir por separado las operaciones de
**relieve de superficie**, **masa rocosa** y **dispersión de rocas** en el backend
y descriptor futuro. No se implementó esa separación ni se retocaron los nodos
en este diagnóstico. También quedan por medir cobertura de la superficie,
fidelidad de borde/silueta, UV/masks/LOD/collision y keep-clear sobre el producto
final. Source-free Shipping permanece fuera de esta ejecución privada.

### Corrección posterior: la malla de autoría debe alimentar al generador

El usuario verificó que extruir el control mesh en los `.blend` originales
modifica la formación. Se reprodujo el defecto de nuestra ruta con Blender real:
`tests/verify_rockify_control_mesh.py` llamó al productor con una malla base de
ocho vértices/seis caras y con su extrusión de doce vértices/diez caras. Ambas
devolvieron exactamente `163eb68d7e9a559fcb929b959ae8844663a09f546816e22ff3523e71bcd4ddf8`;
el test falló con `test.control_mesh_ignored`. Evidencia:
`_local/rockify-control-20260909/red/`.

Causa verificada: se instalaba el grupo de estilo individual (`Granite`, etc.)
sobre un cubo generado por nuestra prueba. Los objetos `Rock_0` a `Rock_10`
originales incluso tienen mallas base vacías: esos grupos crean piezas por sí
mismos. El original `Cube` usa **RockGenerator**, cuya entrada pasa por
Scale Elements → Distribute Points on Faces → Instance on Points, alimentada
por Collection Info (`hs_rocks`) → Realize Instances. Esa etapa que faltaba es
la que recibe la forma y las extrusiones de autoría.

Corrección en `private_library_route.py`:

- carga el objeto `Cube` original y su dependencia RockGenerator/colección;
- conserva su modifier stack y las semillas internas de las piezas;
- sin `--control-mesh` conserva la malla original; con ese argumento reemplaza
  sólo la malla en la copia del job, manteniendo el generador;
- asigna la semilla del scatter a su socket real y registra lectura de retorno;
- valida tamaño/valores finitos/índices del JSON antes de lanzar Blender;
- conserva la instantánea JSON en el job, su hash y el ID fuente en evidencia;
- los drivers constantes revisados se fijan como valores, sin ejecutar Python;
  dos drivers originales (`var`, `var + 1.0`) tienen target ausente
  `["hs_noise_factor"]`: se conserva su valor de socket guardado y se documenta,
  sin inventar una propiedad de Scene. Otros drivers desconocidos siguen bloqueados.

La repetición del test pasa en `_local/rockify-control-20260909/green/`:
base `2c2292cd93bd3a212e1c74ce5435dc7488ddd87b3d1fd7bfe90acefd16635f62`;
extruido `e63ae4edbb8e7326930cb7f6ae84ecf252981212f3bdd60ee8419c28e277cae8`.
El CLI independiente con el mismo JSON extruido reproduce este último hash.
Esto prueba dependencia de fuente y repetición de ese caso, no aceptación
artística ni todos los parámetros del addon. Las capturas actuales ajustan
encuadre automáticamente; no son una comparación píxel a píxel de cámara fija.

Entrada mínima completa de la interfaz privada:

```json
{"schemaVersion":1,"sourceId":"rock-cage","vertices":[[-1,-1,-1],[1,-1,-1],[1,1,-1],[-1,1,-1],[-1,-1,1],[1,-1,1],[1,1,1],[-1,1,1]],"faces":[[0,3,2,1],[0,1,5,4],[1,2,6,5],[2,3,7,6],[3,0,4,7],[4,5,6,7]]}
```

M10.5 debe enviar la instantánea posterior a la edición/extrusión, no cambiar
solamente un hash o una caja envolvente. Este transporte privado no admite aún
GUIDs por cara, keep-clear/BSP ni atributos PWAS/EPLS. Esas integraciones nativas
siguen **INTEGRATION REQUIRED**, a cargo del owner nativo en M10.5, con input
selección/malla/semántica/transform y el contrato congelado cuando corresponda.

`--quality preview` sigue siendo 512² y 50.000 vértices. `--quality reference`
es explícito, 1024² y 500.000 vértices, coherente con el perfil privado existente;
no cambia los nodos ni reduce detalle para obtener PASS. Ambos mantienen dos
threads, 30 segundos por hijo y la ventana del owner. En la extrusión probada:

| Preset completo | Resultado reference | Vértices / polígonos |
|---|---|---:|
| Granite | GLB + dos capturas | 22.288 / 22.120 |
| Limestone | GLB + dos capturas | 301.236 / 304.904 |
| Stalactite | GLB + dos capturas | 157.086 / 159.448 |
| Stalagmite | GLB + dos capturas, UVMap presente | 142.068 / 144.320 |
| Volcanic | GLB + dos capturas | 150.274 / 88.652 |
| Igneous | REJECTED_BUDGET | Supera 500.000 vértices |
| Sedimentary | REJECTED_BUDGET | Supera 500.000 vértices |

Evidencia: `_local/rockify-control-20260909/reference-*/`. Los siete PASS de la
fase anterior, descritos más abajo, corresponden al grupo aislado retirado de
la ruta predeterminada; **no** demuestran siete formaciones completas corregidas.
49 pruebas privadas de código pasan después de esta corrección.

### Extensión solicitada: auditar todos los generadores por dependencia de fuente

Preparado `tests/verify_private_control_mesh.py` para Nature, RockForm, OCD y
Cracker, reutilizando sus rutas de datos existentes. Carga la fuente original,
captura base, extruye una cara en una copia del mesh conservando su topología
y vuelve a evaluar. Si el objeto devuelto es sólo un controlador sin caras, falla
con `test.input_binding`: hay que localizar la dependencia real, no extruir el
objeto equivocado. Si no cambia la salida, falla `test.control_mesh_ignored`.
No se ejecuta Python de esos proveedores en este test.

Esta tanda se ejecutó después, en una ventana de cinco minutos concedida por el
owner y cerrada antes de 00:59:40 UTC. Se reutilizó el runner de 30 segundos por
hijo, 2 threads, BELOW_NORMAL, Blender fijado, offline/factory/noautoexec y datos
originales READ_ONLY. `tests/replay_private_control_mesh.py` proporciona ahora
la invocación serial repetible con pin, deadline y logs. No registra proveedores.

Evidencia en `_local/source-extrusion-20260909/`: `first`, `rockform-guide` y
`repeat`. Resultado contra la edición de fuente, no aceptación artística:

| Generador | Vértices base → extruido | Dependencia de fuente | Repetición |
|---|---:|---|---|
| Nature | 1.571 → 8.077 | PASS | Hash fuente y geometría iguales en ambas tandas |
| RockForm | 29.686 → 36.320 | PASS | Hash fuente y geometría iguales en ambas tandas |
| OCD | 2.277 → 1.828 | PASS | SOURCE_FIXTURE_MISMATCH en la etapa extruida |
| Cracker | 120 → 131 | PASS | Hash fuente y geometría iguales en ambas tandas |

RockForm no debe recibir la extrusión sobre `RockForm Settings Object`: ese
objeto no tiene caras. El test original falló explícitamente por ello; la
dependencia comprobada es `RF_SetShape` → `Input_3` → `RockForm Guide Object`.
El test corregido modifica una copia de esa guía y evalúa el Settings Object.
Nature conserva su objeto original `NG_Fractured_Rock_Formation` y su stack.

En aquella prueba de OCD la base era idéntica entre tandas, pero el hash de la propia fuente extruida
ya difiere. No es válido atribuir el mismatch al generador ni declarar
determinismo fallido del addon: debe estabilizarse/reproducirse exactamente la
instantánea de extrusión del fixture BMesh y repetirse con inputs idénticos.
Tampoco se ha determinado si la diferencia es sólo orden de índices o geometría.

Aquella prueba demuestra reevaluación de los grupos originales al modificar su fuente
en memoria; **no añade** entrada JSON/controles Qt al CLI de estos cuatro
adaptadores ni prueba todavía BSP/PWAS/EPLS. Conectar esa instantánea de autoría al
job nativo sigue INTEGRATION REQUIRED para M10.5. Las imágenes son previews clay
512² de encuadre automático; no son métricas de Beauty a cámara fija.

Comando de replay (deadline suministrado por el owner para cada ejecución):

```powershell
python -B tests/replay_private_control_mesh.py --blender _local/runtime/blender-5.2.1-windows-x64/blender.exe --source F:/JericoEngine/_local_reference/blender_addons --out _local/new-source-dependency-run --deadline $approvedDeadline
```

Al cerrar no quedaban procesos Blender propios. A petición del owner se pausaron
también pruebas CPU hasta su aviso para reservar las mediciones JIT/Vulkan.

### Actualización: entrada externa conectada y replay exacto de los cuatro GN

La ventana posterior del owner, 01:36:33–01:46:33 UTC del 2026-09-10, permitió
cerrar la carencia de entrada externa descrita arriba. Se liberó a las 01:46:21
UTC sin procesos Blender; Blender y pruebas CPU quedaron pausados para JIT/Vulkan.
La evidencia histórica anterior se conserva: esta sección sustituye su estado
pendiente de conexión JSON y de repetición del fixture OCD, no su registro.

`private_addon_suite.py` acepta ahora `--control-mesh` para **Nature, RockForm,
OCD y Cracker**. `run_case(..., control_mesh=None)` conserva el comportamiento
anterior si se omite. `bind_control_mesh` valida el JSON con el contrato existente
de `private_library_route.py` y asigna una malla propia a la fuente de la copia
del job antes de evaluar, conservando los modificadores del generador. Para
RockForm resuelve `RF_SetShape` → `Input_3` → `RockForm Guide Object`; no sustituye
la malla vacía del Settings Object. Las familias no admitidas se rechazan antes
de ejecutar el proveedor cuando reciben esta entrada.

El archivo está limitado a 2 MB, 4.096 vértices, 8.192 caras, coordenadas finitas
y caras de 3–64 índices válidos y no repetidos. La instantánea se conserva en
`control-mesh.json`. `authoredSource` registra ID, hash, objeto fuente y conteos;
el request fingerprint incluye `controlMeshHash` sólo cuando hay entrada.
La identidad del adaptador incluye ahora `private_library_route.py`, del que
depende la validación. No se modifican identidades ni resultados históricos.

Es una entrada **geometry-only en coordenadas locales del objeto fuente**:
transporta posiciones e índices, no transform externo, UV, índices de material,
IDs por cara, atributos ni máscaras de entrada. Sustituir la malla no conserva
automáticamente esos datos de la malla original. BSP/keep-clear, selección/loops,
layer system, PWAS/EPLS, integración cook/runtime y controles Qt siguen fuera de
esta prueba y requieren el contrato nativo correspondiente. No es una escena
arbitraria ni un conversor universal de addons.

Evidencia nueva: `_local/source-binding-20260910/`.

| Gate | Resultado y evidencia |
|---|---|
| RED de API ausente | `red/ocd/stderr.log`: `test.input_api`, hijo 73 |
| Fuente base frente a extrusión | `first/`: 4/4 PASS, salida cambia con la fuente |
| Replay del JSON exacto | `repeat/`: hashes de fuente y geometría iguales para ambos estados en 4/4 |
| Replay con asserts incorporados | `verified-repeat/summary.json`: 4/4 PASS; comprueba ambos hashes contra `first/` |
| CLI real con `--control-mesh` | `cli/summary.json`: 4/4 `PASS_AUTHORED_CLI`; hash de salida igual a la extrusión de `first/` |
| Integridad de originales | Verificación del catálogo privado antes y después de cada una de las cuatro invocaciones CLI, sin cambios |
| Regresión de código | 50 pruebas `test_private*.py` PASS; `git diff --check -- Tools/BlenderWorker` PASS |

El replay consume `--replay-fixtures <directorio-first>`: no reconstruye la
extrusión con BMesh. Cada caso compara `sourceHash` y `geometryHash` por estado;
si difieren falla con `test.fixture_changed` o `test.repeat_changed`. OCD pasa
cuando recibe el mismo snapshot. Esto no establece por qué cambiaba el fixture
BMesh anterior ni demuestra determinismo universal entre máquinas/builds.

Los CLI usaron seed 3, perfil preview, catálogo
`_local/intake-all-20260908/catalogue.json` y el JSON extruido de cada caso.
Cada `cli/<familia>/` conserva invocación, logs, captura y resultado. Se mantuvo
Blender 5.2.1 fijado, offline/factory/noautoexec, dos threads, BELOW_NORMAL y
30 segundos por hijo mediante el runner existente; sin Python/Text del proveedor.
No se ejecutó otra vez la suite completa de siete operaciones en esta ventana.

Los cuatro están conectados por esta ruta privada de fuente, **no admitidos en
Jerico nativo**. Las imágenes son pruebas clay de baja resolución y encuadre
automático: no cierran calidad artística, materiales finales, HIGH→LOD, bake,
collision ni aceptación de todos los addons. No se tocaron Engine/Editor/UI,
originales de addons ni Git.

### Autoría regional privada: compilador de selecciones por identidad

`private_regions.py` incorpora `compile_recipe(recipe)` y
`select_run(recipe, layer_id, region_id, generator)`. No cambia control-mesh/v1,
el contrato nativo del worker ni `private_suite_contract.CASES`. El ejemplo
completo es `examples/private-regions.json`: una fuente con dos partes,
región A Nature y región B Rockify/Sedimentary con detalle preview explícito.

La receta privada contiene `controlMesh`, `revision`, `faceIds`, `rootSeed`,
`regions` y `layers`, además de `schemaVersion: 1`. `faceIds` corresponde uno a
uno a los polígonos de entrada; cada región tiene GUID y lista de esos IDs.
Las capas tienen GUID, `generatorId`, `enabled`, `selection.regionIds` y
`parameters`. Son regiones de fuente, no índices de polígonos generados.

El compilador:

- valida IDs únicos, fuente finita acotada y referencias completas; rechaza
  regiones superpuestas, parámetros desconocidos e identidades ausentes;
- ordena las caras por identidad, compacta los vértices usados y emite un
  control-mesh/v1 por región, con las coordenadas locales sin recentrar;
- deriva seed de rootSeed + sourceId + regionId + layerId + generatorId;
  la reduce al rango 0–10000 admitido por los adaptadores, sin prometer que
  todas las combinaciones tengan semillas distintas;
- conserva en `provenance` sourceId, sourceRevision, hash privado completo de
  fuente, IDs de caras, índices de vértices fuente y hash del control mesh;
- distingue `generationKey`, dependiente de los datos de esa región, de
  `recipeHash`, que identifica la solicitud completa y su revisión;
- devuelve `preservedFaceIds` para las caras sin generación habilitada;
  no elimina ni modifica la fuente original;
- acepta Nature/RockForm/OCD/Cracker y los presets Rockify revisados. RockForm
  declara `original-preset-not-bound`: no se inventa un binding de seed;
- limita a 16 regiones y 16 capas, con **un generador habilitado por región**.
  Dos capas activas sobre la misma región fallan `private.region_composition`.
  Guardar/deshabilitar capas no equivale a haber implementado su composición.

`runs` conserva el orden de autoría de `layers`; dentro de una capa las regiones
se ordenan por GUID. Reordenar capas cambia el orden del plan y `recipeHash`,
pero no la `generationKey` individual de regiones con los mismos inputs. No se
promete serialización idéntica del plan completo tras editar su orden.

Es una extracción geométrica de parches, posiblemente abiertos; no tapa bordes,
no suelda, no hace booleanas y no transfiere máscaras a través de nodos vendor.
Las salidas se declaran `separate-products-no-weld-no-mask-transfer`.
Los adaptadores conservan matrices y dependencias de los objetos originales;
las coordenadas de entrada se interpretan en el espacio local de su fuente.
No se declara todavía alineación espacial común entre productos de proveedores
distintos. El ensamblado requiere conversión de espacios explícita y validación
de las matrices de fuente/producto, además de la prueba de unión geométrica.
`outputFaceMapping: not-established` evita presentar la lista de caras fuente
como correspondencia exacta de caras generadas. Cambiar topología obliga al
autor de la fuente a emitir IDs nuevos y pertenencia regional explícita; el
compilador no infiere automáticamente la herencia de extrusiones ni el interior
de loops. Los IDs nuevos deben permanecer estables en replays posteriores.

El hash privado SHA-256 aquí **no es** el `uint64_t sourceContentHash` nativo.
Se revisaron READ_ONLY `GeologyMask.hpp:121–131` (documentGuid, sourceGuid,
sourceRevision, sourceContentHash, falloffMeters) y `HardSurfaceStack.hpp:25–38`
(AssignFaceMaterial por regiones GUID y sourceSolidHash). No hay conversión
implícita de hashes ni correspondencia nativa inventada. Los índices de Blender
no sustituyen esas identidades.

Validación del compilador: RED por módulo inexistente y RED por selección CLI
ausente; después **57 pruebas privadas PASS**, incluidas siete nuevas pruebas
regionales, a 01:58:30 UTC del 2026-09-10. Se verificaron extracción exacta,
ausencia de mutación, aislamiento de claves/semillas, reordenamiento de caras,
capas deshabilitadas y rechazos de solapamientos/controles no soportados.
La ventana posterior 02:07:04–02:12:04 UTC permitió conectar y ejecutar los CLI.
Se liberó a las 02:11:14 UTC, sin procesos Blender. Ambos admiten
`--region-recipe`, `--layer-id` y `--region-id`. La entrada es mutuamente
exclusiva con `--control-mesh`; la receta gobierna seed/preset/detail, y pasar
esos overrides junto con ella se rechaza. `--quality` sigue siendo una opción
independiente de ejecución. Sin receta se conservan los defaults previos.

La ruta del catálogo anterior (`private_addon_suite.py`, Nature/RockForm/OCD/
Cracker) guarda la instantánea completa y el control mesh extraído. Registra
`regionalSource` y liga su hash al requestFingerprint; `adapter_identity`
incluye `private_regions.py`. Rockify (`private_library_route.py`) incorpora
controlMesh y regionalSource a su request y requestHash. Su padre escribe una
instantánea de receta privada y la pasa al hijo, no reenvía el archivo editable
original. La procedencia regional está en el JSON lateral; todavía no se
agregaron etiquetas regionales específicas a los extras del GLB.

`tests/replay_private_regions.py` ejecutó dos regiones y una extrusión sólo en B:

| Gate | Evidencia `_local/regions-20260910/` | Resultado |
|---|---|---|
| RED CLI Nature | `red/base-nature/stderr.log` | Argumentos regionales no reconocidos |
| RED CLI Rockify | Salida de terminal de esta ventana | Argumentos regionales no reconocidos, antes de lanzar Blender |
| Cuatro hijos del CLI | `green/summary.json` | `PASS_REGIONAL_EDIT_ISOLATION` |
| Padre→hijo Rockify | `parent-rockify/result.json` y `request.json` | PASS, misma geometría que B base |
| Regresión privada | `python -B -m unittest discover -s tests -p 'test_private*.py'` | 60 tests PASS al liberar la ventana |

La revisión posterior detectó y reprodujo una regresión de compatibilidad del
parser: `--preset ""` se convertía en Granite por un fallback basado en valor
falso. Se corrigió para aplicar el default sólo a `None`; el vacío explícito
vuelve a rechazarse antes de Blender. Dos ejecuciones CPU breves autorizadas
produjeron RED (9 tests, un fallo esperado) y **GREEN de 61 tests privados**
(1,019 segundos de tests, 1,547 segundos de proceso). No se volvió a lanzar
Blender en ese cierre; las capturas regionales conservan la identidad de código
registrada en su propia ventana, antes de la corrección posterior del parser.

Los asserts comprueban el hash del control mesh realmente vinculado, la
procedencia de selección y las salidas evaluadas. Nature A conserva exactamente
`sha256:9decf332bf17074cac8eb1926c3fe87fdad050b59d99d8201d43f69dd20269c6`
al extruir B. Sedimentary B cambia de
`sha256:48a9c78ed0c6a1b034992b46e1d2e80cbf0e755bb1824b076f08e1c32cafff80`
a `sha256:5a5ecd84e2d70107bb72a78cecb43d955356c9d999edc6943e67f058015e293b`.
Se verificó el paquete Nature antes/después de cada ejecución y la biblioteca
Rockify fijada antes/después de cada hijo. No se importó Python del proveedor.
RockForm/OCD/Cracker usan el mismo selector conectado, pero **no tuvieron una
nueva prueba regional real en esta ventana**; conservan su prueba previa de
entrada control-mesh/extrusión, que no sustituye esa cobertura nueva.

Las cuatro capturas están en `green/{base,extruded}-{nature,rockify}/captures/`.
Sedimentary usa `detail_limit: 0`, preview explícito de bajo detalle. Sus formas
angulosas no constituyen aceptación de roca realista ni igualdad con el anuncio.
El éxito es funcional y de aislamiento, no un PASS artístico ni un ensamblado.

Conexión futura del front, sin implementar Qt: escribir una instantánea con
IDs de fuente/caras/regiones y capa seleccionada; ejecutar fuera del hilo UI
una sola selección región/capa mediante el runner supervisado existente;
comprobar procedencia/revisión vigente al recibirla y conservar la fuente y las
caras no generadas. No interpretar `runs` como autorización de lanzar 16 Blender
simultáneos ni de fusionar outputs. El ejemplo de flags de Rockify es:

Los campos de región, cara y capa son autoría común de Jerico, no sockets
descubiertos automáticamente dentro de un addon. Estos flags regionales están
documentados en los CLI; su incorporación al descriptor común de selección del
front sigue INTEGRATION REQUIRED en M10.5. No se afirma que el catálogo actual
ya genere por sí solo los widgets de selección regional.

```text
--family rockify --region-recipe examples/private-regions.json
--layer-id 30000000-0000-0000-0000-000000000002
--region-id 20000000-0000-0000-0000-000000000002
```

Se añaden los argumentos existentes `--source`, `--out`, `--blender` y
`--deadline` de una ventana autorizada. Para Nature se usa el CLI hijo
`private_addon_suite.py` con `--catalogue`, `--id nature`, `--out` y los GUIDs
terminados en `0001`, siempre detrás de verificación de paquete y runner padre.
No es una nueva ruta de instalación ni reemplaza la admisión de cada proveedor.

Revisión independiente READ_ONLY del cambio: sin defectos críticos señalados;
cobertura real Nature/Rockify aceptada como gate funcional acotado, y pendiente
regional RockForm/OCD/Cracker conservado explícitamente. El orden de capas de
autoría no se normaliza silenciosamente. No se autorizaron escrituras ni pruebas
adicionales al revisor.

INTEGRATION REQUIRED, owner nativo M10.5: puente BSP/selección/loops/Undo y
persistencia de GUIDs; entrada de document/source revision/hash y relaciones
padre-hijo tras extrusión. INTEGRATION REQUIRED, backend antes de admisión:
composición de capas sobre una misma región, transición/soldadura, máscaras y
linaje tras remesh, validación keep-clear sobre el producto final. INTEGRATION
REQUIRED, owners cook/materials/PWAS/EPLS: transportar semántica validada a
material/physical material, bake, LOD, collision, navegación y destrucción.
Esta entrega no cambia el pipeline source-free Shipping ni habilita UI.

`generationKey` es una clave de entrada regional, no un caché de artefactos
implementado. Cualquier reutilización real debe incluir versión de adaptador,
hash de biblioteca, Blender fijado, calidad y contrato de coordenadas, y renovar
la procedencia de la solicitud actual aunque la geometría se pueda reutilizar.

### Rockify: control explícito para preview de los presets densos

Se agregó `--detail-limit` y el control tipado `detail_limit` al descriptor de
operación. Es opcional (0–5): limita niveles de nodos Subdivide Mesh/Subdivision
Surface no conectados en la copia del job y registra cada valor anterior y
aplicado en `detailBindings`. No cambia densidad/semilla ni escribe bibliotecas.
Omitirlo conserva los niveles originales, también en `reference`. No se ejecutan
expresiones de proveedor para resolver ese control; un Level conectado se rechaza.

Con la misma malla extruida y seed 3:

- Igneous, `--detail-limit 1`: 16.660 vértices, 16.320 polígonos;
  `_local/source-extrusion-20260909/preview-igneous/`.
- Sedimentary, `--detail-limit 0`: 19.074 vértices, 15.334 polígonos;
  `_local/source-extrusion-20260909/preview-sedimentary-detail0/`.

Ambos producen GLB y dos previews 512² bajo el límite original de 50.000
vértices. Sedimentary con límite 1 aún excede ese presupuesto; fallo retenido en
`preview-sedimentary`. Los productos de detalle reducido son previews explícitos,
no sustitutos HIGH ni LOD aceptados. La formación original completa a máxima
resolución de esos dos presets sigue excediendo el perfil reference de 500.000.

Validación de código previa al cierre: **50 pruebas privadas aprobadas**,
incluida validación del control y preservación del default original. Los siete
presets Rockify ya tienen al menos una ejecución de formación completa, cinco
con referencia original y los dos densos con detalle limitado; no se afirma que
los siete hayan pasado la regresión base/extrusión ni aceptación artística.

Para Edge Decal, limpieza, remesh, segmentación, UV y bake, editar la fuente
requiere volver a ejecutar la operación, no mutar su resultado terminado. La
prueba correspondiente debe alimentar la fuente antes de esa operación y
verificar geometría, máscaras, UV o bake según su función. Para rigs, la prueba
debe validar actualización de binding/pesos; una extrusión no debe calificarse
automáticamente como un nuevo generador de huesos. Stair usa su guía/curva.
Estos casos siguen **NOT_EXECUTED** en la auditoría nueva; los smoke tests
anteriores no los sustituyen. No se modificó `private_suite_contract.CASES`.

### Registro histórico de la primera integración

- `private_addon_catalogue.py --components`: descubre directorios, ZIPs y ZIPs
  anidados, separa componentes/versiones y deduplica por contenido. Nunca instala
  ni importa proveedores. El modo anterior del catálogo y los siete casos de
  `private_suite_contract.CASES` permanecen intactos.
- `private_component_intake.py`: identidad SHA-256 incluyendo assets; controles
  AST tipados, panel/owner, descripción, valor inicial, límites, enums literales
  y atributos no resueltos. Los callbacks dinámicos no se ejecutan. Cancelación,
  límites de lectura/descompresión y rechazo de enlaces, traversal, ADS,
  dispositivos Windows y colisiones de rutas.
- Selección de componentes: rechaza identificadores desconocidos, duplicados,
  dos versiones de una familia, Auto-Rig Pro junto con Rig Tools, glTF antiguo
  y Proxy Picker. Seleccionar no concede permiso de ejecución.
- `private_library_route.py`: carga un grupo original Rockify por preset, asigna
  semilla con la API real de sockets de Blender 5.2, evalúa, captura dos previews
  512x512 y exporta GLB con extras de procedencia/semilla mediante glTF incorporado.
  No importa Python del addon, no ejecuta instaladores ni binarios del proveedor.
- Sólo la operación Rockify se anuncia ejecutable en el catálogo nuevo, y sólo
  si los siete hashes de biblioteca coinciden. El paquete Python completo sigue
  con `executionEnabled: false`. Los campos tienen significados distintos:
  `operations[].executable` habilita la ruta revisada, no `register()` del proveedor.

Inventario real generado: `_local/security-intake-20260909/components-v2.json`.
Contiene 20 componentes identificados por bytes, no veinte productos distintos.
Incluye tres glTF legacy y dos variantes de contenido para Smart Remesh y Smart
Segmentation. Rockify tiene una ruta ejecutable; no se inventan rutas ejecutables
para los componentes nuevos restantes. Los siete casos privados anteriores
mantienen su entrada de ejecución anterior, independiente de este catálogo.

## Estado por familia nueva

| Familia | Resultado actual | Límite pendiente |
|---|---|---|
| Rockify | 7/7 presets: malla, previews, GLB | Calidad comercial, entrada BSP, UV/bake/LOD/collision no admitidos |
| Stair Builder v2.02 | Biblioteca identificada e intento real retenido | El grupo incorpora Text datablocks; fallo cerrado antes de evaluar |
| Auto-Rig Pro | Componente separado, 1.034 controles candidatos | Adaptador de rig/bind/retarget con dependencias revisadas y prueba real |
| Rig Tools | Componente separado, 216 controles candidatos | Ruta IK/FK; incompatible con registrar Auto-Rig Pro simultáneamente |
| Proxy Picker | Visible, diez controles candidatos | Ejecución de texto de assets excluida |
| glTF legacy | Tres variantes visibles y bloqueadas | Usar exclusivamente exportador del Blender fijado |
| Instant Clean | 65 controles candidatos | Adaptador de limpieza sin updater/reinicio/presets globales |
| Zen UV | 1.596 controles candidatos | Adaptador UV sin red, exec arbitrario ni ZenRelax no admitido |
| SimpleBake | 378 controles candidatos | Adaptador de bake sin upload/updater ni procesos propios |

Estas cantidades son declaraciones estáticas, no botones operativos. Un addon
no se vuelve compatible por ser visible. No existe aún instalación automática
segura de todos los paquetes, ni conversión universal de sus operadores.

## Entrada física y conexión futura del front

Entrada privada actual, sin escrituras del worker:
`F:/JericoEngine/_local_reference/blender_addons`.
Se pueden colocar nuevos paquetes allí y regenerar el catálogo con una salida
nueva. Cambiar bytes invalida la identidad; no se reemplazan admisiones por nombre.
El modo de descubrimiento no necesita Blender y no debe correr en el hilo UI.
La actualización de Git de esta carpeta corresponde al owner del checkout.

Desde `F:/JericoEngine/Tools/BlenderWorker`:

```powershell
python -B private_addon_catalogue.py F:/JericoEngine/_local_reference/blender_addons --components --out _local/new-intake/components.json
```

La salida debe ser nueva; no sobrescribir evidencia previa. Jerico M10.5 podrá
leer los registros y mostrar tanto disponibles como bloqueados con su motivo.
`controlSchema` contiene candidatos no escribibles; la lista de parámetros de
una operación ejecutable es la única lista admitida para esa operación.

Ejemplo abreviado de operación real Rockify:

```json
{
  "id": "geometry.rock",
  "entrypoint": "private_library_route.py",
  "family": "rockify",
  "executable": true,
  "status": "PRIVATE_PREVIEW_EXPORT",
  "parameters": [
    {"id": "preset", "widget": "enum", "default": "Granite", "options": ["Granite", "Igneous", "Limestone", "Sedimentary", "Stalactite", "Stalagmite", "Volcanic"]},
    {"id": "seed", "widget": "integer", "default": 0, "minimum": 0, "maximum": 10000}
  ],
  "nativeAdmission": "NOT_ADMITTED",
  "publishable": false
}
```

Jerico conserva autoridad de jobs, selección, layer system y BSP. Esta ruta
acepta ahora la instantánea acotada `--control-mesh` descrita arriba, no una
selección semántica arbitraria ni una escena completa del usuario. No modificar
el contrato nativo a partir de este prototipo.
El front futuro debe construir argv como lista, nunca evaluar Python/callbacks
ni concatenar parámetros en shell. No se ha implementado ningún control Qt.

Argumentos de ejecución: `--family rockify --preset Sedimentary --seed 3`,
`--source` raíz original, `--out` directorio nuevo bajo `_local`, `--blender`
ejecutable fijado, `--deadline` fin ISO8601 con zona de la ventana vigente del
owner; `--cancel-file` opcional bajo `_local`. Una fecha escrita no concede una
ventana de ejecución: la coordinación con el owner sigue siendo obligatoria.

El padre verifica hash del ejecutable y biblioteca antes de lanzar; el hijo
comprueba versión/build y modo. Entorno de usuario/temporales por job,
background/factory-startup/offline/disable-autoexec, dos threads, prioridad
BELOW_NORMAL, techo de 30 segundos y 8 MB de logs con el runner existente.
Cancelación antes/durante/al completar es observada por el padre; el hijo se
termina y recoge sin tocar la fuente. Los fallos conservan logs/salidas parciales
y no son caché válida. No hay nuevo scheduler ni reintento automático.

`--offline-mode` y los audit hooks no son una sandbox OS completa. La ruta no
admite scripts arbitrarios, pero tampoco promete confinamiento general de todo
Python. No habilitar globalmente los addons sobre esa suposición.

## Pruebas reales y fidelidad

Runtime: Blender 5.2.1 LTS, build `9e2066aef7ef`, ejecutable fijado en
`blender.lock.json`. Ventana autorizada 00:14:13–00:29:13 UTC, liberada antes
de tiempo a las 00:25 UTC. Comprobación Win32_Process: ningún blender.exe vivo.

Pruebas en `_local/security-intake-20260909/`:

| Preset | Directorio | Vértices / polígonos |
|---|---|---:|
| Granite | `rockify-granite-preview-03` | 130 / 128 |
| Igneous | `rockify-igneous-preview` | 6.146 / 6.144 |
| Limestone | `rockify-limestone-preview` | 1.410 / 1.416 |
| Sedimentary | `rockify-sedimentary-preview` | 13.642 / 13.632 |
| Stalactite | `rockify-stalactite-preview` | 818 / 824 |
| Stalagmite | `rockify-stalagmite-preview` | 820 / 824 |
| Volcanic | `rockify-volcanic-preview` | 799 / 472 |

Cada directorio aprobado contiene `result.json`, `request.json`,
`runtime-interfaces.json`, `front.png`, `three-quarter.png`, `asset.glb` y logs.
Son polígonos antes de la triangulación del exportador, no triángulos de LOD.

Sedimentary, seed 3, original y repetición:
`sha256:29f8c5aa4f9b182caba167fad35930ea393c5d6b1ae1bcf07f59d976290fa987`.
Seed 4: `sha256:bc6cd07f2dbd833a8557f2bc1a321508ee7a859c38030232b94be7d01b87df0b`.
Esto demuestra repetición en esta máquina/build y variación de semilla, no
determinismo universal de drivers/GPU ni igualdad byte a byte del GLB.

Se carga el grupo de estilo individual, no el scatter `RockGenerator`. Se
eliminan datos de animación de los node trees sólo en memoria de la copia del
job, registrándolo en `headlessAdaptation`; las bibliotecas originales no se
guardan. Esa adaptación impide prometer equivalencia con toda la escena del
addon. Granite se ve como una pieza cúbica de baja resolución; Sedimentary
muestra estratos. **No se ha aceptado calidad igual o mejor que la publicidad.**
No hay UV en estas mallas. Tampoco son HIGH final, LOD, collision ni máscaras
validadas. La ruta de modelado PWAS/EPLS existente continúa siendo la autoridad
propuesta para completar esas etapas, no una capacidad probada por estos GLB.

Los intentos Granite anteriores fallaron durante arranque y quedan retenidos;
`stair-preview` falla por la dependencia Text. No se ejecutó ese texto. El código
actual registra nombres de textos inesperados en intentos futuros, todavía sin
repetir Stair después de esa mejora diagnóstica.

Pruebas de código ejecutadas después de los cambios:

```powershell
python -B -m unittest discover -s tests -p 'test_private*.py'
```

Resultado: **47 pruebas aprobadas**. Incluyen los contratos privados anteriores,
intake anidado, colisiones, cambios de identidad, límites/cancelación, esquemas
de controles, hashes admitidos de la ruta y cancelación previa sin Blender.
La cancelación en vuelo utiliza el runner existente; esta ventana no incluyó
una prueba nueva de cancelación de Rockify mientras renderizaba.

## Antivirus y publicación

Tras la confirmación explícita del usuario se ejecutó Defender sobre toda la
raíz de addons mediante `-Scan -ScanType 3 -File ... -DisableRemediation
-CpuThrottling`. Terminó exit 0: `found no threats`. No hubo remediación ni cambios
de preferencias. Se utilizó la configuración de protección existente; no se
afirma que haya sido una operación sin red o sin envío automático de muestras.

Ese resultado no sustituye revisión de dependencias, compatibilidad ni licencia.
Los paquetes y salidas derivados de bibliotecas pagas siguen privados y con
`publishable: false`. Poseer una compra no cambia automáticamente sus permisos
de redistribución. No se copió Python de proveedores al código propio ni se
publicaron paquetes. La procedencia y condiciones particulares siguen sujetas
a la revisión descrita en el informe de seguridad.

No se hizo commit/push desde esta tarea; publicación y coordinación nativa son
del main. No se ha cerrado la integración de todos los addons: el siguiente
trabajo ejecutable requiere adaptar y probar las rutas pendientes por operación
en una nueva ventana, manteniendo los hallazgos sensibles excluidos.
