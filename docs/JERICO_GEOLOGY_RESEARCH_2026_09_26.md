# Geología procedural para JericoEngine
## Investigación técnica, matemática y de herramientas

**Fecha de corte: 26 de septiembre de 2026.**

**Destino:** geología de JericoEngine y su Geology Playground. **Estado:** investigación y propuesta de integración; no implementación ni aprobación de dependencias.

Este dossier reúne publicaciones, bibliotecas, addons gratuitos, herramientas científicas y documentación de motores. Cada recurso tiene una función y un límite explícitos: generar estructura geológica, producir geometría, preparar assets o simular una parte de su comportamiento. No se presupone que una demostración académica pueda integrarse directamente en el motor.

La revisión es documental. No se compilaron estas bibliotecas, no se instalaron los addons y no se midieron sus costes en Jerico. Se consultaron documentos oficiales, repositorios de autores y publicaciones primarias. Los casos en los que solo se pudo leer una ficha o un resumen están marcados en el catálogo de fuentes. El PDF de Pedro Velasco que inició la conversación no pudo recuperarse directamente; no se atribuyen a su texto las conclusiones nuevas de esta investigación.

**Corrección de la respuesta anterior:** la cifra «80 % del resultado por una fracción del coste» no estaba medida y se retira. La utilidad de estos enfoques debe demostrarse con escenas comparables y presupuestos reales.

## 1. Conclusión ejecutiva

La dirección recomendada no es construir un simulador minero universal. Es dar a la geología procedural una **estructura interna coherente**, conservarla durante la extracción de malla y compartirla con materiales, desgaste y destrucción.

