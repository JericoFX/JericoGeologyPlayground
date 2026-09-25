# Ficha para incorporar una capacidad de otro addon

Orden vigente: backend no visual construido, integrado y probado antes del foco
M8 posterior a M9; después selección de addons concretos y adaptación de
GeneratorSchema/bindings; front Qt en M10.5. Véase la
[auditoría del backend](BACKEND_COMPLETENESS_AUDIT.md). Esta ficha no cambia esa prioridad.

Esta publicación contiene sólo los nueve MD de [PLAN.md](PLAN.md). Código,
catálogos/ejemplos JSON y evidencia referidos son archivos locales excluidos;
los comandos requieren el paquete local, no sólo esta ficha.

Copiar esta ficha al documentar una capacidad concreta. Su estado inicial es
**PENDIENTE**. No añadir una fila al catálogo operativo hasta que el control esté
validado y conectado a código propio. Los datos de origen ayudan a estudiar el
comportamiento; no autorizan cargar el Python o los graphs del addon.

| Campo | Contenido necesario |
| --- | --- |
| Capacidad / resultado esperado | Una acción observable y las entradas donde debe funcionar. |
| Origen | Addon/versión, rutas leídas, `sourceHash`, bibliotecas y SHA-256, licencia observada. |
| Identidad Jerico | `toolId`, versión, `profileId` si convierte valores fuente; compatibilidad con recetas anteriores. |
| Entrada | Schema, coordenadas/unidades, IDs/revisión, selección y límites de tamaño/topología. |
| Implementación | Función propia en `transformer/<operacion>.py`; reutilizar la ruta Blender existente cuando corresponda. |
| Salida y linaje | Geometría/campos, dominio, fuente de cada elemento, material/región/fractura, hashes y consumidor previsto. |
| Determinismo | Inputs ordenados, seed local cuando aplique, fingerprint y regla para versión/deriva. |
| Fallos y lifecycle | Validación, cancelación/deadline, no output parcial, preview/diff y transacción futura. |
| Aceptación | RED retenido, medición geométrica/semántica, valores A/B, negativos, procesos sin referencia y evidencia. |
| Integración requerida | Subsistema, datos/interfaz esperada, owner, dependencia y milestone. |

Para cada control, completar una fila con estos campos:

| Campo de control | Regla |
| --- | --- |
| `id`, `label`, `group` | ID estable por herramienta, nombre legible y agrupación. |
| `type`, `unit` | Tipo validado y unidad física o semántica. |
| `default`, `minimum`, `maximum` | Valores reales del validador propio; no copiar límites enormes de sockets sin presupuesto. |
| `options` | IDs estables de opciones enumeradas; lista vacía si no hay enum. Un default de un menú no revela todas sus opciones. |
| `dependencies` | Condiciones de visibilidad/habilitación y restricciones entre valores; ocultar no borra el valor authored. |
| `binding` | Backend, `parameters.<id>` o ruta de layer, operación real y necesidad de reconstruir candidato. |
| `status`, `effect` | IMPLEMENTADO, PENDIENTE o NO SOPORTADO y efecto medible. No presentar sockets inspeccionados como funciones operativas. |

Los campos anteriores ya existen en `suite.py`/`presentation.py`; la ficha no es
un segundo formato ejecutable. Un nuevo tipo de parámetro requiere validación
explícita antes de admitirlo. No hay rutas importables ni funciones indicadas por
JSON externo. La implementación actual valida booleanos, enteros y números para
las operaciones CPU, además de enums con IDs de opción explícitos; los tipos
geológicos usan su validador ya establecido. Cara/región requiere escritura NFC
para evitar aliases de identidad entre distintas representaciones Unicode.

## Ejemplo real: fragmentación de cadenas Edge

