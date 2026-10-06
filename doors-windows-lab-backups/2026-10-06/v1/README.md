# Jerico Doors & Windows Lab · v1

Laboratorio independiente de autoría procedural de puertas y ventanas con madera gastada, hierro y daño controlado. Es un prototipo Python de generación/exportación; no implementa un runtime C++, física completa ni animación de manos.

## Código y dependencias

Python estándar genera recetas, geometría, pivotes, sockets y puntos de agarre. Blender 4.3.2 prepara materiales, renders y exportación. No se incorporan modelos externos ni referencias fotográficas a este respaldo.

Los nodos mantienen transformaciones locales respecto al padre y pivotes independientes de la apertura. Las unidades del núcleo son metros, con Z arriba; glTF convierte a Y arriba. Los UV métricos originales se conservan en un canal separado del atlas PBR de exportación.

## Pruebas

```sh
python3 -m unittest discover -s tests -v
```

Fuera de Blender, las pruebas específicas de su API se omiten de forma explícita. Para ejecutar la suite completa dentro de Blender:

```sh
blender -b --python-exit-code 1 --python-expr "import sys,unittest; sys.path.insert(0,'.'); result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.discover('tests')); sys.exit(0 if result.wasSuccessful() else 1)"
```

## Generar y exportar

```sh
python3 generate.py --recipe recipes/door_reinforced.json --out output/door.json
python3 generate.py --kind window --width 1.4 --height 1.25 --thickness 0.048 --handle pull --angle 45 --out output/window.json
blender -b --python-exit-code 1 --python render_evidence.py -- --preset reinforced_timber --views reference_front closed open hardware damage uv
blender -b --python-exit-code 1 --python export_assets.py -- --recipe recipes/door_reinforced.json --out output/exports_reinforced
```

Las opciones CLI sustituyen los valores de receta. Las recetas incluidas cubren puertas y ventanas pequeñas, grandes, reforzadas, deterioradas y rotas. El daño sin soporte desactiva la interacción correspondiente.

## Alcance

Los sockets describen montaje e interfaces; los grips son referencias de autoría para futura IK, no poses completas de dedos. La apertura se valida con fixtures discretos, no mediante colisión dinámica continua.

El GLB conserva jerarquía, pivotes, semántica y texturas PBR preparadas; OBJ necesita el JSON para recuperar esa información y tiene menor fidelidad de transmisión. La resolución del atlas es finita. No se afirma rendimiento de motor ni paridad numérica con un runtime C++.

Este respaldo incluye solo código, pruebas, recetas y este README. Excluye GLB, OBJ, PNG, escenas, cachés, referencias, informes y manifiestos de entrega. No se ha añadido una licencia pública nueva.
