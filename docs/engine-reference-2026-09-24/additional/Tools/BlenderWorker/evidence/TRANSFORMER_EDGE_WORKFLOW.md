# Orden combinado de selección, hendiduras y longitud

Estado local: **PASS del contrato CPU acotado**. Addons completos, integración
Editor/cook/runtime y M8: **PARTIAL**.

Estado inicial: RED, cinco errores por operación ausente en seis métodos.
El sexto ya confirma el candidato previo de `edge-decals` contra una fixture
retenida antes de separar la selección de hendiduras. Log: `transformer-edge-workflow-red.log`.

Segundo RED: siete métodos, uno falla por perfiles todavía ausentes, en
`transformer-edge-workflow-profiles-red.log`. La verificación final ejecutó
**83 métodos PASS en 2.569 s**, incluidos los siete del flujo; log común
`transformer-expanded-conformance.log`. La primera tanda focal, antes de añadir
el caso de perfiles, conserva seis métodos PASS en 0.062 s en
`transformer-edge-workflow-green.log`.

Implementación: `sha256:bc5bf594f51ea99c7f52486e555e601328431b27bc61964a97ff5eea1e7e10a8`.
Raíz geológica intacta: `sha256:e9ebc0058092d185a27389f033d5f78e0b537eda13299ef9309c8af2b330d289`.
Run: `../_local/operations-d3b756ed-400d-453a-9567-c509a088d092/`.
Los perfiles manual y automático se convirtieron desde una lectura AST fresca de
Edge Decal 1.2.1 bajo el hash revisado. Diez procesos de generación por caso
produjeron resultados idénticos byte a byte. La auditoría Python prohíbe lecturas
de referencias en cada proceso CLI; no es sandbox OS ni prueba de rendimiento.

| Caso real del replay | Resultado |
| --- | --- |
| Manual, cadena 0.06 m, mínimo 0.05 m por cadena | AO ejecuta 24 rayos y elimina el centro. El mínimo descarta dos tramos de 0.02 m; salida vacía correcta. |
| Automático, cadena 0.60 m, mínimo 0.15 m por arista | 24 rayos AO, centro eliminado, laterales admitidas: 2 fragmentos y 0.40 m de bandas. |

Los tests también bajan la distancia AO a 1e-12 m: ninguna arista se elimina,
la cadena vuelve a superar el mínimo y se genera geometría. `rayCount=24` prueba
que AO se ejecuta una sola vez en ese caso, no antes y después del mínimo.
La comparación con el candidato previo incluye geometría, UV, procedencia,
métricas, receta e informes/hash semántico. Sólo excluye `implementationHash` y
`jobKey`, que dependen de la nueva identidad de módulos. La fixture se obtuvo del
run anterior `operations-7847bf8a-acd1-4e20-b30c-efc55f1a7ea4/crevices/` antes del refactor.

## Auditoría anterior a la implementación

Fuente triangulada + parámetros + visibilidad → selección inicial sin mínimo
→ hendiduras/AO → mínimo por arista/cadena sobre lo retenido → fragmentación,
taper, bandas y UV. La intención es reutilizar los módulos ya probados mediante
una operación compuesta explícita, conservando recetas/versiones/defaults previos.
No se necesita un sistema general de grafos ni imports de addons.

Prueba observable: cadena original de 0.06 m, arista central cóncava y laterales
convexas de 0.02 m cada una. El mínimo de 0.05 m acepta la cadena original; tras
quitar el centro, debe rechazar ambos tramos. No basta filtrar longitud primero.

| Área / consumidor | Disposición |
| --- | --- |
| Fuente y CPU | Malla, IDs de fuente/cara/material, revisión y coordenadas en metros conservados; no mutación de escena. |
| Linaje/serialización/debug | Receta compuesta, hashes encadenados, parámetros efectivos, métricas/cadenas/rechazos por etapa y procedencia de las caras finales. No duplicar mallas intermedias en cada etapa. |
| Cancelación y presupuesto | Reutilizar límites/checkpoints de selección, raycast y generación; AO una sola vez por evaluación. |
| Editor/UI/transacciones/save-load/eventos | INTEGRATION REQUIRED: broker/Editor M10.5 consume receta/resultado compuesto, conserva su linaje y ofrece preview/diff/Apply/Undo/Redo; la operación offline no lo implementa. |
| Asset pipeline/cook/runtime/materiales/render/shaders | INTEGRATION REQUIRED: mismos candidatos de superficie abierta y contrato del generador existente, por cooker y owners de materiales/Editor. Sin nueva ruta Shipping. |
| GPU/física/ECS/Luau/animación/Sequencer/nav/audio/partículas | Sin nuevo consumidor directo de los filtros offline. La admisión posterior de bandas conserva sus límites existentes; no hay sólido ni colisión nueva. |
| LOD/streaming/destrucción | Dependen del asset final cocinado; no de ejecutar este flujo en runtime. No se declara integración por producir JSON. |
| Pruebas/rendimiento | RED/verde, compatibilidad del candidato previo, orden real AO→mínimo, modos/visibilidad, hashes, determinismo, cancelación y procesos limpios. Sin afirmación de rendimiento interactivo. |

La referencia revisada aplica hendiduras antes del filtro mínimo
(`geometry.py` 4944–5007 y rutas de generación ya estudiadas en
`TRANSFORMER_SELECTION.md`). Corregir el orden no reproduce su algoritmo AO,
agrupación por curvatura, seguimiento de quads, centerlines ni solución de uniones.

## Auditoría posterior

Las tres etapas registran parámetros efectivos y hashes encadenados. INPUT conserva
selección/visibilidad normalizadas por hash, métricas y procedencia; CREVICE conserva
aristas retenidas/rechazadas y mediciones; LENGTH conserva las cadenas que quedaron
y los rechazos por longitud. El resultado final conserva `sourceHash` original,
`selectedSourceHash` final y caras/aristas/fragmentos/UV de generación. Las mallas
intermedias no se duplican en el informe.

Se añadieron un módulo compuesto y perfiles/descriptores en tablas existentes;
el filtro de hendiduras se extrajo como función y el wrapper anterior conserva su
comportamiento. No hay un framework general de pipelines ni registro de código
externo. UI_CONTROLS.md §3.0f y ADDON_TEMPLATE.md documentan los 19 controles
reutilizados y sus bindings; los informes generados incluyen defaults por perfil.
Las integraciones de la tabla anterior permanecen explícitamente pendientes.
No se ejecutó Blender, GPU ni build, ni se modificó Git o archivos fuera de la carpeta.

Revisión READ_ONLY final de flujo/filtro/registro/perfiles/pruebas: sin defectos
P1/P2 materiales. Alcance de revisión limitado a este paquete; no es admisión
de todos los addons, de la suite completa ni de M8.
