# Familias geológicas: investigación y ruta de nodos propios

Implementación posterior y límites comprobados: [FAMILY_MIX_STATUS.md](FAMILY_MIX_STATUS.md).
Imágenes reales: [FAMILY_GALLERY.html](FAMILY_GALLERY.html). El catálogo posterior
de arquitectura/props está en [PROCEDURAL_ASSET_CATALOG_RESEARCH.md](PROCEDURAL_ASSET_CATALOG_RESEARCH.md).

Actualizado: **2026-09-10**. Investigación de fuentes primarias, inventario del
backend y diseño de implementación. No es admisión ejecutable de todas las
familias, aceptación Beauty global ni modificación del roadmap del Engine.

La continuación actual del usuario favorece aprender de las referencias y
extender nuestros nodos. El experimento vigente utiliza exclusivamente nodos
nativos y código de esta biblioteca: no carga archivos de proveedores. Esto no
revoca su autorización previa de reutilización privada de addons ni las
obligaciones de licencia; `PRIVATE_ADDON_REUSE_DIRECTIVE_20260908.md` conserva ese
contexto. Una compra no convierte código o graphs de terceros en autoría propia.

## 1. Decisión principal: cuatro rutas, no un ruido para todo

La morfología determina la operación. Una roca no se define únicamente por un
material, y erosión no es una familia litológica. Granite, basalt, limestone,
shale y slate describen materiales/estructuras distintas; cliff, boulder y cave
describen disposición espacial; weathered, fractured y glacial describen procesos
o estados. El catálogo debe poder combinarlos sin contar cada combinación como
un generador nuevo.

| Ruta de diseño | Entrada y resultado | Uso correcto | Prohibición |
|---|---|---|---|
| `SURFACE_RELIEF` | Soporte teselado → mismos vértices/caras desplazados, UV/IDs conservables | Pared grande, acantilado adherido, detalle de techo/cueva, decal geométrico | No crear islas, suelo, espesor o bloques ocultos automáticamente |
| `SOLID_VOLUME` | Masa cerrada y envolvente permitida → sólido evaluado, caras nuevas con lineage | Boulder, columnas reales, voladizos, corte kárstico autorizado | No deducir nuevas salas, túneles o espacio jugable desde un preset |
| `CURVE_ATTACHMENT` | Anclas y curvas de autoría → perfiles barridos/adheridos | Estalactitas, estalagmitas, carámbanos, cascadas y flowstone | No colocar formas por todo el techo sin selección/presupuesto |
| `OPT_IN_SCATTER` | Región de depósito permitida → fragmentos/instancias con IDs | Talus, scree, bloques caídos | No activarlo por defecto para hacer que una pared parezca rocosa |

Son categorías de diseño de este informe, no nuevos enums aceptados por el
protocolo. Un grupo de relieve puede decorar las otras tres rutas, pero no las
sustituye. La selección de una ruta distinta debe ser explícita en la receta.
La preocupación del usuario sobre culling se resuelve primero evitando geometría
innecesaria, no generando miles de piezas y confiando en descartarlas después.

## 2. Inventario real y estado de cobertura

Inspección: `node_library.py`, `rock_library_v2.py`, `geology_library.py`,
`surface_relief_nodes.py`, `surface_relief_presets.py`,
`GEOLOGY_FAMILY_IMPLEMENTATION.md` y `GEOMETRY_NODES_CAPABILITY_ATLAS.md`.

| Identidad existente | Qué hay en el catálogo | Relación con esta continuación |
|---|---|---|
| `sedimentary_strata` y su v2 | Familia base de estratos | Relieve `stratified` probado; no admite automáticamente el generador sólido anterior |
| `fractured_boulder` y su v2 | Familia base de juntas/boulder | Preset `fractured` probado sólo sobre soporte, no fragmentación sólida |
| `columnar` y su v2 | Familia base columnar | Diseño de mejora descrito aquí; sin nuevo render/prueba de columnas en esta ventana |
| `rounded_weathered` y su v2 | Familia base redondeada | `weathered` aporta relieve; no demuestra boulder cerrado y redondeado |
| `cave_cliff_shell` y su v2 | Familia combinada de pared/cueva | Fixtures de pared plana y semicilindro, no generación de cueva completa |
| `cellular_fracture` | Interfaz `candidate-unverified` | Celdas y caras reales pendientes de admisión geométrica/visual específica |
| `block_strata` | Interfaz `candidate-unverified` | Camas volumétricas y truncamiento son una ruta diferente del relieve actual |
| `karst_cave_wall` | Interfaz `candidate-unverified` | Controles de flujo/disolución existentes; no se declaran validados por el preset de cueva |
| `glacial_ice_wall` | Interfaz `candidate-unverified` | `ice_wall` experimental aporta dirección/relieve; no cierra esa familia ni hielo volumétrico |

