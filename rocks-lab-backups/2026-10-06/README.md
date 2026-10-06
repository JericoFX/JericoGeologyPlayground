# Respaldo de código del laboratorio de rocas

Fecha: 2026-10-06.

Se conservan dos implementaciones independientes:

- `lab-families-expansion`: candidate6.
- `lab-slate-candidate7`: candidate7, variante separada de pizarra.

Cada carpeta incluye fuentes C++ y headers, CLI, visor, recetas, tests, herramientas y licencias. Los algoritmos se copiaron sin modificaciones.

Para compilar desde la carpeta de cada candidato:

```sh
bash build_direct.sh
```

C++20, Python 3 y Node son necesarios para las pruebas. La generación de las superficies se ejecuta con `python3 tools/collect_surface_families.py`. El visor se inicia con `python3 tools/serve.py --port 8000`.

Este respaldo de código no incluye documentación histórica, planes, manifiestos de recuperación, evidencia de pruebas, imágenes externas, texturas, binarios ni grandes salidas generadas. No afirma una nueva ejecución de las pruebas. Candidate6 y candidate7 se mantienen separados.
