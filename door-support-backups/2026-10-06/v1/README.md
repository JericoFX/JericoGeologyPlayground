# Contrato de soporte de puerta y Jolt · v1

Banco independiente con la puerta reforzada aprobada, daño local y Jolt Physics 5.6.0 real en C++. Conserva los laboratorios anteriores. No integra código de addons ni un motor de juego.

## Qué hace

El contrato Python evalúa material bajo las superficies de montaje. Tres registros de apoyo sostienen una única bisagra ideal de Jolt. Al perder el último apoyo, el banco retira esa restricción y activa la puerta dinámica para caer y contactar con el suelo. Las vistas Blender usan poses registradas por Jolt, sin animar manualmente la caída.

El marco se eleva un metro para hacer visible la caída. La geometría propia congelada de `fixtures/approved_door.json` permite reproducir el hash aprobado; no es un recurso externo.

## Dependencias y ABI

Se requiere Python 3, g++ C++17 y una biblioteca Jolt 5.6.0 compatible. Blender 4.3.2 se usa para pruebas Boolean y vistas. `THIRD_PARTY.md` conserva versiones, fuentes oficiales, hashes, licencias, flags ABI y límites de verificación.

Si las dependencias ya están compiladas, establecer `JOLT_SOURCE` al directorio de headers y `JOLT_LIBRARY` a su `libJolt.a`. El script reutiliza esas rutas sin descargar otra vez. La configuración verificada es Release estático float, SSE2, assertions activas, capas de 16 bits, RTTI/excepciones/LTO desactivados. Los flags del consumidor están en `scripts/build_bench.sh` y deben coincidir con la biblioteca.

Para preparar dependencias desde cero existe `scripts/setup_jolt.sh`, que descarga únicamente las distribuciones oficiales fijadas. Las dependencias, herramientas descargadas y binarios no forman parte de este respaldo.

## Construir y comprobar

```sh
bash scripts/build_bench.sh
python3 -m unittest discover -s tests -v
blender -b --python-exit-code 1 --python-expr "import sys,unittest;sys.path.insert(0,'.');r=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.discover('tests'));sys.exit(not r.wasSuccessful())"
```

El build genera contrato y fixture, compila C++ con advertencias como errores y ejecuta el banco. Incluye regresión para crear las carpetas de salida en una extracción limpia. Las dos pruebas Boolean se omiten explícitamente fuera de Blender.

## Salidas y límites

`out/contract_examples` contiene decisiones de soporte con `physics_executed=false`. `output/physics` contiene el informe y CSV de simulación real con `physics_executed=true`. Renders, escenas y vídeo se generan aparte; no están incluidos en Git.

La hoja colisiona mediante envolventes convexas de piezas de madera; el marco usa proxies rectangulares con cortes de caja. El metal decorativo se omite de colisión. El puente físico rechaza daño de hoja o eliminación de piezas que aún no puede reconstruir de forma coherente.

La bisagra ideal restante no simula flexión, resistencia de tornillos ni rotura por carga. El soporte se estima mediante muestreo 7×7 por superficie y conectividad de esta geometría fijada. El daño volumétrico sobre agarres, desgaste artístico avanzado y una integración interactiva de motor quedan fuera del checkpoint.
