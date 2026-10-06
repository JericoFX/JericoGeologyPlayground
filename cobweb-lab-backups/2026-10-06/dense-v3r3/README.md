# Jerico Cobweb Lab · variante densa v3-r3 (generador 4)

Copia independiente del checkpoint v3-r2, con un único ajuste final de daño localizado. Conserva los 33 apoyos, 12 paños y 8.400 triángulos de la receta principal. No modifica el laboratorio original ni sus capturas. Los recursos geométricos y atlas de radial/esquina permanecen idénticos a v2 para las mismas recetas.

## Cambios visuales

- Tres focos elípticos de densidad asimétricos, con tamaños y posiciones distintos
- Armazón con direcciones y separaciones irregulares, evitando los arcos equidistantes del techo
- Trama secundaria amplia fuera de los focos, para mantener continuidad entre masas y apoyos
- Tres capas parciales con más concavidad y diferentes desplazamientos
- Anchos separados entre armazón, conexiones medianas y fibras finas; no se engrosó toda la tela

La primera pasada de esta variante quedó demasiado vacía. La revisión r2 amplió los focos y recuperó la trama secundaria. Las imágenes `variantv3_r2` conservan el baseline de composición. Esta entrega añade solo `r3damage`, siempre con la misma cámara, iluminación y material.

La r2 avanza hacia la referencia, pero mantiene paños de aspecto tensado. La r3 rompe localmente los recortes elípticos con bordes irregulares y dos cortes direccionales discontinuos por hueco. No modifica fibras, malla, apoyos, grosor ni material. Sigue siendo un estudio, no un acabado orgánico final.

## Compilar y comprobar

Requisitos: g++ con C++17, Python 3 y Blender 4.3.2 para las comprobaciones de render.

```sh
bash scripts/test.sh
```

Ejecuta compilación estricta, pruebas C++ de bake, 19 pruebas del generador y cinco contratos del renderer más escena real Blender. CMakeLists.txt también está incluido; la ruta verificada en este entorno es el script g++.

## Reproducir r2

```sh
bash scripts/build.sh
build/cobweb_lab --preset corridor --seed 42 --density .9 --broken .18 --width .0045 --layers 2 --atlas 4096 --out outputs/new_v3r3_seed42
blender -b --python scripts/render_lab.py -- --input outputs/new_v3r3_seed42 --output renders/new_v3r3_front.png --view beauty --light frontal
```

Para la segunda semilla, sustituir 42 por 99 y usar otra carpeta y PNG. Las salidas existentes se rechazan por defecto; `--overwrite` solo se usa si se desea explícitamente reemplazarlas. Las capturas compartidas quedan congeladas en rutas con hash.

Controles CLI disponibles: `--preset`, `--seed`, `--density`, `--sag`, `--tension`, `--broken`, `--width`, `--layers`, `--quality`, `--atlas`, `--out`. No hay sliders gráficos.

Salida: OBJ único con UV/normales; atlas TGA de cobertura y orientación; grafo de autoría JSON con identidad de strand; receta, apoyos, AABB, focos y estadísticas. Es un objeto portador con islas; no hay tubos ni objetos individuales por fibra.

## Validación y presupuesto

Receta r3, semilla 42:

- 4.662 vértices, 8.400 triángulos, 12 paños
- 58.494 segmentos/curvas de autoría, horneados al atlas
- Cobertura media de atlas: 14,6510%, frente a 14,7354% de r2 y 11,7562% de v2
- 33 apoyos idénticos a v2 y paso central de 1,44 × 2,40 m libre para las caras completas
- Semilla 99: 59.469 curvas de autoría, misma malla y apoyos

La densidad está localizada: las pruebas comprueban mayor cobertura cerca de cada foco que en su entorno. La media del atlas no equivale a cobertura de pantalla, tiempo GPU o calidad visual.

Dos atlas 4096² RGBA8 sin compresión ocupan 128 MiB. El bake usa cuatro buffers float (256 MiB a 4096²), grafo y memoria temporal adicional. No se midió rendimiento en tiempo real.

## Límites

Modelo geométrico artístico y shader de seda aproximado. No hay física, integración con motor, OIT, LOD, mipmaps, medidas GPU o validación temporal. Los anchos son de inspección artística, no diámetros biológicos reales. Las vistas de 16 muestras tienen ruido visible.

El daño sigue siendo una máscara espacial de cobertura; el grafo guardado conserva las curvas sin cortar. Las superficies pierden silueta al verlas de canto. La referencia contiene una escena mucho más compleja y no se pretende haberla reproducido con esta composición.

El código es propio y utiliza la biblioteca estándar de C++. No se incorporó código de addons o generadores externos ni se asignó una licencia nueva.

Se detectó y corrigió un auto-cruce de paños de la tercera capa en controles extremos. La corrección limita su desplazamiento en profundidad; la receta entregada de dos capas permanece byte-idéntica. La suite verifica intersecciones entre caras no adyacentes.

## Daño localizado r3

El perfil angular de cada hueco usa un flujo aleatorio local e independiente, por semilla, paño y hueco. Los centros originales permanecen fijos. Los dos cortes tienen centros fijos, orientación asociada al paño/capa y radios que aumentan con la rotura; no hay máscara de ruido global. Las pruebas comprueban determinismo, cobertura intacta idéntica con rotura cero y daño monotónico al aumentar el control. Radial y esquina conservan sus atlas y geometría previos byte a byte.

Esta fue una única variante posterior al checkpoint r2. Se entregan frontal, oblicua y depuración con los mismos ajustes. No se continuó modificando estética después de esas capturas.

La revisión independiente del daño evaluó ocho semillas, quince paños/capas y quince niveles de rotura, incluidos extremos positivos diminutos: valores finitos, deterministas y monótonos. Cuarenta bakes de radial/esquina conservaron cobertura y momentos exactos frente al checkpoint. No quedaron hallazgos críticos o importantes de esa revisión.
