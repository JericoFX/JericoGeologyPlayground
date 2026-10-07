# Cobertura de materiales y exportación nativa

Fecha de revisión: 2026-10-06. Este inventario cubre las siete rutas de generador expuestas por el ejecutable. Distingue el cálculo del material, su representación finita en imágenes y el formato de entrega. La evidencia visual corresponde a escenas concretas; no demuestra identidad para cualquier semilla, tamaño, cámara o motor.

## Resultado de la auditoría

La ruta productiva actual `receta JSON → C++ → geometría evaluada → atlas/materiales PNG → GLB` no ejecuta Python, `bpy`, Blender ni grafos procedurales externos. No lee texturas originales prehorneadas. Crea de nuevo cada textura a partir de la receta, la pieza, la posición métrica y las fórmulas portadas.

Las dependencias del ejecutable son C++17, la biblioteca estándar, nlohmann JSON incluida y ZLIB. Blender/Python quedan en las referencias, pruebas y visores offline. El visor de salida nativa conecta imágenes y constantes a Principled/Normal Map; no genera ruido, máscaras, altura, geometría o atlas.

**La equivalencia de funciones con todos los scripts antiguos no está completa.** OBJ/MTL ya dispone de escritor C++ con PNG nativos y límites explícitos del formato. No se reproduce el atlas global de Blender ni hay equivalente integrado de sus envoltorios de ZIP, nombres inmutables por hash y presupuesto de 20 MB. Esas funciones pendientes no se llaman ocultamente para producir GLB u OBJ nativos. Se detallan más abajo.

## Familias y selección de la referencia

Las rutas de la columna «Fuente» son relativas a `fixtures/sources/`; los snapshots originales permanecen sin cambios.

