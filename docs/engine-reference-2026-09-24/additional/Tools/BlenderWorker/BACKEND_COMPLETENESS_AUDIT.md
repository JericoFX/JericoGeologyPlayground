# Backend Blender no visual: cobertura real y faltantes

Fecha: 2026-09-08. Estado global: **PARTIAL**. Esta auditoría no declara COMPLETE
el backend, M8 ni JericoEngine. Se realizó mediante lectura de código, pruebas y
resultados retenidos; no ejecutó Blender, GPU, builds ni nuevas pruebas bajo las
regresiones Vulkan de M9.

## 1. Autoridad, orden y alcance

Autoridad vigente: [directiva reconciliada §0R](../../docs/JERICOENGINE_M8_BLENDER_HEADLESS_GEOLOGY_AUTHORITY_DIRECTIVE.md)
y GOAL §4.1, publicados por M9 en `981976d1e18f82e0782ec1129c1801d9bd8898a6`.
La aclaración del owner exige **backend construido, integrado y probado antes de
entrar al foco M8 posterior a M9**. Después se seleccionan addons/capacidades,
se construyen sus adaptadores GeneratorSchema/bindings y se integra el front.

M10.5 corresponde al front y controles Qt. El soporte no visual de snapshot,
selección, preview/diff como datos, Apply/Undo/Redo, cancelación, persistencia,
cache, admisión y cook no se difiere a M10.5 por carecer de widgets. La aceptación
visual/Beauty conserva sus gates correspondientes y no se sustituye por esta auditoría.

Escritura de esta task: sólo `Tools/BlenderWorker/`. M9 conserva contratos
compartidos, integración nativa y Git. Las conclusiones «no implementado» se
refieren a esta carpeta; **el estado real de componentes C++ externos debe
reconciliarlo su owner**, no se presume ausente ni aprobado a partir del worker.

Esta publicación contiene Markdown propio. Código, JSON, lockfile, logs, capturas,
ejemplos de datos, runtime privado y todos los archivos de `evidence/` y `_local/`
son **rutas locales no incluidas en esta publicación documental**. Se citan como
procedencia del examen, no como evidencia descargable de este commit. Los cinco
informes generados voluminosos de addons tampoco se publican aquí.

## 2. Qué se comprobó realmente

| Evidencia local retenida | Resultado observado | Alcance real |
| --- | --- | --- |
| `evidence/unit-conformance.log` | 26 métodos PASS, 0.050 s | Protocolo, geometría y vallas Windows; no broker de producto. |
| `evidence/worker-conformance.log` y `conformance-summary.json` | 10 métodos PASS, 283.869 s; 32 procesos Blender limpios: 23 candidatos y 9 rechazos previstos | Siete familias propias acotadas bajo el pin; no admisión productiva. |
| `evidence/transport-conformance.log` | 2 métodos PASS, 0.666 s | Geometría y atributos semánticos GLB/sidecar; no valida calidad UV, normal bake ni tangentes. |
| `evidence/transformer-expanded-conformance.log` | 83 métodos PASS, 2.569 s | Intake, perfiles, documentación y operaciones CPU propias. |
| `evidence/transformer-operations-summary.json` | 10 conversiones, 100 procesos; diez candidatos idénticos por caso | Repetibilidad de casos CPU; no rendimiento runtime ni equivalencia completa de addons. |
| `evidence/transformer-documents-summary.json` | Cinco informes, diez procesos; dos MD idénticos por addon | Lectura/documentación de 343 declaraciones Python y 3552 sockets, no 3895 controles operativos. |

Worker retenido: Blender 5.2.1, build `9e2066aef7ef`, dos hilos CPU, hash de
ejecutable `8f7a131ad8bc148edc218b334f07d92a57f5a357fa66d913b290537fd8353c06`.
La versión/binario fueron comprobados por las ejecuciones retenidas; no se
volvieron a lanzar ni admitir durante esta auditoría.

