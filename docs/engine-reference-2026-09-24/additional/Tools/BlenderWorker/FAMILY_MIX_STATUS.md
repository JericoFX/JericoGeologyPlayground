# Familias geológicas y Layer System — estado verificable

2026-09-10. **Experimento funcional de backend; no admisión de producción.**

La propuesta del usuario está implementada en su parte geométrica: una receta
elige familia, región y controles; Blender construye un grafo nativo y devuelve
una sola superficie mezclada. JericoEditor no fue modificado. No se afirma que
todas las familias planeadas estén terminadas, que el front esté conectado ni
que el resultado esté admitido por cook/runtime.

[Galería local con imágenes originales](FAMILY_GALLERY.html).
[Investigación del catálogo posterior de assets](PROCEDURAL_ASSET_CATALOG_RESEARCH.md).

## 1. Lo que existe en código

| Archivo | Responsabilidad |
|---|---|
| `surface_relief_nodes.py` | Campo base nativo v2 y descriptores únicos de sus controles |
| `surface_relief_presets.py` | Cuatro configuraciones anteriores: weathered, stratified, fractured, ice_wall; preservadas |
| `surface_family_nodes.py` | Catorce perfiles nuevos y validación de controles antes de crear nodos |
| `surface_family_mix.py` | Grafo de mezcla regional sobre la misma topología y aplicación/reconstrucción con bindings nuevos |
| `structural_family_nodes.py` | Seis formas sólidas experimentales, boolean EXACT, feature IDs y procedencia de caras |
| `tests/probe_family_gallery.py` | Evaluación real, superficies/soporte curvo/sólidos, imágenes y payload completo |
| `tests/probe_surface_relief.py` | Supervisor existente, pin, background/offline/no-autoexec, threads y deadline |
| `tests/compare_family_captures.py` | Verificación de payloads, dependencias actuales y secuencia de procesos |
| `tests/test_family_evidence.py` | Negativos del comparador sin lanzar Blender |

Todos estos grupos usan nodos nativos construidos por nuestro código. No cargan
datablocks ni código de los add-ons de referencia. Las capturas verifican ausencia
de librerías externas. Esto no cambia el estado de admisión del worker completo.

## 2. Contrato de layers implementado

```json
[
  {
    "id": "strata",
    "style": "stratified",
    "mask": "jrc_region_strata",
    "values": {"Seed": 17, "Depth": 0.6, "Spacing": 1.0, "StrataAxis": [0, 0, 1]}
  },
  {
    "id": "dissolution",
    "style": "karst",
    "mask": "jrc_region_karst",
    "values": {"Seed": 23, "Depth": 0.5, "Stretch": 1.8, "StrataAxis": [0, 0, 1]}
  },
  {
    "id": "ice",
    "style": "glacial_ice",
    "mask": "jrc_region_ice",
    "values": {"Seed": 41, "Depth": 0.4, "Stretch": 3.0, "StrataAxis": [0, 0, 1]}
  }
]
```

El ejemplo supone una pared XZ cuyo eje Z es tangente al soporte. Las coordenadas
son métricas locales. No usar indiscriminadamente el mismo frame en cualquier
superficie: los perfiles nuevos utilizan un frame superficial y requieren un
eje tangente adecuado; no hay reparación automática general de seams/polos.

Entrada acotada a 1–8 layers; IDs únicos, estilos conocidos, máscaras con nombres
válidos. Controles escalares/vectoriales finitos y dentro de rango; Seed entero;
axis no nulo. Se rechazan nombres de entrada que colisionen con cualquier salida
reservada de los layers de la receta. Una máscara presente debe ser FLOAT/POINT y finita. Máscara ausente
equivale a influencia cero. Pesos finitos fuera de 0–1 se saturan en el grafo.

Cada rama recibe **la geometría original**, no la salida deformada de la anterior.
Sample Index obtiene posiciones y masks de cada rama en POINT; por ello este
compositor acepta exclusivamente familias que preservan topología y orden.

```text
w_i = clamp(mask_i, 0, 1)
n_i = w_i / max(1, sum(w))
P_final = P_source + sum(n_i * (P_family_i - P_source))
```

