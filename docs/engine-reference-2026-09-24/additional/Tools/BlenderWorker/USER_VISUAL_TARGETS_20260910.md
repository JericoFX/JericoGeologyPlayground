# Referencias visuales del usuario — viabilidad procedural y objetivo de calidad

2026-09-10. **Análisis de 27 originales; propuesta, no implementación ni aceptación Beauty.**

## 1. Resultado

Sí: casi todas las **piezas repetibles** de estas imágenes son buenas candidatas
a generadores propios de Blender. Lo más accesible es el kit modular de muelles,
tablones, postes, barandas, escaleras, tuberías y puertas básicas. Lo exigente es
la **calidad final cercana**: astillas longitudinales convincentes, roca con masa
y fractura coherentes, hielo con espesor y materiales que reaccionen correctamente
a la luz. Un generador sencillo de forma no equivale a un asset terminado.

La dirección que interpreto de las imágenes es **realismo industrial envejecido,
de lectura cercana**, con roca angular de grandes masas, madera fibrosa gastada,
metal oxidado, humedad selectiva y contraste frío/cálido. Esta es una inferencia
artística a partir de las referencias, no una afirmación de rendimiento ni de
paridad con un motor comercial. No corresponde asignarles un nivel técnico
“AAA” demostrable, FPS, resolución de texturas o presupuesto de polígonos a partir
de una imagen AI.

Los renders actuales de nuestro mezclador demuestran composición de formas y
preservación de datos, pero **todavía no alcanzan el acabado de estas referencias**.
La escena completa requiere asset pipeline, materiales y renderer además de
Geometry Nodes. Penumbra es el caso visual de este lote, no una restricción del
motor: las mismas herramientas deben servir para FPS, automóviles, isométrico y
otros proyectos.

## 2. Recepción, integridad y alcance

Commit de origen: `32a51769e1c7710928af537f4ec94da54fe15810`.

Carpeta local:
`F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/`.

El owner realizó fetch/sincronización segura y el traslado de 20 PNG desde la
raíz. Su `manifest.json` identifica source/destination, blob Git y SHA-256 de
cada original. SHA-256 del manifest:
`97bf7148e00fe87464a9d4d365851f2af0cb338d566d86e14386080265235c3b`.

Las 20 ausencias en la raíz son cambios locales **sin staging ni eliminación
remota publicada**. Los originales siguen recuperables del commit. No incluir
esas ausencias en la publicación del backend. Este informe no mueve más archivos.

Segundo lote: commit `0a3f3a01b959e56b3d5e2fe184af91ac65fa8b38`, siete PNG en
`F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/0a3f3a01/`.
SHA-256 de su manifest:
`7e708d9e230e088cd7529cfcff3292741717f41ddcaff01957ea515c540b795c`.
El owner hizo su traslado local conservando blobs/hashes y sin publicar las
ausencias de raíz. Total: **20 + 7 = 27 originales**, en dos carpetas y manifests.

Se inspeccionaron los 27 PNG mediante lectura de imagen a resolución original,
sin montaje, reescalado escrito, generación AI ni ejecución Blender. Los enlaces
a originales son **locales**: no se suben los PNG, add-ons o runtimes al publicar
este documento. Las palabras “fácil/media/alta” son estimaciones de dificultad
de diseño, no tiempos prometidos ni capacidad ya implementada.

## 3. Inventario visual completo

