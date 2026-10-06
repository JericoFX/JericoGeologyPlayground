# Penumbra: catálogo acumulativo de requisitos procedurales

Actualizado: 2026-10-06. Alcance de este documento: requisitos, lectura visual y seguimiento de familias. Redactar o actualizar el catálogo no constituye implementación ni generación de imágenes.

## Instrucciones nuevas del usuario

- «faltarian ventanas de madera vieja, madera comun nueva, osea todo humedo igual»
- «y el resto de las cosas solo que no quiero llenarte con cosas, suma siempre a la lista y usa las matematicas para no dejar todo cuadrado que se pueda añadir daño a los edges, cracks y demas»

Aplicación al catálogo:
- Mantener una lista acumulativa de familias y requisitos. Integrar nuevos elementos sin perder lo recibido ni confundir una referencia con una función terminada.
- Incorporar ventanas de madera vieja y de madera común nueva. Ambas pertenecen al entorno húmedo; madera nueva no equivale a material seco ni tiene que parecer podrida.
- Evitar una construcción visual formada únicamente por cajas perfectas: definir perfiles, transiciones y deformaciones con intención material, conservando las superficies de contacto que el montaje necesita.
- Permitir daño real de bordes, cracks/grietas y otras pérdidas de material, además de las capas de color/normal/roughness.
- «El resto de las cosas» expresa continuidad del catálogo, pero no especifica una lista cerrada de objetos. Añadir las nuevas familias concretas cuando se reciban o se identifiquen; distinguir propuestas de solicitudes explícitas.

## Estado de las familias

Corte de seguimiento: 2026-10-06. El estado de una familia no constituye prueba de que cada detalle de las imágenes esté realizado.

- Cajonera y cajones: referencias recibidas y requisitos catalogados; implementación pendiente de confirmar.
- Cama tubular y componentes: referencia recibida y requisitos catalogados; implementación pendiente de confirmar. Textiles fuera del daño geométrico común de props.
- Banco de trabajo: referencia recibida y requisitos catalogados; implementación pendiente de confirmar.
- Estantería con libros y distribución procedural: referencia recibida y requisitos catalogados; implementación pendiente de confirmar.
- Puerta de tablones deteriorada/reparada: referencia recibida y requisitos catalogados; implementación pendiente de confirmar.
- Puerta reforzada de madera/hierro con mirilla: referencia recibida y requisitos catalogados; implementación pendiente de confirmar.
- Puertas industriales y de vidrio, familias existentes: base verificada; revisiones de materiales en cierre. Esto no confirma que todos los herrajes, composiciones o detalles de la referencia 5 estén implementados.
- Ventanas de madera vieja y común nueva, ambas húmedas: en curso.
- Muelles: sólo diagnosticados; no marcados como corregidos, implementados ni verificados. Su trabajo es independiente de estas siete referencias.
- Daño geométrico común de bordes/grietas/deformación: requisito transversal ampliado en este documento; disponibilidad en cada familia pendiente de verificación.

## Requisitos comunes de geometría, material y montaje

### Geometría real de bordes y grietas

- RC-GEO-01. Conservar parámetros de escala y proporción del objeto, con perfiles/cantos controlados, radios variables y asimetrías acotadas donde correspondan. Evitar jitter independiente por vértice y la deformación uniforme de todos los cantos.
- RC-GEO-02. El daño capaz de cambiar silueta, profundidad o espesor debe existir en la geometría. Pintar una línea oscura o aplicar sólo un normal map no satisface por sí solo un requisito de crack abierto, desconchado profundo o esquina perdida.
- RC-GEO-03. Separar clases de operación: desgaste/bisel irregular del canto; muesca o astillado; grieta con apertura y profundidad; fractura/pieza ausente; y parche de reparación. La intensidad global no sustituye controles independientes y regionales.
- RC-GEO-04. Aplicar operadores matemáticos deterministas con semilla, máscaras regionales y transiciones suaves. La distancia a bordes, extremos, uniones y zonas de contacto puede modular dónde aparece el daño. Los controles deben permitir modificar una región sin destruir el resto del objeto.
- RC-GEO-05. Guiar grietas y deformaciones por ejes materiales y estructurales. En madera, respetar veta y juntas; en metal, continuidad de chapa/tubo y zonas de esfuerzo visual. No reutilizar el mismo patrón de ruido para todos los materiales.
- RC-GEO-06. Mantener espesores mínimos, caras y normales válidas, apoyos y volúmenes de montaje. No crear huecos o intersecciones accidentales; los bordes abiertos deben responder a una rotura explícita. La pérdida deliberada de material no debe dejar remates, herrajes o piezas flotantes.
- RC-GEO-07. Mantener independientes las semillas/controles de estructura, daño, material y distribución de contenido. Cambiar humedad o suciedad no debe reorganizar libros ni alterar automáticamente el número de cajones.

