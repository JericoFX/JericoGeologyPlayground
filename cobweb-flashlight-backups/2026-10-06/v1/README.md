# Linterna y tres clases de telaraña · estudio v1

Estudio visual independiente en Blender 4.3.2/Cycles. Tres clases artísticas comparten geometría y anclajes; cambian sus parámetros estáticos de cobertura y material. Al mover o apagar la linterna, cámara, exposición, ambiente, material y cobertura de cada clase permanecen fijos.

La seda usa un material anisótropo y translúcido aproximado. No es una validación biológica ni una medida de rendimiento en tiempo real o integración de motor.

## Dependencias

Python 3, NumPy y Pillow; g++ compatible con C++17 para recompilar el generador; Blender 4.3.2 para renderizar. Las herramientas de vídeo requieren ffmpeg. Los scripts no instalan dependencias.

`vendor/` contiene fuentes y renderer propios copiados del laboratorio dense-v3r3, sin modificar el laboratorio original.

## Comprobar el código

Las seis pruebas de contrato son autocontenidas:

```sh
python3 -m unittest discover -s tests -p test_contract.py -v
bash scripts/build_generator.sh
```

Las tres pruebas de `test_evidence.py` requieren los recursos completos de las tres clases, `assets/identities.json` y los resultados numéricos de `renders/final01`. Esos datos se conservan en los paquetes de recuperación separados y no están en este respaldo Git.

Tras restaurarlos, ejecutar las nueve pruebas:

```sh
python3 -m unittest discover -s tests -v
```

## Regenerar una prueba

En una copia sin assets ni resultados generados:

```sh
bash scripts/build_generator.sh
python3 scripts/bake_classes.py
blender -b --python-exit-code 1 --python scripts/render_study.py -- --run final01 --samples 192
blender -b --python-exit-code 1 --python scripts/read_coverage.py -- renders/final01
python3 scripts/analyze_study.py final01
```

El bake crea los recursos y sus identidades. Los scripts rechazan reemplazar resultados existentes; usar otro nombre de ejecución para renders posteriores. Las pruebas de evidencia esperan específicamente `final01`.

La escena portátil ya montada y sus seis texturas externas relativas se recuperan por separado. Este checkpoint solo incluye scripts, fuentes C++, pruebas y este README; excluye assets, PNG/EXR, vídeo, .blend, manifiestos de entrega e informes privados.

## Límites

Cada clase cambia varias propiedades conjuntamente: la comparativa no aísla una sola causa. La cobertura se verifica mediante AOV lineales, no mediante el alfa del PNG con fondo. La orientación es una aproximación basada en momentos UV; los paños pierden silueta al mirarlos de canto. No se atribuyen garantías de invisibilidad universal ni de rendimiento GPU a estos renders offline.