| ID | Original local | Lo observado | Viabilidad / dificultad |
|---|---|---|---|
| 01 | [Catálogo de Puertas Industriales Oxidadas.png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/Catálogo de Puertas Industriales Oxidadas.png>) | Catálogo de seis puertas envejecidas: Madera vertical, refuerzos, ventanillas, rejillas, marcos y una escotilla. | Media; ensamblar partes parametrizadas, no seis generadores independientes. |
| 02 | [Caverna helada con pasarelas de madera.png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/Caverna helada con pasarelas de madera.png>) | Caverna con pasarelas: Roca masiva angular, salientes, agua/humedad, andamios y escalera de madera; luz azul al fondo y cálida cercana. | Alta para roca hero/escena completa; media para pasarelas. El título dice helada, pero el azul solo no demuestra volumen de hielo. |
| 03 | [image-gen-1(20260810-004001).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-1(20260810-004001).png>) | Muelle rectangular con poste central frontal: Tablas paralelas, vigas, diagonales, postes redondos, abrazaderas y suciedad basal. | Baja para estructura; media para desgaste convincente. |
| 04 | [image-gen-1(20260814-021756).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-1(20260814-021756).png>) | Segundo catálogo de puertas: Seis variantes de madera, escotilla, metal y rejilla; marco/hoja/herrajes diferenciados. | Media; separar apariencia de mecánica real. Las etiquetas AI no certifican cómo abre cada una. |
| 05 | [image-gen-1(20260814-021900).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-1(20260814-021900).png>) | Puerta de madera con ojo de buey: Tablones verticales, travesaños, aro remachado, bisagras y manilla. | Media; recorte real para ventana, perfiles reutilizables. |
| 06 | [image-gen-2(20260808-014452).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-2(20260808-014452).png>) | Puerto nevado con barco: Acantilado de bloques y repisas, muelle con rieles, cables de farolas, nieve, mar, barco y casetas. | Alta como escena; baja/media para piezas repetidas. Barco y composición hero requieren diseño adicional. |
| 07 | [image-gen-2(20260810-001033).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-2(20260810-001033).png>) | Muelle irregular en L: Postes, tablas con extremos variables, neumáticos y amarres de cuerda. | Media; conectividad en unión, orientaciones locales y sockets para accesorios. |
| 08 | [image-gen-2(20260810-004004).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-2(20260810-004004).png>) | Muelle en T: Tramo transversal y acceso perpendicular; cambios de orientación de tablas. | Baja/media; resolver junta sin solapar tablas ni dejar colisión inconexa. |
| 09 | [image-gen-2(20260814-021903).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-2(20260814-021903).png>) | Escotilla metálica con volante: Contorno redondeado, ojo de buey, remaches, volante y barras laterales. | Media en geometría; alta si se exige mecanismo funcional completo, no inferible de la imagen. |
| 10 | [image-gen-3(20260808-014455).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-3(20260808-014455).png>) | Mina con rieles y vacío central: Roca fracturada, barandas, traviesas, vagoneta, caños elevados, pasarela y luz fría profunda. | Alta para espacio/roca; media para kit industrial. Preservar volumen de paso y guardas. |
| 11 | [image-gen-3(20260810-001037).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-3(20260810-001037).png>) | Muelle ancho con brazo frontal: Plataforma ancha, acceso, neumáticos, argollas, abrazaderas y cuerdas. | Media; misma familia modular, no una implementación aislada. |
| 12 | [image-gen-3(20260810-004008).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-3(20260810-004008).png>) | Módulo de muelle compacto: Madera fibrosa con extremos visibles, herrajes y argollas metálicas. | Baja/media; buen primer asset para comparar en luz neutra. |
| 13 | [image-gen-3(20260814-021905).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-3(20260814-021905).png>) | Puerta industrial con visor y ventilación: Hoja de metal, ventana mallada, rejilla de lamas, bisagras y manilla. | Media; aperturas y lamas reales cercanas, desgaste por material. |
| 14 | [image-gen-4(20260808-014458).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-4(20260808-014458).png>) | Sala industrial subterránea: Generador/motor, tableros, cables en suelo y techo, estanterías, cajas, concreto y roca. | Media para kit modular; alta para maquinaria hero y raccord de toda la escena. |
| 15 | [image-gen-4(20260810-004011).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-4(20260810-004011).png>) | Muelle roto con barandas: Grandes huecos transitables/no transitables, fibras largas, tablones partidos, vigas expuestas y carámbanos. | Alta para destrucción creíble; no basta eliminar tablones aleatoriamente. |
| 16 | [image-gen-4(20260814-021908).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-4(20260814-021908).png>) | Puerta reforzada de mina: Tablas, refuerzos diagonales y horizontales, marco, pasador y tirador. | Media; buen caso de roles semánticos y partes articuladas. |
| 17 | [image-gen-5(20260807-195006).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-5(20260807-195006).png>) | Tubería con codo, T y bridas: Tramos rectos, unión de tres vías, codo, mangas, pernos y corrosión. | Baja/media; conexiones por sockets, radios y diámetros compatibles. |
| 18 | [image-gen-5(20260810-004015).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-5(20260810-004015).png>) | Tres postes: sano, astillado y reforzado: Anillos en testa, veta longitudinal, rotura en astillas y bandas metálicas. | Media/alta; la rotura debe respetar fibra y tener caras interiores. |
| 19 | [image-gen-5(20260814-021911).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-5(20260814-021911).png>) | Puerta de reja: Marco rectangular, retícula abierta, bisagras, cerradura y tirador. | Baja/media en forma; gameplay/colisión se admite por separado. |
| 20 | [image-gen-6(20260810-004018).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/32a51769/image-gen-6(20260810-004018).png>) | Escalera vertical metálica: Dos montantes curvos, peldaños, placas atornilladas y pequeños carámbanos. | Baja/media; el comportamiento escalable no viene de Geometry Nodes. |
| 21 | [Túnel de mina entre sombras y luz.png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/0a3f3a01/Túnel de mina entre sombras y luz.png>) | Túnel de mina con abertura cenital: Paredes de masas amplias y planos de fractura, suelo con fragmentos, abertura superior y haz de luz. | Alta para roca cercana; abertura y clearance reales. La luz volumétrica no viene de la malla. |
| 22 | [image-gen-6(20260814-021914).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/0a3f3a01/image-gen-6(20260814-021914).png>) | Puerta auxiliar metálica: Hoja lisa, marco, bisagras, manilla y desgaste basal. | Baja-media; variante de puerta, no familia nueva. |
| 23 | [image-gen-7(20260810-001653).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/0a3f3a01/image-gen-7(20260810-001653).png>) | Poste de madera con lámpara: Poste, ménsula diagonal, pantalla, jaula y cable anclado. | Media; kit madera+herrajes+curvas, con socket de luminaria. |
| 24 | [image-gen-7(20260810-004021).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/0a3f3a01/image-gen-7(20260810-004021).png>) | Poste con dos reflectores: Base atornillada, tubo, caja, horquillas, reflectores y bucles de cable. | Media; perfiles y piezas con pivotes. |
| 25 | [image-gen-8(20260810-001657).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/0a3f3a01/image-gen-8(20260810-001657).png>) | Doble reflector arriostrado: Travesaño reforzado, cableado y carámbanos. | Media; variante del mismo ensamblaje y capa ambiental. |
| 26 | [image-gen-8(20260810-004025).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/0a3f3a01/image-gen-8(20260810-004025).png>) | Tablero eléctrico con palanca: Caja doble, rejilla, señal, selector, luces y conduits inferiores. | Media en geometría; circuitos e interacción son otros contratos. |
| 27 | [image-gen-9(4).png](<F:/JericoEngine/Tools/BlenderWorker/_local/references/user_targets/0a3f3a01/image-gen-9(4).png>) | Armario eléctrico doble: Caja profunda, hojas, bisagras, manillas, paneles y conexiones. | Media; reutiliza caja/puertas/herrajes. |

