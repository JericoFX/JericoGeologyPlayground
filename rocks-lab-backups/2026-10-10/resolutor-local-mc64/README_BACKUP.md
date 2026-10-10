# Resolutor local MC64: respaldo original y corrección aislada

Este directorio conserva el resolutor de rocas recibido en el ZIP del 8 de octubre de 2026. El primer commit importa sus fuentes sin cambiar un byte. El segundo commit se reserva para la corrección del 10 de octubre, después de revisión y QA. Este snapshot original no afirma que el defecto esté resuelto.

## Procedencia

- Archivo: JericoRocas-Resolutor-Local-MC64-20261008.zip
- Identidad de Library: libfile_cfa96f89c5048191afddb8d142e09857
- SHA256: 728612df6473b86b880167c3be9817d6eba77fba28eef0551dc88fcf47002ab2
- Tamaño: 7.495.030 bytes.

La identidad identifica el entregable original completo. No es una URL pública. No se verificó una copia de este ZIP final en Drive.

## Contenido y límites

Se conservan los 205 archivos de fuente, pruebas, recetas, herramientas y visor del laboratorio, y 19 dependencias necesarias del checkpoint original. Se añaden sus seis archivos de procedencia y atribución. El manifiesto enumera los 125 archivos del checkpoint que quedan fuera por no ser necesarios para este resolutor. Sus bytes siguen en el ZIP original.

La estructura relativa entre prototype-v3-erosion-resolved y damage-checkpoint se mantiene para build_erosion_probe.sh. Los archivos bajo original/build-chain/deps/engine-inputs son fuentes requeridas; no son cachés. El subconjunto no permite construir la aplicación completa de pipes ni Manifold.

Los README, manifiestos y sumas históricos internos describen sus paquetes de origen completos. BACKUP_MANIFEST.json delimita este snapshot y registra los hashes reales de cada archivo incluido. Se preservan las licencias MIT de Paris y nlohmann; los avisos históricos del checkpoint y sus referencias de licencia se conservan aunque el código Manifold no forme parte de este subconjunto.

La evidencia textual de 2026-10-08 se conserva separada. Los 8 OBJ y 8 PNG originales están en el ZIP de Library; sus tamaños y SHA256 permiten verificarlos. No se convierten imágenes en texto ni se presentan los registros históricos como pruebas nuevas.

## Comandos soportados para el snapshot original

Desde prototype-v3-erosion-resolved, crear build y ejecutar bash build_erosion_probe.sh. Ese script compila la CLI y los probes del resolutor con las dependencias conservadas, y ejecuta los tests originales de DamageField y RockAddonMath. Luego ejecutar build/v3_erosion_tests y python3 -B tests/test_localized_resolver.py. El test de caras se compila con g++ -O2 -std=c++20 -Icore/include tests/face_resolution_tests.cpp build/librocks_core.a build/liboriginal_damage.a -Wl,--gc-sections -o build/face_resolution_tests, y se ejecuta build/face_resolution_tests.

Estos comandos corresponden al snapshot histórico. El test de rechazo de seed 4 registra deliberadamente el comportamiento anterior. La corrección y sus nuevos criterios se documentarán por separado. No usar el CMake ni las instrucciones de los demos del checkpoint de pipes: aquel producto completo no está incluido.

La auditoría inicial verificó dependencias de cabeceras con g++ -MM para 43 unidades C++. La comprobación de empaquetado desde copia limpia valida únicamente los comandos anteriores y no representa la batería completa ni el QA de la corrección. El estado histórico era 25/32 semillas aceptadas, con siete rechazos y sin ejecución de toda la batería del proyecto.

No se modifica rocks-lab-backups/2026-10-06 ni ninguna otra familia. No hay integración con JericoEngine2, O3DE ni la PC del usuario.
