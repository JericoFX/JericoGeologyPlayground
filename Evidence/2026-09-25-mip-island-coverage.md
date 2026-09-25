# Cobertura de islas UV en mips de bake — 2026-09-25

Estado: **PASS para los paquetes auditados de cuatro niveles; no aceptación general de calidad visual**. `Tests/BakeMipAudit.py` ahora descubre cualquier carpeta `<forma>/lod-*/bake.json`, además de `rock-*`; el [RED previo](2026-09-25-mip-islands-cave-red.log) no descubría ningún bake del arco ni del túnel. El GREEN ejecutó el mismo auditor sobre esos seis paquetes y sobre pizarra, cliff y estratos. No se modificó `Source/NormalBake.cpp`.

| Paquete | Bakes / mips examinados | Mezcla entre islas | Muestras bilineales de otra isla / gutter | p95 angular máximo contra mip0 |
|---|---:|---:|---:|---:|
| [Pizarra fuente](2026-09-25-mip-islands-slate-audit.json) | 3 / 9 | 0 | 0 / 0 | 20,12° |
| [Cliff erosionado](2026-09-25-mip-islands-cliff-audit.json) | 3 / 9 | 0 | 0 / 0 | 6,21° |
| [Horizontes fracturados](2026-09-25-mip-islands-strata-audit.json) | 3 / 9 | 0 | 0 / 0 | 12,70° |
| [Arco y túnel cuadrado](2026-09-25-mip-islands-cave-audit.json) | 6 / 18 | 0 | 0 / 0 | 17,05° |

El [barrido histórico de seis familias](2026-09-25-mip-islands-legacy-six-family-audit.json) agregó 17 bakes / 51 mips sin contaminación detectada; es evidencia de una versión anterior del bake, no sustituye los paquetes actuales. La auditoría reconstruye rectángulos de isla desde UV, padding y alpha, comprueba todos los bloques 2×2 en cada nivel y cuatro puntos por triángulo para el muestreo bilineal. Los ceros quedan acotados a esas entradas, niveles y muestras; el ángulo contra mip0 mide pérdida de detalle, no fidelidad frente al high ni estabilidad temporal.

Hay un RED real fuera de la configuración retenida: el [ensayo de cinco niveles](2026-09-25-slate-mip-audit-summary.json) tuvo hasta 6.473 texeles mezclados con padding 16, o 6.080 muestras de gutter con padding 4 en mip4. Ambos candidatos ya estaban rechazados por contaminación/costo o peor comparación high→low. El filtro C++ actual no conserva explícitamente identidad de isla durante downsample; si se vuelve a necesitar mip4 o padding distinto, habrá que propagar esa identidad y repetir el gate antes de admitirlo. Para los cuatro niveles actuales, la siguiente investigación debe medir el p95 angular de formas exigentes contra el high y con cámara móvil antes de modificar el filtro.

Los JSON incluyen el SHA-256 del auditor. Los originales completos de bake y las capturas pertenecen a los paquetes enlazados. JericoEngine siguió en lectura; no se usaron Blender ni OpenVDB.
