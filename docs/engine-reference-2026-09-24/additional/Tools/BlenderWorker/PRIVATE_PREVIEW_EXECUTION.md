# Preview ligero y ejecución privada — preparación M10.5

Fecha: 2026-09-09. Alcance: `Tools/BlenderWorker/**`, sin Engine/Editor/Git.
Estado: implementación privada; integración nativa **NOT_ADMITTED / UNFROZEN**.
Este documento amplía [la suite de siete addons](PRIVATE_ADDON_SUITE_M10_5.md).

## Contrato implementado

`private_preview_policy.plan_for(addon_id)` selecciona **preview** por defecto:
512×512 píxeles, dos threads Blender y máximo agregado de 50.000 vértices evaluados.
No es sólo un thumbnail de la malla alta: se aplican controles originales de coste
antes de solicitar la evaluación del depsgraph.

| Addon | Reducción del preview | Referencia explícita |
| --- | --- | --- |
| Nature | `NG_Fractured_Rock_Formation_Generator`: `Socket_4=1`, `Socket_12=0` | Preset original sin estas asignaciones |
| RockForm | `RF_SetMat.Input_4=0` y `RF_SetShape.Input_19=0.4` | Conserva Resolution Levels y Voxel Size originales |
| OCD | `OCD_GN_2.5.0.Socket_5=0.7` | Conserva Voxel Size original |
| Cracker | `Cracker_GN.Socket_15=1` | Mantiene detail 2 de la fixture |
| Edge Decal | Fixture original pequeña; no borrar UV ni overlay para abaratar | Misma geometría de prueba |
| Smart Segmentation | Fixture pequeña; mantener etiquetas por cara | Misma geometría de prueba |
| Smart Remesh | Fixture pequeña; no ejecutar remesher externo | Misma geometría de prueba |

El nombre del grupo es parte del binding: `RF_SetShape.Input_4` es Shape X Rot,
no resolución. No enlazar controles sólo porque coincida `Input_4`.

Las tres rutas por operadores/algoritmo (Edge Decal, Smart Segmentation y Smart
Remesh) validan antes de operar un input de hasta 1.024 vértices y 1.024 polígonos
y registran sus conteos. Sólo usan las fixtures internas; no aceptan una malla
arbitraria del usuario por esta CLI. Esta acotación no promete un límite OS de
memoria para cualquier implementación de terceros.

`--quality reference` conserva 1024×1024 y admite hasta 500.000 vértices agregados.
Es una opción **diagnóstica explícita**, no un cambio automático de calidad ni un
producto final del cooker. El front M10.5 debe empezar por preview y reservar el
trabajo final para una petición/admisión distinta.

La geometría reducida es aproximación de autoría, no LOD runtime aceptado. No
acredita fidelidad de silueta, collision, keep-clear ni high-to-low bake. Si supera
el presupuesto, se rechaza; no se convierte silenciosamente a calidad alta ni se
acepta una caja de reemplazo como resultado del addon.

## Procesos, CPU y main thread

La suite sigue reutilizando `transformer.library.run_process`. El proceso hijo
arranca oculto con `BELOW_NORMAL_PRIORITY_CLASS`; el test consulta la prioridad
Windows real desde el hijo. Conserva deadline de 30 segundos y logs de hasta 8 MB.
La suite es serial: no crea siete Blender simultáneos. OMP/OpenBLAS/MKL/NumExpr
reciben límite de dos threads y Blender recibe `--threads 2`.

**El harness CLI es bloqueante para su propio proceso. No debe invocarse en el
hilo UI de Qt.** No se agregó wrapper async público, servicio de hilos, scheduler,
broker, DDC ni coordinador alternativo. Los dos threads de cálculo no equivalen
a dos llamadas Python concurrentes a `bpy`.