Los dos lotes contienen cinco vistas de escenarios, dos catálogos de puertas,
seis puertas individuales, seis módulos de muelle, una lámina de postes,
una tubería, una escalera, tres luminarias y dos tableros/armarios:
**27 imágenes, no 27 generadores distintos**.

Las luminarias comparten poste/ménsula/carcasa/cables y articulaciones;
los tableros reutilizan caja, hoja, bisagras, lamas, conexiones y controles.
Son candidatos de dificultad media con material envejecido. La luz efectiva,
eventos de interruptor y alimentación pertenecen a los sistemas del Engine;
una malla de lámpara no ilumina por sí sola.

Cables, cuerdas, rieles y barandas aparecen en escenarios/amarres. No hay una
lámina específica de telarañas ni una demostración clara de cadena completa.
Ventanas arquitectónicas, columnas de concreto y pisos interiores siguen siendo
peticiones anteriores, no objetivos visuales específicos aportados en este lote.
No inventar observaciones para completar esas categorías.

**Aclaración explícita del usuario durante esta revisión:** telarañas, cuerdas,
cables, tubos, cadenas y demás siguen siendo requisitos del catálogo aunque no
tengan una lámina específica. La ausencia de imagen no los elimina ni los vuelve
opcionales. Sólo distingue evidencia visual recibida de alcance solicitado.
La primera versión estática y su extensión dinámica se aceptan por separado;
ninguna se declara ya terminada en este análisis.

