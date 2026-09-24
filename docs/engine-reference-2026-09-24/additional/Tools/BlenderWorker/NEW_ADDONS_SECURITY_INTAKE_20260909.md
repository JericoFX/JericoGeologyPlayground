# Nuevos addons: revisión estática y condiciones de integración

Fecha: 2026-09-09. Estado: **PARTIAL / REVIEW_REQUIRED / NOT_ADMITTED**.

Actualización posterior a esta revisión: se implementó el catálogo por componente
y la ruta privada de siete presets Rockify. Defender completó un análisis sin
amenazas detectadas. Véase [integración y pruebas reales](NEW_ADDONS_BACKEND_INTEGRATION.md).
Las afirmaciones de no ejecución de Blender/no escaneo que siguen describen
exclusivamente la fase estática inicial, no el estado posterior.

Esta revisión responde a la petición de inspeccionar los paquetes nuevos antes
de integrarlos. No certifica ausencia de malware. No se importó, instaló ni
ejecutó ningún addon; tampoco se lanzó Blender ni un ejecutable incluido en
estos paquetes. Engine, Editor y los originales no se modificaron.

El owner M9 confirmó la excepción de escritura limitada a `Tools/BlenderWorker/**`.
La interfaz nativa continúa sin admisión; el frontend corresponde a M10.5.
Las propuestas de este documento no son adaptadores ya implementados.

## Evidencia y cobertura real

Raíz examinada: `F:/JericoEngine/_local_reference/blender_addons`.

- 2.073 archivos físicos, 2.966.371.359 bytes, inventariados con SHA-256.
- 778 contenidos Python distintos analizados mediante AST; copias idénticas
  se deduplicaron por hash, incluyendo archivos dentro de ZIPs.
- 13 contenedores ZIP distintos leídos en memoria, incluyendo ZIPs anidados.
  No hubo extracción a disco. No se detectaron rutas absolutas, `..`, enlaces,
  duplicados de nombre sin distinguir mayúsculas ni archivos cifrados en los
  contenedores inspeccionados. Se aplicaron límites de tamaño, entradas,
  profundidad y ratio de compresión; no se reportaron excedencias.
- 36 contenidos `.blend`/`.blend1` distintos inventariados como datos opacos.
  Se hizo además una lectura estructural limitada de los siete presets Rockify
  y del archivo Stair Builder, sin evaluar nodos, drivers ni textos.
- Se identificaron bytecodes y ejecutables nativos Windows/Linux/macOS; sus
  hashes no prueban identidad con las descargas oficiales.
- Las coincidencias de llamadas sensibles requieren interpretación: por
  ejemplo, quitar un objeto de una colección Blender no equivale a borrar
  archivos del sistema. El detector de alias es heurístico, no un call graph.

Informe privado de evidencia, no catálogo ejecutable:
[`static-audit.json`](_local/security-intake-20260909/static-audit.json).
Incluye fingerprints por carpeta/ZIP, metadatos, ubicaciones y líneas de
coincidencias, binarios y el análisis complementario de `.pkz` y `.blend`.
Sus fingerprints **no sustituyen** los baselines anteriores de la suite.

Verificación posterior: se volvieron a calcular las huellas de los 2.073
archivos; cero paquetes cambiados y cero paquetes faltantes respecto de esta
inspección. Esto confirma preservación de contenido durante la revisión, no
seguridad del contenido.

La inspección AST no es una lectura manual completa de cada línea ni una
prueba de comportamiento. Imágenes, HTML, PDF, datos, código nativo y bytecode
no han recibido una certificación de seguridad. Tampoco se ha demostrado
compatibilidad operativa con Blender 5.2.1.

## Paquetes nuevos y componentes separados

