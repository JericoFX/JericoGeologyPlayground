# Corrección local de las siete semillas MC64

Se conserva el campo aprobado de irregular_beds, las recetas y los umbrales de calidad. Se corrigen las semillas 4, 5, 8, 14, 25, 29 y 31 del barrido original 0–31, en sus etapas Base, Stratified y Eroded.

## Cambios

- El resolutor sigue usando las dos triangulaciones de la tabla original. Admite una cara ambigua o dos caras ambiguas opuestas (78 máscaras); todas las caras de una celda deben pedir la misma alternativa.
- La decisión se aplica a todo el componente conectado de celdas o a ninguna. Una cara sin evidencia, una combinación incompatible o un vecino no soportado conserva el rechazo.
- Los triángulos microscópicos usan la contracción acotada ya existente, después de resolver las ambigüedades. Se conservan raíces y normales originales, la condición topológica de enlace y la orientación. La mayor distancia real observada fue 0,00000173789 m. No se cambiaron umbrales.

## Resultados verificados

- Barrido original: 32/32 admitidas. Controles dirigidos 0–31 más 48, 117, 305 y 999: 36/36; repetición de sus 108 OBJ idéntica byte a byte.
- Los 25 éxitos anteriores y los controles 48/117/305/999 conservan los 87 OBJ exactos. Los 3 OBJ de strata-v4 seed48 también permanecen idénticos.
- Las 21 mallas reparadas son manifold cerradas, tienen una componente, normales válidas, volumen positivo y cero caras degeneradas/duplicadas. Cero intersecciones detectadas por el auditor independiente, con tolerancia aproximada de 2,92e-9 m, en 2.468.424 pares candidatos.
- Holdout fijo 32–63: baseline 26/32, candidato 31/32. Los 26 éxitos anteriores permanecen idénticos; se admiten además 39, 43, 47, 49 y 63. Seed40 sigue rechazada.
- Exploración 0–127 más 305/999: 130 semillas únicas en 132 ejecuciones (48 y 117 repetidas); 125 únicas admitidas. Se conservan los rechazos 40 (guarda de vértice/normal), 73, 98, 111 y 120 (ambigüedades sin resolver).
- Controles reales fuera de alcance, grid128, wear distinto de cero, otra familia y otra versión: mismo resultado y diagnóstico estable que el baseline.
- La medición actual del prepass local fue de 3,576 a 6,482 ms por etapa en este equipo. Incluye el nuevo recorrido previo aun cuando no haya ambigüedades; no es el coste histórico del resolutor del 8 de octubre.

## Límites

Esta corrección conserva el alcance experimental rocks-v3 / irregular_beds / paris-mc-v1 / MC64 / wear0. La cota L=8 y la guarda numérica aportan evidencia conservadora del signo del campo; no son aritmética de intervalos ni prueba exacta universal. Se mantienen las hipótesis previas de MC sobre una raíz por arista y detalle resuelto por la rejilla. La auditoría de intersecciones usa tolerancias y no demuestra ausencia de bucles ocultos dentro de los voxels. No se presenta el resultado como extractor universal.

## Reproducción

La carpeta de fuente debe tener como hermana la carpeta real damage-checkpoint, con sus dependencias originales incluidas. Desde esta carpeta ejecutar:

    bash build_erosion_probe.sh

El comando compila el código, prueba las dependencias matemáticas originales y ejecuta las pruebas de cara, componente atómico, contracción, las siete semillas y la CLI. El resto de pruebas y sus resultados se conservan con la evidencia de entrega.

El campo aprobado sigue teniendo SHA-256:

    2ddf7b9a15d3c4fa83bb33f5554c888f62d67cb3bcba62c50141c8017c4abdc7

Esta entrega no instala nada en la PC ni integra cambios en el motor.
