# JericoBookshelfCppLab

Generador procedural C++17 de estanterías viejas, libros, archivadores y cajas. Crea los vértices, las UV, los materiales PNG16, GLB y OBJ sin Blender ni Python en producción. Blender se usa únicamente para importar y renderizar el resultado en las herramientas de revisión.

## Compilar y usar

Requisitos: compilador C++17, CMake 3.16+, zlib. No hay descargas en el paso de compilación. nlohmann/json y el código de ruido con sus licencias se incluyen en third_party.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/jerico_bookshelf --recipe recipes/reference_four.json --out generated/reference --bake 256
```

El proyecto CMake admite un generador de Visual Studio y el ejecutable de la configuración Release. La compilación verificada aquí es Linux/GCC; MSVC no se ha probado. Los directorios de salida son argumentos; no hay rutas de la máquina de desarrollo codificadas en el generador.

La API de la biblioteca expone `default_bookshelf_recipe`, `build_bookshelf`, `validate_bookshelf` y `bake_bookshelf_materials`. El exportador/evaluador C++ es el núcleo checkpoint03, con deduplicación de familias del incremento del muelle. El snapshot fuente original no se modifica.

## Controles

- width: 0.90–2.10 m; height: 1.45–2.70 m; depth: 0.30–0.55 m
- compartments: entero de 3 a 6, con hueco útil mínimo de 0.24 m
- shelf_thickness: 0.025–0.055 m; post_width: 0.050–0.095 m
- wear: 0–1 controla el desgaste geométrico; fill: 0–1 controla población
- book_scale: 0.70–1.15, limitado por margen superior de 16 mm
- seed: entero con signo de 64 bits; variación derivada del ID de cada pieza/grupo

Se entregan tres recetas: reference_four, narrow_four y archive_five. Cambiar población conserva exactamente el bastidor; no consume una secuencia aleatoria global. Las inclinaciones tienen un vecino real más alto y los apilados descansan sobre el libro o la caja inferior. Hay espacios libres intencionados.

## Piezas y engine

Cada libro consta de dos tapas deformadas, bloque cerrado de hojas con cantos ondulados, lomo convexo, nervios, líneas y una etiqueta simple cuando corresponde. Los archivadores añaden un aro visual. Cada objeto tiene ID estable, OBB, support_id y metadatos grab con sus piezas agrupadas, conservados en GLB extras. No hay simulación de física, agarre interactivo, UI, ni integración en un engine.

La estructura incluye cuatro montantes, patas, baldas niveladas, apoyos laterales, trasera de tablones, travesaños, cornisa y clavos. Los ensambles tienen penetraciones intencionadas documentadas; no se certifica resistencia estructural. La tolerancia declarada de contacto geométrico es 1 mm, por biseles y deformación limitada.

## Materiales y formatos

Máximo 16 familias compartidas: seis maderas, seis cubiertas, papel, etiqueta, dorado e hierro. No hay atlas por libro. Cada familia produce siete mapas PNG16; GLB incluye seis mapas por familia y OBJ los mapas que puede representar. Las UV métricas originales se conservan además de las UV de textura. Los normales tangentes y las UV float32 se validan sobre la exportación real.

Entrega ligera: 256 px por atlas familiar. Original de revisión: 384 px, preservado por separado. Esto es una resolución compartida, no 256 px exclusivos por libro; el primer plano pierde detalle respecto a una textura de autoría de mayor tamaño. Puede regenerarse con `--bake 512` o más, aumentando almacenamiento. Los PNG16 y metadatos de autoría hacen que el GLB sin comprimir sea mayor que su ZIP.

OBJ/MTL aplana la geometría en metros Z-up; no conserva jerarquía de agarre/OBB ni extensiones PBR completas. Importadores pueden interpretar de modo distinto rugosidad y normales. Para metadatos y materiales completos, usar GLB. El README de formato junto al OBJ detalla estos límites.

## Verificación

Pruebas C++ de estructura/semilla, materiales compartidos, deformación de cantos y matriz de recetas. Oracle independiente Python de sólo lectura: topología float32, OBB, contactos reales, normales y GLB. El visor Blender no repara ni vuelve a exportar los assets.

Accesorios pendientes de otro incremento: taza, farol y botella. No se incluyen imágenes de referencia ni assets de terceros; la geometría y texturas de esta entrega son procedurales originales.
