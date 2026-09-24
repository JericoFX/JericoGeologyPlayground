# Catálogo procedural multifunción — investigación para el backend

Fecha: 2026-09-10. Estado: **diseño investigado, no generadores admitidos**.

Pedido adicional del usuario: después de las familias geológicas, investigar
puertas con/sin ventana y con pestillo, ventanas/vidrios, maderas y pisos variados,
rotaciones, roturas, concreto, columnas y assets comunes. El alcance sigue siendo
Tools/BlenderWorker: no modifica Engine, Editor, física, navegación ni gameplay.
JericoEditor será el único front en M10.5; Blender construye/evalúa geometría.

Los IDs de generador de este documento son propuestas, no entradas registradas.
Se complementa con GEOMETRY_NODES_CAPABILITY_ATLAS.md, GEOLOGY_FAMILY_RESEARCH.md
y los contratos existentes de cook/runtime. No los sustituye ni declara M8 cerrado.

## 1. Decisión de construcción

Crear grupos propios con nodos nativos cuando el problema sea un perfil,
extrusión, distribución de piezas, deformación o composición acotada. Usar bpy
para construir esos grafos, preparar datos/atributos y extraer resultados. Un
operador de add-on dependiente de contexto no se convierte automáticamente en
un generador headless por ejecutarlo en background.

