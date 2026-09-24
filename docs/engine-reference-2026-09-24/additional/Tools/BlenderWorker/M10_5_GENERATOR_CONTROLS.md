# Contrato de controles M10.5 para Geometry Nodes Jerico

La secuencia completa de modelado, HIGH, retopología, UV/bake, LOD y aplicación
de PWAS/EPLS está trazada en
[MODELING_TO_LOD_PWAS_EPLS.md](MODELING_TO_LOD_PWAS_EPLS.md).
Es diseño de integración; no declara esa cadena implementada ni admitida.

El perfil ligero por defecto, límites de threads/prioridad, cancelación y el
contrato de integración asíncrona están en
[PRIVATE_PREVIEW_EXECUTION.md](PRIVATE_PREVIEW_EXECUTION.md). La ejecución Qt no
bloqueante sigue siendo integración requerida; no se introduce otro scheduler.

## Estado adicional: siete addons privados, 2026-09-08

La ampliación de paquetes instalados y su evidencia real están en
[PRIVATE_ADDON_SUITE_M10_5.md](PRIVATE_ADDON_SUITE_M10_5.md). Siete rutas privadas
ejecutadas no implican producción ni controles Qt conectados. Los schemas de
terceros conservan identidad propia y estado de inspección; no sustituyen ni
rehabilitan la Beauty rechazada de las familias propias v1/v2.

Estado: backend CPU/headless implementado como candidato offline; integración Qt y
transacciones nativas pendientes de sus owners. M10.5 consume los controles sin
importar Blender, Python, addons ni archivos BLEND.

## Ampliación v2 aprobada — 2026-09-08

Cinco entradas adicionales mantienen intactas las siete v1:
`sedimentary_strata_v2`, `fractured_boulder_v2`, `columnar_v2`,
`rounded_weathered_v2`, `cave_cliff_shell_v2`. Sus IDs semánticos usan `/v2` y
`generatorVersion=2.0.0`. No se migran silenciosamente recetas v1. Las nuevas
definiciones están en `rock_library_v2.py`, agregadas por `node_library.py` al
catálogo existente. Perfiles de resolución: `rock-preview-v2` y `rock-final-v2`.

El front sigue persistiendo `layers[].parameters.<id>` y solicita un nuevo job.
`FacetInset`, los controles de fractura y los de columnas construyen helpers;
modificar solamente un socket RNA en un modifier existente no reconstruye esos
helpers. `binding.rebuildCandidate=true` es obligatorio. Los sockets describen el
job completo, no prometen evaluación interactiva parcial de todos sus cambios.

El Layer System, selección por source GUID, máscaras heredadas, spline destructivo
y decal3d usan la ruta existente; v2 no crea un segundo sistema de layers. La
selección de loops/vértices no se implementa aquí ni se simula mediante un checkbox.
El host BSP aporta las cavidades y keep-clear; `cave_cliff_shell_v2` no abre el
rectángulo fijo que usaba la fixture v1.

`rock_preview_material.py` es un shader procedural propio para comparar imágenes.
No equivale a material runtime, UV final ni normal bake high→low. Los nodos pagados
pueden seguir siendo referencias/plugins privados, identificados como tales; no se
publican bajo los IDs `jerico.*` ni se convierten en datos propios por renombrarlos.

## 1. Autoridad y fuente de datos

`node_library.py` es la única fuente de identidad, versión, panels, controles,
unidades, defaults, límites, dependencias, masks y composición. El backend expone
esa información por dos rutas equivalentes:

- `transformer catalogue`, sin iniciar Blender, para poblar herramientas;
- `output/generator-schemas.json`, reconciliado con identifiers RNA reales durante
  un job Blender 5.2.1.

M10.5 usa `generatorId` como identidad semántica, `toolId` para crear la receta y
`runtimeIdentifier` sólo como diagnóstico. Nunca persiste orden, nombre visible o
`Socket_N` de Blender como autoridad.

## 2. Modelo Qt-neutral

```text
GeneratorToolModel
  generatorId, toolId, version, label, status
  panels[], controls[], composition[], outputs, constraints
  actions.preview/cancel/apply

GeneratorControlModel
  id, label, description, type, widget, unit
  default, minimum, maximum, options[]
  panel, order, dependencies[]
  binding.recipePath, binding.target, binding.socket
```