Fingerprint raíz **recalculado durante esta auditoría documental**, coincidente:
`sha256:e9ebc0058092d185a27389f033d5f78e0b537eda13299ef9309c8af2b330d289`.
Identidad del último transformer verificado:
`sha256:bc5bf594f51ea99c7f52486e555e601328431b27bc61964a97ff5eea1e7e10a8`.
No se cambiaron fuentes ejecutables en esta auditoría. Los MD y sus correcciones
de calendario no alteran esos fingerprints.

Retenciones locales: `_local/0ffda00a-d11f-47eb-aebf-2443166d2c18/`,
`_local/operations-d3b756ed-400d-453a-9567-c509a088d092/` y
`_local/documents-68103e7f-b25b-4964-a5a9-94f293db4df8/`.
La independencia usa auditoría de IO Python, factory startup y recursos aislados;
no demuestra contención de IO nativo, red, procesos hijos o memoria a nivel OS.

## 3. Matriz de cobertura del backend

Todas las áreas siguientes conservan estado agregado **PARTIAL** salvo ausencia
local explícita. Una prueba focal aprobada no sustituye el gate integral.

| ID / área | Implementación y prueba local | Faltante y criterio observable necesario |
| --- | --- | --- |
| B01 Pin, instalación y admisión | `blender.lock.json`, `tests/setup_blender.ps1:1-20`, `worker.py:82-94`: ZIP/binario/build fijados; setup portable privado; versión errónea rechazada. `productionAdmission:false`. | INTEGRATION REQUIRED: admisión formal/licencia, instalación y resolución offline del pin, upgrade/rollback y concurrencia de instalación según contrato de producto. Pin ausente/cambiado debe bloquear Generate sin fallback y conservar assets previos. |
| B02 Jobs y lifecycle | `worker.py:23-94,198-254`: job individual, NDJSON con secuencia, heartbeat por etapas, resultado tipado y un proceso limpio por job en harness. | INTEGRATION REQUIRED: broker ejecutable con comando fijo, validación streaming del protocolo, límites de concurrencia/backpressure mediante servicios existentes, staging propio y reap completo. El harness de tests no es ese servicio. |
| B03 Cancelación y límites | `file_gate.py:129-142`, `worker.py:44-50,223-231`: cancel/commit serializados, handles inmutables y presupuestos de tiempo/bytes/geometría. Cancelación real y límites tienen negativos retenidos. | INTEGRATION REQUIRED: cancelGrace y terminación del Job Object/árbol, RSS, hijos, heartbeat/hang por fase, ACL/red/IO nativo. `peakRssBytes=None`; una llamada Blender monolítica no observa checkpoints. Probar hang/OOM/cancel sin promoción parcial. |
| B04 Recovery, atomicidad y stale | `protocol.py:149-154` escribe por rename; `result.json` es el último marcador. Inputs se revalidan dentro de la valla. Un root con `output/` previo se rechaza. | INTEGRATION REQUIRED: journal/limpieza de staging huérfano, política de reintentos/quarantine, disk-full y recuperación de transacción conforme §35. No existe recuperación local completa. El ticket viaja pero el host debe comparar ticket/revisiones/schema antes de aceptar o Apply; no basta jobKey válido. Mantener separado jobKey de contenido y ticket de vigencia. |
| B05 Carpeta física y catálogo | `intake.py:177-288`, `library.py:36-68`, `blend_probe.py:59-106`: scan explícito AST/TOML, hashes, `.blend` en proceso acotado y join por hash. Catálogo local: 14 herramientas/92 entradas, incluidas 19 reutilizadas por el flujo compuesto. | INTEGRATION REQUIRED: raíces de catálogo/staging del producto, conflictos/duplicados, paquetes admitidos, dependencias/versiones/licencias, reescaneo e invalidación. Dejar una carpeta y ejecutar scan no demuestra un catálogo integrado persistente. |
| B06 GeneratorSchema y bindings | `nodes.py:349-362` produce diagnóstico de sockets; `suite.py`/`presentation.py` dan IDs, tipos, defaults/rangos/unidades/opciones/dependencias y bindings de operaciones propias. Recetas versionadas rechazan cambios implícitos. | INTEGRATION REQUIRED: contrato GeneratorSchema de producto con asset/package IDs, entradas/salidas/dependencias, validación e invalidación, serialización/presets/migraciones y round-trip con documento/broker. Metadata no resuelta no se evalúa ni se admite por nombre. No se seleccionan nuevos addons antes de completar infraestructura. |
| B07 Transporte de solicitudes/resultados | Worker: perfil privado `blender-offline-candidate/v1`, hashes de inputs/artifacts, NDJSON y resultado `publishable:false`. Transformer: receta/candidato JSON separado; CLI valida versión y snapshot obsoleto. | INTEGRATION REQUIRED: puente a contratos nativos y handshake, consumidor C++ de eventos/artifacts, errores normalizados de todas las fases, invalidación y round-trip. No renombrar el perfil acotado como protocolo de producción. |
| B08 BSP, selección y autoridad espacial | `worker.py:97-193`, `geometry.py:150-191`, `nodes.py:319-344`: snapshot triangular con GUID/revisión, selección sourceIds, AABB Allowed y Protected+margin; resta SDF, validación por capa/LOD, cruces y volumen protegido encerrado rechazados. | INTEGRATION REQUIRED: BSP analítico y su GLB de conveniencia con comprobación de concordancia; semántica de Playable/Portal/Nav/Camera, ContactBand, márgenes derivados del engine, Selection AST/placement/contornos y doble validación nativa. AABB authored no equivale a la autoridad espacial completa de §§9-12. |
| B09 Layer System y generación propia | Capas ordenadas/enabled con IDs, root seed/seed por capa, before/after hash; siete familias GN acotadas y conservación explícita de máscaras upstream. AttributeOnly comprueba igualdad geométrica. | INTEGRATION REQUIRED: fases/dependencias, operadores de composición y masks/selection completos del corpus, locks, invalidación por prefijo, guardado y migración. No todos los controles/pipelines de los cinco estilos están implementados; README conserva sus límites. Necesario probar una receta combinada con el pipeline de assets, no sólo cada kernel. |
| B10 Reuso Decal 3D / Spline | `nodes.py::build` ofrece proyección/atributos de decal y cinta de corte por polilínea; transformer tiene bandas/selección/AO/taper/UV originales. Hay pruebas geométricas por esas rutas. | INTEGRATION REQUIRED: adaptadores de recipe/selección/capas/semántica autoritativa para Decal3D y splines del engine, volumen/mesh decal y cortes/chunks/bonds según §§20-21. No duplicar backends ni confundir una cinta de fixture con Spline Destruction integrado. |
| B11 Export GLB y provenance | `output.py:53-112,115-181`, `transport.py:8-62`: GLB nativo seleccionado, geometría/atributos contra sidecar; GUIDs/slots/regiones/layer records y artifact hashes. Materiales excluidos y tangentes desactivadas en export. | INTEGRATION REQUIRED: extras/atributos completos del contrato, domains requeridos, source/layer/region/material/physical y provenance por producto/elemento, transforms y validación import/canonización nativa. El validador actual no compara TEXCOORD/NORMAL/TANGENT. GLB es transporte, nunca formato Shipping. |
| B12 UV y tangentes | `output.py:96-112` usa Smart Project con margin fijo 0.02. Bandas CPU usan UV métrica y escala por parche probada. | INTEGRATION REQUIRED: policies preserve-source/unwrap/hybrid/triplanar según material, seams/locks de chunks, margin por resolución/mips/compresión, finite/range/overlap/stretch/density/occupancy y base tangente tras triangulación. Exportar TEXCOORD no prueba calidad ni cumplimiento de §27. |
| B13 Bake high→low | **NOT STARTED en esta carpeta**: no operación de bake high/low/cage ni mapas. El filtro AO de aristas y las máscaras vertex no son texturas baked. | INTEGRATION REQUIRED: Cycles CPU, high/low y cage explícitos, targets/espacios/dilation y mapas de §28; detectar misses, backfaces, proyección a otro chunk, NaN/Inf y cancelación. Dos cold bakes con canonical texture bytes iguales y comparación high/low declarada. No ejecutar aquí durante Vulkan. |
| B14 LOD y chunk seams | `output.py:115-181`, `geometry.py:194-236`: decimation 0.5^LOD, nueva validación y restauración exacta de regiones pequeñas colapsadas/desaparecidas; GLB/sidecar y slots probados. | INTEGRATION REQUIRED: budgets de error de superficie/normal/UV por producto, límites físicos/materiales/semánticos, boundary locks y seam identity entre chunks, transferencia/revalidación UV y prueba entre vecinos. Preservar componentes no demuestra estos contratos. |
| B15 Collision | LOD0 aporta referencia a static-triangle-mesh-candidate validado y `requiresJericoJoltCook:true`. | INTEGRATION REQUIRED: hulls/chunks y presupuestos apropiados, error/coverage contra render mesh, correspondencia chunk/bond/anchor, physical material revalidado, `.rcoll` canónico y Jolt create/add/remove/reload. No hay cook Jolt demostrado en la carpeta. |
| B16 Cache, determinismo y rendimiento | Job key incorpora inputs/pin/runtime/schema/calidad; máscaras/regiones/hash canónicos. Diez procesos limpios repetibles en worker y en cada caso CPU. `cacheHit=False`. | Cache de producto **NOT STARTED aquí**. INTEGRATION REQUIRED: DDC existente, SourceAdmission/LayerPrefix/surface/LOD-UV/bake/transport/cook keys y manifests, invalidación/corrupción/promoción atómica, concurrent/cold/warm/cancel tests. No crear cache paralelo. Faltan benchmarks de RSS/picos/límites y perfiles representativos; tiempos de tests no son presupuesto de producto. |
| B17 Materiales y consumidores | Masks de punto y slots source/layer/material/physical; regiones, caras y cadenas según operación. Sidecars sólo aceptan INT/FLOAT POINT/FACE; no materiales Blender exportados. | INTEGRATION REQUIRED: MaterialDefinition/PhysicalMaterial, mask/class tables y texturas, PWAS/EPLS, bonds/anchors/fresh surfaces, nav queries y World Cells. Resolver GUIDs y hashes de definiciones en Apply/cook. Medir pérdida/reconstrucción de semántica en cada conversión; no asumir que un slot ya conecta al consumidor. |
| B18 Transacciones, cook y runtime | Resultados offline contienen revisión/ticket/inputs/receta y son no publicables. Las operaciones CPU conservan etapas y fuente. | INTEGRATION REQUIRED: admisión nativa, CAS de vigente, preview/diff de datos y ProjectTransactionCoordinator/Undo/Redo/journal, save/load/reimport, canonización por la única ruta cook y paquete source-free Blender/Python/GLB/source/red-free. También migración v29→v30 y retiro del productor viejo tras sus gates, bajo autoridad del owner compartido. No se cambia ni retira código del engine desde esta carpeta. |