| Campo | Valor existente |
| --- | --- |
| Origen | Edge Decal 1.2.1, `features/generation.py::EDGEDECAL_PG_settings`; fingerprint `sha256:892aad2f860cb7985abb705b4c95c4773e7faf0353b0aa4cb45e6c1cbae5cbea`. |
| Identidad | `toolId=jerico.edge-chains`, versión `1.0.0`, perfil `edge-chains/v1`. |
| Control de origen | `maximum_decal_length`, FloatProperty, default 0. |
| Control propio | `maximumLength`, «Longitud máxima de fragmento», grupo Selección, number/m, default 0, rango 0..1000000, sin enum. |
| Binding | `parameters.maximumLength` → `chains.py::generate_chains` → `fragment_intervals`. |
| Dependencia | Cero desactiva; la combinación de cap, ancho y longitud puede reducir cobertura y debe informarse en preview. |
| Efecto | Ningún fragmento excede el cap. Los fragmentos se separan por huecos reales de al menos dos anchos. |
| Entrada/salida | Malla/selección estable → cadenas/intervalos → superficies, UV y caras/aristas/fragmentos fuente. |
| Evidencia | `test_transformer_chains.py`, `test_transformer_conversion.py`, `replay_transformer_operations.py` y `evidence/TRANSFORMER_OPERATIONS.md`. |
| Límite | Distribución propia; no replica taper, AO, atlas/pins o el solver completo de unión del addon. |

La secuencia real del ejemplo está en [examples/README.md](examples/README.md):
reporte revisado → conversión de valores → receta propia → candidato reproducible.
El addon no participa en la generación del candidato.

Este ejemplo añadió `jerico.edge-chains` y `edge-chains/v1` sin sustituir
`jerico.edge-ribbons` ni su perfil básico. Los IDs y la versión 1.0.0 de las
bandas aisladas permanecen estables; sus pruebas siguen exigiendo el comportamiento
anterior. Para un addon diferente se sigue la misma regla: nueva operación,
descriptor y perfil propios; las implementaciones existentes se reutilizan como
funciones cuando su contrato coincide, sin cambiar sus controles ni defaults.
Sólo se añade la entrada a las tablas comunes; la CLI y la futura UI genérica
leen esas tablas y no necesitan un caso especial por nombre de addon.

La prueba `test_chain_profile_converts_fraction_and_cap_and_keeps_basic_profile`
demuestra este alta independiente: el nuevo perfil admite fragmentación mientras
el perfil anterior conserva sus límites. Cada incorporación debe conservar las
pruebas de las operaciones previas y añadir sus propios casos de conformidad.

## Segundo ejemplo: nueva selección sin modificar los generadores

`jerico.edge-selection` 1.0.0 añade `selection.py` y una entrada a las tablas
existentes. Las implementaciones, controles, defaults y versiones de
`edge-ribbons` 1.0.0, `edge-chains` 1.0.0 y `edge-decals` 1.1.0 se conservaron.
El nuevo `selectionSnapshot` se entrega como entrada al generador existente;
`replay_transformer_operations.py` demuestra esa composición en procesos limpios.
Es el ejemplo concreto de incorporar una capacidad sin reescribir las anteriores.

Sus dos perfiles usan `ownDefaults` en la tabla cerrada para fijar el modo manual
o automático. Las propiedades fuente revisadas se aplican encima y el validador
común completa los demás defaults. El documento generado muestra el resultado
final por perfil: manual 0.05 m por cadena, automático 0.30 m por arista.

Una declaración dinámica no se evalúa para convertirla. El único ángulo admitido
aquí tiene `declarationDefault` igual a la forma AST no resuelta revisada y
`default=π/6` decidido tras leer `radians(30)` bajo el fingerprint exacto. Esto
no autoriza expresiones arbitrarias: un cambio de archivos/declaración requiere
nueva revisión. Las unidades de entrada son radianes; 30 como valor se rechaza.

Para otro addon se repite la ficha del comienzo: nuevo ID/módulo/descriptor,
perfil sólo para comportamientos realmente implementados, pruebas que midan sus
efectos y conservación de los tests anteriores. No hace falta un segundo registro
ni un cargador de plugins ejecutables. Los controles inspeccionados sin operación
continúan PENDIENTES aunque el informe pueda mostrarlos como futura UI.

## Componer capacidades cuando el orden cambia el resultado