| Componente | Datos observados | Resultado actual | Ruta propuesta |
|---|---|---|---|
| Auto-Rig Pro | 3.78.50; mínimo declarado 4.2 | Revisado estáticamente, ejecución bloqueada | Rig, skin y bake mediante operaciones explícitas en worker |
| Rig Tools | 3.78.47 | Alternativa al principal, no coinstalar | Perfil separado para rigs ya producidos |
| Proxy Picker | 1.1.11 | No admitir ejecución de textos del asset | Jerico selecciona huesos por ID; no reproducir `exec` de botones |
| glTF incluido | Tres ZIPs: Blender 3.6, 4.0 y 4.1 | No aplicable al pin 5.2.1 | Conservar exportador del Blender fijado |
| Rockify | 1.3.0; siete presets; once archivos | Procedencia por confirmar; no ejecutado | Carga explícita de grupos y controles verificados |
| Instant Clean (`instaclean`) | 2.2.5 | Revisado estáticamente, no registrado | Limpieza por selección sobre copia del asset |
| Zen UV | Manifest 5.4.0; mínimo 4.2 | Requiere aislamiento de rutas sensibles | UV/seams/islas/texel density mediante operaciones acotadas |
| Stair Builder | Biblioteca `.blend` guardada con 3.6 | No es un addon Python instalable | Importar sólo grupos admitidos, no escena ni scripts |
| SimpleBake | ZIP 2.9.11; mínimo declarado 5.0 | Revisado estáticamente, no instalado | Bake dentro del worker existente, no scheduler propio |

El paquete exterior de Auto-Rig contiene `auto_rig_pro_3.78.50.zip`,
`rig_tools_3.78.47.zip`, `proxy_picker_1.1.11.zip` e `io_scene_gltf2.zip`.
Este último contiene tres exportadores antiguos. Se inspeccionaron también
esas capas, no solamente los cuatro nombres de archivo.