## 4. Linaje y consumidores que deben cerrarse

```text
BSP analítico + semántica + revisión + selection/keep-clear
→ snapshot admitido + recipe/GeneratorSchema/versiones
→ generación/capas y campos geológicos
→ high/low + UV/cage/bake + LOD/chunks/collision candidates
→ resultado validado + provenance + manifest
→ transacción Jerico + cook/DDC
→ materiales/física/destrucción/nav/World Cells y runtime source-free
```

Hoy la carpeta cubre partes del recorrido hasta candidato. Conservar hashes no
equivale a conservar todos los datos semánticos. `output.stamp` asigna slots por
fuente/última capa; no demuestra procedencia espacial multicapas ni class/material
boundaries completos. Las máscaras upstream se remuestrean por nearest surface
tras cambios SDF; falta medir si sobreviven clasificación, estratos, cavidades,
fresh-break, propiedades físicas y superficies expuestas con la precisión que
requieren consumidores, UV/bake/LOD y destrucción. Las caras originales/tapas de
partition y las etapas de selección ya se distinguen y deben seguir distinguiéndose.

| Integración requerida | Datos / interfaz esperada | Owner / dependencia / momento |
| --- | --- | --- |
| Proceso/recuperación | Request + pin/paquete + job root → eventos/metrics/result; cancel fence, Job Object, staging/retry/retention del contrato existente. | BlenderAuthoring/host; §§23-25,35; backend previo a M8. |
| Catálogo/schema | Raíces/manifests/hashes/deps → catálogo y GeneratorSchema versionado + diagnóstico, recetas/presets sin widgets. | Host/asset registry/ProceduralAuthoring; §§6-8; infraestructura previa a M8, bindings de addons concretos después de selección. |
| Autoridad espacial | BSP analítico + surface transport + semantic fields/loops → snapshot/selection/Allowed/Protected/ContactBand validados. | BSP/geometry/ProceduralAuthoring; §§9-12; backend previo a M8. |
| Preparación/cook | High/low/UV/maps/LOD/collision + slots/IDs → import/admission y writers canónicos Jerico. | AssetPipeline/materials/Jolt; §§26-33; backend previo a M8, sujeto a evidencia y admisión de su owner. |
| Transacciones y persistencia | Ticket/revisiones/recipe/result/artifact hashes → revalidación, journal/Apply/Undo/Redo/save-load/reimport. | ProceduralAuthoring/Editor ProjectTransactionCoordinator; §§31,35; soporte no visual previo a M8. |
| Consumidores | Assets cocinados + semántica estable → materiales/PWAS/EPLS/physical/destruction/nav/world, sin cargar sources. | Owners runtime correspondientes; §§32-33,37; cerrar los gates aplicables antes de declarar backend integrado. |
| Front diferido | GeneratorSchema/bindings + datos preview/diff/diagnóstico/cancel + transacciones ya operativas. | Qt/Editor, M10.5; widgets/panels y navegación visual no condicionan construir el backend ahora. |

