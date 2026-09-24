# Los siete addons instalados: backend privado y conexión M10.5

Actualización de ejecución 2026-09-09:
[preview ligero y cancelación](PRIVATE_PREVIEW_EXECUTION.md). La nueva CLI usa
preview 512 px por defecto; los resultados 1024 px de este documento son evidencia
histórica del perfil de referencia, no la nueva resolución por defecto.

Fecha: 2026-09-08. Owner nativo: Astra. Alcance de implementación:
`Tools/BlenderWorker/**`. Estado: **siete rutas privadas ejecutadas; gate de repetición de inputs parcial;
NO backend de producción completo, NO M10.5 admitida**.

Esta ampliación atiende la instrucción de integrar todos los paquetes que existen
ahora. «Todos» en la evidencia significa una ruta real por cada uno de los siete
paquetes, no todos sus presets, operadores, controles ni combinaciones. No sustituye
los gates pendientes por una etiqueta de catálogo. Los generadores propios v1/v2
con Beauty rechazada no se rehabilitan por estos resultados de terceros.

## 1. Implementación entregada

- `private_addon_catalogue.py`: descubrimiento estático sin importar Python del
  addon; identifica archivos, declaraciones, clases, bibliotecas y diagnósticos.
  El fingerprint cubre **todos** los archivos, incluidas texturas, licencias,
  BLEND y ejecutables. Un archivo nuevo o cambiado invalida la identidad.
- `private_addon_suite.py`: siete adaptadores privados concretos. Carga las
  bibliotecas originales o llama al algoritmo instalado; no copia ni renombra el
  código comprado como código Jerico. No llama a `register()` de ningún addon.
- `private_suite_contract.py`: IDs permitidos, petición acotada de seed y rechazo
  de resultado con exit code, identidad o estado incorrectos.
- `tests/replay_private_suite.py`: harness de prueba serial que reutiliza
  `transformer.library.run_process`; **no es otro scheduler de producción**.
  Comprueba fingerprint completo antes y después, pin del ejecutable, exit code,
  identidad del resultado y hashes de capturas.
- `tests/verify_private_suite_evidence.py`: verifica dos ejecuciones retenidas,
  identidad geométrica y decodificación de las catorce capturas originales.
- Tests CPU nuevos: cuatro de inventario y seis del contrato de suite, incluidos
  RED observados antes de implementar los contratos.

## 2. Carpeta física y descubrimiento para el futuro front

Los paquetes comprados permanecen, sin modificar, en:

```text
F:/JericoEngine/_local_reference/blender_addons/
  The Nature Generator/
  rockform_1.4.1/
  ocd_2/
  cracker/
  Edge_Decal_Generator_v1.2.1/
  Smart Segmentation v1.0/
  smart Remesh/
```

Dejar un nuevo paquete **extraído en una subcarpeta** permite que el scanner lo
liste. Esta versión no descomprime ZIP ni registra automáticamente código. Carpetas
nuevas reciben un ID `unreviewed-*` y estado `adapter-required`; no desaparecen del
inventario porque aún no tengan adaptador. Un nombre cambiado no se asocia por
parecido: requiere revisar su mapping y versión.

```powershell
python -B Tools/BlenderWorker/private_addon_catalogue.py `
  F:/JericoEngine/_local_reference/blender_addons `
  --out F:/JericoEngine/Tools/BlenderWorker/_local/new-intake/catalogue.json
```

El destino debe ser nuevo; no se sobrescribe un inventario anterior. Límite de
inventario por paquete: 8.192 archivos y 3 GB leídos en streaming. Nature ocupa
2.391.091.255 bytes. Este límite **no** habilita una malla mayor ni más tiempo de
Blender. Symlinks/junctions y escapes de la raíz se rechazan.

En M10.5, JericoEditor consumirá estos datos a través del backend nativo aprobado;
no importará bpy ni ejecutará Python del addon dentro de Qt. La conexión Qt todavía
no está implementada. Hasta entonces los JSON son la fuente de inspección:

```text
Tools/BlenderWorker/
  private_addon_catalogue.py
  private_addon_suite.py
  private_suite_contract.py
  tests/replay_private_suite.py
  tests/verify_private_suite_evidence.py
  _local/intake-all-20260908/catalogue.json
  _local/private-suite-20260908-final/
    suite.json
    evidence-audit.json
    <addon-id>/
      invocation.json
      stdout.log
      stderr.log
      temp/
      user/
      captures/
        process.json
        runtime-interfaces.json
        result.json
        front.png
        three-quarter.png
```

