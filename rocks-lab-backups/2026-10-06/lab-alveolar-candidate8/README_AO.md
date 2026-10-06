# Renderizador AO opcional

`tools/render_alveolar_ao.py` compara la misma geometría con y sin un refuerzo suave de oclusión ambiental en Blender Cycles. El generador de geometría no cambia.

Pruebas autocontenidas, sin archivos de render externos:

```sh
python3 -m unittest discover -s tests -p 'test_alveolar_ao.py' -v
```

Para renderizar hacen falta Blender y las entradas originales de `renders/alveolar-v14`: provenance, imágenes y OBJ/archivos auxiliares cuyos hashes se verifican. Esas salidas grandes no se incluyen en este respaldo de código; deben regenerarse o recuperarse antes.

```sh
blender -b -t 8 -P tools/render_alveolar_ao.py -- --seeds 48 117 305
python3 tools/render_alveolar_ao.py --compose-only --seeds 48 117 305
```

Las comparativas requieren Pillow. El script conserva las salidas previas y rechaza sobrescribir su propia salida.
