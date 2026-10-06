# Industrial Props Lab · checkpoint 01

Estado: PARCIAL. Se conserva el código de 2026-10-06 18:45 UTC antes del ajuste final de bisel fino. No se presenta como entrega completamente validada.

La suite registrada del checkpoint tiene 156 pruebas Blender aprobadas. Siete variantes completaron GLB/OBJ/JSON/PBR: chapa ciega, chapa con lamas, chapa con malla y lamas, tres estados de ventana y daño macro izquierdo.

Los exports `industrial_edge_damage` e `industrial_edges_cracks` están pendientes: el bisel decorativo introduce una microcara o UV degenerada y el validador rechaza el resultado. Pasar las pruebas unitarias no resuelve ese bloqueo. Las correcciones posteriores deben conservarse en otro checkpoint.

## Código y alcance

Python estándar implementa geometría y recetas propias; Blender 4.3.2 prepara materiales, renders, bake y exportación. No depende de otros laboratorios ni distribuye addons, referencias gráficas o activos externos.

- Corte: resta de perfiles poligonales extruidos y reconstrucción de interior/superficie cerrada.
- Grieta: caminos de anchura variable, ramificación acotada y profundidad escalonada; no simulación de tensiones.
- Desgaste: contornos reales y máscaras suaves sobre una cara y región seleccionadas; el cambio de material no elimina volumen.
- Vidrio: paño único entero, roto con hueco real o ausente.

La protección funcional rechaza por defecto el daño en montajes. Si se permite expresamente, se recalculan soporte, sockets y grips. Los límites geométricos son explícitos; no es un booleano universal para mallas arbitrarias.

## Ejecutar

```sh
python3 generate.py --recipe recipes/industrial_louvered.json --out output/scene.json
python3 generate.py --recipe recipes/industrial_edges_cracks.json --out output/damaged.json
python3 -m unittest discover -s tests -v
bash scripts/check_all.sh
```

La suite completa usa Blender. Las pruebas que dependen de su API se omiten explícitamente en Python normal.

```sh
blender -b --threads 3 --python-exit-code 1 --python export_industrial.py
blender -b --threads 3 --python-exit-code 1 --python render_industrial.py -- --recipe recipes/industrial_edges_cracks.json --detail --clay
```

El exportador intenta las nueve recetas, pero este checkpoint no puede completar las dos variantes finas indicadas. Los exports y renders generados no forman parte del respaldo Git.

## Contrato y límites

La fuente usa metros, Z arriba y frente −Y. GLB convierte a Y arriba; JSON conserva la base fuente. Materiales interiores, UV métricos y atlas PBR son datos separados de la geometría y de los montajes. El importador externo puede variar en transmisión y PBR.

No incluye runtime C++, Jolt, masas, colisiones de motor ni integración física. Las pruebas de protección y holgura no certifican resistencia real ni todos los fragmentos posibles.

Este respaldo contiene únicamente código, pruebas, recetas y este README. Excluye manifiestos de entrega, imágenes, GLB/OBJ, cachés, referencias y binarios. No se ha añadido una licencia nueva.