Las cinco versiones v2 son revisiones de cinco familias, **no cinco geologías
adicionales**. `destructive_spline` y `decal3d` son utilidades reutilizables, no
tipos de roca. El atlas conceptual separa cliff y cave-shell; el catálogo actual
mantiene `cave_cliff_shell`: no inventar IDs registrados a partir del atlas.

Esta ventana produjo cuatro recetas propias: `weathered`, `stratified`,
`fractured`, `ice_wall`. Su grupo tiene 14 controles, cuatro paneles y siete
máscaras. El estado es `EXPERIMENTAL_NOT_COOK_ADMITTED`. La suite final pasó
17/17 y comparó posiciones entre dos procesos del mismo host/pin. Los renders
reales y límites están en `SURFACE_RELIEF_EXPERIMENT.md`; ningún resultado de esta
suite promueve por herencia las familias candidatas anteriores.

## 3. Cobertura de todas las formas geológicas previstas

La lista siguiente cubre las familias y extensiones expresamente previstas en
los documentos inspeccionados. No pretende inventariar toda la geología terrestre
ni todos los generadores de items, vegetación o vehículos del Engine multifunción.
Las construcciones y controles propuestos son **inferencias de ingeniería propias**,
no algoritmos proporcionados por NPS/USGS, ni una simulación física validada.