## 4. Qué conviene hacer procedural primero

Las rutas siguientes son propuestas Jerico independientes. No se selecciona
ningún add-on pagado o gratuito como dependencia necesaria.

| Familia reutilizable | Forma básica | Acabado de la referencia | Ruta propuesta | Controles útiles en el futuro front |
|---|---|---|---|---|
| Tablas, vigas y postes | Baja | Media; alta en rotura hero | Perfil de sección + longitud + irregularidad orientada por fibra; caras de testa diferenciadas | Largo, ancho, espesor, fibra, curvatura, desgaste, rotura, seed, herrajes |
| Muelle / pasarela / piso | Baja-media | Media | Ensamblaje por tramos, regiones y juntas con instancias de tablas/postes; orientación por zona | Planta, ancho, módulos, dirección de tablas, separación, soportes, barandas, huecos permitidos |
| Tubos / caños | Baja-media | Media | Curva barrida y fittings con sockets para T, codo, brida, tapa y abrazadera | Diámetro, espesor, radio de giro, segmentos, conectores, material, corrosión |
| Barandas / escaleras / rieles | Baja-media | Media | Dos perfiles longitudinales + repetición parametrizada de postes, traviesas o peldaños | Path, ancho/vía, paso, altura, perfil, apoyos, lado abierto/cerrado |
| Puerta simple / reja | Baja-media | Media | Marco, hoja, huecos y catálogo de herrajes propios con roles separados | Dimensiones, hoja, ventana, bisagra/lado, pestillo, refuerzos, estado |
| Escotilla con volante | Media | Alta si funcional | Perfiles y anillos + conjunto mecánico definido, no barras colocadas sin lógica | Radio de esquinas, espesor, ventana, volante, recorridos y piezas |
| Cable / cuerda estáticos | Baja-media | Media | Curva con puntos de anclaje y flecha; sección circular o cordones helicoidales acotados | Anclas, longitud, holgura, diámetro, cordones, tensión visual, colisión |
| Cadenas | Media | Media-alta | Eslabón propio instanciado a paso controlado, alternando orientación y comprobando contactos | Perfil, paso, escala, giro, anclas y holgura |
| Telarañas | Media | Alta para temporalidad/transparencia | Red de anclajes, radios y conectores con irregularidad restringida; tier de hebras o representación reducida | Anclas, densidad, radios, roturas, caída, grosor |
| Roca, cliff, cueva | Media para relieve | Alta para resultado cercano | Soporte espacial autoritativo + macroestructura + fracturas/erosión y capas superficiales | Regiones, orientación de lecho, escala de masa, juntas, profundidad, erosión y máscaras |
| Madera destruida | Alta | Alta | Fractura longitudinal con caras interiores, piezas retenidas y apoyos identificados | Fibra, región de impacto, profundidad, fragmentos, extremos conservados |
| Maquinaria, barco, luminaria hero | Media por componentes | Alta como conjunto | Kit parametrizado alrededor de diseños de referencia propios aprobados | Dimensiones/tipos/parts; no “noise” universal que invente una máquina |

“Fácil” significa que la regla de construcción es sencilla y reutilizable; no
que Geometry Nodes resuelva automáticamente colisión, interacción, navegación,
UV, bake, LOD, streaming y todo el acabado.

### Fundamentos Blender contrastados