| Ruta nativa / generador mostrado | Fuente del material | Cálculo productivo C++ actual | Evidencia y límite |
|---|---|---|---|
| `fracture`: ventana industrial con vidrio aprobado, intacto/roto/vacío | `JericoGlassFractureStudy/glass_materials.py`, `industrial_materials.py`, `blender_adapter.py` | `sample_material`: vidrio con polvo de rebaje y parte baja, 11 segmentos analíticos de rayado, rugosidad/transmisión/IOR/coat, vidrio de canto separado; metal industrial para marco y herrajes | Muestras de dos dimensiones, coordenadas de rayas y cantos. Comparación PNG16/1024 de 52 piezas medida; los estados vacío/intacto cambian la presencia de superficies, no dependen de texturas fijas. Rayas de 0,24 mm pueden quedar subtexel. |
| `industrial`: paneles macizos/ventilados/con lamas y ventanas R2; cortes/grietas/desgaste dirigido | `JericoIndustrialPropsMaterialsR2Checkpoint_20261006_1927/industrial_materials.py` y `blender_adapter.py` | Acero gris/oscuro, ruido macro/meso/micro, corrosión inferior, vetas, desgaste perimetral, máscaras por lado/región/protección, metal y vidrio de corte, hierro/óxido | Semillas normales y extremas signed64, frente/dorso, umbrales ColorRamp y hardware. Imagen final de puerta con lamas. R2 ignora la clave libre `fine_breakup`, como su adapter original. |
| `finewear`: desgaste fino dirigido y continuo | `JericoIndustrialPropsFineWearCheckpoint_20261006_1921/fine_wear_adapter.py`, `industrial_materials.py`, `fine_wear_math.py` | Base industrial y mezcla adicional de color/rugosidad/metallic por ruptura de pintura de 1–6 mm; sólo frente y `fine_breakup is True`. `src/fine_wear.cpp` conserva validación/contornos y marca el perfil explícito | 20 casos de autoría/contornos; muestras originales del shader y prueba de aislamiento R2/FineWear, incluidos atlas. No se atribuye a esta familia una comparación final de color propia que aún no consta. No añade geometría ni un segundo bump: tampoco lo hace el shader fuente. |
| `wood`: puertas de tablones, refuerzo y ventanas de madera básicas | `JericoDoorsWindowsLab/blender_adapter.py`; materiales compartidos usados también por el adapter R2 | Madera vertical/horizontal, madera áspera/expuesta, elección de eje de la pieza, grano/micrograno, color/rugosidad/altura, hierro/óxido/suciedad y vidrio ordinario | Muestras de variantes de material y comparación final de puerta reforzada, más acercamiento independiente. No sustituye la validación geométrica de las variantes de rotura. |
| `singlepane`: ventanas de madera de un solo vidrio | `JericoWindowsSinglePaneLab/blender_adapter.py` y materiales básicos de madera/vidrio | Misma ruta de materiales básicos; la geometría y grupos nativos deciden dónde se aplican madera, hierro, suciedad y vidrio | Cubierto por el port numérico de los materiales compartidos y fixtures geométricos; no se inventa un render final específico de esta familia. |
| `damp`: ventanas de madera húmeda, envejecida y rota | `JericoDampWoodWindowsDirectedWIP_20261006_1956/wood_materials.py`; misma implementación de shader que `JericoDampWoodWindowsDirectedCheckpoint_20261006_1949`; vidrio aprobado | `wood_finish` controla edad, humedad, eje X/Z y altura; vetas finas/gruesas, humedad inferior/superior, manchas, rugosidad y bump. Alias real `damp_wood` y cara expuesta. Vidrio/herrajes por sus materiales nativos | Pruebas de edad/humedad/ejes y de `frame.left` real, además de imagen final old/broken. SHA del shader compartido: `6ef1f0a8b3faf997c9a61cef5250a0e6fd76c083b177cefa825898bb545c3383`. La referencia de desarrollo no implica una nueva aprobación estética del usuario. |
| `furniture`: mueble de cajones y banco de trabajo | `JericoWoodFurnitureCheckpoint_20261006_1956/blender_adapter.py`; revisión 1951 sólo si se pide explícitamente | Eje de grano X/Y/Z, desplazamiento estable por SHA-256 del ID, abrasión anisotrópica 1956, capa oscura/madera, suciedad por altura mundial, madera de canto, hierro/óxido/papel/suciedad | Muestras de tres ejes 1956 y seis muestras legacy 1951. Imagen final del mueble abierto con offset mundial cero. Cada pieza del mueble horneado registra procedencia 1956. El banco comparte shader pero no se le atribuye la imagen del mueble. |

`_material_revision="1951"` conserva el shader anterior de muebles, con sus parámetros anteriores de abrasión, rampas, roughness, bump y specular. La revisión por defecto es 1956; no se mezclan ambas calibraciones.

## Correspondencia de pasos productivos

