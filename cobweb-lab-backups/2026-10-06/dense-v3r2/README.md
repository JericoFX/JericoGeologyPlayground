# Jerico Cobweb Lab · variante densa v3

Copia independiente del checkpoint v2, dedicada a mejorar la composición del corredor. Conserva los 33 apoyos, 12 paños y 8.400 triángulos de la receta principal. No modifica el laboratorio original ni sus capturas. Los recursos geométricos y atlas de radial/esquina permanecen idénticos a v2 para las mismas recetas.

## Cambios visuales

- Tres focos elípticos de densidad asimétricos, con tamaños y posiciones distintos
- Armazón con direcciones y separaciones irregulares, evitando los arcos equidistantes del techo
- Trama secundaria amplia fuera de los focos, para mantener continuidad entre masas y apoyos
- Tres capas parciales con más concavidad y diferentes desplazamientos
- Anchos separados entre armazón, conexiones medianas y fibras finas; no se engrosó toda la tela

La primera pasada de esta variante quedó demasiado vacía. La revisión r2 amplió los focos y recuperó la trama secundaria. Las imágenes `variantv3_r2` son la comparativa aceptada para esta entrega, siempre con la cámara, iluminación y material de v2.

La r2 avanza hacia la referencia, pero sigue siendo un estudio: algunos paños conservan aspecto de abanico tensado y los recortes elípticos de daño se ven demasiado limpios, especialmente en vista oblicua. No representa un acabado orgánico final.

## Compilar y comprobar

Requisitos: g++ con C++17, Python 3 y Blender 4.3.2 para las comprobaciones de render.

```sh
bash scripts/test.sh
```

Ejecuta compilación estricta, pruebas C++ de bake, 19 pruebas del generador y cinco contratos del renderer más escena real Blender. CMakeLists.txt también está incluido; la ruta verificada en este entorno es el script g++.

## Reproducir r2

```sh
bash scripts/build.sh
build/cobweb_lab --preset corridor --seed 42 --density .9 --broken .18 --width .0045 --layers 2 --atlas 4096 --out outputs/new_v3_seed42
blender -b --python scripts/render_lab.py -- --input outputs/new_v3_seed42 --output renders/new_v3_front.png --view beauty --light frontal
```

Para la segunda semilla, sustituir 42 por 99 y usar otra carpeta y PNG. Las salidas existentes se rechazan por defecto; `--overwrite` solo se usa si se desea explícitamente reemplazarlas. Las capturas compartidas quedan congeladas en rutas con hash.

Controles CLI disponibles: `--preset`, `--seed`, `--density`, `--sag`, `--tension`, `--broken`, `--width`, `--layers`, `--quality`, `--atlas`, `--out`. No hay sliders gráficos.

Salida: OBJ único con UV/normales; atlas TGA de cobertura y orientación; grafo de autoría JSON con identidad de strand; receta, apoyos, AABB, focos y estadísticas. Es un objeto portador con islas; no hay tubos ni objetos individuales por fibra.

## Validación y presupuesto

Receta r2, semilla 42:

- 4.662 vértices, 8.400 triángulos, 12 paños
- 58.494 segmentos/curvas de autoría, horneados al atlas
- Cobertura media de atlas: 14,7354%, frente a 11,7562% de v2
- 33 apoyos idénticos a v2 y paso central de 1,44 × 2,40 m libre para las caras completas
- Semilla 99: 59.469 curvas de autoría, misma malla y apoyos

La densidad está localizada: las pruebas comprueban mayor cobertura cerca de cada foco que en su entorno. La media del atlas no equivale a cobertura de pantalla, tiempo GPU o calidad visual.

Dos atlas 4096² RGBA8 sin compresión ocupan 128 MiB. El bake usa cuatro buffers float (256 MiB a 4096²), grafo y memoria temporal adicional. No se midió rendimiento en tiempo real.

## Límites

Modelo geométrico artístico y shader de seda aproximado. No hay física, integración con motor, OIT, LOD, mipmaps, medidas GPU o validación temporal. Los anchos son de inspección artística, no diámetros biológicos reales. Las vistas de 16 muestras tienen ruido visible.

El daño sigue siendo una máscara espacial de cobertura; el grafo guardado conserva las curvas sin cortar. Las superficies pierden silueta al verlas de canto. La referencia contiene una escena mucho más compleja y no se pretende haberla reproducido con esta composición.

El código es propio y utiliza la biblioteca estándar de C++. No se incorporó código de addons o generadores externos ni se asignó una licencia nueva.

Se detectó y corrigió un auto-cruce de paños de la tercera capa en controles extremos. La corrección limita su desplazamiento en profundidad; la receta entregada de dos capas permanece byte-idéntica. La suite verifica intersecciones entre caras no adyacentes.