## 3. Rutas ejecutadas, no nombres de fachada

| ID | Implementación original ejecutada | Fixture / resultado | Alcance no cubierto |
| --- | --- | --- | --- |
| `nature` | `Nature_Generator_Assets.blend`, objeto `NG_Fractured_Rock_Formation`, grupo `NG_Fractured_Rock_Formation_Generator` | Preset original: 17.987 vértices, 17.720 polígonos | Resto de assets/presets, mapeo BSP, cuevas y superficies de hielo |
| `rockform` | `RockForm_v140_B5.blend`, colección `RockForm Assets`, objeto `RockForm Settings Object`, grupos `RF_SetShape` y `RF_SetMat` | Preset original: 110.598 vértices, 110.594 polígonos; sin UV en esta salida | Todos los styles/guide sources y controles; `RF_Master` es shader, no el generador geométrico |
| `ocd` | `data.blend`, grupo `OCD_GN_2.5.0` sobre cubo | 2.818 vértices, 1.402 polígonos; UV presente | Damage masks, todos los noise menus, UV transfer y composición con otra geometría |
| `cracker` | `data/gn_bl5.blend`, grupo `Cracker_GN` sobre cubo y Empty explícito | Corte: 284 vértices, 350 polígonos; UV presente | Glass Breaker modal, Stomper, herramientas interactivas, splitting en productos de destrucción |
| `edge-decal` | `object.generate_edge_decals_by_angle`, que llama a `mesh.generate_edge_decal_strips` | Cubo original 8/6 más decal 72/24; decal con dos UV layers | Booleans/intersections, todas las capas, materiales/texturas y edición interactiva de pins |
| `smart-segmentation` | `smartseg.analyze`: proxy, SDF, clustering y mapping a caras originales | Suzanne 507/500; seis regiones, array de 500 etiquetas y atributo FACE `private_segment_id` | Split físico en piezas, merge/paint modal, holes/caps y semántica nativa |
| `smart-remesh` | `core.hardsurface.rebuild`, NumPy del paquete instalado | Cubo reconstruido a 32 vértices/30 polígonos, sin UV | AUTO/STABLE/Instant Meshes, solver de otras superficies, LOD/bake/fidelidad |

Los conteos son vértices/polígonos, no triángulos ni presupuestos runtime. La prueba
de Remesh demuestra la ruta original de paneles/chamfers en una fixture sencilla;
no una retopología de roca compleja. La prueba de Cracker demuestra el corte del
grupo, no una destrucción física completa.

## 4. Introspección y reglas de controles M10.5

El inventario estático contiene 490 declaraciones de propiedades Python, sumando
los siete paquetes. No equivalen a 490 controles operativos: hay duplicaciones,
propiedades de UI, operadores modales, preferencias y controles dependientes.

En ejecución se devuelve:

1. `runtime-interfaces.json`: interfaces de los GN modifiers realmente usados,
   panels, sockets, descriptions, defaults, min/max y tipos de nodos. Cada entrada
   INPUT incluye `currentValue`, `usesAttribute` y opciones RNA de menú cuando están
   disponibles en `value`.
2. `result.json.rnaControls`: propiedades reales del PropertyGroup utilizado:
   93 para Edge Decal y 17 para Smart Segmentation. Incluyen nombre, descripción,
   tipo, valor, hard limits, read-only y enums.
3. `result.json.appliedBindings`: asignaciones explícitas y lectura de vuelta.
   Sólo estas acreditan una asignación; no acreditan automáticamente su efecto.

API verificada en el pin Blender 5.2.1:

```python
input_property = getattr(modifier.properties.inputs, socket_identifier)
input_property.value = requested_value
observed = input_property.value
```

No usar el antiguo `modifier["Socket_N"]` en esta versión. Para enums, conservar
identificadores RNA string, no índices visuales. Cracker documenta para side/pin
los identificadores `A`, `None`, `B`; su exploración no habilita aún esos controles.