La documentación oficial advierte que la integración Python de Blender no es
thread-safe; sus datos deben manipularse dentro del proceso Blender, sin intentar
paralelizar llamadas a bpy con threads Python. La separación útil es **Jerico main
→ job nativo asíncrono → proceso Blender**, no añadir threads alrededor de bpy.
[Blender: Python Threads are Not Supported](https://docs.blender.org/api/current/info_gotchas_threading.html).

Workbench puede usar GPU incluso en background. `--threads 2` limita el trabajo CPU
configurado, no garantiza GPU libre ni memoria máxima del proceso. Por eso las
pruebas visuales se coordinan con Astra; no se solapan con gates GPU de M9 sin una
ventana explícita. El límite de vértices es una validación de salida, **no** un
medidor/cap OS de memoria durante la evaluación de un addon.

## Cancelación y resultados incompletos

La CLI acepta `--cancel <archivo-privado>`; sin ese argumento usa
`<out>/control/cancel.requested`. El caller crea ese archivo para solicitar cancelar.
Sólo se admiten rutas bajo `_local`, sin escapes/symlinks/junctions.

La solicitud se comprueba:

1. Antes de verificar el ejecutable o crear el directorio de salida.
2. Durante el fingerprint de paquetes, entre archivos y cada bloque de 1 MiB.
3. Antes del siguiente addon, durante la espera del proceso y antes de aceptar
   cualquier resultado o iniciar la verificación posterior.
4. En checkpoints del worker: startup, carga de ruta, binding de calidad,
   evaluación y antes/después de cada captura.

Un GN o render que esté ejecutándose dentro de Blender puede no atender un
checkpoint Python a mitad de operación. El runner padre sigue observando el
archivo y mata/recoge **su hijo inmediato** al cancelar o vencer el plazo. El
containment OS de descendientes y el gate nativo completo siguen pendientes; no
se presentan como resueltos por esta mejora. Las rutas privadas revisadas bloquean
lanzamientos externos mediante audit hook, que tampoco sustituye un sandbox OS.

La suite conserva logs/artefactos parciales para diagnosticar. Un PNG existente no
es aceptación: `suite.json.status` queda `CANCELLED` o `FAILED` y nunca debe usarse
como resultado nuevo aplicable. La auditoría rechaza suites canceladas, parciales
o en curso. No se publica ningún asset, cache nativa ni resultado en el mundo.

`write_json` mantiene la escritura mediante archivo temporal y replace existente;
no se crea otro journal. La cancelación/transacción atómica con source revision,
admission release y terminal-once pertenece al gate nativo del owner, no a una
promesa inferida de los JSON de esta prueba.

## Identidad y observabilidad

Cada resultado incluye:

- `previewPlan`: versión, calidad, tamaño, presupuesto, threads y bindings.
- `appliedBindings`: valores pedidos y leídos de vuelta, incluidos los de calidad.
- `adapterHash`: identidad del código del adaptador y sus helpers de schema/captura.
- `requestFingerprint`: paquete, pin, adaptador, perfil y parámetros efectivos.
- `stageTimesMs`: tiempo entre checkpoints, útil para localizar coste de carga,
  evaluación y render. No son tiempos de frame de Jerico ni GPU timestamps.
- PID/UTC/argv, geometryHash, atributos/UV, hashes y dimensiones de las capturas.

La suite separa verificación inicial del paquete, duración del proceso Blender y
tiempo total de cada caso. La identidad de todos los archivos comprados se comprueba
antes y después; no se eliminó el hashing para aparentar mejor rendimiento.

Comparar preview y reference con geometría diferente no prueba determinismo. La
auditoría exige misma calidad, packageHash, build, adapterHash, requestFingerprint y
bindings antes de aceptar repetición. Dos directorios privados distintos son
obligatorios; un run nunca puede validarse contra sí mismo.

## Conexión nativa para M10.5 — INTEGRATION REQUIRED

| Owner/seam existente | Trabajo requerido antes de afirmar «no bloquea main» |
| --- | --- |
| `AuthoringWorkCoordinator` | Admitir memoria/coste, limitar concurrencia y coalescer cambios de parámetros; sólo la petición vigente puede producir preview actual |
| `SmartImportCookJob` / QProcess | Arranque y recepción por señales; no `waitForFinished`, hashing pesado ni evaluación dentro del hilo UI |
| `CancellationSource` / `CancellationToken` | Propagar cancelación/cierre del documento; containment/reap nativo y un solo evento terminal |
| `SmartImportCookCache` | Reutilizar cache existente con perfil en la key; nunca responder a final con un preview ni mezclar paquetes/versiones |
| `ProjectTransactionCoordinator` | Apply/Undo/Redo y recovery transaccionales; un preview nunca cambia el documento |
| Modelo de preview M10.5 | Mantener el último preview válido al editar; marcarlo como desactualizado mientras llega el nuevo; descartar resultados con generationTicket/revision obsoletos |

«Siempre tener preview» significa: pedir baja resolución primero y conservar el
último válido sin bloquear la edición. En el primer job fallido puede no existir
ningún preview; mostrar error/estado vacío, no inventar una imagen ni presentar
un resultado antiguo como nuevo. El almacenamiento/retención de ese último preview
en el editor todavía no está implementado por este paquete privado.

Antes del front siguen siendo obligatorios los contratos BSP/keep-clear, layers,
materiales y material físico, PWAS/EPLS, UV/bake/LOD, collision/destruction/navigation,
cook y source-free Shipping descritos en `PRIVATE_ADDON_SUITE_M10_5.md`.

## Evidencia CPU actual

`_local/private-preview-cpu-20260909/tests-final.log`: 164 tests ligeros PASS, incluidos
prioridad Windows real, cancelación, límites de logs al salir, coherencia del
snapshot y negativos de calidad/provenance. Se excluyen las suites de worker,
transport y own-node-groups que necesitan Blender. La ejecución visual nueva se
registra a continuación; no sustituye estos negativos de contrato.

La revisión también exige el conjunto exacto de targets aplicados (incluida la
secuencia detail inicial + detail preview de Cracker). No se aceptan controles
extra aunque su readback coincida. Para pruebas nuevas, el verificador recalcula
adapterHash/requestFingerprint contra el código actual: dos resultados viejos
iguales no bastan para validar una implementación modificada.

## Ejecución Blender real del 2026-09-09

Ventana autorizada 01:06–01:21 UTC, ampliada explícitamente a 21 probes seriales;
liberada a las **01:14:23 UTC**, con cero procesos Blender vivos. Se ejecutaron:

1. `_local/private-preview-20260909-baseline`: seis rutas PASS y RockForm FAILED.
   Su binding de subdivisión se aplicó, pero no bastó para respetar 50.000 vértices.
   Este fallo y sus artefactos permanecen retenidos, sin promoción ni sobreescritura.
2. `_local/private-preview-20260909-corrected`: siete rutas PASS tras añadir el
   control Voxel Size de RockForm. No se elevó el presupuesto.
3. `_local/private-preview-20260909-repeat`: siete rutas PASS con el mismo código,
   perfil, parámetros y paquetes que corrected.

La auditoría `tests/verify_private_suite_evidence.py` comparó **corrected contra
repeat**, no el baseline de otra versión. Resultado:
`_local/private-preview-20260909-repeat/evidence-audit.json`,
`sameInputRepeatAccepted:true`, siete geometryHash coincidentes y siete conjuntos
de bindings/readbacks coincidentes. Recalculó identidad del adaptador y petición,
verificó hashes y decodificó las dos capturas de cada ruta en ambos runs: 28 PNG
de 512×512. No es una prueba de determinismo entre máquinas o versiones.

Las dos suites válidas usan Blender 5.2.1 LTS build `9e2066aef7ef`, dos threads,
BelowNormal y Workbench con `render_aa='8'`; ambas vistas se completaron sin error.
Los paquetes originales coinciden con su fingerprint completo antes y después.

| Ruta | Vértices referencia histórica → preview | Polígonos preview | Proceso Blender repeat / total caso | Captura real |
| --- | ---: | ---: | ---: | --- |
| Nature | 17.987 → 1.501 | 1.462 | 1,616 s / 11,623 s | [512 px](_local/private-preview-20260909-repeat/nature/captures/three-quarter.png) |
| RockForm | 110.598 → 29.686 | 29.682 | 3,890 s / 4,469 s | [512 px](_local/private-preview-20260909-repeat/rockform/captures/three-quarter.png) |
| OCD | 2.818 → 2.355 | 1.527 | 1,421 s / 1,472 s | [512 px](_local/private-preview-20260909-repeat/ocd/captures/three-quarter.png) |
| Cracker | 284 → 120 | 118 | 1,375 s / 1,485 s | [512 px](_local/private-preview-20260909-repeat/cracker/captures/three-quarter.png) |
| Edge Decal | 80 → 80 | 30 | 1,684 s / 1,871 s | [512 px](_local/private-preview-20260909-repeat/edge-decal/captures/three-quarter.png) |
| Smart Segmentation | 507 → 507 | 500 | 1,714 s / 1,720 s | [512 px](_local/private-preview-20260909-repeat/smart-segmentation/captures/three-quarter.png) |
| Smart Remesh | 32 → 32 | 30 | 1,592 s / 1,632 s | [512 px](_local/private-preview-20260909-repeat/smart-remesh/captures/three-quarter.png) |

Los conteos históricos provienen de `_local/private-suite-20260908-final`, no de
un benchmark reference nuevo bajo idéntico adaptador. Por ello la tabla demuestra
reducción geométrica observada, **no un speedup temporal controlado**. En OCD bajan
los vértices pero aumentan los polígonos respecto de los 1.402 históricos: no afirmar
reducción universal de todas las métricas. Nature sigue pagando hashing completo
de su paquete de aproximadamente 2,39 GB; ese trabajo nunca debe ir en el hilo UI.

Inspección visual a resolución original: Nature conserva planos/fracturas con
detalle deliberadamente reducido; RockForm muestra la formación de boulders;
OCD una fixture cúbica suavizada; Cracker la división de la fixture; Edge Decal
el overlay de aristas; Smart Segmentation seis regiones coloreadas; Smart Remesh
su fixture reconstruida. Las capturas son clay/Workbench, no Beauty final ni
prueba de paridad artística con el material comercial. No acreditan todos los
presets ni las funciones completas de cada addon.

No se ejecutó el cancel probe Blender opcional para no exceder los 21 procesos.
La cancelación está probada con el hijo real ligero de los tests CPU; la prueba
Blender en vuelo y el containment/reap nativo siguen pendientes. Ninguna suite
privada se presenta como admisión de BSP/LOD/cook/Shipping o UI M10.5.

## Comando de prueba coordinada

Usar un destino nuevo y la fecha/hora real concedida por Astra:

```powershell
python -B Tools/BlenderWorker/tests/replay_private_suite.py `
  --blender F:/JericoEngine/Tools/BlenderWorker/_local/runtime/blender-5.2.1-windows-x64/blender.exe `
  --catalogue F:/JericoEngine/Tools/BlenderWorker/_local/intake-all-20260908/catalogue.json `
  --out F:/JericoEngine/Tools/BlenderWorker/_local/private-preview-run `
  --deadline $approvedDeadlineUtc `
  --quality preview
```

`$approvedDeadlineUtc` es la ventana concedida, no una fecha futura inventada.
No ejecutar este comando si la ventana está cerrada. La CLI exige los siete IDs
exactamente una vez; no disfraza un subconjunto como suite completa.

## Prueba posterior roca → Smart Remesh: diagnóstico rechazado

El 2026-09-09 se abrió otra ventana, con fin 03:52 UTC, y se liberó a las
03:49:17 UTC por orden del owner. Se lanzó **un único proceso Blender**, PID
86328, a las 03:44:05 UTC. La cueva se bloqueó antes del lanzamiento. No se
ejecutó Instant Meshes, no quedan procesos propios y no hay renovación de ventana.

`tests/probe_geology_smart_remesh.py` probó el core NumPy `OURS` comprado sobre
la malla alta original Nature, seed 0, target 2.000 caras, iteraciones 40/40,
sin suavizado ni adaptividad. El resultado se retuvo en
`_local/remesh-geology-20260909/rock-2000`:

| Métrica | Antes | Después |
| --- | ---: | ---: |
| Vértices | 17.987 | 1.990 |
| Polígonos | 17.720 | 2.056 |
| Triángulos evaluados | 35.440 | 3.073 |
| Componentes conectadas | 3 | 20 |
| Aristas non-manifold (>2 caras) | 0 | 129 |
| Aristas de borde | 528 | 971 |
| UV layers | 1 | 0 |

Tiempo del core: 3.205 ms. Desviación máxima muestreada bidireccional: 0,2186
unidades, **4,673 % de la diagonal fuente**, por encima del umbral diagnóstico
fijado de 2 %. Se muestrearon vértices y centroides de triángulos; esto no es
Hausdorff exacto ni aceptación runtime. El criterio conservador de topología
también falla. Menos polígonos no compensa superficie perdida, nuevas piezas,
aristas inválidas o UV desaparecidas.

[Roca antes, 512 px](_local/remesh-geology-20260909/rock-2000/before-three-quarter.png)
— [Roca después, RECHAZADA, 512 px](_local/remesh-geology-20260909/rock-2000/after-three-quarter.png).

### Corrección de interpretación del addon instalado

La lectura completa de `smart Remesh/__init__.py::_engine_items` establece que
**Windows con el bin presente ofrece AUTO/STABLE**, no OURS. OURS es el fallback
interno para plataformas sin ese bin. El core probado no representa el resultado
del modo orgánico normal de esta instalación. Esta aclaración reemplaza cualquier
descripción anterior de OURS como el modo orgánico seleccionado del addon.

La prueba correcta pendiente requiere el bin comprado ya instalado:

```text
F:/JericoEngine/_local_reference/blender_addons/smart Remesh/bin/Instant Meshes.exe
sha256:f1fe5b5f56d1002ae61dbd76a3fd646229fc1fe15322e7a3744f8ce442c76b2d
```

El paquete incluye `bin/LICENSE-instant-meshes.txt`. No se descargó, instaló,
ejecutó ni redistribuyó este bin. Su uso necesita autorización específica y una
nueva ventana. La propuesta mantiene tres etapas seriales: Blender exporta copia
OBJ; el runner existente ejecuta el bin fijado con `-f 2000 -c 30 -S 2 -d -b -t 2`;
Blender importa y aplica `close_engine_holes` original del addon, mide y captura.
Cada hijo inmediato tiene su límite de 30 s. Blender no lanza el bin por dentro:
se evita atribuir al runner un containment de nietos que todavía no implementa.

El harness privado ya tiene fases `export` y `compare`, **todavía no ejecutadas**.
La cueva preparada es una shell procedural rugosa con espesor y dos entradas,
no una caja ni un cilindro tapado. Sigue siendo benchmark de retopología, no un
generador de cuevas aceptado artísticamente ni una integración BSP. Se medirán
topología, superficie y 25 rayos de paso axial; estos rayos no sustituyen pruebas
volumétricas completas de keep-clear, collision o navegación.

### Incidente de bytecode y recuperación pendiente

Mi nuevo harness omitió inicialmente `sys.dont_write_bytecode=True`. Blender
ignoró la intención expresada sólo mediante la variable de entorno y generó siete
`.cpython-313.pyc` en `smart Remesh/core/__pycache__` durante el import. El control
postejecución detectó el cambio y rechazó la aceptación del job. No se acepta una
baseline nueva ni se omiten esos archivos del fingerprint operativo para ocultarlo.

La auditoría sólo lectura `tests/audit_remesh_bytecode.py` confirma **21 archivos
originales intactos y exactamente siete añadidos**, sin archivos originales
modificados o ausentes. Al excluir exactamente esos siete añadidos, la identidad
coincide con el catálogo original. Sus rutas absolutas, hashes, tamaños, UTC y
PID/argv asociados están en
[_local/remesh-geology-20260909/bytecode-incident.json](_local/remesh-geology-20260909/bytecode-incident.json).

La omisión está corregida y tiene regresión CPU. **No se ha movido ni borrado
ningún archivo del addon.** El owner mantuvo la escritura limitada a
`Tools/BlenderWorker/**` y pidió autorización del usuario para remediar fuera de
esa carpeta. Propuesta recuperable: mover solamente esos siete archivos, tras
revalidar sus hashes, a `_local/remesh-geology-20260909/bytecode-quarantine`, dejar
intactos los originales y verificar de nuevo el fingerprint completo original.
La carpeta vacía de caché puede conservarse. Hasta resolverlo no se lanzan más
pruebas del paquete ni se declara el backend operativo para esta cadena.