Con suma menor que uno queda parte de la base. Con solape mayor que uno no se
acumula desplazamiento sin límite. El resultado es conmutativo; reordenar layers
no crea una pila destructiva. Esto **no** implementa todavía modos ordered-add,
subtract, boolean, erosion-after-deposition ni clipping BSP general.

Las siete máscaras por layer son `macro`, `strata`, `cavity`, `height`, `joint`,
`resistance`, `erosion`, bajo `jrc_<layer-id>_<suffix>`. Se multiplican por el peso
normalizado de su layer. Son campos artísticos/procedurales, no una clasificación
física calibrada ni Material IDs admitidos. El namespace de salida pertenece al
layer; el integrador debe reservarlo y no usarlo para datos ajenos.

### Aplicación segura en Blender

```python
from surface_family_mix import apply_surface_mix

modifier = apply_surface_mix(source_object, recipe_layers)
# Después de editar la receta, reconstruir mediante el mismo punto de entrada:
modifier = apply_surface_mix(source_object, revised_layers, replace_modifier=modifier)
```

`build_surface_mix()` sólo construye un grafo separado. **No reasignar su resultado
a un modificador existente**: Blender puede conservar valores de antiguos socket
identifiers bajo otro layer. El helper crea wrapper/bindings/modificador nuevos y
sustituye únicamente el modificador indicado. Preserva los demás modificadores.
No implementa Undo/Redo de Jerico ni rollback completo de errores internos bpy.

El helper valida las máscaras de `obj.data`, no una cadena arbitraria de
modificadores anteriores. El objeto de entrada del job debe ser el soporte
materializado que Jerico autorizó. Los grupos temporales sobreviven dentro del
proceso Blender hasta terminar el job; no se certificó aún un proceso persistente
con reconstrucciones ilimitadas y recolección de todos los datablocks huérfanos.

## 3. Conexión al front en M10.5

Jerico guarda la receta, selección y versiones. Una selección de loop/caras/región
es una operación de autoría de Jerico: debe resolverse a source IDs y una máscara
POINT con falloff/borde protegido antes de enviar el job. Blender no decide por
ruido cuál es el loop, cuál es el espacio jugable ni qué cara puede reemplazar.

El grupo mixto contiene paneles por layer y sockets reales como `strata.Depth`,
`strata.Spacing` o `ice.Stretch`. `interface.json` se obtiene mediante el lector
existente `transformer.blend_probe.describe`, con identifier, tipo, panel,
descripción, default y límites de cada socket. En la mezcla capturada hay tres
ramas y una salida geométrica, no tres objetos superpuestos.

Regla del adaptador futuro:

| Dato autoritativo | Binding |
|---|---|
| Layer ID + nombre semántico de control | Clave durable guardada por Jerico |
| Socket identifier | Binding transitorio de ese grafo concreto; nunca identidad persistente |
| FLOAT | Control escalar Qt con unidad definida por contrato |
| INT Seed | Entero acotado, no conversión silenciosa de float |
| VECTOR | Tres componentes; eje y escala tienen restricciones distintas |
| PANEL/description/min/max | Agrupación, ayuda y restricciones presentadas en el front |
| Input Geometry/masks | Binding del job, no selector libre de datablocks Blender |

La introspección obtenida **no equivale** a una nueva entrada admitida de
`generator_schema.py`/`node_library.py`. El registro existente exige reconciliación
contra su definición, IDs/versiones, unidades, binding y contratos de salida.
No se añadió un atajo que saltara esa validación. Queda como trabajo de admisión
backend antes de exponerlo como disponible en M10.5. No hay nuevos widgets Qt aquí.

## 4. Cobertura real por ruta