| Paso del pipeline Python/Blender original | Equivalente productivo actual | Diferencia real o condición |
|---|---|---|
| Construcción de materiales y variantes por cara en `blender_adapter.py` | `src/materials.cpp::sample_material`, selección por material y metadatos; grupos `face_materials` y `face_material_sides` | No se serializa ni ejecuta un grafo de Blender. Se traducen las fórmulas y el orden de las mezclas. La tabla anterior delimita las ramas implementadas. |
| Texture Coordinate Object, Geometry Position, mapeos anisotrópicos y offsets | Posición local de cada objeto en metros; `world_point` sólo para altura mundial de muebles y transformación de base normal | La traslación del nodo no contamina las coordenadas Object. La rotación entra en la base del bump y vuelve a espacio local para evaluar altura. |
| Noise Texture 3D normalizado y ColorRamp | Cycles Perlin/fBM nativo: hash, gradientes, normalización 0,982, detalle fraccionario, lacunarity 2, floats y wrapping; LUT ColorRamp de 257 entradas | Es el algoritmo adaptado de Blender 4.3.2, no un ruido alternativo. Las fuentes y licencias Apache-2.0/BSD-3-Clause están en `third_party/blender_cycles/`. |
| Máscaras de región, feather, lado, protección y distancia a contorno | `directed_wear` y matemáticas nativas de selección/contornos | Se mantiene la semántica booleana exacta de FineWear y su perfil separado. La distancia al contorno se evalúa analíticamente, no mediante una textura de referencia. |
| Python `Random(203)` para arañazos y offsets por semilla | `PythonRandom`, offsets modulares seguros signed64 y SHA-256 nativo para muebles | El dominio nativo de semillas es signed64, no enteros Python arbitrariamente grandes. INT64_MIN/MAX y >2³² tienen regresiones. El caché de rayas guarda coeficientes derivados de bounds, nunca imágenes. |
| Aplicación de modificadores Bevel/WeightedNormal antes de exportar/hornear | `src/evaluate.cpp`, ejecutado antes del baker, suministra caras, UV métricas, grupos y normales por esquina | No es la topología exacta de Blender. La política flat/all-smooth se ha medido; los casos mixtos y miter/clamp conservan diferencias documentadas en la cobertura geométrica. |
| Bump de Cycles sobre la altura procedural | `sample_material_normal`: stencil forward y fórmula Cycles, normal geométrica/shading normal, distancia y fuerza fuente | La huella nativa es la media física del texel UV, independiente de cámara. El shader vivo de Cycles usa diferenciales de cada rayo; no son idénticos para todas las distancias de cámara/LOD. |
| `_prepare_atlas`: modifiers, soldado/bmesh, subdivisión de bordes largos, triangulación, `smart_project`, reparaciones | Evaluación y triangulación C++; atlas por pieza y chart de cara con giro a lado largo, ancla interior real, rejilla y padding | Se reemplaza la estrategia, no se porta `smart_project` ni su retesselación de exportación. UV métricas originales permanecen en `uvs`; atlas separado en `texture_uvs`. Cobertura cero causa rechazo, no color inventado. |
| `_joined_bake_mesh` y Cycles EMIT/NORMAL bake de mallas temporales | Rasterizador C++ muestrea material y normal en cada superficie; mezcla e interpolación por triángulo | No hay unión temporal ni bake en Blender. Atlas finito, padding e interpolación/mipmaps del consumidor pueden diferir. |
| Imágenes y empaquetado roughness/metallic | `src/png.cpp`, PNG16 nativo con ZLIB sin pérdidas; mapas de color, parámetros, superficie, normal, ORM, transmisión y specular | `material_params` no se llama ORM: sus canales son roughness/metallic/transmission. ORM estándar separado es (1,roughness,metallic). PFM sólo diagnóstico, excluido del GLB y paquetes. |
| `_portable_material` con imágenes → Principled | JSON nativo define mapas/constantes; `src/export_glb.cpp` escribe PBR glTF | Color sRGB; parámetros/normal Non-Color. IOR uniforme exacto, sin degradarlo a una textura cuantizada; si no es representable en el contrato de exportación se rechaza, no se inventa un IOR. |
| Exportador `bpy.ops.export_scene.gltf` | Escritor GLB2 C++, índices/atributos float32, jerarquía, transformaciones, UV0/UV1, tangentes y PNG embebidos sin modificación | Usa `KHR_materials_transmission`, `ior`, `clearcoat`, `specular`. El motor consumidor debe soportarlas. No prueba por sí solo el aspecto en Jerico/Playground/Diligent ni otros motores. |
| Normal tangent y conversión de coordenadas del exportador Blender | Basis (x,y,z)→(x,z,−y), atlas `TEXCOORD_0=(u,1−v)`, MetricUV en UV1, tangentes C++ | El GLB canónico embebe `textures.normal` original y usa `TANGENT.w=−rawUVsign`. `normal_gltf` verde invertido es diagnóstico legacy, no se embebe. Ver `glb-tangent-convention.md` y su prueba de 53.325 esquinas. |
| Exportación original OBJ/MTL | `src/export_obj.cpp::write_obj_mtl`, `--obj` y `export_native_obj`; posiciones/normales en mundo Z-up, atlas UV, MTL y PNG relativos | Copia color/normal intactos; separa roughness/metallic R/G a PNG16 gris cuando varían, conserva constantes/IOR. No conserva transmisión física, coat, jerarquía, pivotes, sockets ni segunda capa UV. `norm`/PBR dependen del importador; Blender 4.3.2 ignora `norm`. Ver revisión independiente abajo. |
| Wrappers originales de variantes, hashes inmutables, ZIP y límite <20.000.000 bytes | CLI genera una receta por invocación; JSON, GLB y OBJ/MTL con PNG; reportes/hashes de evidencia separados | **No hay equivalente nativo completo de esos wrappers/presupuesto.** PNG16 por pieza y gutters pueden producir más tamaño; ZLIB reduce sin pérdida pero no garantiza 20 MB. |
| Reimportación y render de verificación | Oráculos/visores offline Python+Blender, validador binario independiente | Son pruebas/consumidores. Nunca crean los assets productivos C++. La reimportación portátil se mide aparte del preview de JSON. |