- Un perfil barrido por una curva es suficiente para la base de caños, barandas
  y montantes. El nodo Curve to Mesh ofrece perfil, escala y tapas; los empalmes
  de tres vías requieren tratamiento adicional. [Manual oficial, Curve to Mesh
  5.1](https://docs.blender.org/manual/id/5.1/modeling/geometry_nodes/curve/operations/curve_to_mesh.html).
- Resample Curve permite muestras por cantidad o longitud aproximada. Para
  traviesas o eslabones con paso exacto hay que resolver también el remanente de
  longitud, no asumir que Length garantiza cualquier longitud exacta.
  [Manual oficial, Resample Curve 5.2](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/curve/operations/resample_curve.html).
- Instance on Points referencia la geometría repetida sin duplicarla y conserva
  atributos en el dominio de instancias. Es adecuado para tablas, herrajes y
  peldaños; qué se mantiene instanciado al exportar/cocinar es otro contrato.
  [Manual oficial, Instance on Points](https://docs.blender.org/manual/fi/latest/modeling/geometry_nodes/instances/instance_on_points.html).
- Noise da un campo fractal, no una geología completa. Escala, detalle y
  distorsión permiten jerarquizar variaciones; aumentar octavas/dimensiones tiene
  coste. El nodo GN acepta campos y posición implícita. [Manual oficial del nodo
  GN, versión histórica 4.0](https://docs.blender.org/manual/id/4.0/modeling/geometry_nodes/texture/noise.html).
  La existencia del campo nativo en nuestro pin se ha probado en el experimento;
  esta referencia histórica no certifica toda la API de 5.2.
- Selected to Active y cage permiten transferir detalle del high al low;
  rayos, separación de piezas y márgenes de UV deben controlarse. [Manual oficial
  de baking](https://docs.blender.org/manual/uk/latest/render/cycles/baking.html).
- Normal Map produce una normal perturbada, no una superficie nueva. De ello se
  deduce que no arregla por sí solo la silueta ni la colisión del low.
  [Manual oficial, Normal Map 3.6](https://docs.blender.org/manual/en/3.6/render/shader_nodes/vector/normal_map.html).

Método de consulta: fuentes oficiales recuperadas mediante búsqueda indexada y
auxiliar de investigación de sólo lectura. Varias aperturas directas de páginas
inglesas /5.2 y /latest devolvieron HTTP 402; no se presentan como páginas leídas
con éxito. Las referencias históricas respaldan conceptos, no un pin nuevo ni
certificación de compatibilidad. No se instaló ni descargó ningún add-on.

## 5. Qué nivel de roca queremos exactamente

### 5.1 Lectura de las referencias, no identificación geológica científica

En puerto y minas se ve roca con **masas angulares grandes**, planos parcialmente
fracturados, repisas de anchura irregular, pérdida de material localizada y
transiciones de pared/suelo. No predomina una espuma homogénea ni una textura de
pequeñas piedras pegadas a una pared. No puede determinarse la litología exacta
desde estas imágenes AI.

La referencia de “caverna helada” combina roca oscura, luz azul, brillo y agua.
Hace falta un objetivo adicional de hielo en luz neutra para distinguir cuánto
de ese aspecto debe venir del material helado y cuánto de la iluminación. Para
la variante de hielo propuesta: espesor legible, zonas opacas/transparentes
controladas, estrías y fracturas estructurales; azul pintado no basta.

### 5.2 Jerarquía de forma propuesta

1. **Macro:** perfil de pared, masas dominantes, techo, repisas y huecos. Debe
   leerse sin texturas. El espacio jugable está fijado antes de aplicar detalle.
2. **Meso:** lechos, planos de fractura, juntas, erosión de capas débiles,
   cavidades selectivas y roturas orientadas. Distribución de tamaños, no todas
   las celdas de la misma escala.
3. **Micro:** grano, poro, pequeñas fisuras, desgaste y material. Usar mapas
   donde el detalle no modifica la silueta necesaria.
4. **Ambiente:** humedad, óxido en elementos asociados, sedimentos, escarcha,
   hielo, suciedad basal y contacto; capas con causa espacial, no ruido idéntico
   cubriendo todo.

Rangos iniciales orientativos para una prueba de tamaño humano: masas dominantes
del orden de metros, fracturas del orden de decímetros, grano mucho menor.
Son elecciones artísticas para ensayo, no medidas extraídas de las imágenes;
deben adaptarse a escala y cámara de cada proyecto.

### 5.3 Noise con función, no ruido indiscriminado

Propuesta de responsabilidades independientes:

- campo de masa: frecuencia baja, amplitud limitada por soporte/keep-clear;
- campo de lechos: distancia en un frame geológico, espesor/resistencia variables;
- campo de juntas: familias de direcciones con separación y apertura controladas;
- domain warp de frecuencia baja para romper repetición sin borrar orientación;
- erosión condicionada por resistencia, exposición y drenaje, no “erosión física”
  fingida mediante ruido sin tal simulación;
- microcampo para bake/material, sin subdividir toda la cueva a escala de poro;
- seeds estables por fuente/región/capa/part, no regeneración completa al cambiar
  un accesorio o el orden de UI.

Voronoi es una herramienta para distribuir celdas/distancias, no un requisito de
que todas las paredes parezcan un panal. Ruido anisotrópico ayuda a madera/estrías,
pero la orientación de fibra/lecho tiene que provenir de la receta.

### 5.4 Comparación con nuestro resultado actual

Se releyeron los dos PNG originales 512×512 de `tangent-closure-clay/stone`.
Observación: el lado estratificado muestra salientes repetidos pequeños; el centro
tiene cavidades de perfil suave y el lado de hielo usa una apariencia azul simple.
Son útiles para probar mezcla sobre una superficie. No muestran aún la
composición volumétrica, microdetalle, material húmedo y escala de masa de las
cinco escenas de referencia. El túnel adicional refuerza la necesidad de planos
amplios y techo irregular con una abertura real, no solamente poros superficiales.

Por tanto no declarar “igual o mejor” ni usar la oscuridad para tapar diferencias.
La siguiente prueba autorizada debería atacar macroplanos y juntas de una pared
hero, comparada en iluminación neutra y rasante, antes de añadir más familias
nominales. Esta recomendación **no inicia un job ni cambia M9**.

## 6. Layer System y selección: dónde va cada cosa

Mantener dos niveles distintos:

- **Layers de forma de una misma superficie:** regiones strata/fractura/erosión/
  hielo, mezcla por máscara, conservando el soporte cuando la ruta no cambia
  topología. Eso tiene un experimento real en `surface_family_mix.py`.
- **Layers de ensamblaje/ambiente:** tablas, herrajes, tuberías, cables, depósitos,
  carámbanos, escombros. Añaden partes y necesitan IDs/anchors/procedencia nuevos;
  no encajarlas por fuerza en la mezcla por índice de una superficie.

Propuesta de selección que el front debe resolver y serializar:

| Selección del usuario | Datos que recibe Blender | Ejemplo |
|---|---|---|
| Caras/región pintada | Máscara y IDs de fuente con frame/espacio explícitos | “Esta pared es strata y esta franja es hielo” |
| Edge loop / cadena de aristas | Aristas ordenadas, orientación, cierre y validación | Baranda en el borde o refuerzo siguiendo un contorno |
| Path/spline | Curva canónica, frames, anclas y restricciones | Cable entre dos soportes, tubería por pared |
| Parte semántica | GUID/role de marco, hoja, tabla, poste o herraje | Cambiar sólo pestillo o romper sólo una tabla |
| Exclusión | Keep-clear y zonas bloqueadas | No cerrar una puerta ni ocupar el espacio sobre el muelle |

No prometer loop selection inequívoca sobre cualquier malla triangulada:
cuando no existe un loop topológico útil, se necesita una cadena/path explícita.
La resolución del gesto/selección pertenece a JericoEditor; Blender recibe datos,
no simula clicks en una UI.

Los controles propuestos son independientes de Qt. Su futura exposición requiere
GeneratorSchema admitido, IDs estables, valores/unidades/límites, preview, diff,
cancelación y undo/redo. La introspección ya experimentada no significa que la
UI esté conectada. El único front seguirá siendo JericoEditor en M10.5.

## 7. BSP, PWAS, EPLS, decal 3D y destrucción

Fronteras a preservar; no hay nueva implementación en este informe:

- **BSP:** source-of-truth del volumen y estructura de paso. La roca se construye
  hacia el sólido autorizado; no tapar puertas, estrechar pasarelas ni invadir
  altura libre con estalactitas/cables. Comprobar caras/triángulos y colisión,
  no solamente posiciones de vértices.
- **Procedural Studio:** fabrica piezas/assemblies y variantes con roles y
  sockets. Muelle en T y muelle rectangular reutilizan el mismo kit.
- **PWAS:** propone colocación/repetición de esos assets y resuelve restricciones.
  No reemplaza silenciosamente el source BSP ni decide qué puede bloquearse.
- **EPLS:** aplica capas ambientales como humedad, nieve, escarcha y suciedad a
  través de máscaras/procedencia y las reglas de la superficie.
- **Decal 3D:** útil para cortes, parches o relieve local. No convertir toda
  corrosión en geometría adicional; no construir de ese modo toda la cueva.
- **Spline destruction:** reutiliza path/frame/selección para delimitar una
  intervención. La fractura debe dejar caras nuevas/material interior y devolver
  collision/lineage; la misma curva no significa el mismo algoritmo para madera
  y piedra.
- **Physical Material/Jolt:** material visual y físico vinculados por IDs/roles,
  no por color aparente. La puerta tiene hoja, marco, pivote y pestillo; Blender
  puede devolver metadata, pero el actor, joint, eventos y comportamiento real
  pertenecen al motor. Un objeto no es “escalable” por tener peldaños.
- **Cook/Shipping:** sólo assets cocinados; ningún addon, bpy o Geometry Nodes
  requerido durante el juego. Una validación offline no sustituye esa prueba.

Todos estos consumidores siguen **INTEGRATION REQUIRED** hasta su aceptación por
el owner en el milestone correspondiente. No crear autoridades paralelas a
`MODELING_TO_LOD_PWAS_EPLS.md`, `FAMILY_MIX_STATUS.md` o los masters del Engine.

## 8. Optimización sin perder la forma

Para paredes: favorecer superficie continua sobre el soporte, con partes
volumétricas sólo donde aportan techo/saliente/fractura real. No llenar el interior
del sólido con miles de rocas invisibles. Esto reduce trabajo oculto, pero no
elimina la necesidad normal de culling, partición y streaming.

Para muelles/puertas: conservar estructura modular mientras permita reutilizar
geometría; agrupar al cocinar por criterios de visibilidad/material/partes móviles,
no unir el mundo entero en un mesh ni crear un draw independiente por cada clavo.

Ruta propuesta, ya alineada con el plan high→low:

```text
Fuente BSP/path/partes + selección/keep-clear + receta determinista
  -> preview ligero con la misma macroforma
  -> high autoritativo para detalle
  -> low preservando silueta, huecos, articulaciones y regiones materiales
  -> UV + bake high→low con cage y separación de piezas
  -> LODs derivados + transferencia/verificación de masks/IDs
  -> collision separada + metadata de parts/anchors
  -> cook Jerico + prueba runtime
```

El LOD **no puede ser geométricamente idéntico** al high si elimina triángulos
necesarios para describirlo. El objetivo es error visual controlado a la distancia
de uso, no identidad imposible. No borrar ojo de buey, silueta de astillas,
huecos de muelle ni cortes grandes y esperar que un normal map los recupere.

Cables/cadenas/telarañas necesitan un tier por tamaño en pantalla: hebras y
eslabones detallados sólo cuando se perciben. Evitar miles de transparencias
superpuestas y no asumir física dinámica para todos los elementos decorativos.
Los tests de performance se ejecutarán por el owner con hardware/cámara/budgets
declarados; este análisis no propone cifras de FPS como si estuvieran medidas.

## 9. Cómo demostrar el nivel gráfico

Criterios de aceptación propuestos, aún no ejecutados contra estas referencias:

1. **Forma:** clay neutro y luz rasante; tres seeds, mismos parámetros de escala.
   Roca reconocible por masas/planos, no sólo por color. Silueta y huecos en más
   de una vista; ninguna celda periódica dominante no deseada.
2. **Material:** vistas iguales en seco/húmedo, luz cálida/fría/neutra. Óxido,
   pintura, metal expuesto, veta y testa tienen respuestas diferenciadas.
   No confundir brillo generalizado con humedad creíble.
3. **Asset hero:** un módulo de muelle sano/roto, una puerta con ventana y un
   tramo de tubería; comprueba caras interiores, grosor, unión y piezas móviles.
4. **Espacio:** pared y techo junto a una pasarela, no un plane aislado. BSP
   keep-clear y collision pasan; herrajes, cables y carámbanos no invaden la ruta.
5. **LOD:** comparación high/low/LOD con cámara/distancia fijadas, contorno,
   agujeros, normal bake y masks; medir error, no aprobar sólo reducción de caras.
6. **Motor:** captura original del renderer Jerico más recorrido y métricas
   separadas. Un render Blender bonito no prueba integración runtime.
7. **Procedencia:** repetir con Blender limpio sin add-ons de referencia;
   mismos seeds/inputs/pin, identidad de datos donde se exige determinismo.
8. **Legibilidad por género:** cercana FPS, perspectiva de conducción e
   isométrica de conjunto. La misma receta cambia tier y presentación sin
   convertir Penumbra en política del Engine.

Una comparación pixel-a-pixel contra una imagen AI desde una sola vista no
demuestra geometría correcta: hay zonas ocultas y posibles mecanismos incoherentes
inventados por la imagen. Usar los originales como dirección de arte y muestras
de materiales, no como un blueprint mecánico sin revisión.

## 10. Orden recomendado de trabajo futuro

1. **Mayor retorno y bajo riesgo:** tabla/poste + herrajes propios; con ellos
   muelle rectangular/T/L, piso/pasarela y marco/puerta básica.
2. **Kit de curvas:** tubería con conectores, baranda, escalera, riel; luego cable
   estático y cuerda con anclajes. Reutilizar contratos de path existentes.
3. **Calidad de roca:** un vertical pared/cliff/cueva con macroplanos, juntas,
   relieve mixto y material seco/húmedo comparado contra los escenarios.
4. **Casos complejos:** astillado/destrucción con física, hielo hero/cueva helada,
   cadenas dinámicas, telarañas, mecanismos de escotilla y maquinaria compleja.

Este orden es una propuesta de prioridad para el backend cuando el owner renueve
una ventana concreta; **no cambia el orden M9 → M8 → M10 → M10.5** ni autoriza
implementar esos cuatro paquetes ahora. No hace falta comprar un addon para
iniciar las bases geométricas de este lote. La decisión de adquirir herramientas
se toma tras comparar evidencia real, no por la mera existencia de una miniatura.

## 11. Publicación y cierre de esta revisión

Se entregó primero al owner el inventario local
`_local/backend-publication-candidate-20260910.json`, SHA-256
`4f6de5aa07ef6b23a14946665971d1d1b8cb4db5a0a802e7e30dfb11dc938db5`.

Incluye 212 paths textuales con hash: 15 archivos de la ventana reciente,
9 soportes necesarios, 139 anteriores cuya autoría no se certificó en este turno
y 49 exclusiones/candidatos privados a auditar. El cierre mínimo propuesto de
relief tiene 24 paths; **no equivale al backend completo**. El owner revisa la
autoría de módulos anteriores y la selección final. No usar un header SPDX como
única prueba ni copiar material extraído de add-ons a un commit propio.

Git/publicación permanecen exclusivamente con la tarea
**M9: Actor, Controller y posesión**. El usuario autorizó subir nuestro trabajo
de backend, no los add-ons. No afirmar commit/push hasta recibir hashes reales.
Este informe es un nuevo candidato documental separado del inventario ya cerrado.

No se ejecutaron nuevos tests, renders, Blender ni Python para este análisis.
La única escritura de esta revisión, además del inventario expresamente pedido,
es este MD. Estado del backend: **EXPERIMENTAL/PARTIAL**. La inspección visual
queda cubierta en 27/27 originales; eso no constituye aprobación del backend.
