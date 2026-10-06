# Industrial Materials R2 · corrección del bake multimat

Checkpoint independiente basado en el cierre industrial de 18:54 UTC. El único cambio de producción corrige `_joined_bake_mesh` en `export_assets.py`: los slots de la copia temporal se reemplazan sin vaciar el array, para conservar los índices de material por cara. Vaciarlo reiniciaba los índices y perdía materiales secundarios durante el bake.

No incorpora finewear, nuevo vidrio ni cambios de geometría, UV, shaders o límites de validación. Los checkpoints previos quedan separados.

## Verificación y alcance

Las dos regresiones nuevas comprueban índices y píxeles de un bake real con dos materiales. El registro de la revisión compara las nueve geometrías, todos los accesores GLB y los nodos: coinciden con la entrega anterior. Los quince mapas de los tres controles no afectados son byte-idénticos; seis variantes corrigen sus mapas PBR.

No se midió una máscara raster por cara con padding, por lo que no se afirma identidad píxel a píxel fuera de cada región de material secundario.

## Ejecutar

Requiere Python estándar y Blender 4.3.2 para los tests y la exportación asociados a su API.

```sh
bash scripts/check_all.sh
blender -b --threads 3 --python-exit-code 1 --python export_industrial.py
```

La suite contiene 159 casos: fuera de Blender se omiten sus pruebas específicas; también se omite el diagnóstico del GLB histórico ausente en el checkpoint aislado. No contar esos casos como ejecutados.

Los scripts de comparación y empaquetado conservan el flujo de la entrega original: requieren los exports anteriores/nuevos y sus registros restaurados o regenerados, con las rutas relativas indicadas por esos scripts. El respaldo de código no incluye esas imágenes, GLB o informes. Para obtener assets nuevos, partir de las recetas incluidas y del exportador.

## Límites

Los formatos se verifican desde sus buffers y con reimportación Blender, pero un atlas finito aproxima el material procedural. No hay declaración de calidad AAA, simulación física ni integración de motor. OBJ necesita sus mapas y JSON para recuperar material y semántica.

Se preservan únicamente código, pruebas, recetas y este README. No se incluyen addons originales, referencias, imágenes, assets, manifiestos de entrega ni IDs Library. La licencia existente no cambia.
