# Jerico Cobweb Lab · v2-r1

Laboratorio independiente de telarañas procedurales con núcleo C++17 y controles CLI. No integra ni reutiliza código del Playground. Incluye presets radial, esquina y corredor, atlas de cobertura y orientación, exportación OBJ y renderer Blender.

Este checkpoint contiene la revisión del parser de semillas: solo acepta dígitos decimales sin signo. Conserva las recetas y la geometría de la versión 2.

## Compilar y probar

Requiere g++, Python 3 y, para las pruebas del renderer, Blender 4.3.2. CMake se incluye, pero la ruta comprobada usa g++.

```sh
bash scripts/build.sh
build/core_tests
python3 -m unittest discover -s tests -v
python3 scripts/test_render_contract.py --blender
```

También se puede ejecutar todo con `bash scripts/test.sh`.

## Generar y renderizar

```sh
build/cobweb_lab --preset radial --seed 42 --density .5 --broken .03 --out outputs/my_radial
build/cobweb_lab --preset corner --seed 42 --density .65 --broken .25 --out outputs/my_corner
build/cobweb_lab --preset corridor --seed 42 --density .9 --broken .18 --width .0045 --layers 2 --atlas 4096 --out outputs/my_corridor
blender -b --python scripts/render_lab.py -- --input outputs/my_corridor --output renders/corridor_001.png --light frontal --view beauty
```

Las salidas deben ser nuevas o vacías; reemplazar requiere `--overwrite` explícito. `scripts/generate_presets.sh outputs/new_run` genera las recetas para semillas 42 y 99.

Vistas: `beauty`, `debug`, `oblique`, `edge`. Luces: `frontal`, `side`, `back`. Cada render escribe su registro de parámetros; estos archivos generados no forman parte del respaldo fuente.

## Alcance

Código, pruebas, recetas y scripts propios, sin addons ni referencias gráficas incorporadas. El material es una aproximación artística. No hay simulación física, interfaz gráfica, integración en motor ni medición de rendimiento GPU. La geometría portadora permanece separada del detalle del atlas. Las superficies abiertas pierden silueta cerca de una vista tangente.

Este respaldo excluye renders, grandes salidas, documentos de investigación e informes de ejecución. No se ha añadido una licencia pública nueva.