`jerico.edge-decal-workflow` 1.0.0 reutiliza controles y módulos existentes para
selección inicial → hendiduras/AO → mínimo de longitud → bandas/taper/UV. No crea
un framework de grafos. Su nueva entrada en las tablas comunes y dos perfiles
combinan las correspondencias revisadas anteriores; el documento automático
obtiene los 19 controles y sus defaults sin un caso especial en la CLI.

La selección de hendiduras se extrajo como función reutilizable. El wrapper de
`edge-decals` conserva versión/defaults/salida: una fixture previa al cambio
comprueba el candidato entero, excepto el hash de implementación y la identidad
del job que necesariamente cambian al cambiar los módulos. La nueva operación
tiene pruebas propias para el orden observable; no se limita a sumar controles.

Este ejemplo es para capacidades acopladas por orden, no una exigencia de crear
otra herramienta por cada parámetro. Añadir sólo un descriptor no implementa
algoritmos. Las correspondencias nuevas todavía requieren lectura y pruebas;
los pasos automáticos continúan siendo intake, documentación, conversión de
perfiles revisados, validación y generación de operaciones registradas.

## Cambios mínimos al añadir la próxima capacidad

1. Leer la nueva carpeta con `scan`; si hay `.blend`, inspeccionar metadatos en una
   ventana Blender coordinada y enlazarlos por SHA con `join-library`.
   Usar `document <informe.json> --out <archivo.md>` para generar automáticamente
   el inventario legible, las tablas de controles propios y los pendientes.
2. Escribir pruebas del comportamiento útil y retener un fallo esperado.
3. Implementar la operación propia y registrarla en `suite.py::OPERATIONS`, o
   componer un generador existente si ya resuelve el problema.
4. Registrar controles efectivos en el descriptor; `presentation.py` completa los
   campos comunes y condiciones. Añadir perfil en `adapters.py::PROFILES` sólo para
   los controles de referencia cuyo significado fue revisado.
5. Ejecutar pruebas, regenerar `transformer/catalogue.json` y actualizar
   `UI_CONTROLS.md`, esta ficha concretada y la evidencia del resultado.

El alta no necesita un parser ni una CLI por addon: `scan`, `document`, `convert`, `recipe`
y `generate` ya usan esa ruta común. El registro es una tabla cerrada del código
propio. El paso manual imprescindible es comprender el comportamiento y escribir
su operación; el lector no convierte algoritmos arbitrarios automáticamente.
Cada nuevo descriptor debe superar las comprobaciones comunes de catálogo en
`test_transformer.py` y las pruebas de efecto de su operación. Para reproducir la
conversión de una referencia real, añadir un caso en
`tests/replay_transformer_operations.py`: carpeta, perfil, fuente original y valores;
el harness ejecuta diez procesos independientes y compara sus JSON completos.

Un ejemplo de evolución sin nuevos mecanismos es `edge-decals/v2`: amplía el
perfil revisado existente con `taper_sliced_ends` → `taperEnds` y
`slice_taper_length` → `taperLength`, reutiliza `generate_chains`, agrega campos de
control y prueba anchos/UV/extremos. También convierte `uv_scale` en
`uvIslandScale` mediante el procesamiento común de parches UV. La herramienta cambia a 1.1.0, las recetas
anteriores se rechazan y la reconversión v1 desactiva taper explícitamente.
Esta decisión es visible en el contrato y conserva IDs de controles estables.
El plan sólo ofrece perfiles cuando existen todas las declaraciones revisadas:
identificador único, tipo y default. Un hash de addon conocido sin esos controles
no anuncia una conversión operativa. Es la misma comprobación que usa `convert`.

**INTEGRATION REQUIRED**: Editor/ProceduralAuthoring, backend no visual antes de
M8, para selección, preview/diff como datos y Apply/Undo/Redo; controles/front Qt
en M10.5. AssetPipeline/geometry, backend/admisión antes de M8, para broker/cook
y consumidores de nuevos datos semánticos. Añadir una herramienta
al catálogo no exige cambios al renderer; usar nuevos datos en runtime puede
necesitar una integración explícita de su owner. Nunca declarar completitud por
la presencia de una ficha o un panel.