El modificador Geometry Nodes expone los inputs del grupo; la interfaz bpy
permite inspeccionar identificadores, tipos y descripciones. Jerico debe traducir
esa interfaz a un GeneratorSchema versionado, con unidades, dependencias,
identidad de control y estados no soportados explícitos. No debe reconstruir la
interfaz completa de Blender ni depender del orden visible de sockets.
[Geometry Nodes Modifier](https://docs.blender.org/manual/en/5.0/modeling/modifiers/geometry_nodes.html),
[NodeTreeInterfaceSocket](https://docs.blender.org/api/current/bpy.types.NodeTreeInterfaceSocket.html).

Separar cuatro productos de autoría:

| Producto | Construcción | Identidad que conserva |
|---|---|---|
| Surface layer | Deformación/máscaras de una superficie existente | Source face IDs, UV, región y layer |
| Solid part | Perfil/extrusión/boolean dentro de volumen autorizado | Part GUID, origen de caras y operación |
| Assembly | Piezas independientes con jerarquía y pivots | Parent/child GUID y transformaciones locales |
| Scatter | Instancias opt-in dentro de una región autorizada | Instance GUID, candidato, seed y ancla |

Una puerta articulada **no** debe convertirse en una malla fusionada con su marco.
Una pared geológica de layers sí puede conservar una superficie continua. Son
contratos diferentes, aunque ambos utilicen Geometry Nodes.

## 2. Biblioteca nativa propuesta

| Grupo/generador propuesto | Construcción concreta | Controles principales |
|---|---|---|
| `jerico.arch.door/v1` | Contorno de apertura → perfiles de marco/tapajuntas → hoja por paneles extruidos → bisagras/pestillo separados → vidrio opcional | Width, Height, Thickness, LeafCount, PanelLayout, WindowEnabled, WindowBounds, SwingSide, HingeCount, LatchHeight, PreviewAngle, Seed |
| `jerico.arch.window/v1` | Contorno → marco y montantes → alféizar → paneles de vidrio separados | Width, Height, FrameDepth, MullionRows/Columns, SillDepth, PaneThickness, DoublePaneGap, OpeningMode, DamageVariant |
| `jerico.surface.wood-plank-floor/v1` | Footprint → patrón en coordenadas métricas → instancias de tablas → recorte del borde → realización tardía | BoardLength/Width/Thickness, Gap, Pattern, RowOffset, PatternRotation, VariationRotation, Warp, ChipDepth, MissingBoardRatio, Seed |
| `jerico.arch.concrete-kit/v1` | Perfil rectangular/circular/poligonal → extrusión → biseles → superficies de daño explícitas | Width, Depth, Height, Profile, Taper, Base/Capital, Bevel, AggregateScale, DamageRegion, Seed |
| `jerico.props.container/v1` | Caja/palet/barril/estante compuesto de tablas, aros, postes y fijaciones | Dimensions, SlatCount, LidMode, FastenerStyle, DamageVariant, Seed |
| `jerico.arch.trim-profile/v1` | Curva autorada → perfil barrido → ingletes/contactos | ProfileId, Width, Depth, CornerPolicy, EndCap, MaterialRegion |
| `jerico.props.modular-kit/v1` | Ensamble de piezas aprobadas por sockets de conexión | KitId, Layout, PartVariant, Dimensions, AttachmentMode, Seed |

Primero validar una puerta simple con marco, hoja y bisagra; después agregar
ventana, pestillo y variantes. Primero un piso rectangular con IDs estables;
después footprint irregular, patrones y roturas. No construir un mega-grupo con
cientos de sockets sin dependencias y validación por familia.

### Puertas y ventanas

Los perfiles pueden barrerse con Curve to Mesh; curvas con perfil cíclico y caps
permiten una salida cerrada. La transferencia de atributos existe, pero debe
probarse en la versión fijada y después de cada realización/boolean.
[Curve to Mesh](https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/curve/operations/curve_to_mesh.html).

El marco, hoja, vidrio, manija, pestillo, bisagras y umbral tienen Part IDs
separados. WindowEnabled elimina/crea un componente por receta, no cambia la
identidad del resto del ensamble. La abertura de vidrio se corta en la hoja antes
del bake; no es un rectángulo transparente encima de una hoja opaca.

PreviewAngle sólo cambia la pose de inspección. Exportar closed/rest pose y pivot
canónicos por separado. Rotate Instances admite un pivot explícito y sirve para
esa preview; no implementa la articulación de Jerico.
[Rotate Instances](https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/instances/rotate_instances.html).

Pestillo: geometría independiente, punto de enganche en la hoja y receptor en el
marco. Emitir `latch_closed_transform`, dirección de carrera y recorrido máximo
como candidatos de autoría; no inventar reglas de llave, interacción, bloqueo o
rotura. Esas reglas pertenecen a los sistemas de Jerico.

### Madera y pisos

Un piso usa un footprint y una orientación autorados. La rotación del patrón
transforma el sistema de colocación antes del recorte; la variación por tabla
afecta la pieza dentro del presupuesto de separación. No aplicar rotación aleatoria
sin volver a comprobar solapes, huecos y borde del footprint.

Patrones iniciales: tablones alineados, juntas alternadas, herringbone y cuadrados
por módulos. Cada patrón define coordenadas enteras y una regla de identidad:

```text
board_guid = stable_hash(asset_guid, region_guid, pattern_version, tile_x, tile_y, slot)
variation_seed = stable_hash(board_guid, authored_seed, variation_channel)
```

Separar canales de seed para longitud, rotación, alabeo, nudos, desgaste y daño.
Cambiar el color o roughness no debe recolocar tablas. Cambiar una tabla no debe
renumerar todo el piso. `Index` no es una identidad durable.

Instance on Points permite rotación/escala y selección de instancias; Realize
Instances habilita operaciones sobre geometría real pero puede aumentar el costo.
Mantener instancias hasta que export/boolean/LOD/collision requiera realizarlas.
[Instance on Points](https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/instances/instance_on_points.html),
[Realize Instances](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/instances/realize_instances.html).

Daño geométrico: borde astillado, extremo partido, tabla ausente, alabeo y grieta
seleccionada. Material masks: veta, testa, corte fresco, humedad, pintura y
desgaste. La veta sigue el eje longitudinal local de cada pieza; la testa usa su
propio dominio. No estirar una textura mundial indistintamente sobre todas las
orientaciones. Las caras nuevas de una rotura necesitan UV y procedencia nuevos.

### Concreto, columnas y assets comunes

Una columna arquitectónica usa sección, altura, taper, base/capital y anclajes;
no reutiliza un basalt columnar preset como estructura edilicia. Concreto roto
separa superficie exterior, interior/árido y armadura opcional. La armadura sólo
se crea cuando está solicitada; no implica una simulación estructural.

Kits comunes reutilizan perfiles y ensambles: cajas, palets, estantes, barriles,
vigas, escaleras, barandas, conductos, zócalos, marcos y mobiliario modular. Cada
familia debe pasar su prueba geométrica mínima antes de aparecer como disponible.
Un catálogo con nombres y miniaturas no acredita un backend operativo.

## 3. Estalactitas, estalagmitas y hielo

Las estalactitas crecen desde el techo; las estalagmitas son depósitos del suelo,
habitualmente más anchos y con cima redondeada. Una columna de cueva requiere el
encuentro de ambas. Flowstone es depósito en capas por agua que fluye.
[NPS Speleothems](https://www.nps.gov/subjects/caves/speleothems.htm).

Ruta nativa recomendada:

```text
ancla autorada (GUID, face ID, barycentric, frame, lado)
→ curva orientada por gravedad + desviación controlada
→ remuestreo y parámetro longitudinal
→ perfil de radio por familia + tilt
→ Curve to Mesh con caps
→ contacto/union explícita con el host
→ IDs de formación, masks, UV y validación de espacio protegido
```

Estalactita: base adherida, estrechamiento acusado, asimetría y opción separada de
soda straw hueca. Estalagmita: base amplia y perfil de crecimiento redondeado;
no basta invertir un cono. Carámbano: perfil de hielo más delgado, alargado y con
otro control de fusión/estrías. Cortina y cascada: perfil/ribbon con espesor,
no una nube de conos distribuidos automáticamente.

El experimento actual añade sweeps sólidos nativos sobre anclas de fixture; eso
no acredita todavía entrada de anclas GUID desde BSP ni una cueva completa.

Una cueva rocosa con hielo conserva el host rocoso y la acumulación de hielo como
dominios distintos. Una glacier cave tiene host de hielo. No se aceptará un cambio
de color azul como demostración de ambas familias.
[NPS Ice Caves](https://www.nps.gov/subjects/caves/ice-caves.htm),
[NPS Glacier Ice Caves](https://www.nps.gov/articles/000/glacier-ice-caves.htm).

## 4. Física, destrucción y navegación: frontera obligatoria

Blender entrega candidatos: piezas, pivots, ejes, límites, material físico lógico,
collision proxies y procedencia. Jerico valida/cocina y crea cuerpos, constraints,
interacciones, navegación y destrucción. No trasladar una simulación bpy o GN al
Shipping ni afirmar que un mesh exportado tiene física funcional en Jerico.

La hinge de Blender sirve como referencia conceptual de una articulación de un
grado de libertad y límites angulares, no como serialización runtime de Jerico.
[Hinge constraint](https://docs.blender.org/manual/en/latest/physics/rigid_body/constraints/types/hinge.html).

Ejemplo de sidecar **propuesto**, sin IDs de catálogo inventados como admitidos:

```json
{
  "schema": "jerico.procedural-assembly-candidate/1",
  "status": "NOT_COOK_ADMITTED",
  "assetGuid": "a8ce213d-6f74-4bc4-96ef-052c2b81c278",
  "units": "metres",
  "frame": "support-local-right-handed-z-up",
  "parts": [
    {
      "partId": "frame",
      "parentPartId": null,
      "physicalMaterialKey": "wood.dry",
      "bodyIntent": "static",
      "collisionIntent": "compound-convex"
    },
    {
      "partId": "leaf-left",
      "parentPartId": "frame",
      "physicalMaterialKey": "wood.dry",
      "bodyIntent": "movable",
      "restTransform": {"translation": [0, 0, 0], "rotationXyzw": [0, 0, 0, 1]},
      "pivotLocal": [0, 0, 1.05],
      "axisLocal": [0, 0, 1],
      "angularLimitsRadians": [0, 1.5707963267948966],
      "collisionIntent": "convex",
      "latchSocketId": "latch-left"
    }
  ],
  "runtimeAuthority": "Jerico-cook-and-runtime",
  "constraintCreated": false
}
```

Las claves de material son ejemplos lógicos, no registros existentes verificados.
El owner debe resolverlas contra materials/physical materials y los contratos
PWAS/EPLS vigentes. Una hoja móvil no usa automáticamente una trimesh cóncava como
collision dinámica. Un vidrio necesita política explícita de colisión y rotura,
distinta de su transmisión visual. Una puerta abierta/cerrada puede afectar
navegación, pero ese cambio se resuelve en Jerico, no recocinando ruido en runtime.

Destructive spline y 3D decal reutilizan selección, máscara, frame, perfil y
procedencia. La operación final debe declarar si es relieve, corte, depósito o
ensamble. No reutilizar transferencias por índice después de un boolean/remesh.
Después de cambiar topología, recocinar UV, masks, collision y LOD desde la nueva
superficie canónica de alta resolución según el contrato existente.

## 5. Add-ons gratuitos: referencias y decisión de intake

No se descargó ni ejecutó ninguno de estos paquetes durante esta investigación.
No se encontraron copias de estos cuatro bajo Tools/BlenderWorker. La existencia
de una página no demuestra compatibilidad headless con el pin local 5.2.1.

| Referencia | Evidencia primaria | Uso propuesto | Estado de compatibilidad |
|---|---|---|---|
| Archimesh | [Código oficial archivado](https://github.com/blender/blender-addons/tree/main/archimesh), GPL-2.0-or-later en el código | Estudiar composición de puertas, ventanas, habitaciones y columnas | No validado en 5.2.1; operadores/contexto requieren adaptación |
| Archipack histórico gratuito | [Repositorio del autor](https://github.com/s-leger/archipack), GPL-3.0, orientado a Blender 2.78/2.79 | Referencia histórica de arquitectura paramétrica | Rechazado como dependencia directa del pin; no confundir con otras ediciones comerciales |
| Building Tools | [Repositorio del autor](https://github.com/ranjian0/building_tools), MIT, declara Blender 4.0 | Estudiar generación modular de edificios/aberturas | No validado en 5.2.1 ni headless |
| Cell Fracture | [Extensions](https://extensions.blender.org/add-ons/cell-fracture/), [código histórico oficial](https://github.com/blender/blender-addons/tree/main/object_fracture_cell) | Candidato a adaptación offline con semillas, bounds y IDs controlados | Paquete actual declara Blender 4.2+; requiere prueba local exacta, no garantiza 5.2.1 |

Cell Fracture no es un grupo Geometry Nodes: crea fragmentos/objetos y contiene
supuestos de contexto. Su salida no es destrucción runtime. La licencia del
paquete actual puede diferir de la del código histórico; conservar licencia y
hash de la distribución realmente auditada, no copiar una etiqueta de otra versión.

Antes de cualquier intake: inventario de archivos y hashes; revisión estática de
scripts, operaciones de archivos/red/subprocesos, handlers y auto-run; manifiesto
de dependencias y licencias; extracción segura; evaluación aislada; prueba de
repetición; límites de recursos; interface schema; salida GLB/sidecar verificable.
“Comprado” o “gratuito” no es un resultado de auditoría ni compatibilidad.

El código nuevo debe registrar su procedencia. Si se incorpora código GPL, mantener
sus avisos y obligaciones aplicables; no cambiar nombres para presentarlo como
independiente. Revisar por separado los derechos de assets incluidos. La política
de Blender distingue el programa de las obras producidas; las Extensions tienen
reglas adicionales de licencias.
[Blender License](https://www.blender.org/about/license/),
[Extension licenses](https://docs.blender.org/manual/en/latest/advanced/extensions/licenses.html).

## 6. Conexión al front y pruebas de aceptación

Jerico conserva la receta serializada y el stack de layers/parts. Selecciones de
caras, loops y regiones se convierten a IDs/atributos/máscaras por una operación
de autoría. Blender no elige qué espacio pertenece al jugador ni infiere gameplay.
La preview de baja resolución y el cook final usan la misma receta, coordenadas,
anclas y semillas; sólo cambia el presupuesto de evaluación aprobado.

Para una familia nueva, exigir en este orden:

1. Validación de schema: tipos, unidades, rangos, enums, dependencias, versión y
   rechazo de controles no soportados; sin crear datablocks ante receta inválida.
2. Prueba geométrica real: dimensiones, huecos, piezas, pivots, orientación,
   densidad y bordes; no colores como única diferencia entre variantes.
3. Estabilidad: seeds repetibles, edición local sin renumerar el ensamble,
   preservación de source IDs o procedencia nueva cuando cambia la topología.
4. Material/UV: seams, densidad de texel, interior/corte/veta/testa separados,
   vidrio con espesor/política explícita y bake high→low por pieza.
5. LOD: deriva de la misma forma canónica, conserva silueta, huecos, pivots y
   material/part boundaries; mide error, no promete igualdad exacta con menos caras.
6. Collision/destruction: candidatos cerrados y acotados, IDs estables, validación
   por cooker existente, sin usar LOD visual como collision implícita.
7. Protocolo: cancelación, worker restart, límites, cache key completa y ausencia
   de resultados parciales publicados.
8. Integración por owner: Jerico cook/runtime, materiales físicos, PWAS/EPLS,
   navegación y Shipping source-free; ninguna de estas pruebas se sustituye por
   el render de Blender.

Orden recomendado posterior a geología: puerta simple articulable → ventana/vidrio
→ piso de madera → concreto/columnas → kits comunes → variantes de daño. Este es
el orden de implementación propuesto, no una alteración unilateral del roadmap.