### Deformaciones según el material

- RC-MAT-01. Madera: permitir arqueo, alabeo, ligera torsión y variación de canto mediante campos continuos y acotados; añadir astillado y grietas localizadas. No deformar en igual medida todos los tablones ni eliminar encajes por defecto.
- RC-MAT-02. Madera vieja húmeda: admitir desgaste fuerte, acabado perdido, oscuridad variable, erosión inferior y rotura localizada. La pudrición o pérdida severa es un control separado, no consecuencia obligatoria de activar humedad.
- RC-MAT-03. Madera común nueva húmeda: conservar una construcción más íntegra y cantos relativamente sanos, con diferencias de tono, humedad superficial y deformación moderada opcional. No heredar automáticamente cracks profundos o partes podridas del preset viejo.
- RC-MAT-04. Metal: permitir abolladuras, leves desviaciones del perfil y deterioro de bordes; diferenciar deformación de corrosión/acabado. Respetar placas de fijación y continuidad de soportes. La oxidación no implica que toda la pieza sea blanda o irregular.
- RC-MAT-05. Vidrio: mantener su tratamiento separado. Una revisión de material no acredita que existan rotura, piezas desprendidas o simulación. Fracturas geométricas, si se requieren, deberán declararse y verificarse como alcance propio.
- RC-MAT-06. La humedad debe poder variar espacialmente y por exposición: partes bajas, recovecos, chorretones y superficies expuestas. Evitar una capa de brillo idéntica para todos los materiales y todas las regiones.
- RC-MAT-07. El daño inspirado en math addons se limita a props. No extender este sistema a rocas ni telas; colchón, almohada y manta conservan su tratamiento propio.

### Preservación de sockets y grabpoints por defecto

- RC-MON-01. Proteger por defecto las zonas de anclaje y las superficies de encaje frente a deformación/erosión. Una máscara de protección con transición acotada debe impedir que el daño borre el soporte de un socket.
- RC-MON-02. Conservar identificadores, posición/orientación de referencia y relación pieza/socket de acuerdo con el contrato común existente. El daño no debe desplazar sockets silenciosamente ni desalinear herrajes respecto de sus apoyos.
- RC-MON-03. Mantener grabpoints vinculados a la pieza y a una superficie agarrable que siga existiendo. Si una variante explícita de rotura elimina esa región, comunicar el resultado de forma verificable, no exportar puntos flotantes como válidos.
- RC-MON-04. Mantener carcasa/cajones, marco/hoja y tapas u otros componentes interactivos como piezas separadas cuando se requieran. La distribución procedural del contenido tampoco debe fusionar inadvertidamente libros interactivos con el mueble.
- RC-MON-05. El generador entrega meshes, piezas, sockets y grabpoints. Apertura/cierre, colisiones de runtime, uniones físicas, restricciones, fuerzas y lógica de interacción pertenecen al engine. No incorporar un nuevo sistema de mecanismos en este catálogo.

## Ventanas de madera húmeda

Estado: en curso; no se declaran terminadas en este catálogo.

Requisito explícito:
- Variante de madera vieja húmeda.
- Variante de madera común nueva húmeda.
- Ambas deben admitir dimensiones y construcción procedural, materiales propios y el mismo contrato de salida de piezas/sockets/grabpoints cuando corresponda.

Parámetros propuestos para revisar con el trabajo de ventanas:
- Ancho, alto y profundidad del marco; sección de montantes/travesaños; número y proporción de divisiones; espesores y holguras.
- Diferenciar marco, hojas cuando existan y paños de vidrio. El número de hojas, tipo de apertura y división concreta aún no vienen fijados por estas dos frases.
- Control independiente de antigüedad, humedad, suciedad, alabeo y daño de bordes/grietas. La ventana de madera nueva puede estar húmeda sin parecer una versión idéntica de la vieja.
- Proteger juntas y sockets por defecto, conservando proporciones y regiones de montaje aun con deterioro.
- No inferir medidas exactas, mecanismos adicionales o un estado de finalización por la mera inclusión de la familia en la lista.