| Ruta | Binding privado aplicado | Estado del resto |
| --- | --- | --- |
| Nature | `Socket_6`: Mesh Random Seed | Perfil anterior de cinco controles en `private_preset_controls.py`; sólo seed tiene prueba de cambio geométrico en evidencia previa |
| RockForm | Se conserva el estado del preset completo | Todos los controles: inspección; no se promete seed externo |
| OCD | `Socket_22`: seed | Otros sockets inspeccionados, no editables por esta CLI |
| Cracker | `Socket_8`: Empty; `Socket_24`: radio 1,5; `Socket_4`: seed; `Socket_15`: detail 2 | Offsets, sides, pins, material, UV y shape: inspección |
| Edge Decal | `Scene.edge_decal_settings.seed` | Otras propiedades RNA inspeccionadas; ruta automática usa defaults originales |
| Smart Segmentation | `Scene.smart_seg.part_count=6` | Resto inspeccionado; seed no es parámetro de esta ruta |
| Smart Remesh | Defaults de `hardsurface.rebuild` | No se inventan sockets GN ni seed para una función NumPy |

La CLI de suite acepta `--id` y `--seed` (entero 0..10.000). En las tres rutas sin
seed conectado se registra `seedStatus`; esta CLI es un harness diagnóstico, no el
contrato editable final del usuario. El front no debe presentar ese seed como
control universal. Un adaptador de producción debe rechazar parámetros sin binding
o declararlos de sólo lectura, sin aplicar silenciosamente valores ineficaces.

### Modelo que debe consumir Qt

Separar tres estados: visible en catálogo, ejecutable en prueba privada, admitido
en producción. Mostrar todos los paquetes; deshabilitar acciones no admitidas con
un motivo visible. El botón Apply no se deriva de `PASS_PRIVATE_ROUTE_SMOKE_ONLY`.

| Tipo de dato | Control Qt previsto | Obligación |
| --- | --- | --- |
| Bool/int/float | CheckBox/SpinBox/DoubleSpinBox | Intersectar límites declarados con presupuesto admitido; preservar unidad |
| Vector/rotation/color | Editores tipados | Conversión de espacios/unidades explícita; no reinterpretar números |
| Enum/menu | ComboBox | Persistir identifier, nunca posición ni label |
| Object/collection/material/image | Asset picker Jerico | Resolver GUID a datablock job-local; no aceptar Python ni ruta arbitraria |
| Geometry/field/attribute | Selector de source/layer/mask | No convertir un field en float constante sin semántica |
| Modal/view-dependent action | Acción deshabilitada hasta adaptador explícito | Jerico implementa interacción y envía geometría/selección, no invoca Blender UI |

`Socket_N` es un binding de la versión concreta, no un GUID estable de receta.
Antes de persistir presets desde M10.5, el manifest del adaptador debe fijar IDs
semánticos y sus mappings por packageHash. Si cambia una interfaz, invalidar el
schema/cache y requerir migración explícita, no enlazar por nombre aproximado.

Ejemplo de manifest de **diseño**, no entrada productiva ya registrada:

```json
{
  "schemaVersion": 1,
  "adapterId": "private.vfxguide.cracker.gn-cut",
  "packageId": "cracker",
  "packageHash": "sha256:57666a2179fb01f2c1327eb77d4d156606883c2612616603e8d2cbee979cef85",
  "blenderBuild": "9e2066aef7ef",
  "route": "Cracker_GN",
  "controls": [
    {
      "id": "fracture.seed",
      "type": "integer",
      "binding": {"group": "Cracker_GN", "identifier": "Socket_4"},
      "minimum": 0,
      "maximum": 10000,
      "status": "private-assignment-observed"
    }
  ],
  "capabilities": {"privatePreview": true, "nativeApply": false},
  "publishable": false
}
```

## 5. Layer System, loops, decals, spline y PWAS/EPLS

No crear un Layer System del addon dentro de Jerico como segunda autoridad. La
receta Jerico conserva orden, layer GUID/revision, selección, masks, seed, locks,
material/physical material IDs, dependencias y región dirty. Las capas internas
de Edge Decal se representan como estado derivado del candidato de esa receta.

Una selección de loop debe llegar con source GUID, revision/hash de topología,
IDs de edges y orden cuando importe. Validar continuidad/cierre/ramificación antes
de pasar la selección a Blender. Un índice de cara después de remesh no es el ID
original. Selecciones obsoletas deben rechazarse o remapearse con evidencia.

- BSP sigue siendo source-of-truth espacial. Enviar geometría resuelta, IDs y
  semántica, allowed regions y keep-clear; los presets de esta suite no cumplen
  por sí solos ese contrato.
- Cracker: mapear el Empty job-local desde transform/shape del corte o spline,
  después validar el resultado contra la región autorizada. No sustituir la spline
  por una operación modal con coordenadas de pantalla.
