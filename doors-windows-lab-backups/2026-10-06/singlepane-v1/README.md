# Ventanas de paño único · checkpoint v1

Variante independiente con marco fijo, sin cuadrícula ni hojas interiores. Incluye vidrio entero, vidrio roto con hueco geométrico real y sólo marco, en tamaños estándar y alto. El vidrio ausente desactiva su socket; el marco fijo no introduce pivotes móviles ni agarres ficticios.

Es un prototipo Python de autoría/exportación con adaptador Blender 4.3.2, no un runtime C++ ni una simulación dinámica. Conserva recetas y funciones de la base anterior; las pruebas comprueban que la puerta reforzada previamente aprobada permanece idéntica.

## Comprobar

Python estándar cubre el núcleo y las recetas. Blender aporta bpy/mathutils para materiales, escena y exportación. No hay dependencias pip adicionales.

```sh
bash scripts/check_all.sh
```

El script ejecuta primero unittest con Python normal y después la suite completa dentro de Blender. Las pruebas que requieren Blender se omiten explícitamente en la primera ejecución.

## Generar y exportar

```sh
python3 generate.py --kind window --glazing intact --width 1.12 --height 1.28 --thickness .06 --out output/window.json
python3 generate.py --recipe recipes/singlepane_standard_broken.json --out output/window-broken.json
blender -b --python-exit-code 1 --python render_singlepane.py
blender -b --python-exit-code 1 --python export_assets.py -- --recipe recipes/singlepane_standard_empty.json --out output/exports
```

Las seis recetas singlepane cubren los tres estados y dos tamaños. El router mantiene disponibles las recetas históricas de puerta y ventana.

## Límites y datos

El núcleo usa metros y Z arriba; glTF aplica su conversión a Y arriba. Se conservan UV métricos además del atlas PBR de exportación. Los sockets expresan interfaces de montaje; los grips de las piezas móviles heredadas son referencias de autoría, no IK de manos completa.

No se añadió desgaste nuevo de cantos, física de colisiones de runtime ni integración de motor. El roto del vidrio sí pertenece a esta revisión.

Este respaldo solo contiene código, pruebas, recetas y documentación mínima. No incluye GLB/OBJ, PNG, escenas, cachés, referencias ni informes/manifiestos de entrega. No se ha añadido una licencia pública nueva.
