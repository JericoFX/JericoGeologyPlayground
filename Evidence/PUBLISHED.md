# Evidencia seleccionada para el repositorio privado

Repositorio: [JericoFX/JericoGeologyPlayground](https://github.com/JericoFX/JericoGeologyPlayground), rama `main`, visibilidad `PRIVATE` verificada tras la publicación inicial.

El repositorio conserva el código, las referencias documentales M8 copiadas con SHA-256, el checkpoint vigente, las fotos USGS y texturas Poly Haven CC0 con sus manifiestos, y la prueba visual más reciente de las seis familias. Los builds y los aproximadamente 7 GB de paquetes intermedios de `Evidence/` permanecen sólo en el playground local y se pueden regenerar; su ausencia en Git no significa que los bakes estén aceptados para M8.

La corrida de exportación de mips precedente está identificada por `specialist-mips-input-artifact-hashes.json`. Incluye el resumen `2026-09-24-specialist-mips-six-family-lods/rock-lod-bakes-proof.json`, la auditoría de los 17 bakes/mips, el comparador visual de 17 LOD, los logs RED/GREEN/Release y capturas originales `high.png`, `low.png`, `low-normal.png`, AO, contact, cavity y convex-edge para cada LOD válido. Las bases/mips PNG de textura y OBJ completos se regeneran localmente.

Desde el directorio raíz, con el build Release configurado y sus dependencias M8 de lectura disponibles:

```text
build/Release/JericoGeologyPlayground.exe --auto-uv-rock-lod-bakes-test Evidence/2026-09-24-specialist-mips-six-family-lods
python Tests/RockLodBakeAudit.py Evidence/2026-09-24-specialist-mips-six-family-lods Evidence/specialist-mips-six-family-lods-audit.json --require-auto-uv --require-specialist-mips
python Tests/RockLodVisualAudit.py Evidence/2026-09-24-specialist-mips-six-family-lods Evidence/specialist-mips-six-family-lods-visual-audit.json
```

El comparador de imágenes conserva los SHA-256 de High y Low+normal, de modo que permite verificar que el nuevo filtrado de mapas no cambió la macroforma. Las imágenes actuales aún muestran el parche liso del cliff y los estratos repetitivos. No se ejecutó Blender ni se modificó JericoEngine.

La continuación `2026-09-24-specialist-gpu-mip-final-six-family/` publica el proof JSON, auditorías y 60 capturas lejanas seleccionadas: AO, contacto, cavity y convex-edge con una sola textura base y con cuatro niveles, más High y Low+normal a la misma distancia. `Tests/SpecialistPreviewAudit.py` verifica las seis familias, exige cambio visible por cada mapa no plano y comprueba niveles y estimación de almacenamiento RGBA **por mapa**; no mide VRAM real. `specialist-gpu-mip-final-input-artifact-hashes.json` fija 84 hashes de fuente, documentos, ejecutable, logs, auditorías y capturas. Los paquetes OBJ y los mapas completos siguen siendo artefactos locales regenerables.

```text
build/Release/JericoGeologyPlayground.exe --auto-uv-rock-lod-bakes-test Evidence/2026-09-24-specialist-gpu-mip-final-six-family
python Tests/RockLodBakeAudit.py Evidence/2026-09-24-specialist-gpu-mip-final-six-family Evidence/specialist-gpu-mip-final-six-family-bake-audit.json --require-auto-uv --require-specialist-mips
python Tests/SpecialistPreviewAudit.py Evidence/2026-09-24-specialist-gpu-mip-final-six-family Evidence/specialist-gpu-mip-final-six-family-preview-audit.json
python Tests/RockLodVisualAudit.py Evidence/2026-09-24-specialist-gpu-mip-final-six-family Evidence/specialist-gpu-mip-final-six-family-visual-audit.json
```