- Edge Decal: reutilizar generación de strips y UV sobre edges/loops seleccionados;
  separar source y overlay, conservar attachments, material y layer provenance.
- Smart Segmentation: las etiquetas son particiones geométricas, no clasificación
  automática de material físico ni GUIDs de destrucción. Convertirlas a IDs estables
  exige source revision y mapping explícito.
- PWAS posee placement/distribution. EPLS posee estado ambiental superficial, no
  cambia topología: solicita objetos a PWAS cuando corresponda. No delegar esas
  autoridades a un addon. Las masks producidas alimentan estos contratos existentes.
- Destruction/collision/navigation consumen productos validados, no meshes arbitrarias
  de preview. La malla visible no implica collider ni ruta navegable correcta.

Gates de integración pendientes: ningún producto ni LOD invade keep-clear; no se
modifica geometría fuera de selección; provenance por región/cara, no simplemente
el último layer aplicado al objeto; determinismo de IDs y orden; cancelar no cambia
el mundo; aplicar es una transacción fenced por source revision.

## 6. UV, LOD, bake y retorno

UV presente en una captura no acredita unwrap apto para bake, texel density,
islas sin solapamiento ni tangentes compatibles. RockForm y Smart Remesh no
devuelven UV en las fixtures probadas: no ocultar ese dato.

Los LOD deben derivar de la superficie alta y conservar apariencia mediante
error de superficie/silueta y bake high-to-low. No pueden conservar idéntica
topología con menos polígonos. Ni el Decimate de RockForm, ni el GN cache bake de
OCD, ni el proxy de Smart Segmentation cierran normal bake high-to-low.

Antes del retorno nativo: UV/cage, bake normal tangent-space y masks, semántica
material/física, provenance, collider, nav/destruction y GLB/custom attribute
round-trip deben probarse en el cooker existente. La suite actual guarda métricas,
etiquetas y capturas; **no entrega todavía un producto GLB/LOD/collision admitido**.
El worker privado tampoco añade Blender ni fuentes compradas a Shipping.

## 7. Seguridad, identidad y licencias

Runtime usado: Blender 5.2.1 LTS, build `9e2066aef7ef`, executable SHA-256
`8f7a131ad8bc148edc218b334f07d92a57f5a357fa66d913b290537fd8353c06`.

Background/factory startup/offline/disable-autoexec, threads=2. Config, scripts,
extensions y temporales job-local; bytecode writing desactivado. Se registra PID,
argv y UTC. Un audit hook rechaza sockets y lanzamiento de procesos desde Python.
**No es un sandbox OS** ni hace seguro un addon hostil. No se ejecutan módulos
desconocidos sólo por haberlos escaneado.

No se usan fullregister, timers, keymaps, modal operators, update checks, browsers
ni `Instant Meshes.exe`. Edge Decal registra sólo PropertyGroups y los dos operadores
sin interfaz requeridos. Smart Segmentation registra sólo su PropertyGroup y analyze;
su llamada de color del viewport se reemplaza en memoria por no-op en background.
No cambia el algoritmo de segmentación ni escribe en la compra.

Cracker conserva su GPLv3+ y autor; su biblioteca requiere mantener identidad de
asset. Smart Remesh declara GPL-3.0-or-later y contiene un binario Instant Meshes con
licencia BSD-3-Clause separada; el binario no se ejecutó. Para los otros cinco,
véase `PRIVATE_ADDON_REUSE_DIRECTIVE_20260908.md`. Uso privado autorizado por el dueño
no equivale a autorización genérica de redistribución de todos los assets.

Para cache productiva, incluir packageHash de todos los archivos, adapter code hash,
Blender executable/build, schema version, input geometry/transform/source revisions,
ordered layers, parameters, seeds, materials/masks, evaluation mode, resolution y
política UV/bake/LOD. Cache de preset no puede reutilizarse en BSP distinto. Este
harness no crea un DDC paralelo; su fingerprint es evidencia para el DDC del owner.

## 8. Pruebas retenidas y límites

Suite definitiva: `_local/private-suite-20260908-final/suite.json`.
Auditoría: `_local/private-suite-20260908-final/evidence-audit.json`.
Repetición comparada: `_local/private-suite-20260908-2350/`.

- Siete rutas terminaron con exit code 0 en ambos runs.
- Siete listas de geometryHash coinciden entre dos runs con packageHash y build
  iguales. Cinco también tienen idéntico registro de bindings. En Edge Decal y
  Smart Segmentation se añadió el registro de bindings entre runs: la auditoría
  estricta marca `NOT_COMPARABLE_BINDING_METADATA`, no PASS de repetición de inputs.
