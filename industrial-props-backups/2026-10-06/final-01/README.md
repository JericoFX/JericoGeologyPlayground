# Industrial Props Lab · cierre de exportación 01

Checkpoint inmutable de 2026-10-06 18:54 UTC. Conserva separado el checkpoint parcial anterior y no incorpora ajustes visuales posteriores.

El registro final original documenta 157 pruebas Blender aprobadas y nueve variantes GLB/OBJ/JSON/PBR exportadas, reimportadas y verificadas desde sus buffers. Incluye los dos casos finos que estaban bloqueados en checkpoint-01.

## Correcciones de exportación

- El bisel decorativo se omite únicamente donde el detalle cortado es menor que su radio; no se cambia la fuente booleana para ocultar un fallo.
- Las UV microscópicas que se redondean a área cero reciben islas reservadas antes del bake, conservando geometría y MetricUV.
- Los frames tangentes singulares usan una reconstrucción acotada con derivadas UV en doble precisión y validación posterior.

Las nueve salidas registradas no tienen triángulos geométricos/UV de área nula ni tangentes nulas. Estos resultados corresponden a las recetas verificadas, no a toda combinación arbitraria posible.

## Ejecutar y comprobar

Python estándar implementa geometría y recetas; Blender 4.3.2 prepara materiales, render y exportación. No se necesita otro laboratorio, referencia gráfica ni addon.

```sh
python3 generate.py --recipe recipes/industrial_louvered.json --out output/scene.json
python3 generate.py --recipe recipes/industrial_edges_cracks.json --out output/damaged.json
bash scripts/check_all.sh
blender -b --threads 3 --python-exit-code 1 --python export_industrial.py
```

La copia de solo código no contiene un GLB histórico inválido utilizado por un test de diagnóstico: esa prueba se omite explícitamente. En Python normal se omiten además las pruebas de API Blender. No confundir esas omisiones con fallos de generación ni presentarlas como casos ejecutados.

```sh
blender -b --threads 3 --python-exit-code 1 --python render_industrial.py -- --recipe recipes/industrial_edges_cracks.json --detail --clay
```

Renders, exports y archivos PBR se conservan o regeneran aparte; no forman parte de este Git.

## Matemática y límites

El código propio combina máscaras sobre cara/región seleccionadas, resta de perfiles poligonales extruidos, cavidades, muescas, perforaciones y grietas ramificadas acotadas. La protección funcional rechaza por defecto daño en montajes; un override explícito permite recalcular soporte y desactivar sockets/grips afectados.

La fuente usa metros, Z arriba y frente −Y; GLB convierte a Y arriba. JSON conserva la base de autoría, pivotes e interfaces. Materiales interiores, UV métricos y atlas PBR se mantienen separados.

No es un booleano universal, una simulación de fractura ni un sistema físico/runtime de engine. No incorpora Jolt, C++ de producción, cargas ni cerraduras. La matemática no certifica conectividad global de todos los fragmentos ni desgaste natural completo.

Este respaldo contiene código, pruebas, recetas y documentación mínima propios. Excluye referencias, addons originales, imágenes, binarios, manifiestos y reportes de entrega. No se ha añadido una licencia nueva.