La exportación PBR no añade AO, lightmaps, volumen óptico, animaciones, skeletons, física ni collision hulls que no estén implementados. Se conservan sockets/grips/metadatos de interacción, pero una descripción de interfaz no ejecuta el comportamiento del motor.

## Perfil explícito R2/FineWear y regresión adversarial

Durante esta auditoría se encontró y corrigió un fallo real: R2 conserva campos libres de configuración, incluido `fine_breakup`, pero su adapter no llama al shader FineWear. El sampler inicial sí lo activaba sólo por ese campo. Un `fine_breakup=true` alteraba una muestra real de R2 de roughness 0,76 a 0,635085; una cadena en ese campo provocaba una excepción. El primer oráculo también infería incorrectamente la familia a partir de la bandera.

Corrección actual:

- `build_fine_wear` / `apply_fine_wear_operations` escriben `metadata.native_material_profile="finewear_1921"`.
- El baker selecciona `_material_family="fine"` sólo desde ese perfil explícito. Una escena R2 sin perfil nunca activa el añadido FineWear.
- La API de muestras permite seleccionar deliberadamente `_material_family="fine"`; no lo deduce del ID, nombre de pieza o bandera.
- El oráculo recibe `family` explícita. No cambia la fuente según `fine_breakup`.
- La comparación de escenas fuente verifica la anotación nativa exacta y excluye exclusivamente ese campo adicional de procedencia antes de comparar todo el resto.

`tests/material_scope_tests.cpp` falló antes de la corrección y pasa después: cuatro metales con `true`, `false`, cadena, número y null en R2; wrappers FineWear; y el atlas de las seis caras de `door.sheet` real para ambos perfiles. El Cycles fuente confirmó que true/ausente/cadena dan el mismo material R2. Las 151 muestras incluyen 16 casos R2 adversariales nuevos. Esta corrección no modifica las cuatro escenas visuales finales, que no habilitan FineWear.

## Evidencia de material y visual

| Gate | Resultado registrado |
|---|---|
| Shader fuente, `fixtures/material-samples.json` | 151 muestras originales Cycles, error máximo absoluto 1,3113×10⁻⁶; tolerancia 3×10⁻⁵ |
| Bump fuente, `fixtures/material-normal-samples.json` | 12 muestras y tres rotaciones; error vectorial máximo 1,18141×10⁻⁶ con igual huella física especificada |
| Wrapper/contorno FineWear, `tools/fine_wear_oracle.py` | 20/20 casos, 5 rechazos esperados, 286.621 números, máximo 2,5674×10⁻¹⁶ |
| Píxeles reales de atlas, gate anterior PFM/TGA | 104 muestras, máximo no cuantizado 1,19209×10⁻⁶; evidencia histórica independiente del paso posterior a PNG16 |
| Packing de tres escenas completas | Industrial 71 piezas: 0 islas vacías; madera 154: 0; mueble 184: 0. Antes eran 32, 338 y 366 respectivamente |
| UV de atlas tras float32 | 98.972 caras: sin aristas/áreas colapsadas, inversión u overlap; UV métricas y asignaciones de material preservadas |
| PNG | Decodificación independiente y validación CRC/ZLIB; RGB/RGBA16, orientación, alfa y pérdida cero en recompresión |