ECS/Luau/eventos/save-load necesitan interfaces de assets/IDs/resultados, no
ejecución de Python de gameplay. Animation/Sequencer/audio/partículas no reciben
un nuevo generador directo; cuando consuman destrucción/contacto/clasificación,
su owner debe trazar el campo/evento requerido en los contratos existentes.

## 5. Correspondencia con los veinte gates del corpus

No se ejecutó ninguno de estos gates nativos nombrados durante la auditoría.
La columna local identifica contribuciones; **ninguna fila es un PASS del gate
integral**. Su estado fuera de la carpeta requiere comprobación del owner M9/M8.

| Gate | Contribución local / pendiente principal |
| --- | --- |
| `VerifyBlender521Installation` | Pin y portable probados; admisión/distribución de producto pendiente (B01). |
| `VerifyBlenderOfflineSandbox` | Audit Python y entorno del harness; sandbox OS/Job Object/RSS pendiente (B02-03). |
| `VerifyBlenderPluginCatalogAndLicenses` | Intake e inventario; admisión/conflictos/licencias/dependencias integradas pendientes (B05). |
| `VerifyGeneratorSchemaRoundTrip` | Catálogo/recetas parciales; GeneratorSchema de producto y round-trip documento/host pendientes (B06). |
| `VerifyBlenderJobLifecycleAndCancellation` | Vallas/cancel/rechazos probados; lifecycle host y recovery integral pendientes (B02-04). |
| `VerifyBspKeepClearDoubleValidation` | Validación triangular/AABB local; snapshot analítico y segundo validador nativo pendientes (B08). |
| `VerifyGeologyLayerSystem` | Capas/seed y máscaras parciales; fases, selección/composición y receta combinada completas pendientes (B09). |
| `VerifyFiveRockStyleGoldens` | Fixtures propios acotados; no cinco goldens de aceptación completa. |
| `VerifyDecal3DReuse` | Generación propia acotada; adapter nativo y consumo contractual pendientes (B10). |
| `VerifySplineDestructionReuse` | Cinta/polilínea acotada; spline/chunk/structural reuse pendiente (B10). |
| `VerifyGlbAttributesExtrasRoundTrip` | Geometría/semántica del perfil privado probadas; extras y atributos productivos completos pendientes (B11). |
| `VerifyUvBakeLodCollision` | UV simple, LOD y colisión candidata parciales; bake/cage, UV completo y cook físico pendientes (B12-15). |
| `VerifyPwasEplsMaterialPhysicalIntegration` | Masks/slots disponibles; consumidores no demostrados aquí (B17). |
| `VerifyNavigationAndWorldCellSeams` | Bounds/regiones disponibles; nav/vecinos/seams no demostrados aquí (B14,17). |
| `VerifyBlenderGeologyDeterminism` | Repetición limpia parcial; cache/cold/concurrent y canonical cook/texture bytes pendientes (B16). |
| `VerifyBlenderGeologyPerformance` | Tiempos de pruebas y límites locales; perf/RSS/capacidad de producto pendientes (B03,16). |
| `VerifyBlenderGeologyTransactionalApply` | Candidato/revisión/ticket; CAS/journal/Undo/Redo y crash recovery no demostrados aquí (B04,18). |
| `VerifyBlenderGeologySourceFreeShipping` | Sin prueba local de paquete canónico source-free con consumidores (B18). |
| `VerifyBlenderGeologyBeauty` | No hay evidencia Beauty propia de producción; no sustituir con compile/GLB/candidato. |
| `VerifyLegacyProducerAbsent` | No se inspeccionó ni modificó build graph/registry/Shipping externos; gate del owner compartido (B18). |

