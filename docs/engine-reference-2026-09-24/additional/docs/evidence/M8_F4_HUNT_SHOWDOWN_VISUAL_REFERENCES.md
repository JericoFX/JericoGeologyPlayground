# Referencias visuales del usuario: Hunt Showdown

Fecha: 2026-09-05. Siete capturas suministradas por el usuario, inspeccionadas
visualmente en1920x1080. Referencia de resultado para F4/M8; no evidencia de una
técnica interna de CryEngine, ni autorización para copiar código o extraer assets.
Los originales permanecen sin modificar en:
`C:/Program Files (x86)/Steam/userdata/61916515/760/remote/594650/screenshots/`.

Aclaración posterior del usuario: "cryengine solo uso modelos 3d". El objetivo
que se toma de estas referencias es modelado de la masa rocosa: fracturas,
bloques y repisas deben existir en la geometría y afectar su silueta. El material
aporta detalle fino. Esta directiva no cambia la prioridad de terminar M8.

| Archivo | SHA256 |
| --- | --- |
| 20260905220749_1.jpg | DA934D011689EEC969B465F41FAA443D868B7A33340C0D8EC81B176C3B74C73D |
| 20260905220756_1.jpg | 9DB468322B1BA8169C24061C0BCA28920749C2CEF0BF0036BE521A33FCAA2209 |
| 20260905220801_1.jpg | 6081AB418D80181C7BC389423CD9C329581CD489A33DBDD0C3DB5D6223491F5A |
| 20260905220807_1.jpg | CDA289BA636C19F3EE401AEE0B6CC8AFA548E7DBF4F08103CF9B477D54B76395 |
| 20260905220733_1.jpg | 42D44E8C62964B157EDC575839816AC0AE13CBC0B63297290725560D3B1B85A9 |
| 20260905220737_1.jpg | 9E882B00E782E42239FD4FCD1CE823308F3F05579F526B966FFE6991E270892E |
| 20260905220742_1.jpg | 6C4DE5EA3231BD11F73004C195203F4A9513542F9F0364408CEA64E943210AC4 |

## Observaciones y aplicación al M8 actual

- Exteriores20801/20807: masa dividida en bloques grandes, fracturas verticales
  y oblicuas, repisas horizontales de espesor variable y esquinas rotas. Hay
  orientación común de estratos, interrumpida por juntas y salientes desiguales.
  La silueta lateral y superior participa en el resultado; una textura de bandas
  aplicada a un prisma no basta.
- Mina20733/20737/20749: paredes y techo con planos grandes angulosos y roturas
  de varias escalas. Algunas caras son relativamente tranquilas; el detalle no
  ocupa toda la superficie con amplitud uniforme. Hay fragmentos al pie de las
  paredes, separados de la masa principal.
- Pasos20756/20742: techos, estrechamientos y soportes dan escala y continuidad
  espacial. La iluminación cercana y lateral resalta aristas; la bruma reduce
  el contraste al fondo. La madera y el metal sirven de referencias de escala.
- Material: superficies grisáceas con moteado fino, variación tonal y zonas
  oscuras en grietas. Las imágenes no permiten atribuir cada detalle a geometría,
  normal map, desplazamiento, oclusión o un método específico de mezcla.

Para las cuatro rocas de F4: evaluar masa/silueta, planos de fractura, espesor y
ruptura de estratos ANTES del detalle de material. Conservar variación dentro de
una dirección geológica común; evitar tanto bandas idénticas como ruido sin
estructura. Comparar fuente, estructura y resultado completo con cámara y luz
equivalentes, incluyendo material neutro para separar forma e iluminación.

Estas capturas complementan el blueprint geológico y sus ecuaciones publicadas;
no los sustituyen. No abren un showcase nuevo de minas/cuevas ni adelantan UI de
M10.5. La roca actual sigue Beauty RED. La corrección de caché de shaders y el
diagnóstico del LOD real continúan antes del siguiente cambio de forma.