- Catorce PNG definitivos se decodificaron a 1024×1024 y su SHA coincide.
- Suite CPU ligera completa: 149 tests PASS. Excluye explícitamente los módulos
  `test_worker.py`, `test_transport.py` y `test_own_node_groups.py`, que requieren
  pruebas de Blender/lifecycle separadas; no se los declara aprobados por este run.
- Originales completos verificados antes/después. No se vendorizó código comprado.
- No equivale a determinismo cross-machine/GPU/version ni a efecto probado de cada
  control. Tampoco es aceptación Beauty frente a imágenes comerciales.
- Capturas clay originales de Blender; Smart Segmentation usa sus colores por
  región. No imágenes generativas ni screenshots del sitio del vendedor.

La revisión final reforzó la evidencia: views distintas y con rutas exactas,
baseline validado contra status/package/build, rutas CLI absolutas y rechazo de
una suite incompleta. Baseline y repeat deben ser directorios privados distintos;
comparar un run consigo mismo se rechaza. La reauditoría retenida tiene exit code 1 y
`sameInputRepeatAccepted:false` por los dos registros no comparables; las siete
ejecuciones individuales siguen teniendo exit code 0 y catorce imágenes válidas.
Para cerrar ese gate se necesitan dos nuevos runs con el mismo registro de inputs,
en otra ventana autorizada. No se volvió a ejecutar Blender después de liberar
la ventana a las 23:50:49 UTC.

El primer run `_local/private-suite-20260908-2348/` conserva el timeout de Nature.
Había renderizado a 24,7 segundos pero la doble verificación de 2,39 GB dentro del
proceso excedía 30 segundos. El pre/post fingerprint se pasó al harness; el límite
de Blender sigue en 30 segundos y el guard de ventana reserva 120 segundos por caso.
Los procesos hijos se matan y recogen mediante el runner existente al vencer plazo.
Ese runner ya tiene soporte de archivo de cancelación, pero esta CLI de suite aún
no lo expone; el gate compartido de cancelación sigue pendiente. No hay reintentos
automáticos ni reemplazo silencioso de una falla.

### Capturas originales

| Addon | Captura real |
| --- | --- |
| Nature | [Strata / fractured formation](_local/private-suite-20260908-final/nature/captures/three-quarter.png) |
| RockForm | [Preset original](_local/private-suite-20260908-final/rockform/captures/three-quarter.png) |
| OCD | [Damage de cubo](_local/private-suite-20260908-final/ocd/captures/three-quarter.png) |
| Cracker | [Corte del grupo original](_local/private-suite-20260908-final/cracker/captures/three-quarter.png) |
| Edge Decal | [Overlay de bordes](_local/private-suite-20260908-final/edge-decal/captures/three-quarter.png) |
| Smart Segmentation | [Seis regiones](_local/private-suite-20260908-final/smart-segmentation/captures/three-quarter.png) |
| Smart Remesh | [Reconstrucción hard-surface](_local/private-suite-20260908-final/smart-remesh/captures/three-quarter.png) |

## 9. Secuencia obligatoria antes de M10.5

1. Owner admite lifecycle/cancel/recovery e identidad del resultado en el contrato
   compartido. Mantener NOT_ADMITTED/UNFROZEN mientras ese gate no cierre.
2. Expandir cada adaptador a sus funciones/presets restantes, con inputs acotados,
   bindings semánticos y pruebas de cambio de control. Las rutas modales requieren
   diseño headless explícito; no ejecutar INVOKE para simular UI ausente.
3. Conectar BSP/selection/keep-clear y Layer System con provenance espacial, PWAS y
   EPLS; probar composición de geología + corte/spline + decals.
4. Cerrar UV, high-to-low bake, LOD fidelity, collider/nav/destruction, cache,
   cook/result round-trip y Shipping libre de fuentes.
5. Astra congela GeneratorSchema/manifests/version migrations. Entonces M10.5
   implementa modelos Qt, asset/selection pickers, preview/cancel/apply y Undo/Redo
   sobre el backend admitido. No se aprueba «queda sólo el front» antes de estos gates.

La actualización del roadmap compartido y publicación Git pertenecen al owner;
este trabajo no modificó Engine, Editor, M9 ni documentos fuera del backend.
