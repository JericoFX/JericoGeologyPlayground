# Entrega a DOT / Punto: playground independiente de rocas

Fecha: 2026-10-05
Destinatario: DOT / Punto, agente que trabaja en la máquina local del usuario.
Estado: referencias y encargo preparados para ejecución posterior por DOT.
Esta entrega documental no acredita recepción, ejecución ni resultados en esa máquina.

## Petición del usuario

Compartir la investigación de Practical Procedural Volumetric Terrains for Games y sus referencias para que DOT cree en su máquina un playground dedicado únicamente a rocas.

El repositorio actual sirve como canal de entrega. El nuevo laboratorio debe tener un espacio de trabajo y una aplicación independientes; no convertir el laboratorio de pipes, cables y sockets en otro proyecto.

## Límites

- Trabajar únicamente en el laboratorio de rocas. No modificar JericoEngine2 ni integrar resultados en él.
- No borrar ni sustituir el Playground existente ni reactivar en bloque su antiguo GOAL de geología.
- Leer primero el estado actual y las instrucciones aplicables. Identificar matemáticas reutilizables ya verificadas antes de duplicarlas.
- No asumir que las rutas de otra máquina existen en esta. Elegir una carpeta nueva identificada claramente para este laboratorio y registrar su ubicación.
- El objetivo es estudiar y visualizar rocas individuales y piezas rocosas modulares. Un mundo completo, vegetación, ríos, cuevas extensas y simulación geológica global quedan fuera de este encargo.
- No añadir dependencias ni sistemas de físicas, scripting o plugins sin una necesidad concreta del experimento.
- Separar lo implementado, lo probado y lo visualmente demostrado.

La directiva docs/PROCEDURAL_MATH_PLAYGROUND_2026_10_03.md sigue siendo referencia del laboratorio general. Este encargo define un laboratorio separado y no cambia sus prioridades.

## Fuentes para leer

1. Axel Paris, Practical Procedural Volumetric Terrains for Games, 29 de junio de 2026:
   https://newheadstudio.com/blog/volumetric_terrain_modeling.html
   Expone una aplicación práctica de formas implícitas, excavación mediante esferas, deformación para estratos y conversión final a malla. El mundo del ejemplo se prepara al cargar; no es evidencia de regeneración completa cada frame.

2. Paris et al., Terrain Amplification with Implicit 3D Features, 2019:
   https://aparis69.github.io/public_html/projects/paris2019_3D.html
   La página del autor enlaza paper, vídeo, vídeo complementario, diapositivas y código. Estudiar especialmente la separación entre forma y resistencia geológica, los operadores locales y la edición manual combinada con generación procedural.

3. Implementación pública del paper:
   https://github.com/aparis69/Implicit-Volumetric-Terrains
   El README describe C++ sin dependencias, licencia MIT y tres OBJ de ejemplo. Es una reimplementación parcial: no reproduce necesariamente los tiempos del paper y no incluye el Marching Cubes optimizado ni el crecimiento de hoodoos por gramática. Mantener atribución y licencia al reutilizar código.
   No presentar el repositorio como una solución GPU completa.

4. Ju et al., Dual Contouring of Hermite Data, 2002:
   https://www.cs.rice.edu/~jwarren/papers/dualcontour.pdf
   Referencia para intersecciones, normales, QEF y conservación de rasgos. Relacionar con la ruta Hermite existente; no sustituir el extractor sin una comparación que lo justifique. El artículo de Newhead cita por error 2004.

5. Scrawk / Marching-Cubes:
   https://github.com/Scrawk/Marching-Cubes
   Referencia de extracción para comparación, no obligación de introducir Unity.

6. Laine y Karras, Efficient Sparse Voxel Octrees, 2010:
   https://research.nvidia.com/publication/2010-02_efficient-sparse-voxel-octrees
   Lectura secundaria sobre almacenamiento y consultas. Un octree grande no es requisito para demostrar una roca.

## Primer experimento sugerido

Esta es una propuesta inicial para acotar el trabajo, no una arquitectura ya implementada o validada:

- Mostrar una roca individual en un visor con cámara orbital e iluminación sencilla.
- Partir de una masa base y comparar el original con una variante estratificada y erosionada.
- Usar seed explícita y controles comprensibles de tamaño, grosor de capas, resistencia y cantidad de desgaste.
- Aplicar desgaste localizado según una máscara o región delimitada; evitar que el ruido destruya toda la silueta.
- Ofrecer vista neutra y alambre para distinguir geometría real de efectos del material.
- Guardar la receta y exportar la malla para inspección externa.

Priorizar forma, silueta y control artístico. La texturización avanzada, los bakes y la optimización GPU se justifican después mediante un caso medido.

## Precauciones técnicas concretas

- Un campo implícito válido para extraer su superficie cero puede no ser un SDF de distancia exacta. No reutilizarlo como distancia fiable para sphere tracing o contactos.
- Las deformaciones deben tener amplitud y región de influencia controladas.
- Preservar raíces y normales analíticas cuando estén disponibles; no asumir que subir la resolución arregla todos los bordes redondeados.
- Medir por separado evaluación del campo, extracción de malla, transferencias CPU/GPU y memoria. Las cifras del paper sobre el árbol de construcción no son el coste total de la malla final.
- Reutilizar herramientas existentes cuando simplifiquen el primer experimento. No reescribir el motor ni construir un framework general.

## Evidencia de la primera entrega de DOT

Entregar la ubicación del nuevo laboratorio, instrucciones de compilación y ejecución, una receta reproducible, la malla exportada y capturas reales desde varios ángulos con material neutro y alambre.

Registrar seed, parámetros, conteo de vértices/triángulos y tiempos medidos con el hardware utilizado. Comprobar normales y triángulos degenerados; si se declara una roca sólida cerrada, demostrar ese cierre.

Compilar no equivale a calidad visual aceptada. Explicar defectos visibles y limitaciones sin marcarlos como resueltos.

## Git

Sincronizar antes de trabajar y revisar cambios locales para conservar trabajo ajeno. Mantener el encargo de rocas separado de otros experimentos. Al terminar una entrega verificada, commit y push según las reglas del repositorio correspondiente.

No se autoriza borrar proyectos existentes, cambiar sus objetivos ni integrar el laboratorio en JericoEngine2.