## Criterio de lectura

Las imágenes son láminas generadas por ChatGPT que combinan vistas y detalles. Sirven como intención visual, no como planos constructivos exactos. No hay cotas numéricas. La silueta humana de la referencia 7 sólo aporta escala relativa, sin altura conocida. Las dimensiones siguientes son parámetros a exponer, no medidas extraídas.

Requisito recibido: cajones, puertas, estanterías y libros incluidos deben ser procedurales. Mantener el generador en nube separado de Playground. La salida sigue siendo meshes/piezas separadas con sockets y grabpoints según el contrato común; el engine posee runtime y física. Los objetos de estas siete referencias no constituyen un catálogo de muelles.

## 1. Cajonera de madera de cuatro cajones


Visible:
- Mueble alto y relativamente estrecho, cuatro frentes apilados y tapa superior sobresaliente.
- Bastidor de madera, laterales y trasera de tablones; travesaños posteriores; patas cortas.
- Cada frente presenta tirador metálico de cazoleta, marco rectangular para etiqueta y bocallave.
- Un detalle muestra un cajón abierto con paredes y fondo de madera. Otro muestra una unión dentada en su esquina.
- Superficie muy oscura y gastada, aristas desconchadas, arañazos, grietas superficiales, metal oxidado y suciedad inferior.

Reutilización propuesta:
- Arquetipo de carcasa de almacenamiento + módulo de cajón.
- Dimensiones: ancho, alto, fondo, espesor de tablones, vuelo de tapa, altura de patas, número y altura de cajones.
- Separar cada cajón completo de la carcasa; herrajes como piezas reutilizables cuando aporte variación o interacción.
- Variante inicial fiel: cuatro cajones. Otras cantidades son variantes propuestas, no visibles.
- Puntos de agarre en tiradores; sockets de montaje de cajones/herrajes. No exportar comportamiento ni restricciones físicas.
- Daño geométrico localizado en cantos, esquinas y patas; desgaste del acabado y oxidación por material.

## 2. Cama individual de hierro


Visible:
- Estructura tubular con extremos superiores curvados, cabecero y piecero de barras verticales, cuatro patas y largueros laterales.
- Soporte inferior con travesaños; fijaciones y pequeños elementos metálicos de retención junto al colchón.
- Colchón fino, una almohada y una manta gruesa y deshilachada sobre la zona de los pies.
- Pintura muy desgastada, óxido y textiles manchados.
- La lámina no permite establecer todas las uniones ni un inventario inequívoco de barras coherente entre vistas.

Reutilización propuesta:
- Familia de bastidores tubulares y componentes de cama, con ancho/largo/altura, sección tubular, alturas de extremos y número/separación de barras.
- Bastidor, colchón, almohada y manta como piezas independientes.
- Variantes de presencia y colocación de textiles; no deducir que la manta deba simularse ni que la cama deba deformarse en runtime.
- Daño geométrico de props aplicable al bastidor metálico con límites. Textiles fuera del sistema de daño inspirado en math addons; requieren tratamiento propio si se incluyen.

## 3. Banco de trabajo de madera con un cajón


Visible:
- Encimera rectangular gruesa compuesta de tablones.
- Cuatro patas, un estante inferior abierto y faldón bajo la encimera.
- Un cajón en el lado derecho del frente, con tirador metálico de cazoleta.
- Placas, tornillos/remaches y riostras diagonales metálicas; refuerzos en los pies.
- Madera gastada y astillada superficialmente, con metal oxidado.
- No se ve el interior del cajón.

Reutilización propuesta:
- Familia de mesas/bancos que comparte tablones, patas, herrajes y módulo de cajón con la cajonera.
- Dimensiones: ancho, fondo, altura, grosor de tapa, altura de estante inferior y ancho/posición del cajón.
- El cajón debe tener volumen completo generado y ser una pieza separada; esto es requisito de construcción propuesto, no detalle observado en esta imagen.
- Variantes: sin cajón, cajón desplazado o varios huecos, estante inferior opcional. Son ampliaciones propuestas.
- Preservar apoyo real de tapa y estante; no colocar riostras atravesando el volumen de un cajón.