| Forma prevista | Referencia física observable | Construcción propuesta y controles | Gate específico pendiente |
|---|---|---|---|
| Sedimentary / strata cliff | Lechos resistentes sobresalen respecto de otros erosionados. [NPS](https://www.nps.gov/places/differential-erosion.htm) | Coordenada de bedding deformada + espesores/resistencia por lecho + juntas; `Spacing`, `BedVariation`, `Erosion`, `StrataAxis` | Romper repetición de bandas sin perder continuidad ni soporte; usar sólido si hay voladizo real |
| Block strata y truncamiento | Una discordancia puede separar capas antiguas deformadas/erosionadas de depósitos posteriores. [USGS](https://www.usgs.gov/news/science-snippet/earthword-unconformity) | Camas explícitas, distinta orientación por secuencia e intersección con envolvente; espesor, offset y superficie de truncamiento | Caras de corte reales con IDs nuevos; no simular el truncamiento sólo con color |
| Erosión diferencial de pared | La resistencia desigual explica resaltes y entrantes. [NPS](https://www.nps.gov/places/differential-erosion.htm) | Operador sobre resistencia, exposición y campo espacial, con profundidad limitada | Monotonicidad de la pérdida de relieve en fixture controlada, selección intacta; no llamarlo erosión hidráulica |
| Fractured boulder / cellular | La roca puede estar dividida por juntas antes de erosionarse. [USGS granite](https://pubs.usgs.gov/of/2004/1007/granite.html) | Conjuntos de planos/celdas recortados contra masa cerrada; orientación, separación, apertura y presupuesto de fragmentos | Voronoi Distance to Edge no basta: interiores, cierre, componentes y caras de fractura comprobados |
| Columnar basalt | Contracción al enfriarse: polígonos de 3–7 lados, a menudo hexágonos; direcciones variables. [NPS](https://home.nps.gov/subjects/volcanoes/columnar-jointing.htm) | Secciones poligonales variables, sweep por dirección de enfriamiento, juntas transversales, unión al macizo | No una rejilla de hexágonos idénticos ni columnas flotantes; variedad de inclinación y extremo expuesto |
| Rounded / weathered granite | Meteorización en juntas y esquinas favorece bloques redondeados. [USGS](https://pubs.usgs.gov/of/2004/1007/granite.html) | Masa primaria asimétrica + redondeo limitado + exposición/contacto; detalle secundario después | Silueta legible sin convertirse en esfera ruidosa; volumen positivo y apoyo estable |
| Cliff genérico | Cliff es una disposición, no una litología única | Soporte orientado + familia seleccionada; altura/continuidad vienen del BSP | Plano grande y esquina sin piezas secundarias involuntarias; transición intacta con regiones vecinas |
| Cave wall / cave ceiling | El relieve local no determina por sí solo la red de cavidades. [NPS cave geology](https://www.nps.gov/grba/planyourvisit/cave-geology-in-depth.htm) | Campo en frame superficial y lado sólido; fixture curva para techo/pared | Validar el vacío completo, no sólo distancia de vértices al eje de una cueva de prueba |
| Karst / limestone | Disolución y circulación de agua condicionan pasajes y scallops. [NPS](https://www.nps.gov/grba/planyourvisit/cave-geology-in-depth.htm) | Concavidades direccionales, juntas y canales; `FlowAxis`, `ScallopSize`, `DissolutionDepth`, `FlowStretch` | Concavidad diferenciada de una roca con bultos; túneles sólo mediante corte autoritativo separado |
| Limestone / chalk expuesto | La composición sedimentaria y estructura no equivalen a una sola forma. [USGS clasificación](https://apps.usgs.gov/thesaurus/thesaurus-full.php?thcode=4) | Recetas de bedding, roturas y superficie fina; tratar chalk y limestone como materiales distintos | Masa y rotura visibles en clay; la porosidad visual no asigna automáticamente physical material |
| Shale / slate | Slate posee clivaje que permite placas delgadas; no es sinónimo de bedding. [USGS slate](https://apps.usgs.gov/thesaurus/term-simple.php?code=5.2.2&thcode=4) | Frame de laminación/clivaje separado del frame estratigráfico, juntas y espesor de placa | Placas ancladas en pared; fragmentación de lajas sólo en ruta opt-in |
| Basalt / lava no columnar | Pāhoehoe tiene superficies cordadas; ʻaʻā tiene bloques rugosos; vesículas proceden del gas. [NPS](https://www.nps.gov/articles/000/lava-flow-surface-features.htm) | Lóbulos/perfiles dirigidos para forma principal, poros para detalle; variantes separadas | No reducir todo basalto a columnas; poros grandes afectan geometría, micro poros pueden hornearse |
| Scree / talus | Los desprendimientos generan depósitos al pie de pendientes. [USGS rockfall](https://www.usgs.gov/programs/landslide-hazards/science/csi-rockfall-forensics) | Placement explícito sobre región de depósito; talla, angularidad, densidad, inclinación y semilla por instancia | Budget de objetos/triángulos/collision, contacto, rechazo en keep-clear; nunca scatter implícito de pared |
| Estalactitas / estalagmitas / columnas de cueva | Las primeras crecen desde techo; las segundas se acumulan en suelo; pueden unirse. [NPS](https://www.nps.gov/subjects/caves/speleothems.htm) | Perfiles distintos barridos desde anclas estables, gravedad y longitud limitada; unión autorizada de pares | No invertir un cono para ambas; validar ancla, punta/base, espesor y clearance |
| Flowstone / cortinas | El agua que fluye deposita capas y puede producir cortinas. [NPS](https://www.nps.gov/subjects/caves/speleothems.htm) | Ribbons/perfiles adheridos siguiendo trayectorias; amplitud, espesor, pliegues y gravedad | Separar depósito del soporte, eliminar z-fighting y no cerrar pasos accidentalmente |
| Ice cave en roca | Hielo acumulado dentro de una cueva rocosa no equivale a una cueva excavada en glaciar. [NPS ice caves](https://www.nps.gov/subjects/caves/ice-caves.htm) | Dos dominios de material/masa: host rocoso e hielo adherido; máscaras de acumulación y fusión | Conservar ambos sustratos; preset azul opaco actual no demuestra hielo físico |
| Glacier cave / ice wall | El hielo glaciar y sus canales de fusión requieren un host de hielo. [NPS glacier caves](https://www.nps.gov/articles/000/glacier-ice-caves.htm) | Canales direccionales, bandas y corte en host autorizado; espesor de hielo explícito | Geometría diferenciada en clay y luego prueba de material con espesor; no crear layout desde el ruido |
| Frozen waterfalls / icicles | Hielo de acumulación admite formas adheridas y colgantes. [NPS ice caves](https://www.nps.gov/subjects/caves/ice-caves.htm) | Sweep de cascada y perfiles de carámbano; ancla, gravedad, caudal artístico y fusión | Volumen/espesor y unión al host; no reinterpretar automáticamente flowstone mineral como hielo |
| Glacier crevasses | El hielo puede fracturarse cuando no compensa por flujo el esfuerzo aplicado. [NPS](https://www.nps.gov/articles/crevasses.htm) | Redes dirigidas de cortes con apertura/profundidad e interrupciones; separar fractura de melt channels | Grieta abierta con caras interiores; validar puentes, peligros y collision según intención authored |

## 4. Recetas de construcción nativa que debemos implementar

Los nombres de esta sección son **bloques de diseño propuestos**, no funciones
ya presentes ni IDs que el front pueda invocar. Reutilizar `_Group` y nodos nativos;
extraer helpers sólo cuando dos consumidores reales los necesiten. No crear otra
biblioteca genérica de geometría, solver, scheduler o sistema de materiales.

### 4.1 Coordenada estratigráfica y resistencia

Entrada: posición métrica en frame estable, eje de bedding no nulo, semilla,
espesor, warp y campo de influencia. La implementación de relieve actual ya usa
proyección sobre eje, Noise de baja frecuencia, ID de lecho y White Noise para
resistencia discreta. La fase debe ser común entre chunks contiguos, sin reinicio
por objeto ni por teselación. No sembrar con el índice de un vértice.

Para `block_strata`, sustituir la mera fase por límites acumulados de camas con
espesor positivo. Construir masa y recortar contra la envolvente; mantener base
conectada cuando el modo sea pared. Las uniones y las discordancias necesitan
geometría real. Salidas propuestas: ID de cama, resistencia artística, frontera,
cara de truncamiento y source lineage. El relieve y la masa comparten parámetros
de autoría cuando proceda, pero no prometen topología idéntica.

### 4.2 Red de juntas y fractura

Separar un **campo de juntas** de un **sólido fracturado**. El campo puede sumar
distancias a planos con orientaciones y offsets estables y aplicar warp acotado.
Voronoi Distance to Edge ofrece otra máscara, útil para discontinuidades locales.
No devuelve automáticamente polígonos cerrados de celdas volumétricas.

La ruta sólida necesita cortes/fragmentos, caras interiores, atributos de rotura,
límites de cantidad y clipping. `JointSets` no tiene un rango universal: el catálogo
v1 declara 1–4 y v2 1–3; leer el schema concreto, no unificarlo silenciosamente.
Sembrar cada conjunto desde identidad estable del generador/capa. Fresh break es
provenance artística hasta que el owner de destrucción admita su interpretación.

### 4.3 Redondeo y meteorización de masa

Comenzar por una masa convexa/asimétrica o fuente cerrada, no por ruido sobre una
esfera como única estructura. Redondear aristas de manera limitada conservando
rasgos grandes y la zona de apoyo. Una etapa de volumen/remesh nativa puede ser
adecuada, pero introduce presupuesto de voxel, pérdida de UV y nuevas caras.
Medir esas pérdidas; no considerarla obligatoria para relieve de plano.

El campo `weathered` actual es reutilizable después de la forma primaria. No
implementa meteorización química, transporte de material ni estabilidad mecánica.
Separar desgaste de borde, cavidades y grano; no conectar el mismo Noise a todo.

### 4.4 Columnas adheridas

Generar sitios y secciones poligonales deterministas en un plano perpendicular a
la dirección local de enfriamiento. Variar área, lados, fase y altura; barrer por
curvas cuando haya inclinación/curvatura. Construir una base conectada, recortar
contra la masa authored y aplicar juntas transversales y desgaste de extremos.

`Dual Mesh` puede ayudar a construir una partición superficial, pero no es un
Voronoi 3D. `Curve to Mesh` barre perfiles, no resuelve por sí mismo solapes entre
columnas. En modo soporte, el patrón sólo representa extremos/relieve adherido;
columnas largas y siluetas profundas requieren ruta sólida y sus gates.

### 4.5 Flujo superficial, karst y hielo

Usar frame tangente al soporte, dirección de flujo proyectada y escala transversal
separada de longitudinal. Exigir fallback authored cuando el vector proyectado
sea casi nulo; no inventar una orientación diferente en cada cara. Los charts
curvos necesitan continuidad o costuras declaradas. `NoiseStretch` actual prueba
anisotropía de muestreo, no calcula flujo sobre una malla arbitraria.

Concavidades asimétricas y canales se construyen restando relieve o cortando masa
según la ruta. El mismo campo direccional puede alimentar scallops, fusión y
flowstone, pero cada familia necesita un perfil propio. Separar roca host, hielo
depositado y vacío. Para crevasses usar fractura dirigida con interiores, no
simplemente intensificar `JointStrength` de una pared cerrada.

### 4.6 Formaciones ancladas y depósitos

Las anclas son entidades authored con GUID, cara/posición de soporte y frame.
`Resample Curve` y `Curve to Mesh` permiten perfiles, longitud y resolución
controladas; la unión al host requiere evaluación propia. El perfil de estalagmita
puede tener base amplia y cima redondeada; el de estalactita otra distribución de
radio. Flowstone usa lámina/ribbon con espesor, no una nube de conos.

Talus es una operación aparte: filtrar una región de depósito, elegir fragmentos
con semilla por ID, orientar por soporte y rechazar colisiones/keep-clear. No
simular caída con rigid bodies dentro de cada preview. Una simulación de depósito
sería una opción offline separada y presupuestada, nunca comportamiento implícito.

## 5. Ruido, erosión y escalas: contrato común

| Capa de forma | Operación adecuada | Qué no debe reemplazar |
|---|---|---|
| Macro: silueta/masa | Envolvente, planos, perfiles, camas, volumen cerrado | El BSP, anclas y dimensiones authored |
| Meso: juntas/entrantes | Voronoi/planos, perfil de cama, cavities direccionales, warp de baja frecuencia | Caras interiores reales cuando la grieta está abierta |
| Micro: grano/poros pequeños | Noise fBM y máscaras horneables según tamaño proyectado | Silueta ni collision relevante |
| Variación discreta | White Noise o sub-seed de IDs estables de cama/fragmento | IDs de vértices variables con LOD/remesh |
| Desgaste | Resistencia × exposición × campo espacial acotado | Simulación hidráulica, térmica o química validada |

El grupo actual utiliza Noise 4D fBM normalizado, Voronoi Distance to Edge, White
Noise por lecho, warp y estiramiento direccional. Mantener frecuencias en el frame
métrico, amplitudes y unidades separadas; limitar el detalle a lo que puede
muestrear la malla. Subir frecuencia en un preview escaso causa aliasing, no
detalle gratis. Preview y final conservan la misma receta/seed/macroforma; un
preview puede omitir microdetalle explícitamente y declararlo en su manifest.

El control actual `Erosion` resta un campo artístico al relieve; no transporta
sedimentos ni integra agua en el tiempo. Una futura erosión iterativa necesita
estado inicial, número de iteraciones, límites, cancelación y cache propios dentro
del protocolo existente. Un Simulation Zone no convierte por sí solo un graph en
un modelo físico correcto. No se introduce esa simulación en esta continuación.

## 6. Layer System, loop selection y conexión al front M10.5

1. Jerico conserva el BSP y la selección estable. Una selección loop es una
   operación sobre topología; persistir el resultado con IDs de fuente y política
   de remapeo, no índices efímeros de aristas evaluadas en Blender.
2. Cada capa declara región, frame, modo de operación y semilla. `Influence` es
   selección × falloff × permiso espacial. Un slider global no reemplaza el campo.
3. El soporte se evalúa en orden; asignar un namespace estable y único por capa.
   `build_surface_relief(mask_namespace='jrc_layer_a')` ya permite conservar siete
   atributos de una capa al aplicar otra. La suite comprueba dos modificadores;
   no implementa por ello undo/redo, Apply, invalidación incremental o mezcla general.
4. Leer `interface.items_tree`: tipos, identifier, panel/parent, descripciones,
   defaults, min/max. Enumeraciones sólo cuando la interfaz/adapter suministra una
   lista verificable; no deducir opciones desde texto libre. Campos, objetos y
   geometrías requieren pickers/bindings, no convertir todos los sockets en sliders.
5. La introspección experimental ya se serializa y el test cambia `Depth` por su
   identifier con efecto real. GeneratorSchema público y Qt siguen una admisión
   posterior; no hardcodear `Socket_2`, nombres `.001` ni la posición en un panel.
6. Mostrar estado y motivo de indisponibilidad para cada componente. Un addon
   descubierto puede verse en el catálogo sin quedar habilitado para ejecución.
   Las acciones no compatibles con headless deben figurar como tales; introspección
   no puede fabricar un adaptador completo para cualquier Python arbitrario.

Ejemplo **conceptual de composición**, no payload aceptado hoy por `worker.py`:

```json
{
  "documentKind": "design-example-not-worker-protocol",
  "source": {"bspGuid": "bsp-wall-a", "revision": 12},
  "layers": [
    {
      "layerGuid": "layer-strata-a",
      "selectionGuid": "region-upper-wall",
      "route": "SURFACE_RELIEF",
      "experimentalPreset": "stratified",
      "maskNamespace": "jrc_layer_strata_a",
      "seed": 17,
      "spatialPolicy": "owner-solid-side-and-keep-clear"
    },
    {
      "layerGuid": "layer-ice-a",
      "selectionGuid": "region-water-seep",
      "route": "SURFACE_RELIEF",
      "experimentalPreset": "ice_wall",
      "maskNamespace": "jrc_layer_ice_a",
      "seed": 31,
      "spatialPolicy": "owner-solid-side-and-keep-clear"
    }
  ]
}
```

El ejemplo prueba cómo separar intención y máscaras, no que la composición
strata+hielo simule depósito real. Una capa de hielo con espesor exige la ruta de
masa/attachment correspondiente. Los IDs nativos reales los suministra Jerico.
Las semillas del ejemplo son inputs manuales; la integración final deriva semillas
mediante el contrato `sub_seed` existente y la identidad de capa/generador.

Decal 3D reutiliza selección, frame, campo y máscaras para un detalle localizado.
Spline destruction reutiliza curvas/volúmenes autorizados, genera bordes nuevos
y lineage de corte. Ninguno puede saltarse keep-clear por ser una extensión.
Las máscaras de resistencia, erosión o fresh break son artísticas hasta el mapeo
autoritativo de materials, physical material, PWAS/EPLS y destrucción. No asignar
dureza, sonido, daño, humedad activa o navegabilidad automáticamente por un nombre.

## 7. Gates por familia antes de integración

La secuencia es source/receta → high estructural → selección/layers/validación
espacial → low/LOD derivado → UV/bake/masks → collision/provenance → export/cook.
Detalles de orden y contratos existentes: `MODELING_TO_LOD_PWAS_EPLS.md`. El high
aceptado es la referencia de forma; no generar otra roca con otra semilla para LOD.
Una malla con menos triángulos no puede garantizar identidad punto a punto de una
superficie arbitraria: medir error de silueta/superficie y conservar rasgos críticos.
Normal bake high→low no recupera una silueta o abertura eliminada.

| Gate | Evidencia requerida | Rechazo |
|---|---|---|
| Forma | Clay y material con misma geometría, al menos dos seeds, vista general y detalle | Sólo material/color, patrón repetitivo ocultado por iluminación |
| Soporte | Plano grande, pared, esquina, soporte curvo y selección parcial | Islas/espesor/partes no solicitadas; bordes fuera de selección movidos |
| Determinismo | Mismo input/pin/dependencias en procesos distintos; hash integral de asset en admisión final | Hash sólo de posiciones presentado como cache/runtime proof |
| Escala | Preview/final con macroforma coherente, metros/frame constantes | Recalcular ruido con índices de vértice o reiniciar fase por chunk |
| Espacio | Triángulos completos, interiores y collision contra keep-clear; tolerancias del owner | Sólo comprobar vértices, normals o bounding box |
| Topología | Cierre cuando la ruta exige sólido, orientación, degenerados, autointersecciones y componentes | Un plano abierto declarado sólido; fragmentos flotantes sin autorización |
| Semántica | IDs estables, lineage de caras nuevas y namespaces únicos de máscaras | Durability artística convertida directamente en física nativa |
| LOD/bake | Error medido, UV válida, cage/rays, normals/tangents y máscaras proyectadas | Decimation ratio o Smart Remesh exitoso como única aceptación |
| Recursos | Trabajo fuera del main, preview cancelable, límites de memoria/triángulos/tiempo y manifest | Thread count del worker confundido con threads seguros para llamar bpy simultáneamente |
| Integración | Cook, collision, navegación/materials según contrato; replay Shipping source-free | Beauty Blender presentado como runtime Jerico validado |

Los umbrales de error/clearance son parámetros de admisión del owner, dependientes
del asset y uso; si faltan, no publicar el resultado como apto. FPS, autos e
isométrico comparten contratos pero tienen distintos perfiles de escala y espacio.
Ningún generador debe asumir cámara, personaje, altura humana o Penumbra como norma.

Orden recomendado para las siguientes ventanas: (1) admisión de relieve/layers
sobre más soportes y ruido controlado; (2) columnas y boulder cerrado con imagen
de silueta; (3) karst direccional y diferenciación de hielo; (4) perfiles anclados,
flowstone e icicles; (5) lava/lajas y depósito opt-in. Cada paso exige RED, resultado
geométrico y revisión visual propia. Es una prioridad del backend propuesta al
owner, no una modificación de M9, M10 o M10.5 ni autorización de procesos adicional.

## 8. Fuentes y candidatos: antecedentes auditables

La sección siguiente conserva y corrige el resumen de investigación iniciado el
2026-09-08. Las fuentes respaldan mecanismos y operaciones; no certifican nuestros
algoritmos ni resultados.

## Morphology that must exist in geometry

| Family | Observed geological mechanism | Backend design consequence |
| --- | --- | --- |
| Sedimentary cliff | Differential erosion leaves resistant beds projecting and weaker beds recessed | Nonuniform bed thickness, offset and erosion; preserve an attached base mass, not uniform pancakes. [NPS differential erosion](https://www.nps.gov/places/differential-erosion.htm) |
| Truncated strata | An erosional unconformity truncates older beds | Intersect actual bed geometry with the authored envelope; noise color alone cannot represent truncation. [USGS unconformity](https://pubs.usgs.gov/of/2004/1013/unconformity.html) |
| Columnar basalt | Cooling contraction forms polygonal joints; columns can incline and curve | Vary cross sections and local cooling direction, anchor columns and distinguish organized colonnade from irregular entablature. [NPS columnar jointing](https://home.nps.gov/subjects/volcanoes/columnar-jointing.htm) |
| Karst cave wall | Dissolution follows fractures and flowing water creates wall forms/scallops | BSP owns connected space; directional concavity and fracture-guided dissolution modify its solid boundary. [NPS cave geology](https://www.nps.gov/grba/planyourvisit/cave-geology-in-depth.htm) |
| Ice cave / glacier cave | Ice in a rock cave differs from a cavity inside glacier ice | Separate host-wall substrate, ice accumulation, melt channels and layered glacier ice. A blue rock material is insufficient. [NPS ice caves](https://www.nps.gov/subjects/caves/ice-caves.htm) |
| Frozen cave forms | Ice can accumulate in bands, pillars and falls | Distinct deposition and melt directions; ceiling attachment and floor contact are geometric constraints. [NPS exploring ice caves](https://www.nps.gov/articles/000/exploring-ice-caves.htm) |

The implementation consequences are engineering inferences, not algorithms supplied
by these geology references. They apply to FPS, vehicle and isometric scenes;
clearance budgets come from each authored gameplay profile, not the camera genre.

## Native nodes and operations: what they actually provide

- **Dual Mesh** exchanges face/vertex structure on manifold input; it is not a
  complete 3D Voronoi-cell tessellator. **Extrude Mesh** supports individual face
  extrusion and top/side selection. Together they can form polygonal relief, but
  connectedness and joints need explicit construction and validation.
  [Dual Mesh](https://docs.blender.org/manual/en/5.3/modeling/geometry_nodes/mesh/operations/dual_mesh.html),
  [Extrude Mesh](https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/mesh/operations/extrude_mesh.html).
- **Mesh Boolean** provides difference/intersection/union. Solver and topology
  choices affect robustness and cost; this is suitable for geometric envelope
  clipping, not justification for skipping post-evaluation validation.
  [Mesh Boolean](https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/mesh/operations/mesh_boolean.html).
- **Voronoi texture** supplies distance fields such as F1/F2 and distance to edge,
  not a ready-to-export fractured solid. Use fields for joint selection/erosion
  only where the graph also constructs actual macro topology.
  [Voronoi](https://docs.blender.org/manual/id/5.2/render/shader_nodes/textures/voronoi.html).
- **Set Position** can apply directional deformation, but isotropic noise alone
  does not model bedding, joints or glacial flow.
  [Set Position](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/geometry/write/set_position.html).
- Capture identifiers and source fields before topology-changing operations;
  field evaluation context changes with downstream geometry.
  [Fields](https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/fields.html).
- **Decimate** is mesh reduction. **Data Transfer** projects selected data, while
  **Cycles selected-to-active baking** transfers high-surface appearance to a low
  mesh with explicit UV and ray/cage setup. These are separate stages.
  [Decimate](https://docs.blender.org/manual/en/5.2/modeling/modifiers/generate/decimate.html),
  [Data Transfer](https://docs.blender.org/manual/en/5.2/modeling/modifiers/modify/data_transfer.html),
  [Baking](https://docs.blender.org/manual/en/latest/render/cycles/baking.html).

Some online pages are latest/5.3 documentation. They establish mechanisms, not
proof of availability in the pinned 5.2.1 binary. Exact RNA/socket/operator behavior
must be exercised against that binary before executable admission.

## Free candidates and source references

| Candidate | Relevant capability | Intake position |
| --- | --- | --- |
| [Cell Fracture](https://extensions.blender.org/add-ons/cell-fracture/) | Fractured-cell generation | Candidate for structural breakup; separately pin package/license and test headless context, seed and BSP clipping |
| [A.N.T. Landscape](https://extensions.blender.org/add-ons/antlandscape/) | Procedural landscape reference from the earlier pass | Refresh exact package/license; not automatically a cave-solid or geology-layer backend |
| [Terrain Mixer](https://extensions.blender.org/add-ons/terrainmixer/) | Terrain composition reference from the earlier pass | Refresh exact package/license and evaluate current inputs/local operation before exposing controls |
| [Infinigen](https://github.com/princeton-vl/infinigen) | Procedural nature and terrain, including cave and Voronoi-rock source | Study implementations; no automatic heavyweight dependency. [Project license](https://github.com/princeton-vl/infinigen/blob/main/LICENSE) and individual assets/dependencies require separate intake |

Infinigen's inspected cave and Voronoi-rock paths illustrate grammar/mesh/SDF and
warped gap/mask construction. Those techniques can inform graph design, but they
do not prove fidelity or compatibility in Jerico. Free availability alone is not
admission. The September 8 preference for purchased implementations is historical
context; the current experiment develops native graphs without loading suppliers.
An audited private adapter remains a separate possible route, with original
authorship and license notices preserved, never relabelled as independent work.

Refresh on September 10: Cell Fracture's official listing was reachable and lists
version 0.2.1 / Blender 4.2+; this is not a 5.2.1 headless admission. The archived
[Blender source](https://github.com/blender/blender-addons/blob/main/object_fracture_cell/__init__.py)
has a GPL-2.0-or-later header; the extension package and archived source are not
interchangeable license evidence. A.N.T. Landscape and Terrain Mixer pages could
not be refreshed in this pass. Their licenses must be checked against the exact
downloaded manifest/source before intake. The Infinigen primary
repository was reachable, but no new package was downloaded, enabled or admitted.
No vendor code, graph or asset was loaded into the new relief experiment.

## Layer, selection and extension reuse

Every generator operates through the existing ordered layer recipe with source
GUID selection and protected volumes. A loop selection is a topology query whose
stable authored result must be serialized; an ephemeral Blender edge index is
not a durable Jerico selector after remeshing. Spline destruction and 3D decals
reuse these selectors, coordinate frames, masks and provenance, not a separate
spatial authority. Keep-clear validation applies to all returned LODs and collision.

The backend catalogue must distinguish implemented, metadata-only, unsupported
headless and license-unreviewed components. Introspection exposes actual controls
and rebuild requirements; it cannot synthesize unsupported behavior merely by
displaying a slider. Front implementation stays M10.5 and consumes the established
GeneratorSchema/job/result route after the main owner freezes native integration.