| `type` | `widget` | Editor M10.5 |
| --- | --- | --- |
| `boolean` | `check` | CheckBox. |
| `integer` | `spin` | SpinBox con límites enteros. |
| `number` | `scalar` | SpinBox/slider; se persiste el número, no la posición visual. |
| `vector3` | `vector3` | Tres componentes; `direction` se normaliza en backend. |
| `points` | `spline-points` | Modelo ordenado y gizmo existente; no string editable. |
| `enum` | `choice` | Combo por option `id`; label sólo presentación. |

Panels se ordenan por `panels[].order`; controles por `controls[].order`. El front
no reagrupa alfabéticamente. `collapsed` es presentación, no dato de receta.

## 3. Binding exacto

Al cambiar un control, el front escribe `layers[index].parameters[control.id]`.
`binding.target=node-socket` se reconcilia con el socket del modifier.
`binding.target=job-data` cubre estructuras como puntos de spline, que Python
convierte en geometría auxiliar antes de evaluar GN.

El front valida tipo y límites para respuesta inmediata; el backend vuelve a
validar y es autoridad final. Dependencias no eliminan datos. `equals` gobierna
visibilidad/enabled y `maximumFactor` expresa restricciones cruzadas, por ejemplo
`Undercut <= 0.45 * Spacing`.

## 4. Layer System y selección loop

El editor mantiene layers ordenados. Reordenar, habilitar, duplicar o editar crea
un draft; no modifica una escena Blender viva.

```text
LayerRow
  id                  GUID estable
  generatorId         workerGeneratorKey
  enabled
  selection.sourceIds GUIDs BSP/mesh/curve
  parameters          valores por control.id
```

Cada resultado conserva input/output geometry hash, semantic hash, seed, selection
hash, schema hash, bounds y masks. Preview/Diff compara esas identidades con el
draft. Un generation ticket stale nunca puede aplicarse.

La selección “como loop” usa el backend `jerico.edge-selection`. M10.5 puede
encadenarlo con Decal3D o spline, conservando edge/chain IDs; no infiere loops desde
el widget.

## 5. Preview, cancelación y Apply

1. M10.5 solicita catalogue y crea controles desde schema.
2. El usuario edita un draft local validado.
3. Preview serializa snapshot BSP + recipe + pin + budgets.
4. Una nueva edición cancela el preview anterior y espera un único terminal.
5. El backend devuelve candidatos GLB/sidecars, schemas, masks, LOD, collision y
   diagnostics, sin publicar assets.
6. Diff presenta sources/layers/regiones cambiadas y keep-clear violations.
7. Apply entrega candidato + generation ticket a `ProjectTransactionCoordinator`.
8. Undo/Redo/Recover son transacciones nativas, no operaciones Blender ocultas.

Lifecycle, admission y cache se integran mediante
`VerifyBlenderJobLifecycleAndCancellation`; aquí no se crea scheduler, DDC o journal.

## 6. BSP y keep-clear

El front visualiza, pero no puede desactivar silenciosamente:

```text
writeMask = Selection AND AllowedMask AND NOT ProtectedMask AND ClearanceOk
```

La malla evaluada final se valida otra vez. Así se detecta una instancia o
displacement cuyo origen era válido pero cuyo volumen invade espacio jugable,
navegación, portal, cámara o región protegida.

## 7. Errores

`generator.schema_invalid` y `generator.schema_mismatch` bloquean Preview e indican
generator/version/socket. Los errores de valor señalan layer/control. Keep-clear
selecciona la región afectada. Budget, cancel y version mismatch no dejan resultado
aplicable; el front conserva el draft y el último candidato válido.

## 8. Ejemplos y plugins

- `examples/generator-schema.json`: contrato completo de boulder fracturado.
- `examples/layered-geology-recipe.json`: strata + weathering + Decal3D sólo atributos.

Los plugins/adapters sólo aparecen si el backend los registró con manifest,
licencia y compatibilidad aceptadas. Depositar un archivo en la carpeta privada no
lo expone automáticamente.

## 9. Límite de M10.5 y prueba visual

M10.5 implementa modelos, delegates/widgets, acciones y visualización. No ejecuta
`bpy`, descubre addons, abre BLEND, interpreta graphs, cambia seeds ni publica.

La aceptación visual conserva Beauty, wireframe, silueta sin material y máscaras a
resolución original para cada generador, con seed, camera, graph digest,
geometry/semantic hash, LOD y keep-clear en el manifest. Sin esas imágenes el
backend no se declara visualmente aceptado.