## 4. Librería de madera con contenido


Visible:
- Librería alta, abierta por delante, con cuatro huecos horizontales y trasera de tablones reforzada con travesaños.
- Libros verticales, inclinados y apilados en horizontal; distintos tamaños, colores oscuros, grosores y cubiertas gastadas.
- Uno de los volúmenes tiene aspecto de archivador con etiqueta y orificio circular en el lomo.
- También aparecen una taza metálica, un farol, cajas metálicas con cierres, una caja de madera con abertura para agarre y una botella.
- La estantería y el contenido comparten envejecimiento intenso. No se distinguen textos legibles en libros o etiquetas.

Reutilización propuesta:
- Separar dos generadores: mueble modular y contenido/distribución. Generar también los libros, no introducir una hilera fija como textura o bloque de decoración inseparable.
- Dimensiones de mueble: ancho/alto/fondo, espesor, número de huecos y separación entre baldas.
- Dimensiones de libro: alto/ancho/grosor, grosor de cubierta, saliente respecto a páginas y forma de lomo. Las propiedades del interior son propuesta, no algo comprobable en la lámina.
- Libros/archivadores, pilas, cajas y otros props como instancias/piezas independientes, con agrupación visual opcional por eficiencia sin perder la identificación requerida por el engine.
- El contenido no demuestra que farol, taza o botella deban tener un nuevo generador desde cero: reutilizar familias existentes si ya existen.

Reglas de distribución:
1. Resolver la anchura, altura y profundidad disponibles de cada hueco antes de poblarlo.
2. Usar pequeños grupos de libros verticales, algunos inclinados contra un soporte y pilas horizontales. Distribución condicionada por apoyos y huecos, no rotaciones aleatorias independientes.
3. Ningún libro atraviesa otro, el fondo ni las baldas. Un libro inclinado requiere vecino, lateral u otro apoyo.
4. Las pilas se apoyan sobre superficies; limitar su altura a la del hueco. Evitar suspender volúmenes.
5. Variar ocupación y dejar huecos creíbles. Reservar espacio para props de formas distintas; no repetir la misma composición en todas las baldas.
6. Colocar cajas y volúmenes grandes donde quepan y tengan apoyo. Los objetos pesados preferentemente en baldas bajas es una regla propuesta.
7. Variar paleta y desgaste por grupos, con correlación por estancia, en lugar de asignar todos los valores al azar.

## 5. Puerta metálica industrial


Visible:
- Hoja metálica rectangular en marco de metal.
- Ventana pequeña con cuadrícula visible, manilla con cerradura y bocallave, bisagras robustas y rejilla de lamas inferior.
- Elemento cilíndrico con brazo en la parte superior, visualmente semejante a un cierrapuertas.
- Por detrás aparecen refuerzos, conducciones y una caja fijada a la hoja.
- Franja diagonal negra/amarilla desgastada, óxido, desconchados y chorretones.
- No puede confirmarse el material exacto de la ventana ni la función de la caja/conducciones.

Reutilización propuesta:
- Un arquetipo de puerta técnica dentro de la familia común de puertas.
- Separar marco, hoja y elementos que deban poder manipularse, como manilla. Otros herrajes pueden ser subpiezas compartidas.
- Dimensiones: hueco/hoja, grosor, sección de marco, posición/tamaño de ventana y rejilla, distribución de refuerzos.
- Sockets y grabpoints de montaje/agarrado. No implementar cierrapuertas, cerradura, electricidad ni restricciones físicas.
- Variantes visuales de ventana, rejilla y franja; preservar espacio entre herrajes y bordes.
- Si la ambientación WWI es estricta en todas las zonas, conviene revisar la coherencia de los detalles técnicos; la lámina por sí sola no acredita época.

## 6. Puerta de tablones rota y reparada


Visible:
- Hoja formada por tablones verticales irregulares y jambas/postes laterales.
- Bandas metálicas horizontales ligadas visualmente a las bisagras.
- Traviesas y piezas diagonales de madera, con parches añadidos de tamaños desiguales.
- Cierre con barra/pestillo metálico y piezas de sujeción.
- Grietas abiertas, huecos, extremos inferiores rotos y astillados, fuertes manchas y oxidación.
- La relación exacta de algunos parches y herrajes cambia visualmente entre vistas; no tratarlas como plano mecánico.

