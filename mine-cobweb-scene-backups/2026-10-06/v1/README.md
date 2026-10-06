# Mina con roca y telarañas · escena v1

Ensamblaje independiente en Blender de roca matemática y el recurso de telarañas dense-v3r3. Este respaldo conserva código, pruebas y ocho recetas de roca. Las rutas de entrada se resuelven desde esta carpeta.

## Entradas necesarias

Antes de validar o renderizar, restaurar los recursos binarios archivados por separado o regenerarlos con las CLI del proyecto:

- `assets/silk/`: salida completa del corredor de telarañas dense-v3r3, incluida su malla OBJ, atlas, receta y metadatos.
- `sources/mass-48/`, `sources/mass-117/`, `sources/mass-305/`, `sources/mass-791/`: salidas de las recetas `wall-mass` incluidas.
- `sources/layered-48/`, `sources/layered-117/`, `sources/layered-305/`, `sources/layered-791/`: salidas de las recetas `wall-layered` incluidas.

Cada salida de roca debe contener `heightfield.obj` y `recipe.json`. Ejemplo con la CLI C++ de rocas ya compilada: `rocks_cli generate --recipe recipes/wall-mass-48.json --out sources/mass-48`.

Los binarios, imágenes, paquetes de restauración y manifiestos de entrega no están incluidos en Git. Las carpetas de recursos y renders están excluidas mediante `.gitignore`.

## Comprobar y renderizar

Requiere Python 3 y Blender 4.3.2. Las pruebas unitarias no necesitan los recursos binarios:

```sh
python3 -m unittest discover -s tests -v
```

Con las entradas restauradas, validar sin producir imagen:

```sh
blender -b -t 8 --python-exit-code 1 --python scripts/render_mine.py -- --output renders/validacion.png --view front --width 1200 --samples 64 --validate-only
```

Producir vistas nuevas:

```sh
blender -b -t 8 --python-exit-code 1 --python scripts/render_mine.py -- --output renders/nueva-frontal.png --view front --width 1200 --samples 64 --save-blend
blender -b -t 8 --python-exit-code 1 --python scripts/render_mine.py -- --output renders/nueva-oblicua.png --view oblique --width 1200 --samples 64
```

El script rechaza un PNG o registro de salida existente. La validación genera metadatos con `rendered=false`; no demuestra que se haya renderizado una imagen nueva.

## Alcance

Estudio artístico con mallas rocosas cerradas, mezcla geométrica de módulos y telarañas previamente generadas. No añade simulación física, rotura dinámica ni integración en un motor. El código del renderer y las recetas se conservan sin modificar; la escena pesada y los recursos se archivan aparte.