La base propuesta sigue siendo la del proyecto: **BSP y espacio jugable → receta geológica → campo volumétrico → mallado → cook → consumidores**. El índice actual del repositorio remite a la directiva 0T del 15 de septiembre: integrar en el engine el código probado del Playground. Esa orientación sustituye para geología a instrucciones anteriores sobre un productor Blender. Esta investigación no modifica el slice activo ni reinstaura Blender u OpenVDB como autoridad. [J01](#j01)

El orden de valor que propongo es: primero estratos, fallas y juntas; después desgaste localizado que respete esa estructura; luego fragmentos coherentes y soporte; finalmente simulaciones más caras cuando exista una necesidad concreta. La creación visual, la resistencia mecánica y la resolución geométrica no deben confundirse.

Los hallazgos especialmente útiles son **PhaseTree**, **Directable Weathering**, **LoopStructural**, **pySimFrac**, **MultiScaleErosion**, **Voxel Tools** y el **Mesh Terrain experimental de Unreal**. Sus papeles son distintos: representación, envejecimiento, estructura, rugosidad de fracturas, erosión de superficies, arquitectura por bloques y autoría no destructiva. No son siete alternativas para reemplazar todo Jerico. [P01](#p01)[P03](#p03)[G01](#g01)[G04](#g04)[P05](#p05)[E05](#e05)[E01](#e01)

### Cuatro problemas, cuatro responsabilidades

| Responsabilidad | Pregunta que resuelve | Lo que no resuelve por sí sola |
|---|---|---|
| Geología estructural | ¿Qué capas, fallas, contactos y juntas existen? | Mallado robusto, movimiento físico o rendimiento. |
| Geometría | ¿Dónde hay sólido y cuál es su superficie? | Resistencia real, historia de formación o textura convincente. |
| Meteorización / daño | ¿Qué partes cambian, se debilitan o se separan? | Que toda pieza sea simulable o que el resultado conserve el espacio jugable. |
| Runtime y cook | ¿Qué se publica, se dibuja, colisiona y guarda? | Convertir una regla artística en una predicción geotécnica. |

Esta separación es una propuesta de diseño para evitar promesas implícitas. Permite mejorar la apariencia sin activar una simulación costosa en cada fotograma.

## 2. Lecturas prioritarias y qué extraer de ellas

### 2.1 PhaseTree — SDF multifase, 2026

**Qué se verificó.** El trabajo organiza objetos con varias fases mediante una estructura implícita jerárquica. Los autores publican demostraciones GLSL con licencia MIT. La ficha se centra también en el trazado de rayos; eso no demuestra extracción de mallas ni colisiones listas para Jerico. [P01](#p01)[P02](#p02)

**Aplicación propuesta.** Usarlo como referencia para que una roca cortada conserve qué parte era roca, veta, inclusión u otra fase. Actualmente no se debe presuponer que un único valor de distancia contenga toda esa información. La extracción debe poder pedir geometría y procedencia material sin reconstruir una geología diferente en el shader.

**Experimento.** Bloque con tres materiales, corte oblicuo y deformación compartida. Comparar el interior recién expuesto con la clasificación analítica original. El resultado aceptable no es solo una malla cerrada: las regiones deben continuar en la cara nueva y mantenerse entre niveles de detalle.

### 2.2 Directable Weathering of Concave Rock Using Curvature Estimation — 2010

**Qué se verificó.** La publicación trata desgaste de roca, incluido comportamiento en concavidades, mediante representación volumétrica, estimación de curvatura y durabilidad editable. También aborda efectos relacionados con material desprendido. Se revisó el resumen institucional, no todo el PDF. [P03](#p03)

**Aplicación propuesta.** Es una referencia más directa para paredes y techos de cuevas que muchos generadores recientes de montañas. Probar operadores separados para rebajar convexidades, excavar material débil y conservar estratos resistentes. Evitar un filtro de suavizado uniforme que convierta todas las rocas en masas redondeadas.

**Experimento.** La misma pared estratificada con desgaste desactivado, uniforme y guiado por durabilidad. Medir silueta, pérdida de volumen y conservación de bordes; comparar además bajo iluminación plana para no confundir normales con geometría.

### 2.3 Arches: a Framework for Modelling Complex Terrains — 2009

**Qué se verificó.** Combina ideas volumétricas y materiales para representar terreno complejo, incluidas formas no describibles con una sola altura. La ficha primaria fue accesible; el PDF enlazado no lo fue. [P04](#p04)

**Aplicación propuesta.** Recordatorio de que «terreno» puede incluir paredes, huecos, arcos y acumulaciones, no únicamente montañas vistas desde arriba. Para Jerico interesa el reparto de representaciones por función: volumen para la formación principal y representaciones más económicas para elementos repetidos.

**Experimento.** Galería con arco y talud de bloques, manteniendo libre el volumen de paso. Comparar una solución completamente voxelizada con otra híbrida. No adjudicar menor coste a ninguna sin medir generación, colisión, memoria y edición.

### 2.4 Terrain Amplification using Multi-scale Erosion — 2024

**Qué se verificó.** Erosión a distintas escalas sobre terrenos de altura. Hay implementación C++ con compute de OpenGL y licencia MIT. Es una base concreta para estudiar el orden de escalas, no un solver volumétrico listo para erosionar techos. [P05](#p05)[P06](#p06)

**Aplicación propuesta.** Usarlo en exteriores, pendientes, sedimentos sobre el suelo y pruebas de macroforma. Extraer el principio de no gastar resolución fina donde una pasada gruesa resuelve el cambio. Un port a HLSL necesita revisar memoria, barreras y equivalencia numérica; no consiste en cambiar nombres de API.

### 2.5 Stochastic Geomorphological Transport — 2026

**Qué se verificó.** Modelo de transporte para formas como canales, meandros y depósitos. El ecosistema de código distingue geotransport del trabajo más amplio de soillib; este último no quedó admitido por licencia en esta revisión. [P07](#p07)[P08](#p08)[P09](#p09)

**Aplicación propuesta.** Investigación de segunda etapa para cauces, material suelto y depósitos creíbles. Para una mina puede inspirar zonas de arrastre o abanicos de sedimento, pero su contexto superficial no lo convierte automáticamente en disolución volumétrica de cavidades. Reservarlo inicialmente al laboratorio y al cook.

### 2.6 SubsurfaceGen — preprint de 2026

**Qué se verificó.** Generación geológica del subsuelo con operadores cronológicos en PyTorch: deposición, deformación, fallas y otras estructuras. Está orientado a modelos de subsuelo y datos de investigación; no a entregar un asset jugable con LOD y colisión. Es un preprint, no se presenta aquí como resultado revisado por pares. [P10](#p10)

**Aplicación propuesta.** La idea valiosa es una historia editable: depositar, deformar, fallar y después excavar. Una veta anterior a una falla debería responder de forma diferente a una veta posterior. Esa cronología puede inspirar recetas ligeras sin importar toda su infraestructura ni sus resoluciones de investigación.

### 2.7 Procedural Multiscale Geometry Modeling using Implicit Functions — preprint de 2025

**Qué se verificó.** Modelado implícito de microestructuras a distintas escalas, con casos porosos y estratificados. El texto HTML fue accesible. Sus resultados de representación/renderizado no prueban extracción y simulación equivalentes. [P11](#p11)

**Aplicación propuesta.** Separar porosidad visual de porosidad geométrica. Para una roca lejana bastaría un material; para un borde cercano quizá geometría local; para una grieta que cambia el paso, volumen y colisión. No convertir cada microporo en un agujero de la malla de runtime.

### 2.8 Dual Contouring of Hermite Data — 2002

**Qué se verificó.** Usa intersecciones y normales de Hermite junto con ajuste cuadrático para conservar características. Se revisó el texto del PDF y varias páginas mediante capturas, incluidas comparaciones y discusión de topología. [P15](#p15)

**Aplicación propuesta.** Referencia matemática para auditar pérdida de planos y esquinas, no orden de reescribir el QEF existente. Una extracción puede cumplir el ajuste local y aun fallar en conectividad, componentes múltiples o capas subvoxel. Deben probarse por separado error geométrico, orientación y topología.

### 2.9 Transvoxel — transiciones entre resoluciones

**Qué se verificó.** Construcción de celdas de transición para conectar terreno de tipo Marching Cubes entre resoluciones vecinas 2:1. El autor lo relaciona con C4 Engine. No es una solución intercambiable sin cambios con cualquier implementación de Surface Nets. [P16](#p16)

**Aplicación propuesta.** Estudiar el contrato de frontera: qué muestras comparten los bloques, qué resolución se permite a cada lado y quién produce la costura. No copiar tablas antes de decidir si la construcción matemática coincide con el mallador de Jerico.

### 2.10 OcMesher — 2025

**Qué se verificó.** Extracción de mallas con octrees adaptados a cámaras; repositorio BSD-3-Clause. Su uso está ligado a escenas procedurales y conjuntos de vistas. [P12](#p12)

**Aplicación propuesta.** Interesante para distribuir presupuesto en el cooker o en escenarios de cámara conocida. En un juego, una región poco visible hoy puede ser inspeccionada mañana: hay que separar calidad de renderizado y estabilidad de colisión. No sustituir el mallador runtime solo por una imagen de demostración más detallada.

### 2.11 PaMO y CuACD — herramientas GPU especializadas

**Qué se verificó.** PaMO trabaja en optimización de mallas con CUDA y declara AGPL-3.0. CuACD aborda descomposición convexa GPU y declara LGPL-2.1-or-later en el repositorio consultado; su anuncio se refiere a SIGGRAPH Asia 2026, no a un evento que esta revisión dé por celebrado. [P13](#p13)[P14](#p14)

**Aplicación propuesta.** Comparadores offline para reparación y colisión. No incluirlos como requisitos de ejecución, ni traducir benchmarks de sus autores a una promesa para nuestra escena. Licencias, NVIDIA/CUDA, dependencias y tolerancias hacen que sean experimentos separados de una ruta portable Diligent.

### 2.12 Terrain Descriptors e Iso-Contours

**Qué se verificó.** Terrain Descriptors propone análisis de formas del terreno; el trabajo de Iso-Contours trata autoría a partir de curvas de nivel. Sus fichas/páginas primarias fueron consultadas, no se ejecutaron implementaciones. [P17](#p17)[P18](#p18)

**Aplicación propuesta.** El primero puede inspirar métricas para evitar evaluar únicamente «se ve bonito». El segundo sirve como referencia de controles de autoría en zonas representables por altura. Ninguno sustituye la representación 3D necesaria para una cueva.

## 3. Geología real: estructura antes que ruido

### 3.1 LoopStructural y GemPy

LoopStructural ofrece modelado implícito estructural con marcos geológicos y cronología; su proyecto declara MIT. GemPy también trabaja con geología implícita, contactos y orientaciones, pero declara EUPL-1.2. Son herramientas científicas Python, no bibliotecas runtime de videojuegos. [G01](#g01)[G02](#g02)[G03](#g03)

**Propuesta.** Utilizarlas como referencia conceptual o como oráculo offline para casos sencillos. Construir un pliegue, una falla y varias capas con datos conocidos; luego reproducir en Jerico el comportamiento que realmente necesitamos. El objetivo no es clonar todo un software geológico, sino dejar de tratar los estratos como bandas de color independientes de la forma.

En particular, los controles de autoría deberían expresar orientación, espesor, continuidad, desplazamiento y orden de formación. Un ruido puede aportar variación, pero no debe destruir esas relaciones. El usuario del editor necesita entender qué cambió cuando mueve un control, no enfrentarse a veinte parámetros de ruido sin significado.

### 3.2 pySimFrac: caras de fractura que se corresponden

pySimFrac, de LANL, genera y analiza superficies rugosas de fractura mediante métodos como síntesis espectral. Permite estudiar apertura, anisotropía y rugosidad; su código declara BSD-3-Clause. No es un solver de propagación de grietas. [G04](#g04)

**Propuesta.** Cuando se rompa una piedra, las dos caras deberían derivarse de una frontera compartida. Crear dos ruidos independientes puede generar piezas incompatibles. Probar una superficie común con apertura controlada, conservar la identidad del contacto y diferenciar la textura recién expuesta del exterior envejecido.

### 3.3 dfnWorks: redes de fracturas, no «Voronoi = geología»

dfnWorks genera redes discretas de fracturas y tiene herramientas de flujo y transporte. Su archivo de licencia mezcla varios avisos y regímenes; no se clasifica aquí como dependencia permisiva ya aprobada. [G05](#g05)[G06](#g06)

**Propuesta.** Estudiar familias estadísticas de juntas finitas: distribución de tamaños, persistencia, espaciado e intersecciones. Un plano débil puede terminar dentro de la roca. Un corte infinito que atraviesa todas las capas produce bloques poco controlables y puede destruir macroformas que el diseñador quería conservar.

### 3.4 Landlab y modelos de superficie

Landlab es un conjunto de componentes para estudiar procesos de superficie y evolución del paisaje, bajo MIT. Su función útil aquí es experimentar con transporte y erosión a escala de terreno, no resolver automáticamente cuevas volumétricas. [G07](#g07)

**Propuesta.** Mantenerlo fuera del runtime y usarlo para comprobar tendencias de deposición, pendiente y drenaje en escenas de prueba. Una herramienta científica ayuda a construir referencias, pero requiere elegir condiciones de contorno, unidades y resolución; no produce por sí sola un preset correcto para nuestro juego.

### 3.5 Documentación geotécnica y vocabulario

La documentación de Itasca permite estudiar criterios de fallo y comportamiento de juntas, pero que sea accesible no hace gratuito su software ni da permiso para portar sus implementaciones. El capítulo abierto Physical Geology distingue fracturas, juntas, familias de juntas y fallas. [G08](#g08)[G09](#g09)[G10](#g10)

Para Jerico propongo conservar esas distinciones. **Una junta** es una discontinuidad sin el desplazamiento relativo característico de una falla. **Una falla** necesita una dirección y magnitud de desplazamiento. **Un estrato** es una unidad de la formación, no necesariamente una superficie débil universal. Así los presets pueden ser expresivos sin fingir una simulación geotécnica validada.

## 4. Bibliotecas: candidatas, no dependencias automáticas

La tabla indica qué problema conviene evaluar con cada herramienta. «Candidata nativa» significa que su función podría encajar en C++ o en el cooker; no significa que ya esté admitida ni que deba reemplazar código probado.

### 4.1 Candidatas de geometría y matemática

| Recurso | Qué aporta | Ubicación propuesta | Restricción o precaución |
|---|---|---|---|
| FastNoiseLite — MIT | Ruido, ruido celular y deformación de dominio en varios lenguajes. | Campo procedural y pruebas CPU/HLSL. | No demuestra equivalencia bit a bit entre implementaciones. [L01](#l01) |
| FastNoise2 — MIT | Evaluación de ruido mediante SIMD en CPU. | Recetas offline y comparación de generación CPU. | No es un backend GPU. [L02](#l02) |
| Eigen — núcleo MPL-2.0 | Matrices, QR, SVD y solvers numéricos. | Oráculo de referencia y herramientas. | Revisar módulos/terceros; no reemplazar el QEF existente por costumbre. [L03](#l03)[L04](#l04) |
| Manifold — Apache-2.0 | Booleanas sobre sólidos triangulados y propiedades asociadas. | Cortes y herramientas offline sobre entradas válidas. | Un plano abierto no es un sólido de entrada válido. [L05](#l05) |
| geometry-central — MIT | Operadores y estructuras para geometría de superficies. | Curvatura, análisis y validación de mallas. | Sus operaciones no equivalen a una geología completa. [L06](#l06) |
| libigl — núcleo MPL-2.0 | Amplio repertorio de procesamiento geométrico. | Prototipos, diagnósticos y cooker. | Revisar licencias por módulo y dependencia. [L07](#l07) |
| Geogram — BSD-3-Clause principal | Predicados, teselaciones y operaciones geométricas. | Herramientas robustas y experimentos de Voronoi. | Elegir módulos; no añadir todo el paquete sin necesidad. [L08](#l08)[L09](#l09) |
| Voro++ — licencia BSD con avisos | Celdas Voronoi tridimensionales. | Fragmentación offline y análisis de celdas. | Divide espacio; no calcula por qué se rompe una roca. [L10](#l10)[L11](#l11) |
| CGAL — por paquete | Geometría computacional y construcciones robustas. | Referencias u operaciones concretas del cooker. | GPL/LGPL/comercial según paquete: no admitir «CGAL entero» sin revisar. [L12](#l12) |
| meshoptimizer — MIT | Simplificación, organización y compresión de mallas. | LOD y preparación del producto final. | Las regiones geológicas requieren protección y verificación explícitas. [L13](#l13) |
| xatlas — MIT | Parametrización UV. | Bake high/low y materiales de superficies exportadas. | Un atlas no sabe automáticamente qué corte revela cada material. [L14](#l14) |
| CoACD — MIT en revisión consultada | Descomposición convexa orientada a colisión. | Proxies de fragmentos en cook. | Calidad, número de piezas y costes deben medirse. [L15](#l15) |

### 4.2 Herramientas que mantendría separadas inicialmente

| Recurso | Papel útil | Motivo para no convertirlo en núcleo ahora |
|---|---|---|
| Fidget — MPL-2.0 | Estudiar evaluación de expresiones implícitas, intervalos y simplificación. | Rust y arquitectura propia; no es un componente C++/Diligent enchufable. [L18](#l18) |
| OpenVDB / NanoVDB — Apache-2.0 actual | Referencia de almacenamiento disperso e intercambio. | La decisión anterior del proyecto no se revierte por encontrar otra referencia. Versiones históricas pueden tener otra licencia. [L19](#l19)[L20](#l20) |
| NVIDIA Warp — Apache-2.0 | Prototipos de cálculo en CPU/CUDA desde Python. | CUDA no traduce automáticamente a HLSL ni establece portabilidad a AMD. [L21](#l21) |
| FEMFX — MIT | Estudiar FEM tetraédrico, plasticidad y fractura. | Es una biblioteca de CPU multihilo; integrarla sería un proyecto físico adicional. [L22](#l22) |
| NVIDIA Blast | Referencia de destrucción y separación de responsabilidades. | Hay que revisar distribución, extensiones, licencia e interfaz concreta con Jolt. [L23](#l23) |
| PaMO — AGPL-3.0 | Comparar remallado y simplificación GPU. | Obligaciones de licencia, CUDA y compatibilidad del pipeline. [P13](#p13) |
| CuACD — LGPL-2.1-or-later declarada | Comparar preparación GPU de colisiones. | Dependencia CUDA y licencia distinta de la revisión actual de CoACD. [P14](#p14) |

### 4.3 Jolt: qué le entregamos y qué no le pedimos

La documentación actual de Jolt permite MeshShape dinámico con restricciones: no debe colisionar con otras mallas o heightfields y requiere masa e inercia suministradas. Además advierte de coste y ambigüedades interior/exterior. Por ello, para fragmentos que caen sobre una cueva triangulada, propongo **convexos o compuestos convexos**, no asumir que convertir cada malla visual a cuerpo dinámico resolverá la colisión. [L16](#l16)[L17](#l17)

Jolt debería recibir la pieza ya definida, su proxy y sus propiedades. El sistema geológico decide qué pieza se desprende; la publicación coordina la retirada de su volumen original, la nueva colisión y su identidad persistente. La malla visual puede conservar más detalle que la colisión. Estas decisiones son propuestas de integración, no afirmaciones sobre lo que ya está implementado.

## 5. Addons gratuitos y herramientas de autoría

No se probaron sus instalaciones en esta sesión. Las licencias de código, del programa anfitrión y de los assets deben revisarse por separado. Tener el repositorio abierto no permite deducir que todo archivo incluido sea reutilizable en Shipping.

| Herramienta | Uso concreto propuesto | Tipo de resultado y límite |
|---|---|---|
| Cell Fracture | Preparar ejemplos de fragmentación y comparar controles de corte. | Geometría prefragmentada; no criterio físico de rotura. GPL-3.0-or-later. [A01](#a01) |
| Hydra | Probar erosión hidráulica/térmica, dureza y mapas de flujo. | Convierte entradas a mapas de altura; no erosiona techos volumétricos. README declara MIT y avisa de otras posibles limitaciones. [A02](#a02) |
| Sverchok | Explorar matemáticas de geometría con nodos y obtener casos reproducibles. | Laboratorio de Blender; no copiar todo su grafo al motor. GPL-3.0. [A03](#a03) |
| Proc-Rock | Comparar un flujo completo de roca, receta, PBR, exportación y LOD. | Aplicación independiente; código GPL-3.0 y tratamiento de outputs explicado en su README. [A04](#a04) |
| Blender-Rock-Generator | Baseline sencillo de creación de rocas. | MIT; el script puede limpiar la escena. Probar únicamente en archivo desechable. [A05](#a05) |
| Infinigen Nature | Examinar construcción procedural de escenas y assets naturales. | Pipeline offline con Blender; BSD-3-Clause principal. No confundir toda Nature con la preview 2.0 orientada a interiores. [A06](#a06)[A07](#a07) |
| Material Maker | Crear materiales de roca, humedad, depósitos y máscaras de prueba. | Herramienta MIT de materiales PBR; no produce por sí sola volumen rocoso. [A08](#a08) |
| Ucupaint | Pintar/desglosar capas de material para comparar contra nuestros campos. | Addon GPL-3.0 de capas de textura; no genera estructura interna de roca. [A09](#a09) |
| BlenderGIS | Importar datos de elevación o contexto geográfico para exteriores. | Los derechos de los datos se revisan aparte; licencia exacta de versión pendiente aquí. [A10](#a10) |
| A.N.T. Landscape | Baseline de terrenos de altura y ruido. | Fuente histórica consultada; no se certifica compatibilidad con un Blender actual. [A11](#a11) |
| ErosionTerrainGenerator | Experimento adicional de heightfields. | Ficha reciente de 2026, declara Blender 5.2+; licencia y funcionamiento por verificar. [A12](#a12) |
| SideFX Labs | Estudiar herramientas de assets, parámetros y preparación geométrica. | Addons con licencia propia permisiva, pero necesitan Houdini; host separado. [A13](#a13)[A14](#a14) |

### Selección práctica para nuestras primeras comparaciones

Propongo empezar con **Cell Fracture, Proc-Rock, Material Maker, Infinigen Nature y Hydra**, asignando a cada uno una prueba distinta. No hace falta instalar doce programas para avanzar. Un recurso se mantiene solo si aporta un resultado medible o una idea concreta que nuestros controles aún no resuelven.

Hydra solo entra en una comparación de altura/sedimento, no en la de techos o paredes interiores. Material Maker solo entra en materiales. Proc-Rock puede ser referencia del proceso de autoría, pero no demostración de geomecánica. Mantener estas categorías impide comparar productos incompatibles como si fueran versiones del mismo algoritmo.

### Houdini gratuito no equivale a producción comercial gratuita

Houdini Apprentice tiene restricciones no comerciales, de formatos y de integración con otras licencias/Engine. SideFX Labs no elimina esas condiciones del anfitrión. Puede estudiarse documentación pública de Houdini y sus herramientas, pero no se debe planear un flujo comercial gratuito basándose en Apprentice. [A15](#a15)

### Bibliotecas de assets para pruebas y acabados

Poly Haven y ambientCG ofrecen assets bajo CC0. Son útiles para materiales y comparaciones visuales, pero hay que leer qué abarca cada licencia: en Poly Haven no todo el sitio web está bajo la licencia de sus assets. [A16](#a16)[A17](#a17)

Propuesta: escoger un pequeño conjunto fijo de materiales y referencias de roca, sin cambiarlo entre experimentos. Comparar en gris, con material neutral y con material final. Así se puede detectar una mejora geométrica real frente a una mejora que proviene únicamente de la iluminación o el albedo.

## 6. Qué hacen otros motores: capacidades verificadas y límites

### Unreal: no quedarse únicamente con Landscape

La ruta Landscape clásica trabaja con mapas de altura. Pero la documentación actual también describe **Mesh Terrain experimental**, con túneles, voladizos, paredes verticales, resolución variable y modificadores no destructivos. Epic diferencia las secciones de preview de las compiladas para runtime y advierte que la función es experimental. La página no basta para deducir su solver interno o una simulación geológica. [E01](#e01)[E02](#e02)

**Lección propuesta para Jerico:** mantener diferenciados documento editable, preview y producto cocinado. La geometría no necesita resolución uniforme y el diseñador debe poder revisar operaciones sin destruir la base. Esto respalda estudiar nuestro enfoque, no copiar implementación de Epic ni declarar equivalencia de capacidades.

### Unreal Chaos: geometría y relaciones entre piezas

Chaos documenta Geometry Collections, agrupaciones jerárquicas y conexiones entre fragmentos. Es una referencia de preparación y activación de destrucción. No demuestra por sí sola un material geológico calibrado con Mohr–Coulomb. [E03](#e03)

**Lección propuesta:** preparar una fragmentación que parezca pertenecer a la roca, conservar sus vínculos y activar únicamente lo afectado. El sistema de soporte debe poder existir sin simular permanentemente todo el macizo.

### Voxel Plugin: editar volumen no basta para tener derrumbes

La documentación de edición runtime enumera límites y separa escultura de funcionalidades como física automática de piezas flotantes. Su existencia evita concluir que todas las dificultades desaparecen con un terreno voxel. [E04](#e04)

**Lección propuesta:** una operación que borra terreno necesita contratos adicionales para detectar componentes, producir cuerpos, actualizar navegación y guardar cambios. El remallado es solo una etapa.

### Godot Voxel Tools: referencia abierta de arquitectura

El proyecto documenta terreno suave con SDF, Transvoxel y niveles de detalle, además de generadores por bloques y trabajo multihilo. Su código es MIT. No se deduce que todas sus operaciones sean GPU ni que sus tablas encajen directamente en Surface Nets. [E05](#e05)[E06](#e06)[E07](#e07)

**Lección propuesta:** estudiar invalidación, fronteras, persistencia de edición y reparto de trabajo. Es más útil mirar esos contratos que copiar un ruido atractivo de una captura.

### C4 Engine: el problema concreto de las costuras

La página de Transvoxel vincula el algoritmo con C4 y explica transiciones de resolución. Es un ejemplo de resolver explícitamente un problema de mallado, no de añadir simplemente más resolución a todo el terreno. [P16](#p16)

**Lección propuesta:** diseñar primero el contrato de frontera del mallador que ya tenemos. Adoptar transiciones de otro método solo después de demostrar su compatibilidad.

### Unity: distinguir el Terrain nativo del motor completo

El manual del Terrain nativo describe heightmaps. Eso limita esa representación a una altura por posición horizontal, pero no significa que Unity entero sea incapaz de alojar cuevas o mallas volumétricas creadas por otros sistemas. [E08](#e08)

**Lección propuesta:** usar heightfields donde funcionan y no forzarlos para una galería subterránea.

### Decima / Horizon Zero Dawn: colocación procedural

La charla oficial de Guerrilla de 2017 trata colocación GPU de elementos del mundo. Es evidencia de distribución procedural de assets, no de simulación de su formación geológica. No se extrapolan sus detalles al estado interno actual de Decima. [E09](#e09)

**Lección propuesta:** la formación principal y la población secundaria pueden ser problemas diferentes. Una pared volumétrica puede acompañarse de bloques e instancias, con selección y restricciones propias.

### No Man’s Sky: mundo continuo

La ficha oficial de la charla GDC de 2017 describe un pipeline de generación continua que incluye voxels, poligonización, texturizado y población. No se analizó íntegramente el video ni se atribuye aquí un mallador exacto no comprobado. [E10](#e10)

**Lección propuesta:** separar representación compacta, generación bajo demanda y producto visible. No confundir el tamaño del mundo con la necesidad de mantener toda su geometría activa.

### Houdini: preparación de assets, proxies y restricciones

Su documentación distingue fractura, restricciones entre piezas, geometría detallada y representaciones para simulación. Es una referencia de flujo de producción; el anfitrión no es una biblioteca gratuita de runtime para Jerico. [E11](#e11)

**Lección propuesta:** conservar detalle visual fuera del proxy físico y hacer explícita la relación entre ambos. Un buen asset roto necesita interiores, materiales, colisión y estado persistente, no solamente polígonos nuevos.

### Lo que no se pudo verificar con suficiente detalle

No se documentan aquí supuestos algoritmos internos de Teardown, Deep Rock Galactic o versiones actuales de CryEngine sin una fuente primaria suficiente. Tampoco se presenta un catálogo de motores como prueba de que ninguno tenga capacidades adicionales. El alcance es lo que sus fuentes concretas permiten afirmar.

## 7. Modelo matemático propuesto para Jerico

Esta sección es una **síntesis de diseño**, no una especificación aprobada, un port de los proyectos citados o una descripción de código que se haya auditado. Las ecuaciones definen experimentos y responsabilidades; faltan implementaciones, tolerancias y pruebas antes de declararlas productivas.

### 7.1 No almacenar todo dentro de una única distancia

Un modelo lógico mínimo puede distinguir:

| Campo o descriptor | Significado propuesto | Almacenamiento inicial aconsejado |
|---|---|---|
| φ(x) | Límite del sólido; negativo en el interior. | Expresión procedural y muestras del dominio que ya use Jerico. |
| s(x) | Coordenada estratigráfica. | Función derivada de la receta; no necesariamente otra textura 3D. |
| L(x) | Unidad geológica o fase material. | Clasificación analítica / ID de región. |
| J(x) | Familias de juntas, límites y persistencia. | Descriptores compactos; muestras solo donde sean necesarias. |
| R(x) | Durabilidad ante un operador de desgaste. | Parámetro por material y variación de baja frecuencia. |
| D(x) | Daño o debilitamiento cuando el gameplay lo requiere. | Estado local, no un volumen mundial siempre activo. |
| A(x) | Anclaje, protección o pertenencia a espacio reservado. | Autoridad semántica del proyecto; no ruido generado aparte. |

La decisión importante es que la misma capa que aparece en un corte sirva para materiales y para propiedades de desgaste. No tener una versión de los estratos en el shader, otra en la geometría y otra en Damage. PhaseTree y los modelos estructurales motivan estudiar esta separación, pero el contrato concreto sería propio de Jerico. [P01](#p01)[G01](#g01)

### 7.2 Deformación compartida y estratos coherentes

Una prueba sencilla usa una deformación común:

```text
q = W(x)
s(x) = n · q
```

`W` deforma la formación y `n` orienta el sistema inicial de capas. Las fronteras no se inventan después con un ruido independiente para cada banda. Una forma de preservar orden en las coordenadas locales es acumular espesores positivos:

```text
b₀ = referencia
bᵢ₊₁(u,v) = bᵢ(u,v) + tᵢ(u,v)
tᵢ(u,v) = t_min + t_ref · exp(aᵢ(u,v))
```

Con t_ref positivo y expresado en unidades de longitud, y aᵢ adimensional, esto evita espesores negativos por construcción en ese sistema de coordenadas. No garantiza por sí solo que una deformación espacial extrema sea inyectiva: `W` también necesita límites o validación. Si se pliega hasta invertir espacio, la fórmula de espesores no lo arregla.

Para un control artístico más directo puede usarse otra transformación positiva; el exponencial es un ejemplo, no una obligación. Una variación de baja frecuencia compartida mantiene correlación entre capas, mientras cada capa añade una variación secundaria limitada.

### 7.3 Las normales deben seguir la transformación

Cuando una función se evalúa en un dominio deformado, su gradiente debe ser coherente con esa transformación. En notación compacta:

```text
∇x f(W(x)) = J_W(x)ᵀ · ∇q f(q)
```

Un ruido que deforma la forma sin actualizar correctamente la orientación puede entregar normales inconsistentes al mallado. Además, una composición deformada no conserva automáticamente la propiedad de distancia euclídea. Si un algoritmo depende de una distancia verdadera o de una cota de Lipschitz, hay que restaurarla, acotarla o cambiar el supuesto.

**Prueba propuesta:** plano deformado con gradiente analítico conocido frente a diferencia finita en varias resoluciones. La tolerancia debe separar error de muestreo y error de derivación.

### 7.4 Fallas y cronología

Representar una falla con una superficie divisoria y un desplazamiento aplicado a las unidades que existían antes. El orden es parte de la receta. Una unidad depositada después no tiene por qué sufrir la misma deformación histórica.

Conviene distinguir desplazamiento y apertura. Mover ambos lados de una falla no obliga a introducir una grieta de aire visible. En la autoría pueden existir contactos cerrados, cambios de material o desplazamientos que solo se observan porque una capa deja de coincidir.

**Prueba propuesta:** tres capas, falla oblicua y veta posterior. Comprobar continuidad de la veta posterior y desplazamiento de las capas anteriores. No imponer esta cronología al gameplay sin controles que el diseñador pueda comprender.

### 7.5 Familias de juntas finitas

Para una familia de planos idealizados:

```text
pₖ(x) = nₖ · x − dₖ
```

El descriptor completo necesita orientación 3D, espaciamiento, dispersión angular, extensión, persistencia y relación con materiales. Un único ángulo no describe una orientación 3D completa. La persistencia debe limitar el parche de fractura, no únicamente atenuar su color.

Distinguir tres estados: discontinuidad cerrada, superficie debilitada y grieta abierta. Solo el tercero exige representar un hueco o una separación de caras; abrir una grieta no implica necesariamente eliminar masa. Es una diferencia fundamental para conservar bloques conectados y evitar que una roca se convierta inmediatamente en un rompecabezas de piezas separadas.

**Prueba propuesta:** una junta se detiene dentro de una pared y otra atraviesa un borde. El resultado debe depender de su geometría declarada, no del tamaño de bloque usado para evaluarla.

### 7.6 Voronoi condicionado, con límites matemáticos claros

Voronoi puede proponer particiones internas cuando la estructura no define todas las piezas. Una extensión con métrica común es:

```text
dᵢ(x) = (x − cᵢ)ᵀ M (x − cᵢ) − wᵢ
```

Con una matriz positiva definida `M` común, una transformación de coordenadas permite orientar o estirar el patrón. Los pesos `wᵢ` corresponden a una variante de partición ponderada. Esto no significa que toda biblioteca Voronoi admita cualquier formulación; hay que comprobar su API.

Si se da una matriz diferente a cada celda, las fronteras pueden dejar de ser planas. No debe anunciarse esa generalización como una simple llamada a Voro++. Propongo comenzar con un marco común por región y clips por juntas existentes.

Los puntos de partición deben ser estables bajo cambios ajenos a la región. Un ID de receta y un identificador espacial estable son mejores entradas para una semilla que el índice actual de una lista que cambia al editar.

### 7.7 Superficies de fractura compartidas

Generar una frontera común `F(x)=0` para las dos piezas y derivar apertura o separación de esa frontera. El ruido superficial debe tener escalas y amplitudes limitadas para no crear autointersecciones ni delgadas agujas imposibles de colisionar.

La superficie recién expuesta recibe un marcador de procedencia. Su material puede consultarse en las coordenadas geológicas originales y después recibir un desgaste propio con el tiempo. Eso evita mapear el aspecto viejo del exterior en todas las caras nuevas.

La rugosidad visual fina puede ir a normales o desplazamiento de render, mientras el proxy conserva una forma más simple. pySimFrac es una referencia para investigar correlación y apertura, no una autorización para suponer fractura dinámica físicamente resuelta. [G04](#g04)

### 7.8 Booleanas SDF: no es una resta aritmética

Adoptando interior negativo, una diferencia volumétrica se expresa para el signo y la superficie como:

```text
φ_resultado(x) = max(φ_roca(x), −φ_corte(x))
```

No como `φ_roca − φ_corte`. Incluso si los operandos son distancias exactas, las composiciones CSG no garantizan una distancia euclídea exacta en todo el dominio. Para operaciones que usan magnitudes métricas, gradientes o curvatura hay que revisar ese supuesto.

Un plano sin espesor no es automáticamente una grieta abierta. Puede representar una discontinuidad, pero separar material mediante sustracción requiere un volumen de corte definido. La elección afecta pérdida de masa, compatibilidad de las piezas y detectabilidad a la resolución actual.

### 7.9 Desgaste guiado por material y exposición

Como experimento de autoría, separar al menos desgaste de convexidades, excavación de material débil y acumulación. Una combinación artística puede consultar durabilidad, orientación y exposición, siempre marcada como regla visual hasta calibrarla.

La curvatura puede estimarse a partir de la normal:

```text
n = ∇φ / ||∇φ||
κ = div(n)
```

Aquí `κ` usa la convención de suma de curvaturas principales. Cerca de gradientes degenerados y discontinuidades hay que regularizar, limitar la actualización o usar un tratamiento específico. No aplicar indiscriminadamente un suavizado para «estabilizar» si elimina precisamente las esquinas que queremos conservar.

Una formulación de evolución de superficie puede escribirse `∂φ/∂t + v_n ||∇φ|| = 0`. Introducirla exige definir signos, pasos estables y condiciones de contorno. Para el primer experimento puede ser más razonable un operador acotado y repetible que un solver completo presentado como físicamente fiel.

### 7.10 Erosión superficial no es erosión 3D

Un heightfield representa una altura por columna. Puede describir suelo, depósitos sobre pendientes y drenaje en ciertos dominios. No puede representar simultáneamente suelo y techo de una cueva en la misma columna sin extender su representación.

Por eso propongo una estrategia híbrida explícita: el macizo conserva representación volumétrica; los depósitos sobre una superficie elegible pueden usar una representación superficial; el resultado debe reintegrarse con límites claros. La pérdida de material y la deposición no se conservan automáticamente al cambiar de representación: debe existir una contabilidad o se declara efecto artístico.

En ningún caso se debe conectar directamente un mapa de altura a toda pared arbitraria y llamarlo «hidráulica volumétrica». Hydra y MultiScaleErosion son referencias útiles precisamente cuando se respeta su dominio. [A02](#a02)[P06](#p06)

### 7.11 Mohr–Coulomb: criterio no equivale a solver

Con compresión positiva, una formulación ilustrativa de un contacto es:

```text
t = σ · n
σ_n = n · t
τ = ||t − σ_n n||
σ'_n = σ_n − u
τ_límite = c + σ'_n tan(θ)     para la rama compresiva
```

`σ` es el tensor de tensiones, `u` la presión de poro, `c` la cohesión y `θ` el ángulo de fricción. La apertura por tracción necesita un criterio separado. No se deben mezclar convenciones de signo ni usar la ecuación de fricción como una resistencia universal para cualquier estado. La documentación de Itasca permite estudiar esas distinciones. [G08](#g08)[G09](#g09)

Si Jerico solo calcula distancia a un impacto o un valor de desgaste, no dispone de `σ`. Puede usar una regla de rotura artística, pero no debería llamarla solución Mohr–Coulomb. Un grafo que detecta falta de soporte tampoco resuelve automáticamente redistribución de tensiones.

**Prueba física mínima:** bloque sobre plano inclinado con unidades y cargas conocidas. Una prueba de funcionamiento del código no certifica estabilidad real de una mina, ni este sistema debe usarse para decisiones de ingeniería.

### 7.12 Mallado y preservación de características

Un ajuste de Hermite puede plantearse como:

```text
E(v) = Σᵢ aᵢ [nᵢ · (v − pᵢ)]²
```

`pᵢ` son intersecciones, `nᵢ` normales y `aᵢ` pesos. Hay que controlar condicionamiento, ubicación de la solución y desacuerdos entre muestras. Las pruebas deben distinguir geometría y shading. Una normal bonita no repara una esquina físicamente redondeada. La referencia histórica es Dual Contouring, no una promesa de que cualquier variante de un vértice por celda conserve cualquier detalle. [P15](#p15)

Una capa más fina que la rejilla, dos componentes dentro de una celda o una intersección de varias fases pueden requerir refinamiento o una representación distinta. No se solucionan únicamente elevando el peso de una normal. Tampoco «cerrada» significa por sí sola ausencia de autointersecciones o buen comportamiento físico.

### 7.13 Invalidación y remallado local

La región dirty no es necesariamente el conjunto exacto de voxels editados. Cada operador declara su alcance: filtros necesitan vecinos; gradientes necesitan estencils; una modificación de semilla puede afectar toda su región; la conectividad puede atravesar bloques.

Propuesta: calcular invalidación a partir de dependencias y ampliar por los márgenes necesarios. La publicación no debería aceptar un bloque nuevo con fronteras calculadas contra vecinos de otra revisión. Si la implementación actual aún reconstruye todo, la optimización local sigue siendo un trabajo por demostrar, no una capacidad atribuida por esta investigación.

### 7.14 Fragmentos, soporte y activación física

Un grafo de soporte puede representar piezas y contactos. Al romperse vínculos, se buscan componentes sin anclaje. Es un mecanismo de gameplay útil, pero no una solución completa de elasticidad ni de fractura continua.

La transición propuesta es transaccional: generar candidato, validar geometría, preparar proxy, asegurar IDs y solo entonces publicar retirada y aparición de la pieza. Mientras la preparación falla o se cancela, conservar el estado anterior. Separar descaradamente primero y resolver la colisión después puede crear huecos temporales o cuerpos inconsistentes.

Las piezas pequeñas pueden usar un presupuesto simplificado o quedar como decoración; las piezas relevantes reciben simulación. Debe decidirse explícitamente qué interacción permite cada categoría y qué se persiste al guardar la partida.

## 8. Rendimiento, memoria y portabilidad

Todas las cantidades siguientes son razonamientos de presupuesto, no mediciones del motor. No hay una promesa de milisegundos ni de porcentaje de mejora.

### 8.1 Coste de un volumen denso

Un canal de 32 bits ocupa, sin compresión ni márgenes:

| Resolución | Muestras | Un canal | Seis canales |
|---|---:|---:|---:|
| 128³ | 2.097.152 | 8 MiB | 48 MiB |
| 256³ | 16.777.216 | 64 MiB | 384 MiB |

No incluye dobles buffers, halos, normales, vértices, índices, listas temporales, colisión ni copias CPU/GPU. Este cálculo explica por qué «añadamos un campo para cada cosa» necesita una política de almacenamiento. Muchos descriptores deberían evaluarse desde la receta, cuantizarse o almacenarse solo en bloques relevantes.

### 8.2 Tres escalas de detalle

Propongo **macroforma**, **estructura visible** y **microdetalle**. La macroforma controla paso, silueta y masa. La estructura visible incluye estratos, juntas y desprendimientos. El microdetalle puede ir a materiales o a una representación localizada. Es una asignación de presupuesto, no una regla universal de distancia fija.

Para decidir dónde generar geometría, considerar tamaño en pantalla y efecto sobre colisión/interacción. Un hueco minúsculo visual puede ser irrelevante para física; una abertura pequeña que permite al jugador pasar tiene otra importancia. Mantener políticas separadas pero coherentes.

### 8.3 Medir toda la cadena

Registrar por separado evaluación del campo, transferencias, extracción, reparación, simplificación, UV/bake, cook de colisión, subida, publicación y memoria máxima. Un kernel más rápido no garantiza un editor más responsivo si aumenta el readback o bloquea la creación de colisión.

Comparar semillas, cámaras, resolución y geometría equivalentes. Medir tiempos típicos y colas lentas, además de cancelación y recuperación. Una captura estática y una compilación correcta no demuestran fluidez, ausencia de pausas o menor coste total.

### 8.4 CUDA no es Diligent

Los proyectos CUDA son buenos comparadores de algoritmos, pero no establecen que nuestra implementación deba exigir NVIDIA. Para una ruta portable hay que rediseñar recursos, reducciones, atomics, sincronización y límites según el backend real.

Tampoco suponer que usar compute implica una cola asíncrona separada o solapamiento gratuito con gráficos. La evaluación de GPU y la semántica de publicación deben respetar el estado real de Jerico. La ruta CPU puede seguir siendo útil para oráculos pequeños, servidores de cook o hardware diferente.

### 8.5 Determinismo y estabilidad de autoría

Hay varias exigencias distintas: mismo resultado conceptual, resultados dentro de tolerancia, misma topología y reproducibilidad bit a bit. No declarar todas por comprobar solamente una semilla repetida en la misma GPU.

Propuesta: versionar receta, operadores y política de muestreo. Generar identidades estables a partir de la formación y de coordenadas/IDs permanentes, no del orden accidental de evaluación. Preservar IDs de región aunque cambie la malla visible; un índice de triángulo no es una identidad geológica durable.

## 9. Integración propuesta sin crear un segundo motor de geología

El objetivo es enriquecer contratos existentes, no abrir otro registro de entidades, otro sistema de mutación u otro cooker independiente.

### 9.1 Documento editable

La receta conserva unidades, orden de operaciones, parámetros, semilla, referencias al BSP y restricciones. El autor puede pedir pared, roca aislada o formación compleja, pero el significado del espacio protegido no debe perderse al pasar por una deformación.

La historia geológica es una parte del documento. La ejecución puede optimizarse o fusionarse internamente, siempre que preserve el resultado y los límites declarados. «Reordenar para acelerar» no puede cambiar una falla anterior por una posterior sin invalidar la equivalencia.

### 9.2 Preview

La vista previa puede usar menor resolución y trabajo cancelable. Debe indicar diferencias relevantes respecto al producto final. Si una capa es demasiado fina para el preview, no ocultar el problema con un material que sugiera una geometría que todavía no existe.

Mantener las variantes temporales como candidatos. Aplicar debe ser una operación deliberada y reversible. Cancelar no debería dejar regiones, cuerpos o materiales parcialmente actualizados.

### 9.3 Cook y publicación

El cook valida geometría, genera LOD/proxies y produce metadatos para consumidores. Cada salida conserva procedencia: formación, unidad geológica, operación que la creó y clasificación de cara exterior/interior cuando corresponda.

La publicación consulta una revisión del documento y rechaza candidatos obsoletos. Deshacer debe restituir conjuntamente geometría, colisión y estado asociado, sin una ruta alternativa de mutación que evite las transacciones normales del editor.

### 9.4 Materiales, navegación y gameplay

Los materiales reciben información geológica; no necesitan redescubrirla con otro ruido. La navegación se invalida según cambios relevantes de colisión/paso. Audio y partículas pueden responder a tipos de superficie y ruptura, pero no deben forzar a simular todos los granos.

Save/load necesita registrar las decisiones persistentes: fractura aplicada, fragmento separado, daño que cambia interacciones. No guardar toda caché derivable por defecto. La política exacta debe respetar los contratos vigentes del proyecto y revisarse antes de cualquier cambio.

### 9.5 Keep-clear y libertad artística

El diseñador debe poder deformar paredes sin invadir un corredor reservado, tapar una puerta o reducir la altura libre de una sección jugable. Los controles deben mostrar cuándo una operación fue limitada y por qué; no esconder recortes que dificulten interpretar el resultado.

Conviene probar restricciones sobre el volumen final, no solo sobre centros de objetos. Un bloque colocado fuera del camino puede invadirlo con una esquina. Del mismo modo, una textura no puede certificar que un túnel tenga el espacio necesario.

## 10. Plan de experimentos por etapas

No son nuevos slices aprobados: son candidatos de investigación ordenados por dependencia. Cada etapa termina en una decisión sustentada por resultados, no en una lista de código añadido.

### Etapa A — estructura interna y controles

Construir escenas pequeñas con deformación compartida, espesores positivos, desplazamiento de capas, juntas finitas y materiales interiores. Reutilizar el evaluador y el mallador existentes. Elegir una representación compacta antes de asignar texturas 3D adicionales.

**Salida esperada:** rocas que mantengan estructura después de cortar y variar parámetros. **Condición de avance:** capturas neutrales, medición de desviación geométrica y consistencia de regiones. **No incluye:** solver de tensiones, hidráulica completa o nuevo mallador por defecto.

### Etapa B — desgaste controlado y acumulación

Comparar desgaste uniforme y dirigido por durabilidad. Introducir un efecto por vez, con volumen y límites medidos. Separar depósitos superficiales de cavidades volumétricas. La acumulación puede empezar como una regla visual explícita, no como conservación física proclamada.

**Salida esperada:** diferencia visible entre unidades resistentes y débiles sin destruir macroforma ni espacio jugable. **Condición de avance:** comportamiento estable al cambiar resolución dentro del rango declarado y operadores cancelables.

### Etapa C — piezas y soporte

Generar pocos fragmentos con fronteras compartidas e interiores coherentes. Preparar proxies y activar cuerpos únicamente después de validar la transacción. Empezar con una pieza que se desprende de una pared y con un bloque de techo, no con una mina completa en simulación.

**Salida esperada:** geometría, material, colisión y guardado coinciden tras rotura. **Condición de avance:** ausencia de duplicación/pérdida accidental de piezas, IDs estables y test de recuperación ante cook fallido.

### Etapa D — referencias científicas y GPU especializadas

Solo cuando exista un problema concreto: comparar métodos de transporte, modelos de soporte más ricos, prefractura física o preparación GPU de colisión. Mantener prototipos CUDA/Python separados hasta demostrar beneficio, portabilidad o una razón válida para limitar su uso al cooker.

**Salida esperada:** informe A/B con coste total y mejora del resultado. **Condición de avance:** la herramienta resuelve una carencia real y supera una alternativa más sencilla. **No incluye automáticamente:** convertir Jerico en FEM/DEM geotécnico ni importar todas las dependencias de un paper.

## 11. Banco de pruebas propuesto

| Prueba | Escena / operación | Evidencia necesaria |
|---|---|---|
| Estratos | Capas planas y de espesor variable. | Espesores positivos, orden preservado y continuidad en cortes. |
| Pliegue | Deformación común progresiva. | Normales coherentes, sin inversión de dominio dentro del rango admitido. |
| Falla cronológica | Capas anteriores y veta posterior. | El desplazamiento afecta solo a las unidades previstas. |
| Junta finita | Fractura que termina dentro de la roca. | No se convierte en un plano infinito por límites de bloque. |
| Junta cerrada | Debilitamiento sin abertura. | No aparecen huecos ni componentes separados antes de la apertura. |
| Corte | Bloque con varias unidades materiales. | Interior nuevo clasificado con la misma geología. |
| Hermite | Caja y plano oblicuo. | Error geométrico, ángulos y normales medidos por separado. |
| Subvoxel | Lámina delgada y dos componentes próximos. | Diagnóstico explícito cuando la representación no los resuelve. |
| Fronteras | Grieta que cruza chunks y niveles de detalle. | Sin costuras; mismo resultado bajo distintos órdenes de evaluación admitidos. |
| Desgaste | Convexidad, concavidad y estratos resistentes. | Variación prevista, volumen perdido y bordes conservados. |
| Depósito | Suelo con obstáculos y espacio de paso protegido. | No invasión de keep-clear; masa declarada si se promete conservación. |
| Fractura pareada | Dos piezas con rugosidad común. | Ausencia de solapamientos inesperados, apertura controlada e interiores correctos. |
| Soporte | Retirada de un anclaje y ruptura de un vínculo. | Se activan solo componentes previstos; no se afirma cálculo de tensiones. |
| Colisión | Fragmentos cayendo sobre malla de cueva. | Proxies válidos, sin pares de colisión no soportados, masa/inercia coherentes. |
| LOD | Cámara próxima/lejana sobre contactos materiales. | Silueta y regiones protegidas; transiciones sin saltos inadmisibles. |
| Transacción | Cancelar, deshacer y cambiar receta durante un cook. | No se publica un candidato obsoleto ni un estado parcial. |
| Persistencia | Guardar, recargar y regenerar. | Misma identidad y estado; precisión de determinismo especificada. |
| Rendimiento | Misma escena, semilla y resolución en variantes. | CPU/GPU por etapa, memoria pico y latencia de edición/cancelación. |

### No confundir clases de verificación

**Compila:** solo prueba que ese build se pudo generar. **Corre:** añade evidencia de ejecución, no de corrección geométrica. **Se ve bien:** necesita contexto de cámara, iluminación y comparación. **Es más rápido:** exige mediciones equivalentes y repetidas. **Es físicamente fiel:** requiere modelos, unidades y validación adicional. Ninguna etiqueta sustituye a las demás.

Para cada experimento guardar receta, revisión, parámetros, capturas originales y métricas. Si se modifica el mallador y el material a la vez, una mejor imagen no permite atribuir la mejora. Cambiar una variable por comparación facilita encontrar la causa.

## 12. Decisiones que evitaría

No reemplazar Surface Nets/Hermite por un método nuevo antes de aislar un defecto que el actual no pueda resolver. No convertir una necesidad de roca más convincente en una dependencia inmediata de FEM, DEM o CUDA. No reintroducir OpenVDB sin una nueva decisión explícita y un caso de coste/beneficio.

No llamar «erosión 3D» a un addon que proyecta todo sobre altura. No afirmar que Voronoi produce física real de fractura. No usar un valor escalar de impacto como si fuese un tensor de tensiones. No tratar una malla cerrada como garantía completa de validez o de buena colisión.

No generar todos los microporos como triángulos ni hacer dinámicas todas las piedras decorativas. No duplicar la autoridad de estratos entre shaders, geometría y Damage. No introducir otro sistema de transacciones para un prototipo que después no pueda deshacerse o cocinarse por las rutas normales.

No usar un benchmark de una RTX 4090, A100 o escena de paper como presupuesto propio de Jerico. No presentar una licencia permisiva de un repositorio como permiso universal para sus datos, modelos, assets o dependencias. No equiparar «gratis» con «adecuado», ni «comercial» con «técnicamente inferior».

## 13. Licencias, mantenimiento y admisión

La tabla de recursos es un filtro inicial, no una revisión jurídica. Antes de añadir una dependencia debe fijarse una revisión o versión, conservar avisos y comprobar archivos de terceros. La rama principal puede cambiar; las condiciones observadas hoy no describen necesariamente una release anterior o futura.

En particular, OpenVDB publica actualmente Apache-2.0 y una nota de relicenciamiento; no corresponde repetir sin comprobar que todo OpenVDB es MPL-2.0. CoACD y CuACD tampoco deben agruparse bajo una sola licencia por similitud de nombre. [L20](#l20)[L15](#l15)[P14](#p14)

GPL o AGPL no significan «prohibido vender». Significan que hay obligaciones que deben evaluarse según cómo se use, modifique y distribuya el componente. Ejecutar una herramienta para producir un asset y copiar su implementación al motor son decisiones distintas; los outputs y contenidos incrustados pueden tener condiciones propias. Para cualquier caso dudoso debe revisarse el texto aplicable y obtener asesoramiento antes de distribuir.

### Ficha mínima antes de admitir una dependencia

Registrar versión/commit, licencia principal y terceros, propósito exacto, lugar de ejecución, plataformas, costes de build, vulnerabilidades/mantenimiento revisados, tamaño añadido y prueba de salida. Añadir un plan de sustitución cuando el recurso sea experimental.

Una dependencia que solo se necesita para comparar un resultado puede vivir fuera del producto. Una que genera assets durante cook no tiene necesariamente que enviarse con el juego. La decisión se hace explícita en el contrato del proyecto, no por accidente del sistema de compilación.

## 14. Propuesta concreta de prioridad

**Primero: estructura.** Formación con deformación compartida, capas de espesor controlado, juntas finitas y contactos materiales que sobreviven al corte. Es el trabajo que más directamente conecta apariencia, autoría y destrucción futura.

**Después: desgaste que entiende esa estructura.** Comparar operadores acotados sobre la geometría existente. Mantener por separado el desgaste de la superficie y el comportamiento de depósitos. No introducir física completa para resolver una mejora visual local.

**Luego: separación de piezas con estado coherente.** Fronteras compartidas, materiales interiores, vínculos, proxies y publicación transaccional. El beneficio debe verse en una galería pequeña antes de extenderlo a una mina.

**Más adelante: simulación especializada.** Transporte, modelos geomecánicos o herramientas CUDA solo cuando una escena y una métrica justifiquen su coste. Su existencia en un paper es una oportunidad de investigación, no una obligación de integración.

La idea final es sencilla: **el diseñador controla la mina; la estructura geológica hace que sus paredes parezcan pertenecer a una formación; el mallador conserva esa estructura; materiales y Damage la reutilizan; el runtime solo paga por la interacción necesaria.**

## 15. Limitaciones de esta investigación

No es un inventario exhaustivo de todos los artículos o productos existentes. Es una selección extensa centrada en roca volumétrica, cuevas, estratos, fracturas, desgaste y producción de assets para este proyecto. Los resultados negativos de una búsqueda no demuestran ausencia de una capacidad en otro motor.

No se descargaron ni ejecutaron paquetes de terceros en este trabajo. No se verificaron sus velocidades, compatibilidad con la instalación local de Blender, seguridad del código ni calidad de salida. Tampoco se auditó por completo el código actual de Jerico; solo se leyó contexto documental mediante el conector del repositorio.

Las etiquetas del catálogo distinguen texto integral accesible, ficha/resumen, documentación y README. Varias páginas de PDF estaban bloqueadas o fallaron; no se presenta una lectura de resumen como revisión completa del algoritmo. Las fuentes de 2026 marcadas como preprint o anuncio conservan ese estado.

No se modificó el repositorio, no se cambió GOAL.md, no se aprobó una dependencia y no se declaró terminado ningún slice. El dossier está pensado para elegir experimentos verificables antes de convertir una idea en trabajo del motor.

## 16. Catálogo de fuentes primarias

80 registros de fuentes. Algunos son documentación, licencias o repositorios complementarios de un mismo proyecto: no equivalen a 80 papers leídos íntegramente.

### Contexto del proyecto

<a id="j01"></a>
**J01 — JericoEngine — CODEX_INDEX.md**

Fuente: [https://github.com/JericoFX/JericoEngine/blob/main/docs/CODEX_INDEX.md](https://github.com/JericoFX/JericoEngine/blob/main/docs/CODEX_INDEX.md)  
Ámbito: Directiva de integración del Playground; no auditoría completa del código.  
Licencia / reutilización: Repositorio del usuario.  
Alcance de revisión: Lectura mediante conector GitHub, primer tramo del índice.

### Publicaciones

<a id="p01"></a>
**P01 — The PhaseTree: Multiphase Signed Distance Fields (2026)**

Fuente: [https://perso.liris.cnrs.fr/apeytavi/website/publication/hal-05713515/](https://perso.liris.cnrs.fr/apeytavi/website/publication/hal-05713515/)  
Ámbito: SDF multifase; DOI 10.1145/3811379.  
Licencia / reutilización: No se evalúa reutilización de código.  
Alcance de revisión: Ficha y resumen del autor; PDF enlazado no accesible en esta revisión.

<a id="p02"></a>
**P02 — PhaseTree — código de demostración**

Fuente: [https://github.com/Arches-Team/PhaseTree](https://github.com/Arches-Team/PhaseTree)  
Ámbito: GLSL; demostraciones del artículo.  
Licencia / reutilización: MIT.  
Alcance de revisión: README y licencia declarada; no compilado.

<a id="p03"></a>
**P03 — Directable Weathering of Concave Rock Using Curvature Estimation (2010)**

Fuente: [https://scholarsarchive.byu.edu/facpub/110/](https://scholarsarchive.byu.edu/facpub/110/)  
Ámbito: Meteorización volumétrica; DOI 10.1109/TVCG.2009.39.  
Licencia / reutilización: No se evalúa reutilización de código.  
Alcance de revisión: Resumen y ficha institucional; no lectura integral del PDF.

<a id="p04"></a>
**P04 — Arches: a Framework for Modelling Complex Terrains (2009)**

Fuente: [https://perso.liris.cnrs.fr/apeytavi/website/publication/hal-00463263/](https://perso.liris.cnrs.fr/apeytavi/website/publication/hal-00463263/)  
Ámbito: Terreno volumétrico y materiales; DOI 10.1111/j.1467-8659.2009.01385.x.  
Licencia / reutilización: No se evalúa reutilización de código.  
Alcance de revisión: Ficha/resumen primario recuperado; PDF bloqueado.

<a id="p05"></a>
**P05 — Terrain Amplification using Multi-scale Erosion (2024)**

Fuente: [https://research.adobe.com/publication/terrain-amplification-using-multi-scale-erosion/](https://research.adobe.com/publication/terrain-amplification-using-multi-scale-erosion/)  
Ámbito: Erosión multiescala de terrenos de altura; DOI 10.1145/3658200.  
Licencia / reutilización: No se evalúa reutilización de código.  
Alcance de revisión: Ficha de investigación y repositorio asociado.

<a id="p06"></a>
**P06 — MultiScaleErosion — implementación**

Fuente: [https://github.com/H-Schott/MultiScaleErosion](https://github.com/H-Schott/MultiScaleErosion)  
Ámbito: C++ / compute OpenGL; heightfields.  
Licencia / reutilización: MIT.  
Alcance de revisión: README y licencia declarada; no ejecutado.

<a id="p07"></a>
**P07 — Stochastic Geomorphological Transport (2026)**

Fuente: [https://erosiv.studio/publications/stochastic-geomorphological-transport](https://erosiv.studio/publications/stochastic-geomorphological-transport)  
Ámbito: Transporte geomorfológico; DOI 10.1145/3811336.  
Licencia / reutilización: No se evalúa reutilización de código.  
Alcance de revisión: Página de los autores y metadatos; no PDF completo.

<a id="p08"></a>
**P08 — geotransport — núcleo de integración**

Fuente: [https://github.com/erosiv/geotransport](https://github.com/erosiv/geotransport)  
Ámbito: Herramientas del modelo de transporte.  
Licencia / reutilización: MIT.  
Alcance de revisión: README; no compilado.

<a id="p09"></a>
**P09 — soillib — erosión y transporte**

Fuente: [https://github.com/erosiv/soillib](https://github.com/erosiv/soillib)  
Ámbito: C++ / CUDA / Python; herramientas de terreno.  
Licencia / reutilización: Licencia de la revisión a verificar antes de integrar.  
Alcance de revisión: Repositorio consultado; licencia no cerrada en esta revisión.

<a id="p10"></a>
**P10 — SubsurfaceGen (preprint, 2026)**

Fuente: [https://arxiv.org/html/2605.30541v1](https://arxiv.org/html/2605.30541v1)  
Ámbito: Operadores cronológicos de geología del subsuelo; PyTorch.  
Licencia / reutilización: La licencia del artículo no concede por sí sola licencia del código.  
Alcance de revisión: Texto HTML del preprint consultado.

<a id="p11"></a>
**P11 — Procedural Multiscale Geometry Modeling using Implicit Functions (preprint, 2025)**

Fuente: [https://arxiv.org/html/2504.09553v1](https://arxiv.org/html/2504.09553v1)  
Ámbito: Microestructura implícita multiescala.  
Licencia / reutilización: No se evalúa reutilización de código.  
Alcance de revisión: Texto HTML del preprint consultado.

<a id="p12"></a>
**P12 — OcMesher — Mesh Extraction for Unbounded Scenes Using Camera-Aware Octrees (2025)**

Fuente: [https://github.com/princeton-vl/OcMesher](https://github.com/princeton-vl/OcMesher)  
Ámbito: Extracción adaptativa dependiente de cámara.  
Licencia / reutilización: BSD-3-Clause.  
Alcance de revisión: Repositorio / ficha del proyecto; no ejecutado.

<a id="p13"></a>
**P13 — PaMO — Parallel Mesh Optimization (2025)**

Fuente: [https://github.com/SarahWeiii/pamo](https://github.com/SarahWeiii/pamo)  
Ámbito: Remallado y simplificación GPU / CUDA; incluye integración Blender.  
Licencia / reutilización: AGPL-3.0.  
Alcance de revisión: README y licencia declarada; no ejecutado.

<a id="p14"></a>
**P14 — CuACD (repositorio 2026)**

Fuente: [https://github.com/eliphatfs/cuacd](https://github.com/eliphatfs/cuacd)  
Ámbito: Descomposición convexa CUDA; anuncio SIGGRAPH Asia 2026.  
Licencia / reutilización: LGPL-2.1-or-later declarada en el repositorio consultado.  
Alcance de revisión: Repositorio consultado; evento anunciado, no tratado como celebrado.

<a id="p15"></a>
**P15 — Dual Contouring of Hermite Data (2002)**

Fuente: [https://www.cs.rice.edu/~jwarren/papers/dualcontour.pdf](https://www.cs.rice.edu/~jwarren/papers/dualcontour.pdf)  
Ámbito: Hermite, QEF, octrees y preservación de características.  
Licencia / reutilización: No se evalúa reutilización de código.  
Alcance de revisión: Texto PDF y capturas de páginas 1, 2 y 6 consultados.

<a id="p16"></a>
**P16 — Transvoxel — documentación y tesis**

Fuente: [https://transvoxel.org/](https://transvoxel.org/)  
Ámbito: Transiciones 2:1 para mallado de tipo Marching Cubes.  
Licencia / reutilización: Comprobar licencia exacta de las tablas/código al incorporarlos.  
Alcance de revisión: Página técnica del autor consultada; tesis no leída íntegramente.

<a id="p17"></a>
**P17 — Terrain Descriptors (2025)**

Fuente: [https://onlinelibrary.wiley.com/doi/10.1111/cgf.70080](https://onlinelibrary.wiley.com/doi/10.1111/cgf.70080)  
Ámbito: Descriptores para analizar terrenos.  
Licencia / reutilización: No se evalúa reutilización de código.  
Alcance de revisión: Ficha / resumen editorial consultado.

<a id="p18"></a>
**P18 — Terrain Synthesis and Authoring based on Iso-Contours (2026)**

Fuente: [https://h-schott.github.io/p/isos/](https://h-schott.github.io/p/isos/)  
Ámbito: Autoría por curvas de nivel; DOI 10.1111/cgf.70389.  
Licencia / reutilización: No se evalúa reutilización de código.  
Alcance de revisión: Página del autor consultada.

### Geología científica

<a id="g01"></a>
**G01 — LoopStructural**

Fuente: [https://github.com/Loop3D/LoopStructural](https://github.com/Loop3D/LoopStructural)  
Ámbito: Modelado estructural implícito en Python.  
Licencia / reutilización: MIT.  
Alcance de revisión: README y licencia declarada.

<a id="g02"></a>
**G02 — LoopStructural 1.0: time-aware geological modelling (2021)**

Fuente: [https://gmd.copernicus.org/articles/14/3915/2021/index.html](https://gmd.copernicus.org/articles/14/3915/2021/index.html)  
Ámbito: Cronología, marcos estructurales, fallas y pliegues.  
Licencia / reutilización: No se evalúa reutilización de código.  
Alcance de revisión: Artículo / ficha editorial primaria.

<a id="g03"></a>
**G03 — GemPy**

Fuente: [https://github.com/gempy-project/gempy](https://github.com/gempy-project/gempy)  
Ámbito: Geología implícita de subsuelo en Python.  
Licencia / reutilización: EUPL-1.2.  
Alcance de revisión: README y licencia declarada.

<a id="g04"></a>
**G04 — pySimFrac (artículo 2024 y código)**

Fuente: [https://github.com/lanl/pySimFrac](https://github.com/lanl/pySimFrac)  
Ámbito: Superficies rugosas de fractura; DOI 10.1016/j.cageo.2024.105665.  
Licencia / reutilización: BSD-3-Clause.  
Alcance de revisión: README y referencia del artículo; no ejecutado.

<a id="g05"></a>
**G05 — dfnWorks**

Fuente: [https://github.com/lanl/dfnWorks](https://github.com/lanl/dfnWorks)  
Ámbito: Redes discretas de fracturas, flujo y transporte.  
Licencia / reutilización: Avisos y licencias múltiples; revisar componentes.  
Alcance de revisión: README y LICENSE.md; no se considera admisión legal cerrada.

<a id="g06"></a>
**G06 — dfnWorks — licencia**

Fuente: [https://raw.githubusercontent.com/lanl/dfnWorks/master/LICENSE.md](https://raw.githubusercontent.com/lanl/dfnWorks/master/LICENSE.md)  
Ámbito: No resumir todo el paquete como MIT/BSD.  
Licencia / reutilización: Incluye avisos LANL, LGPL-3.0+, GPL-2.0+ y condiciones adicionales.  
Alcance de revisión: Texto de licencia consultado.

<a id="g07"></a>
**G07 — Landlab**

Fuente: [https://github.com/landlab/landlab](https://github.com/landlab/landlab)  
Ámbito: Procesos de superficie y evolución del paisaje.  
Licencia / reutilización: MIT.  
Alcance de revisión: README y licencia declarada.

<a id="g08"></a>
**G08 — Itasca — Mohr-Coulomb constitutive model**

Fuente: [https://docs.itascacg.com/itasca900/common/models/mohr/doc/modelmohr.html](https://docs.itascacg.com/itasca900/common/models/mohr/doc/modelmohr.html)  
Ámbito: Criterios de fallo, elasticidad/plasticidad y tracción.  
Licencia / reutilización: Documentación consultable; software comercial.  
Alcance de revisión: Documentación oficial consultada.

<a id="g09"></a>
**G09 — Itasca — Mohr-Coulomb joint/contact model**

Fuente: [https://docs.itascacg.com/itasca900/common/contactmodel/mohr/doc/manual/cmmohr.html](https://docs.itascacg.com/itasca900/common/contactmodel/mohr/doc/manual/cmmohr.html)  
Ámbito: Juntas, cohesión, fricción, dilatancia y presión de poro.  
Licencia / reutilización: Documentación consultable; no permiso para copiar software.  
Alcance de revisión: Documentación oficial consultada.

<a id="g10"></a>
**G10 — Physical Geology — Fractures, Joints, and Faults**

Fuente: [https://opentextbc.ca/physicalgeologyh5p/chapter/fractures-joints-and-faults/](https://opentextbc.ca/physicalgeologyh5p/chapter/fractures-joints-and-faults/)  
Ámbito: Terminología y patrones estructurales.  
Licencia / reutilización: Consultar licencia del libro y de cada imagen.  
Alcance de revisión: Capítulo abierto consultado.

### Bibliotecas

<a id="l01"></a>
**L01 — FastNoiseLite**

Fuente: [https://github.com/Auburn/FastNoiseLite](https://github.com/Auburn/FastNoiseLite)  
Ámbito: Ruido y domain warping; C++ / HLSL / GLSL.  
Licencia / reutilización: MIT.  
Alcance de revisión: README / licencia; no benchmark local.

<a id="l02"></a>
**L02 — FastNoise2**

Fuente: [https://github.com/Auburn/FastNoise2](https://github.com/Auburn/FastNoise2)  
Ámbito: Ruido SIMD en CPU; no es un backend GPU automático.  
Licencia / reutilización: MIT.  
Alcance de revisión: README / licencia.

<a id="l03"></a>
**L03 — Eigen**

Fuente: [https://gitlab.com/libeigen/eigen](https://gitlab.com/libeigen/eigen)  
Ámbito: Álgebra lineal, QR y SVD.  
Licencia / reutilización: Núcleo MPL-2.0; revisar componentes y versión.  
Alcance de revisión: Repositorio oficial y avisos del espejo consultados.

<a id="l04"></a>
**L04 — Eigen — avisos de licencia del espejo**

Fuente: [https://raw.githubusercontent.com/eigen-mirror/eigen/master/COPYING.README](https://raw.githubusercontent.com/eigen-mirror/eigen/master/COPYING.README)  
Ámbito: La selección real de módulos importa.  
Licencia / reutilización: MPL-2.0 y avisos de terceros.  
Alcance de revisión: Avisos de licencia consultados.

<a id="l05"></a>
**L05 — Manifold**

Fuente: [https://github.com/elalish/manifold](https://github.com/elalish/manifold)  
Ámbito: Booleanas sobre sólidos triangulados manifold.  
Licencia / reutilización: Apache-2.0.  
Alcance de revisión: README / licencia y condiciones de entrada.

<a id="l06"></a>
**L06 — geometry-central**

Fuente: [https://github.com/nmwsharp/geometry-central](https://github.com/nmwsharp/geometry-central)  
Ámbito: Geometría diferencial y estructuras de malla.  
Licencia / reutilización: MIT.  
Alcance de revisión: README / licencia.

<a id="l07"></a>
**L07 — libigl**

Fuente: [https://github.com/libigl/libigl](https://github.com/libigl/libigl)  
Ámbito: Procesamiento y análisis geométrico.  
Licencia / reutilización: MPL-2.0 en núcleo; módulos/dependencias con otras licencias.  
Alcance de revisión: README / licencias declaradas.

<a id="l08"></a>
**L08 — Geogram**

Fuente: [https://github.com/BrunoLevy/geogram](https://github.com/BrunoLevy/geogram)  
Ámbito: Geometría computacional, predicados y teselaciones.  
Licencia / reutilización: BSD-3-Clause en licencia principal.  
Alcance de revisión: README y licencia primaria consultados.

<a id="l09"></a>
**L09 — Geogram — licencia**

Fuente: [https://raw.githubusercontent.com/BrunoLevy/geogram/main/LICENSE](https://raw.githubusercontent.com/BrunoLevy/geogram/main/LICENSE)  
Ámbito: Aviso de reutilización de código.  
Licencia / reutilización: BSD-3-Clause.  
Alcance de revisión: Texto completo consultado.

<a id="l10"></a>
**L10 — Voro++**

Fuente: [https://math.lbl.gov/voro++/](https://math.lbl.gov/voro++/)  
Ámbito: Celdas Voronoi 3D.  
Licencia / reutilización: BSD de 3 cláusulas con avisos del proyecto.  
Alcance de revisión: Sitio de autores / licencia.

<a id="l11"></a>
**L11 — Voro++ — licencia**

Fuente: [https://raw.githubusercontent.com/chr1shr/voro/master/LICENSE](https://raw.githubusercontent.com/chr1shr/voro/master/LICENSE)  
Ámbito: Revisar el archivo completo al fijar versión.  
Licencia / reutilización: BSD con texto adicional sobre mejoras compartidas.  
Alcance de revisión: Texto completo consultado.

<a id="l12"></a>
**L12 — CGAL — licencias**

Fuente: [https://www.cgal.org/license.html](https://www.cgal.org/license.html)  
Ámbito: Geometría exacta y robustez; selección por módulo.  
Licencia / reutilización: GPL / LGPL / comercial según paquete.  
Alcance de revisión: Página oficial consultada.

<a id="l13"></a>
**L13 — meshoptimizer**

Fuente: [https://github.com/zeux/meshoptimizer](https://github.com/zeux/meshoptimizer)  
Ámbito: LOD, simplificación, organización y compresión de mallas.  
Licencia / reutilización: MIT.  
Alcance de revisión: README / licencia.

<a id="l14"></a>
**L14 — xatlas**

Fuente: [https://github.com/jpcy/xatlas](https://github.com/jpcy/xatlas)  
Ámbito: Desplegado UV para bake.  
Licencia / reutilización: MIT.  
Alcance de revisión: README / licencia.

<a id="l15"></a>
**L15 — CoACD**

Fuente: [https://github.com/SarahWeiii/CoACD](https://github.com/SarahWeiii/CoACD)  
Ámbito: Descomposición convexa para colisión.  
Licencia / reutilización: MIT en repositorio consultado; fijar revisión.  
Alcance de revisión: README / licencia; no ejecución.

<a id="l16"></a>
**L16 — Jolt — arquitectura**
Fuente: [https://jrouwe.github.io/JoltPhysics/](https://jrouwe.github.io/JoltPhysics/)  
Ámbito: Cuerpos, colisiones, proxies, restricciones de MeshShape dinámico.  
Licencia / reutilización: Código Jolt: MIT; documentación separada.  
Alcance de revisión: Documentación oficial consultada.

<a id="l17"></a>
**L17 — Jolt Physics — código**

Fuente: [https://github.com/jrouwe/JoltPhysics](https://github.com/jrouwe/JoltPhysics)  
Ámbito: Física de cuerpos; no generador de geología.  
Licencia / reutilización: MIT.  
Alcance de revisión: README / licencia.

<a id="l18"></a>
**L18 — Fidget**

Fuente: [https://github.com/mkeeter/fidget](https://github.com/mkeeter/fidget)  
Ámbito: Implícitos; evaluación/JIT y simplificación; Rust.  
Licencia / reutilización: MPL-2.0.  
Alcance de revisión: README; candidato de investigación, no integrado.

<a id="l19"></a>
**L19 — OpenVDB / NanoVDB**

Fuente: [https://github.com/AcademySoftwareFoundation/openvdb](https://github.com/AcademySoftwareFoundation/openvdb)  
Ámbito: Campos dispersos; no se propone reabrir su integración en Jerico.  
Licencia / reutilización: Apache-2.0 en la rama actual consultada.  
Alcance de revisión: README y nota oficial de relicenciamiento.

<a id="l20"></a>
**L20 — OpenVDB — relicenciamiento**

Fuente: [https://raw.githubusercontent.com/AcademySoftwareFoundation/openvdb/master/RE-LICENSE_NOTE.txt](https://raw.githubusercontent.com/AcademySoftwareFoundation/openvdb/master/RE-LICENSE_NOTE.txt)  
Ámbito: No extrapolar a cualquier release histórica.  
Licencia / reutilización: De aportaciones MPL-2.0 a Apache-2.0.  
Alcance de revisión: Nota oficial consultada.

<a id="l21"></a>
**L21 — NVIDIA Warp**

Fuente: [https://github.com/NVIDIA/warp](https://github.com/NVIDIA/warp)  
Ámbito: Prototipos CPU/CUDA desde Python.  
Licencia / reutilización: Apache-2.0; terceros a revisar.  
Alcance de revisión: README / licencia.

<a id="l22"></a>
**L22 — AMD FEMFX**

Fuente: [https://github.com/GPUOpen-Effects/FEMFX](https://github.com/GPUOpen-Effects/FEMFX)  
Ámbito: FEM tetraédrico multihilo en CPU y fractura.  
Licencia / reutilización: MIT.  
Alcance de revisión: README / licencia; integración y mantenimiento no probados.

<a id="l23"></a>
**L23 — NVIDIA Blast**

Fuente: [https://github.com/NVIDIAGameWorks/Blast](https://github.com/NVIDIAGameWorks/Blast)  
Ámbito: SDK de destrucción; estudiar separación de piezas/soporte.  
Licencia / reutilización: Revisar distribución y extensiones concretas.  
Alcance de revisión: README y enlaces de documentación; licencia no cerrada.

### Herramientas gratuitas

<a id="a01"></a>
**A01 — Cell Fracture**

Fuente: [https://extensions.blender.org/add-ons/cell-fracture/](https://extensions.blender.org/add-ons/cell-fracture/)  
Ámbito: Prefragmentación de mallas en Blender.  
Licencia / reutilización: GPL-3.0-or-later.  
Alcance de revisión: Ficha oficial; no instalado en esta sesión.

<a id="a02"></a>
**A02 — Hydra**

Fuente: [https://github.com/ozikazina/Hydra](https://github.com/ozikazina/Hydra)  
Ámbito: Erosión mediante mapas de altura y OpenGL.  
Licencia / reutilización: MIT declarado; README avisa de otras posibles limitaciones.  
Alcance de revisión: README; compatibilidad y admisión pendientes.

<a id="a03"></a>
**A03 — Sverchok**

Fuente: [https://github.com/nortikin/sverchok](https://github.com/nortikin/sverchok)  
Ámbito: Laboratorio de nodos geométricos y matemáticos.  
Licencia / reutilización: GPL-3.0.  
Alcance de revisión: README / licencia; no ejecutado.

<a id="a04"></a>
**A04 — Proc-Rock**

Fuente: [https://github.com/acfaruk/proc-rock](https://github.com/acfaruk/proc-rock)  
Ámbito: Generador independiente de rocas, recetas, materiales y LOD.  
Licencia / reutilización: GPL-3.0; README distingue los assets generados.  
Alcance de revisión: README; no ejecutado.

<a id="a05"></a>
**A05 — Blender-Rock-Generator**

Fuente: [https://github.com/lupin4/Blender-Rock-Generator](https://github.com/lupin4/Blender-Rock-Generator)  
Ámbito: Script pequeño de rocas; atención: puede vaciar la escena.  
Licencia / reutilización: MIT.  
Alcance de revisión: README / licencia; solo probar en escena desechable.

<a id="a06"></a>
**A06 — Infinigen**

Fuente: [https://github.com/princeton-vl/infinigen](https://github.com/princeton-vl/infinigen)  
Ámbito: Contenido natural procedural, Blender y generación offline.  
Licencia / reutilización: BSD-3-Clause en proyecto; dependencias se revisan aparte.  
Alcance de revisión: README / licencia.

<a id="a07"></a>
**A07 — Infinigen 2 preview — alcance**

Fuente: [https://github.com/princeton-vl/infinigen/blob/main/docs/source/Infinigen2.md](https://github.com/princeton-vl/infinigen/blob/main/docs/source/Infinigen2.md)  
Ámbito: No confundir preview de interiores con toda la ruta Nature.  
Licencia / reutilización: Ver licencia del proyecto y subcomponentes.  
Alcance de revisión: Documentación de alcance consultada.

<a id="a08"></a>
**A08 — Material Maker**

Fuente: [https://github.com/RodZill4/material-maker](https://github.com/RodZill4/material-maker)  
Ámbito: Materiales PBR y pintura procedural; no geometría volumétrica.  
Licencia / reutilización: MIT.  
Alcance de revisión: Repositorio del autor / licencia.

<a id="a09"></a>
**A09 — Ucupaint**

Fuente: [https://github.com/ucupumar/ucupaint](https://github.com/ucupumar/ucupaint)  
Ámbito: Capas de textura y máscaras para Blender.  
Licencia / reutilización: GPL-3.0.  
Alcance de revisión: README / licencia.

<a id="a10"></a>
**A10 — BlenderGIS**

Fuente: [https://github.com/domlysz/BlenderGIS](https://github.com/domlysz/BlenderGIS)  
Ámbito: Importación de datos geográficos / elevación.  
Licencia / reutilización: Verificar licencia exacta de versión y fuentes de datos.  
Alcance de revisión: README; licencia no cerrada en esta revisión.

<a id="a11"></a>
**A11 — A.N.T. Landscape — fuente histórica**

Fuente: [https://github.com/blender/blender-addons/tree/main/ant_landscape](https://github.com/blender/blender-addons/tree/main/ant_landscape)  
Ámbito: Terrenos de altura / ruido en Blender.  
Licencia / reutilización: Verificar paquete actual y licencia de versión.  
Alcance de revisión: Fuente histórica; compatibilidad actual no comprobada.

<a id="a12"></a>
**A12 — ErosionTerrainGenerator — ficha 2026**

Fuente: [https://extensions.blender.org/add-ons/erosion-terrain-extension/](https://extensions.blender.org/add-ons/erosion-terrain-extension/)  
Ámbito: Terreno de altura; declara Blender 5.2+.  
Licencia / reutilización: Verificar licencia de la versión antes de reutilizar.  
Alcance de revisión: Ficha oficial; versión 1 publicada 18-08-2026; no probado.

<a id="a13"></a>
**A13 — SideFX Labs**

Fuente: [https://github.com/sideeffects/SideFXLabs](https://github.com/sideeffects/SideFXLabs)  
Ámbito: Herramientas de autoría, HDA y Python.  
Licencia / reutilización: Licencia permisiva propia; host Houdini separado.  
Alcance de revisión: README y LICENSE.md consultados.

<a id="a14"></a>
**A14 — SideFX Labs — licencia**

Fuente: [https://raw.githubusercontent.com/sideeffects/SideFXLabs/Development/LICENSE.md](https://raw.githubusercontent.com/sideeffects/SideFXLabs/Development/LICENSE.md)  
Ámbito: No confundir con licencia de Houdini.  
Licencia / reutilización: Aviso permisivo propio de SideFX.  
Alcance de revisión: Texto de licencia consultado.

<a id="a15"></a>
**A15 — Houdini Apprentice — restricciones**

Fuente: [https://www.sidefx.com/faq/question/apprentice-restrictions/](https://www.sidefx.com/faq/question/apprentice-restrictions/)  
Ámbito: No es sustituto gratuito de una licencia comercial de producción.  
Licencia / reutilización: No comercial; restricciones de pipeline y formatos.  
Alcance de revisión: FAQ oficial consultada.

### Assets

<a id="a16"></a>
**A16 — Poly Haven — licencia**

Fuente: [https://polyhaven.com/license](https://polyhaven.com/license)  
Ámbito: Materiales, modelos y HDRI para comparación y producción.  
Licencia / reutilización: CC0 para assets; no todo el contenido del sitio.  
Alcance de revisión: Página oficial de licencia.

<a id="a17"></a>
**A17 — ambientCG — licencia**

Fuente: [https://docs.ambientcg.com/license/](https://docs.ambientcg.com/license/)  
Ámbito: Materiales y referencias de superficie.  
Licencia / reutilización: CC0 para assets.  
Alcance de revisión: Página oficial de licencia.

### Motores

<a id="e01"></a>
**E01 — Unreal Engine — Mesh Terrain experimental**

Fuente: [https://dev.epicgames.com/documentation/unreal-engine/mesh-terrain-in-unreal-engine](https://dev.epicgames.com/documentation/unreal-engine/mesh-terrain-in-unreal-engine)  
Ámbito: Voladizos, túneles, resolución variable y modificadores.  
Licencia / reutilización: Código/uso bajo condiciones de Epic; no asumir permiso de portado.  
Alcance de revisión: Documentación oficial actual consultada.

<a id="e02"></a>
**E02 — Unreal Engine — Landscape Heightmaps**

Fuente: [https://dev.epicgames.com/documentation/unreal-engine/importing-and-exporting-landscape-heightmaps-in-unreal-engine](https://dev.epicgames.com/documentation/unreal-engine/importing-and-exporting-landscape-heightmaps-in-unreal-engine)  
Ámbito: Ruta Landscape clásica de mapas de altura.  
Licencia / reutilización: Documentación de referencia.  
Alcance de revisión: Documentación oficial.

<a id="e03"></a>
**E03 — Unreal Engine — Chaos Destruction Overview**

Fuente: [https://dev.epicgames.com/documentation/unreal-engine/destruction-overview](https://dev.epicgames.com/documentation/unreal-engine/destruction-overview)  
Ámbito: Geometry Collections, clusters y conexiones.  
Licencia / reutilización: Documentación de referencia.  
Alcance de revisión: Documentación oficial.

<a id="e04"></a>
**E04 — Voxel Plugin — runtime edits and sculpting**

Fuente: [https://docs.voxelplugin.com/knowledgebase/blueprints/runtime-edits-and-sculpting](https://docs.voxelplugin.com/knowledgebase/blueprints/runtime-edits-and-sculpting)  
Ámbito: Escultura volumétrica; límites explícitos de física de islas.  
Licencia / reutilización: Ver edición, versión y licencia comercial/gratuita aplicable.  
Alcance de revisión: Documentación oficial.

<a id="e05"></a>
**E05 — Godot Voxel Tools — smooth terrain**

Fuente: [https://voxel-tools.readthedocs.io/en/latest/smooth_terrain/](https://voxel-tools.readthedocs.io/en/latest/smooth_terrain/)  
Ámbito: SDF, mallado suave, Transvoxel y LOD.  
Licencia / reutilización: Código del proyecto MIT.  
Alcance de revisión: Documentación oficial.

<a id="e06"></a>
**E06 — Godot Voxel Tools — generators**

Fuente: [https://voxel-tools.readthedocs.io/en/latest/generators/](https://voxel-tools.readthedocs.io/en/latest/generators/)  
Ámbito: Generación multihilo por bloques / modificadores.  
Licencia / reutilización: MIT en código del proyecto.  
Alcance de revisión: Documentación oficial.

<a id="e07"></a>
**E07 — Godot Voxel Tools — repositorio**

Fuente: [https://github.com/Zylann/godot_voxel](https://github.com/Zylann/godot_voxel)  
Ámbito: Arquitectura de terreno editable por bloques.  
Licencia / reutilización: MIT.  
Alcance de revisión: README / licencia.

<a id="e08"></a>
**E08 — Unity — Terrain Heightmaps**

Fuente: [https://docs.unity3d.com/ja/6000.0/Manual/terrain-Heightmaps.html](https://docs.unity3d.com/ja/6000.0/Manual/terrain-Heightmaps.html)  
Ámbito: Terrain nativo basado en altura; no todo el universo de extensiones.  
Licencia / reutilización: Documentación de referencia.  
Alcance de revisión: Manual oficial.

<a id="e09"></a>
**E09 — Guerrilla — GPU-Based Procedural Placement in Horizon Zero Dawn (2017)**

Fuente: [https://www.guerrilla-games.com/read/gpu-based-procedural-placement-in-horizon-zero-dawn](https://www.guerrilla-games.com/read/gpu-based-procedural-placement-in-horizon-zero-dawn)  
Ámbito: Colocación procedural GPU, no formación física de roca.  
Licencia / reutilización: Presentación de referencia.  
Alcance de revisión: Ficha oficial de la charla; no auditoría del motor actual.

<a id="e10"></a>
**E10 — GDC — Continuous World Generation in No Man’s Sky (2017)**

Fuente: [https://www.gdcvault.com/play/1024265/Continuous-World-Generation-in-Noby](https://www.gdcvault.com/play/1024265/Continuous-World-Generation-in-Noby)  
Ámbito: Pipeline de mundo continuo, voxels, mallas y población.  
Licencia / reutilización: Presentación de referencia.  
Alcance de revisión: Ficha oficial; no análisis íntegro del video.

<a id="e11"></a>
**E11 — Houdini — fracturing geometry**

Fuente: [https://www.sidefx.com/docs/houdini/dyno/fracturing.html](https://www.sidefx.com/docs/houdini/dyno/fracturing.html)  
Ámbito: Fractura, constraints, proxies y geometría detallada.  
Licencia / reutilización: Software comercial; documentación consultable.  
Alcance de revisión: Documentación oficial.