| Ruta | Implementado/mostrado | Límite actual |
|---|---|---|
| Surface base | Weathered, stratified, fractured, ice_wall anteriores | Variantes de un campo base, no cuatro sólidos diferentes |
| Surface nuevo | block_strata, karst, column_ends, rounded_granite, chalk, slate, lava, flowstone, glacial_ice, crevasse_relief, stalactite_relief, stalagmite_relief, icicle_relief, frozen_fall | Catorce perfiles; no todas sus combinaciones/controles están admitidos visualmente |
| Solids | columnar_solid, rounded_boulder, fractured_boulder | Envolvente de fixture fija; fracturas simples, columnas todavía regulares |
| Sweeps unidos al host | stalactite_solid, stalagmite_solid, icicle_solid | Anclas de fixture deterministas; no entrada de anchors GUID desde BSP |
| Mezcla curva | strata/karst/glacial ice en soporte anular cerrado | Fixture de pared/cavidad, no generador de layout de cuevas |
| Talus/scree | Investigación y ruta opt-in definidas | No implementado ni fotografiado como nuevo generador en esta entrega |
| Cueva completa/ice cave | Research y prueba local de paredes/espacio protegido | No red de cuevas, portales, navegación ni hielo físicamente admitido |
| Cortinas/flowstone/frozen fall con volumen | Relieve superficial visible | El sweep/union volumétrico correspondiente aún no está admitido |
| Glacier crevasse abierta | Surcos superficiales | No corte abierto ni simulación glaciar |

No confundir un perfil de pared `rounded_granite` con una roca suelta, ni
`column_ends` con columnas completas. Las capturas clay muestran geometría; los
materiales de preview son opacos y no prueban transmisión/espesor óptico de hielo.

La revisión visual considera fuertes los avances en mezcla, perfiles redondeados
y diferencias techo/suelo. Lava necesita una morfología más reconocible; columnas
y fractured boulder todavía parecen demasiado regulares/simplificados. Esas
observaciones impiden declarar “igual o mejor que todos los add-ons” sin una
comparación visual acordada. No se aprobó esa equivalencia por conteo de tests.

## 5. BSP, espacio protegido y procedencia

El compositor no reemplaza al BSP. En producción, Jerico debe enviar superficie,
IDs/semántica, transformaciones y volumen keep-clear autoritativos. El worker debe
validar la geometría completa después de operaciones geométricas; comprobar sólo
vértices es insuficiente.

La prueba curva nueva triangula la malla evaluada y calcula la distancia mínima
exacta de cada triángulo proyectado al eje longitudinal del cilindro protegido.
El requisito se mantuvo en radio 3 m. El soporte original falló con 2,985554 m:
una cara de un polígono inscrito penetraba aunque sus vértices tuvieran radio 3.
La corrección fue autorar el soporte con radio 3,12 m, no reducir el keep-clear.
`fixtureClearance` conserva requisito y resultado en el payload evaluado.

Esta prueba es válida para ese cilindro; no sustituye la validación general
triángulo/volumen, swept-volume, portales o corredores del BSP real.

Las superficies conservan índices de puntos/caras, UV y `source_face_id`, y dejan
sin mover los puntos con máscara cero. Los sólidos cambian topología: usan
`jrc_feature_id` y `jrc_provenance_kind` (1 generado, 2 cara de envolvente).
Estos enteros son procedencia de fixture, no GUIDs finales de Jerico. El mapeo
canónico high→low/masks/materials/physical material/PWAS/EPLS sigue siendo una
obligación de admisión. No transferir por índice después de boolean/remesh.

Los LOD deben derivarse de esa alta resolución canónica y medir error/silueta;
menos polígonos no pueden garantizar igualdad geométrica exacta. Collision y
navegación se validan por sus propias políticas, no se copian del LOD visual.
Shipping no debe necesitar Blender, .blend ni los add-ons.

## 6. Evidencia y fallos retenidos

Raíz local: `_local/family-mix-20260910/`. No publicar `_local`, binarios ni
referencias privadas automáticamente. Git/publicación pertenecen al owner.

Por captura: `beauty.png`, `scene.blend`, `source-mesh.json`, `evaluated-mesh.json`,
`interface.json`, `render.json`, `invocation.json`, `execution.json` y logs.
Las captures son CPU 512×512, sin normal/bump, no benchmark ni Beauty/runtime
de Jerico. `render.json` conserva hash completo de posiciones, caras, UV, IDs y
masks; el comparador recomputa hashes de los archivos y verifica código actual.