## 6. Siguiente trabajo y condición de cierre

La prioridad inmediata cambia de ampliar controles de addons a cerrar el backend.
Primer paquete a coordinar con el owner: reconciliar el broker/job lifecycle real
con B02-B04, seleccionar el primer comportamiento no demostrado y conservar su
RED (hang/cancel duro, stale/recovery según el estado nativo), sin crear otro
planificador ni journal paralelo. Las modificaciones compartidas son del owner;
esta task sólo puede aportar protocolo/worker/tests dentro de su carpeta.

Después se cierran catálogo/schema y BSP/capas, preparación UV/bake/LOD/collision,
cache/transacciones/cook y consumidores con las pruebas de la matriz. El pool
prewarm es opcional en §24.2; no se convierte en requisito nuevo ni excusa para
posponer la ruta autoritativa de un proceso por job.

Los textos históricos de M10.5 en logs/informes generados no publicados no
redefinen la secuencia. La plantilla del comando documental contiene todavía una
frase que agrupa Apply/Undo con M10.5: debe corregirse y regenerarse/probarse en la
siguiente ventana CPU autorizada. No se alteró código ni la evidencia generada
para aparentar que ese cambio ya estaba probado. UI_CONTROLS/ADDON_TEMPLATE/PLAN
y este audit dejan vigente la separación correcta desde esta publicación.

Sólo se podrá declarar backend completo al demostrar todos sus contratos no
visuales aplicables, reintegración y recuperación, sin descartar semántica y con
evidencia de consumidores/cook/runtime. Lo diferido visual tendrá interfaz/datos,
owner y milestone explícitos. La publicación de estos MD documenta **PARTIAL**;
no admite una dependencia, addon, worker de producción o paquete runtime.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