Reutilización propuesta:
- Arquetipo de puerta de tablones: ancho/alto, grosor y cantidad de tablones, juntas, traviesas, refuerzos y parches.
- Hoja y soportes/marco separados; cierre como pieza separada si el engine lo necesita.
- Daño guiado por veta y estructura: roturas en extremos, separación de tablas, piezas faltantes y reparaciones con apoyo.
- El generador debe distinguir desgaste superficial, pérdida de material y sustitución/reparación. No son la misma variable.
- Mantener sockets en apoyos válidos después del daño; si una zona desaparece, no dejar herrajes flotando.

## 7. Puerta de madera reforzada con hierro y mirilla


Visible:
- Hoja de tablones verticales, borde y travesaños de hierro con fijaciones redondeadas.
- Marco metálico, dos bisagras grandes en la vista principal, manilla y cerradura de caja.
- Mirilla rectangular estrecha con tapa metálica. Un detalle muestra la tapa abierta.
- Desgaste de madera, corrosión, pintura/acabado desconchado y suciedad concentrada abajo.
- Una silueta humana ayuda a leer la escala, pero no fija dimensiones numéricas.
- La lámina no basta para resolver con certeza una única construcción/articulación exacta de la tapa entre todas las vistas.

Reutilización propuesta:
- Variante reforzada de la puerta de tablones, compartiendo generador de marco/hoja y biblioteca de herrajes.
- Exponer dimensiones de hoja/marco y mirilla, sección de pletinas, filas de fijaciones y grosor de tablones.
- Marco, hoja, tapa de mirilla y manilla como piezas separadas donde corresponda; sockets y grabpoints apropiados.
- Exportar la geometría necesaria sin programar apertura, bloqueo, física o lógica de mirilla.
- Daño principalmente de cantos, acabado y oxidación; pérdida severa de madera como variante propuesta, no presente en igual grado que la referencia 6.

## Familias compartidas y límites

Estas siete láminas pueden funcionar como siete presets de referencia, usando familias reutilizables:
- Muebles de tablones: cajonera, mesa/banco y librería, con módulos diferentes.
- Cajones: frente, caja y herrajes, usados por cajonera y banco.
- Bastidores tubulares: cama.
- Libros/archivadores y distribución de contenido.
- Puertas: tablones, tablones reforzados y chapa industrial.
- Herrajes/props pequeños compartidos, aprovechando los ya presentes en el catálogo.

Reglas globales:
- Semillas separadas para estructura, contenido y desgaste: cambiar suciedad no debe recolocar los libros ni alterar dimensiones.
- Jerarquía de daño: superficie → canto/astillado/abolladura → pieza rota o ausente → reparación. Son parámetros relacionados, no ruido indiscriminado.
- Desgaste contextual: contacto en tiradores/cantos; suciedad en recovecos y partes bajas; chorretones por gravedad; oxidación sólo en materiales pertinentes.
- La rotura debe respetar dirección de veta, espesores, apoyos y silueta intencional. No aplicar el mismo ruido a madera, metal y tela.
- Daño inspirado en math addons sólo para props. No extenderlo a rocas ni textiles.
- Dimensiones válidas primero; decoración y daño después, manteniendo coherencia de uniones y colocación.
- La pertenencia a excavación/mina/laboratorio puede escoger combinaciones y nivel de desgaste, sin convertir las imágenes en prueba histórica de una época.
- No añadir mecanismos ni simulación. El generador entrega geometría/piezas y puntos de conexión/agarre; el engine decide cómo se comportan.

## Resumen para revisión

Estas referencias añaden mobiliario de madera reutilizable, cama tubular, librería poblada proceduralmente y tres arquetipos de puerta. Lo esencial es generar construcción y contenido con parámetros, conservando cada parte interactiva separada. No hay escalas exactas: antes de implementar debe fijarse el convenio de unidades y dimensiones del proyecto. El catálogo distingue requisitos recibidos, propuestas de detalle y estado de implementación. Su actualización no convierte las propuestas pendientes en una implementación terminada.