Los cuatro renders finales usan PNG16, densidad de referencia 512, cámaras/luces iguales a sus referencias y 64 muestras Cycles. La inspección independiente corroboró vetas largas de madera sin el granulado áspero anterior, corrosión y lamas coincidentes, abrasión 1956 del mueble y humedad/vidrio de la ventana. Persisten diferencias finas de muestreo y geometría. Métricas sobre RGB de PNG, rango 0–1, sin retocar píxeles:

| Escena final | MAE imagen completa | MAE bounds del asset |
|---|---:|---:|
| `industrial_louvered` | 0,001522391 | 0,002973241 |
| `wood_reinforced` | 0,001589579 | 0,002483009 |
| `cabinet_open` | 0,000618264 | 0,001813453 |
| `damp_old_broken` | 0,001128167 | 0,001674347 |

Evidencia: `output/visual-family-final/independent-material-final-review.json` y las imágenes/settings/reportes adyacentes. El acercamiento de madera mide MAE 0,002792698 y RMSE 0,004996316 con el mismo bake, sin ajuste productivo por cámara. La comparación previa de vidrio aprobado PNG16/1024 mide MAE global 0,000416089 y de bounds 0,000702052; sus fuentes, escena e imágenes están fijadas por SHA en `output/visual-png16-1024/visual-evidence.json` y pertenecen a ese checkpoint anterior.

## Residuales que no deben ocultarse

1. El shader es continuo; las imágenes son finitas. Rayas, ruido fino, bordes de máscara, padding y mipmaps pueden variar con densidad/LOD. Subir resolución no elimina automáticamente todos los residuos.
2. El stencil del bump está portado; la huella productiva deriva del texel, no de cada rayo Cycles. Coincidencia de muestra con huella igual no significa identidad desde todas las cámaras.
3. PNG16 tiene cuantización residual; GLB usa float32 en atributos. El antiguo sesgo grande de normal neutra 8-bit se corrigió, no se declara precisión infinita.
4. Bevel/miter, triangulación y algunas normales de hardware mixto conservan diferencias del evaluador nativo frente a Blender. Los materiales no las corrigen ni las disimulan.
5. Atlas y estructura de texturas difieren de los archivos originales. El contrato preserva métrica/asignación y exige cobertura/validez, no igualdad de UV o bytes del atlas antiguo.
6. GLB PBR depende de soporte del consumidor. La reimportación Blender validada no certifica otro motor, física, interacción o todos los efectos ópticos.
7. OBJ/MTL está implementado con las pérdidas propias de ese formato documentadas abajo. Siguen pendientes empaquetado inmutable/ZIP y presupuesto fijo de 20 MB. No existe una dependencia Python oculta que complete esas funciones.
8. La evidencia visual no abarca todas las recetas, semillas o revisiones. Las familias de desarrollo conservan el estado de aprobación estética de su referencia; una métrica baja no cambia ese estado.

Para el detalle numérico y las licencias, ver `material-parity.md`; para tangentes y reimportación, `glb-tangent-convention.md`; para el resto del generador, la cobertura geométrica y `parity-matrix.md`.


## Cierre del contrato material OBJ/MTL

`src/export_obj.cpp` produce el paquete completo desde una Scene C++ evaluada y sus PNG nativos, sin volver a muestrear/hornear ni invocar Python o Blender. Se revisó el código congelado y se ejecutó una prueba independiente de material sobre dos piezas reales del test C++ (vidrio y acero), además de reexportar esos mismos datos con PNG predictivos. No se crearon nuevas escenas ni renders para esta revisión.