La [documentación oficial de Auto-Rig Pro](https://www.lucky3d.fr/auto-rig-pro/doc/install.html)
indica que Rig Tools no debe coexistir con el addon principal y que el
exportador glTF incluido no debe instalarse en Blender 4.2 o posterior.
Proxy Picker es opcional. Los archivos AI también son opcionales y no vienen
en el paquete principal; su ausencia no autoriza descargarlos automáticamente.

## Hallazgos que bloquean la habilitación indiscriminada

### Auto-Rig Pro y Proxy Picker

En el ZIP principal, bajo `auto_rig_pro-master/`:

- `src/auto_rig.py:4625`: consulta de actualización por red.
- `src/auto_rig_ge.py:10598`: ejecución de un archivo de script de exportación.
- `src/auto_rig_smart.py:2100`: proceso externo para funciones Smart/AI.
- `src/auto_rig_prefs.py:169`: extracción de ZIP para instalación adicional.
- `src/auto_rig_prefs.py:160`: rama macOS que retira el atributo de cuarentena.
- `src/export_fbx/json2fbx.py:82`: `eval` sobre datos de la conversión auxiliar.

En `proxy_picker/proxy_picker.py:138`, el botón puede ejecutar el contenido de
un Text datablock seleccionado mediante una propiedad de hueso. No es una
simple selección declarativa y no debe exponerse como comando arbitrario a Qt.

Las coincidencias pueden corresponder a funciones legítimas. La decisión es
excluir actualizadores, instaladores, custom export scripts y ejecución de
texto del contrato Jerico; no acusar al proveedor de malware.

### Zen UV 5.4.0

- `ops/context_utils.py:115/120`: operador de ejecución/evaluación de scripts.
- `ops/auto_uv_unwrap.py:951-954`: crea un contexto TLS sin verificación,
  descarga Ministry of Flat y extrae un ZIP. Esta ruta no se admite.
- `ops/relax.py:158`: lanza el ejecutable ZenRelax.
- `utils/clib/__init__.py:268`: el registro llama a `register_library`.
  `utils/clib/lib_init.py:84` contiene carga de biblioteca nativa.
- `ui/gizmo_draw.py:1530/1545`: proceso Blender adicional y lectura pickle.
- `utils/blender_zen_utils.py:303`: los iconos `.pkz` se deserializan con pickle.
- Las rutas de demos, SVG e interfaz también contienen red, `eval`, `exec`
  y operaciones de borrado. No deben formar parte de una habilitación global.

Los dos `.pkz` no son ZIPs dañados: son zlib + pickle. Se inspeccionaron con
`pickletools.genops`, **sin `pickle.load(s)`**. Expanden a 17.963 y 33.188 bytes.
Contienen `STACK_GLOBAL`, `REDUCE` y `BUILD`; sus referencias iniciales apuntan
a tipos NumPy. Eso explica el formato, pero no convierte pickle en un formato
seguro para fuentes no admitidas. Estos iconos no son necesarios para Qt.

Se identificaron cuatro ZenRelax nativos: Windows, Linux y dos macOS.
El Windows tiene SHA-256
`14440bd25f10995a4892708e2acd889c1007a1afcbfae01066029e13d71ff827`.
La comprobación Authenticode devuelve **NotSigned**; no se ejecutó.
Una firma ausente no demuestra malware, y un MD5 incluido en el mismo paquete
no demuestra procedencia independiente. El manifest declara permiso de archivos,
pero el código contiene también rutas de red/procesos: revisar sólo el manifest
no basta.

### SimpleBake 2.9.11

Dentro del ZIP, bajo `SimpleBake/`:

- `auto_update.py:100-115` y `155-169`: descarga, eliminación del directorio
  del addon, extracción y rollback de actualización.
- `sketchfabapi.py:90`: subida HTTP de contenido a Sketchfab.
- `background_and_progress.py:215`: proceso de bake propio.
- `external_save.py:398`: ejecución de una línea construida para exportación.
- Las rutas de guardado, packing y limpieza pueden eliminar archivos temporales
  o componentes; sus destinos deben estar confinados al job.

La adaptación propuesta admite únicamente bake seleccionado, cage, resolución,
mapas y packing dentro de la sesión del worker. Se excluyen subida, actualización,
scripts de exportación arbitrarios y creación de otro planificador/proceso.
Nada de esto se ha implementado ni probado en Blender en esta revisión.

### Instant Clean 2.2.5

`utils/addon.py:23` consulta una versión por red;
`utils/app.py:14` inicia otra instancia de Blender. El registro instala presets
en recursos del usuario; su función puede reemplazarlos cuando se pide esa
opción. `utils/ui.py:205` incluye importación opcional de `pynput` para cerrar
un diálogo, no una prueba por sí sola de captura maliciosa de teclado.

La adaptación debe invocar sólo limpieza sobre la copia de trabajo, sin reinicio,
diálogos, actualización ni instalación de presets en el perfil real del usuario.

### Rockify 1.3.0

Ahora contiene granito, ígnea, caliza, sedimentaria, estalactita, estalagmita
y volcánica. Los siete `.blend` tienen cabecera `BLENDER-v400`; la lectura
de bloques encontró entre dos y tres NodeTrees y cero bloques Text por preset.
Esto no descarta drivers, bibliotecas externas ni riesgos del parser binario.

No aparecieron llamadas de red/proceso/`exec`/`eval` en su Python bajo las reglas
de esta inspección. Hay drivers de expresión `var` y controles de escena que
apuntan a nodos por nombre. Se requiere comprobar sockets efectivos, rangos,
seed y modificación de la geometría; no basta con leer el panel.

El `bl_info` contiene `Joe Garth, iBlender@taobao.com` y un tracker Taobao.
Se solicitó al usuario confirmar si esta copia procede de su descarga original.
No se infiere malware, infracción ni autenticidad a partir de ese texto.

### Stair Builder v2.02

La carpeta contiene un HTML y un `.blend`, no un paquete Python. La lectura
estructural encontró 33 bloques NodeTree y tres Text. No se evaluaron ni
deserializaron esos textos. El HTML no se abrió en un navegador ni se ejecutó.
La admisión exige revisar textos, drivers y dependencias; el perfil propuesto
extrae únicamente los grupos y recursos necesarios para construir escaleras.

## Otros paquetes y diferencias de identidad

También se incluyeron Cracker, Edge Decal Generator, OCD, RockForm, Nature,
Smart Segmentation y Smart Remesh, junto con los ZIPs repetidos/actualizados.
El nombre del ZIP no concede confianza ni compatibilidad.

- Las copias idénticas se reconocieron por bytes, no por el nombre del producto.
- Existen dos ZIPs de Smart Segmentation: uno incluye un archivo adicional.
  No deben contarse automáticamente como dos generadores.
- Smart Remesh contiene ahora treinta archivos físicos. Los siete `.pyc`
  del incidente anterior siguen presentes. No se borraron ni se aceptó un nuevo
  baseline. El Instant Meshes del ZIP coincide con el hash ya registrado:
  `f1fe5b5f56d1002ae61dbd76a3fd646229fc1fe15322e7a3744f8ce442c76b2d`.
  Su Authenticode también es NotSigned; esta revisión no autorizó ejecutarlo.
- Nature contiene 732 archivos `.fdmdownload`, incluidos en el inventario.
  No se trataron como assets listos ni se eliminaron. Hace falta comprobar su
  significado/completitud antes de incorporar contenido adicional.
- Se observaron rutas de actualización en Cracker, apertura de carpetas en
  Nature, ejecución de fuentes locales en Edge Decal y evaluación de rutas en
  RockForm. Los smoke tests antiguos de rutas acotadas no habilitan todo el addon.

## Controles descubiertos para M10.5

Se reutilizó el extractor AST existente `transformer/intake.py`, sin ejecutar
declaraciones ni callbacks. Los resultados son declaraciones candidatas, no
controles operativos ni un GeneratorSchema validado contra bpy.

| Componente | Declaraciones de propiedad | Clases con metadatos |
|---|---:|---:|
| Rockify | 19 | 3 |
| Instant Clean | 65 | 18 |
| Zen UV | 1.596 | 597 |
| Auto-Rig Pro | 1.034 | 245 |
| Rig Tools | 216 | 64 |
| Proxy Picker | 10 | 4 |
| SimpleBake | 378 | 132 |
| Total | 3.318 | 1.063 |

Una declaración puede pertenecer a un panel, operador, acción peligrosa o
preferencia que no corresponde publicar. Los imports wildcard, enums dinámicos
y namespaces dinámicos quedan sin resolver cuando no hay prueba estática.
No convertir `StringProperty` de código en una caja de texto ejecutable.

Para cada operación futura se propone: ID estable, identidad del paquete,
entrada tipada, controles admitidos, capacidades requeridas, motivo de bloqueo,
salidas esperadas y estado de evidencia. Qt deberá mostrar también operaciones
no disponibles con su motivo, sin activar imports para dibujar el catálogo.
La integración del catálogo en Qt sigue pendiente de M10.5.

## Secuencia de integración propuesta

1. Confirmar procedencia y fingerprints de cada componente; conservar ZIPs
   originales y versiones distintas. No reemplazar baselines de forma implícita.
2. Completar la revisión de código nativo, bytecode y recursos con una herramienta
   antimalware bajo una política de privacidad aprobada y pruebas aisladas.
3. Extraer una copia de trabajo validada bajo `_local`, nunca sobre los originales;
   rechazar rutas peligrosas, colisiones, enlaces y presupuestos excedidos.
   Los bytecodes entregados no se importan; no se modifica la copia original.
4. Construir adaptadores por operación sobre los contratos existentes, sin
   ejecutar registros generales ni callbacks arbitrarios para descubrir controles.
   Confirmar las dependencias transitivas del subconjunto, no sólo su función raíz.
5. Solicitar al owner una ventana Blender separada. Offline/disable-autoexec
   no son una sandbox de Python: además se requiere confinamiento efectivo de
   archivos, red y procesos, y perfil de usuario descartable.
6. Probar cada ruta con entrada pequeña, preview barato, cancelación, hash
   anterior/posterior, límites y fallo sin modificar fuente. Para rigging:
   huesos/skin/rest pose/clip; para UV y bake: seams/tangentes/cage/mapas; para
   geometría: malla, masks, provenance, keep-clear, LOD y collision.
7. Promover sólo resultados demostrados. Mantener original HIGH, autoría
   PWAS/EPLS y contratos de selección independientes del LOD, según
   [la ruta de modelado](MODELING_TO_LOD_PWAS_EPLS.md).

Rigging requiere conservar IDs de huesos, bind/rest transforms, pesos y clips;
no se puede dar por resuelto mediante el exportador estático de geología.
Los consumidores `.rskel/.rrig/.ranim` y su cook/runtime quedan a cargo del owner
nativo. Este documento no agrega otro formato ni modifica el runtime.

## Antivirus y límite de la conclusión

Defender estaba activo con protección en tiempo real y firmas actualizadas
el 2026-09-09 a las 12:06:14 -03:00. Eso **no equivale a un escaneo solicitado
completado**. Se observaron `SubmitSamplesConsent=1` y `MAPSReporting=2`;
no se cambiaron esas preferencias ni se invocó un escaneo que pueda enviar
muestras de los paquetes privados.

Microsoft documenta que un [escaneo personalizado con DisableRemediation](https://learn.microsoft.com/en-us/defender-endpoint/command-line-arguments-microsoft-defender-antivirus)
no aplica acciones de remediación y muestra detecciones por consola. Ese flag
no se considera una garantía de aislamiento de red. La elección de un escaneo
con privacidad adecuada queda pendiente; no se desactiva protección del equipo.

Conclusión: hay evidencia suficiente para **rechazar la habilitación automática
de todos los componentes**, pero no para declarar que hay malware ni para
certificar que no lo hay. No se incorporó código de proveedores al Engine,
no se publicaron paquetes y no se ejecutaron sus funciones sensibles.
