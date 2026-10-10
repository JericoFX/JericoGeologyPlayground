# Resolutor local MC64: corrección del 10 de octubre

El commit anterior conserva las fuentes originales del ZIP del 8 de octubre. Esta versión usa exclusivamente el conjunto congelado de FINAL-SOURCE-MANIFEST.json, SHA256 6b8700d010d31788beaf160bd14292d70afd02977cc7b0dcb320721457d349a6. Los archivos del checkpoint mantienen sus bytes originales y la estructura relativa necesaria.

Desde prototype-v3-erosion-resolved, ejecutar bash build_erosion_probe.sh. El script crea build, compila la CLI y prueba las dependencias originales y las regresiones del resolutor. LEEME-CORRECCION-20261010.md explica la corrección, reproducción y límites. La batería ampliada incluye 34 ejecuciones C++, 53 pruebas Python y 9 pruebas Node; los resultados originales revisados están separados de la repetición posterior de empaquetado desde árbol limpio.

El barrido original 0–31 pasa 32/32. La exploración ampliada contiene 130 semillas únicas, 125 admitidas y cinco rechazos conservadores: 40, 73, 98, 111 y 120. Las 132 ejecuciones históricas incluyen dos semillas repetidas. No es un extractor universal: se mantiene el alcance experimental rocks-v3 / irregular_beds / paris-mc-v1 / MC64 / wear0, las hipótesis previas de MC y las limitaciones numéricas documentadas.

Se conserva sólo el subconjunto necesario de DamageField, RockAddonMath y GeologyNoise. No se puede construir el producto completo de pipes o Manifold con este subconjunto. Los avisos y manifiestos históricos internos describen sus paquetes completos; BACKUP_MANIFEST.json describe esta entrega. Las rutas build-chain/deps/engine-inputs son fuentes necesarias, no cachés.

La evidencia histórica del 8 de octubre queda separada en evidence/2026-10-08. La evidencia congelada del 10 de octubre está en evidence/2026-10-10/frozen. Las cuatro láminas revisadas, sus ajustes y procedencia están en evidence/2026-10-10/visual. El ZIP original íntegro sigue identificado en Library como libfile_cfa96f89c5048191afddb8d142e09857, SHA256 728612df6473b86b880167c3be9817d6eba77fba28eef0551dc88fcf47002ab2, 7.495.030 bytes. No se ha verificado una copia de ese ZIP final en Drive.

No se instala nada en la PC del usuario, no se integra nada en JericoEngine2/O3DE y no se modifica otra familia ni un respaldo anterior.

La repetición de empaquetado desde copia limpia pasó el runner oficial, las 34 ejecuciones C++, las 53 pruebas Python y las 9 pruebas Node. Los registros y comandos exactos están en evidence/2026-10-10/clean-package-qa. Se volvió a verificar el hash de todas las fuentes congeladas y que las dependencias del proyecto se resuelven dentro del paquete. Las pruebas Node usan mocks, no validación de browser/WebGL.