| Canal | Representación OBJ/MTL verificada | Límite |
|---|---|---|
| Color base | `Kd 1 1 1`, `map_Kd` al PNG16 sRGB original copiado byte a byte | Modelo de iluminación del consumidor puede diferir del PBR original |
| Normal | `norm` al PNG16 original OpenGL +Y, sin invertir verde ni alterar UV; bytes idénticos | Extensión de soporte variable; no se sustituye por `bump`, que significaría altura |
| Roughness / metallic | `Pr`/`Pm` exactos si son constantes certificadas; si varían, `map_Pr`/`map_Pm` con canales R/G de `material_params` replicados a RGB16 lineal | Extensiones PBR, no propiedades universales del MTL clásico |
| IOR | `Ni` con constante uniforme exacta (1,52 para vidrio y 1,5 para acero en la prueba) | `Ni` por sí solo no crea vidrio físicamente transmisivo |
| Transmisión | Comentario de limitación y `d 1` opaco | No se finge equivalencia usando `d`/`Tr`; se conserva la advertencia del exportador fuente y se recomienda GLB para transmisión |
| Specular / coat | `Ks`, cuando está presente, es una indicación legacy-Phong derivada de Specular IOR Level | No es conversión física PBR; coat y specular variable no se representan en este MTL |

Resultados de la revisión material independiente:

- Color y normal: archivos completos idénticos a los PNG de origen en ambas piezas.
- Acero: 16.384 texels por mapa de roughness/metallic, cada valor de 16 bits exactamente igual a su canal empaquetado original, incluyendo orientación de filas.
- Vidrio: `Pr=0.16`, `Pm=0`, `Ni=1.52`; `d=1`, sin `Tr`.
- Reader C++: recompresión nativa de entrada con filtros PNG Sub/Up/Average/Paeth; cuatro reexportaciones dieron los mismos valores exactos que la entrada None. El verificador independiente comprobó CRC, descompresión y reconstrucción de cada byte.
- Referencias de texturas relativas y nombres seguros; serialización numérica con locale clásico. La validación independiente del exportador incluye 21 casos corruptos rechazados, documentados por separado.

La prueba de consumidor **Blender 4.3.2**, sin reparaciones de importación, conservó color, roughness/metallic, IOR, geometría y normales de vértice, pero creó **0 nodos Normal Map**: ese importador ignora `norm`. El paquete conserva el mapa para consumidores que lo soporten. Por tanto este gate valida la exportación del contrato y registra una limitación real del consumidor; no demuestra igualdad visual OBJ con el shader original. El [manual oficial de Blender sobre OBJ](https://docs.blender.org/manual/en/4.1/files/import_export/obj.html) describe los parámetros PBR como extensiones del formato.

Evidencia persistente de esta revisión: `output/obj-material-review/report.json`, verificador y paquetes pequeños adyacentes. Las pruebas originales de exportación fuente están en `fixtures/sources/JericoGlassFractureStudy/export_assets.py`, donde también se advierte que OBJ no preserva la transmisión del vidrio.

## Ensayo de compresión PNG sin pérdida

Se midió un candidato C++ de filtros predictivos sobre el mayor PNG de color, normal y ORM del mueble final, manteniendo ZLIB nivel 6. El filtro adaptativo clásico por fila produjo: color −1,52%; normal +7,16%; ORM +2,13%. En conjunto aumentó el tamaño un 2,05%; elegir el menor de None/adaptativo sólo ahorró un 0,72% sobre esos tres archivos. Cada byte de píxel16-bit y cada chunk no-IDAT fueron iguales con decodificación independiente. No se cambió el escritor productivo por este resultado. Evidencia: `output/png-predictive-benchmark/report.json`. Esta medición acotada no promete alcanzar el presupuesto legacy de 20 MB sin cambiar la representación o densidad.
