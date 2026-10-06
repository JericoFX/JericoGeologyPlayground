# Finewear · estudio de material 01

Checkpoint independiente de material fino, sin nuevas cavidades, grietas ni modificación de la geometría verificada. No es una declaración de calidad AAA ni un cambio de física o runtime.

`fine_wear_before.json` y `fine_wear_after.json` conservan la geometría de `industrial_edge_damage`; sólo cambia `fine_breakup: false/true`. El registro comparativo verifica atributos GLB idénticos y once recetas anteriores intactas. Los cambios medidos afectan color, roughness y metallic; el mapa normal permanece idéntico.

## Integración

`edge_wear.fine_breakup: true` activa variaciones de recubrimiento de 1–6 mm dentro de la máscara de contorno frontal. Es opt-in, booleano y se rechaza en la cara posterior. Ausente o falso conserva el material previo.

El importador y el bake PBR usan el mismo adaptador. Se copia el material de forma privada y se conserva el valor o enlace previo fuera de la máscara. El exportador reemplaza slots en lugar de vaciarlos, para no perder los índices de materiales de las caras al crear la copia temporal de bake.

## Ejecutar

Requiere Python estándar y Blender 4.3.2 para adapter, bake, render y pruebas Blender.

```sh
bash scripts/check_all.sh
python3 generate.py --recipe recipes/fine_wear_after.json --out output/fine-wear-source.json
blender -b --python-exit-code 1 --python export_assets.py -- --recipe recipes/fine_wear_before.json --out output/fine-before
blender -b --python-exit-code 1 --python export_assets.py -- --recipe recipes/fine_wear_after.json --out output/fine-after
```

`render_fine_portable.py --glb <archivo.glb>` renderiza la reimportación con sus texturas PBR reales, sin sustituirlas por el shader procedural fuente. Usar archivos generados nuevos para las pruebas.

## Pruebas y límites

La suite del checkpoint registra 170 casos. En Blender, un caso se omite porque el GLB histórico de diagnóstico no está incluido en la copia aislada; Python normal omite además las pruebas que requieren Blender. No presentar esos casos como ejecutados.

Las comparativas BEFORE/AFTER deben conservar cámara, iluminación, resolución y muestras. Un atlas finito aproxima el shader; no se promete equivalencia píxel a píxel con el material analítico ni rendimiento de motor.

La receta CSG experimental `dense_wear_continuous.json`, rechazada por exportación degenerada, queda fuera de este checkpoint validado de material. No forma parte del resultado BEFORE/AFTER.

Sólo se respaldan código, pruebas, recetas pertinentes y este README. No se incluyen addons originales, imágenes, GLB, outputs, manifiestos ni la prueba de 2048. Los cierres anteriores permanecen separados.