Par actual de mezcla: `tangent-closure-clay` / `tangent-closure-stone`, verificado en
`tangent-closure-comparison.json`. Incluye el fix de namespace, máscaras completas finitas
en [0,1], apariencias clay/stone distintas y comprobación del PNG 512×512 (chunks,
CRC, datos comprimidos y tamaño de scanlines). Hash geométrico:
`sha256:bad190bcfe186972ef695f82437672354ce48c750e49d45742bede129d2f35b3`.
El cambio de material no cambió esa geometría. La repetición es de dos procesos
independientes con el mismo pin y dependencias, no de un resultado reusado en RAM.

Par histórico de fixture curva: `cave-interior-clay` / `cave-interior-stone`; geometría
`sha256:6120660153635b159e6b7372decc18d0bd56bfc66b39ad5fade80e888ac2b75c`.
La imagen de túnel es prueba de soporte/mezcla, no una cueva final aceptada.
Su manifest precede al último fix de namespace/validación; no sustituye al par
`closure` como evidencia de las dependencias finales.

Fallos importantes preservados y corregidos:

- Orden de layers/IDs de sockets: graph replacement directo conservaba defaults.
- Recetas inválidas creaban datablocks antes de validar controles.
- Máscaras FACE/vector/no finitas se aceptaban por conversión implícita.
- Máscaras de entrada con nombres reservados podían ser sobrescritas por la salida.
- Blender 5.2: Resample Curve usa input Menu `Mode="Count"`.
- Blender 5.2: Curve to Mesh necesita su input `Scale`; Set Curve Radius solo
  producía cilindros. El test nuevo mide ancho real de punta y detectó 2 m en vez
  de menos de 0,05 m. Los renders cilíndricos previos no son entregables válidos.
- Keep-clear de vértices no bastaba; se añadió comprobación de triángulos.

Incidente de recursos retenido: `mixed-verified-clay` y `mixed-verified-stone`
tuvieron intervalos de supervisor solapados 0,412356 s. Ambos finalizaron exit 0,
pero **no** acreditan ejecución estrictamente serial ni rendimiento. Se informó
al owner, que ordenó conservar el incidente y repetir dentro del mismo presupuesto.
Los pares `mixed-serial-*` y finalmente `closure-*` son repeticiones seriales; el comparador ahora exige
completion exit 0 e intervalos no solapados.

Actualización final: `apply_surface_mix` rechaza antes de crear nodos los ejes
paralelos, antiparalelos o casi paralelos a normales de puntos seleccionados de
las familias con chart 2D (seno <= 1e-4). La fábrica desacoplada no valida objetos;
su caller debe hacerlo. `tangent-axis-red` conserva tres fallos esperados y
`tangent-axis-green` pasa 12/12. Los puntos sin selección no bloquean el eje.
El nuevo par `tangent-closure-*` es estrictamente serial y conserva el hash
geométrico anterior. No cierra las restantes fronteras de integración.

## 7. Siguiente admisión, sin alterar el roadmap

1. Resolver entradas reales de selección/loop, anchors, transformaciones,
   semántica BSP y keep-clear con los contratos existentes del owner.
2. Registrar/reconciliar los grupos contra GeneratorSchema y el protocolo real
   del worker; no promover archivos de experimento automáticamente.
3. Completar formas pendientes y revisión visual comparativa, incluidos lava,
   fractura, talus, cortinas y cuevas completas.
4. Añadir UV de caras nuevas, bake high→low, transferencia medida de masks/IDs,
   LOD, collision y materiales físicos contra PWAS/EPLS existentes.
5. Probar cache/cancel/recovery y source-free cook/runtime por la ruta canónica.
6. El owner decide admisión/publicación. UI sólo en M10.5.

El pedido posterior de puertas, ventanas, maderas, pisos, concreto y props está
investigado por separado. Las referencias AI de cables, tuberías, telarañas,
rieles, escaleras, cuerdas y cadenas **aún no se recibieron**: el usuario indicó
que las subirá a una carpeta en Git. No se afirma haberlas revisado. La cola de
mensajes no está expuesta por las herramientas disponibles; se revisarán cuando
la app entregue los adjuntos o se comunique la ruta del commit.
