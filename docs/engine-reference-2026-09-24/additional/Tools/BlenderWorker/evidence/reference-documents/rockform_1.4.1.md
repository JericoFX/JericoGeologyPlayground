# Informe de incorporación de addon

Estado: IMPLEMENTATION REQUIRED: ninguna conversión operativa admitida.
Ningún addon completo se declara equivalente por este inventario. Datos leídos del informe; no se ejecuta el addon.

Fuente: sha256:b90ef5e99cc67ece588d67847b1fbbe7127e0abd2a52bf9420552f44a7c38fc5
Análisis: sha256:1d6c8c4e1fcd320be08bc88cbeeb93973a2748ae760f6b5c1600ea3bceec84b5
Informe recibido: sha256:660892525a4512a611169399248df09b1940902be7291d3a890ca530b6c6ffda
Catálogo propio: sha256:38361ae1e4526f45eb21136c5b1fae957359990cea7a1bc190e95c77c6151bde

## Metadatos de origen

| Tipo | Archivo | Datos declarados |
| --- | --- | --- |
| bl&#95;info | rockform/&#95;&#95;init&#95;&#95;.py | {&quot;status&quot;: &quot;literal&quot;, &quot;value&quot;: {&quot;author&quot;: &quot;Brian Y&quot;, &quot;blender&quot;: &#91;4, 5, 0&#93;, &quot;category&quot;: &quot;3D View&quot;, &quot;description&quot;: &quot;Procedural Rock Formation Designer&quot;, &quot;doc&#95;url&quot;: &quot;https://pinto-search-a2e.notion.site/RockForm-v1-0-8b-for-Blender-4-0-4a891ed5556b4639af12082f351c0c6f?pvs=4&quot;, &quot;location&quot;: &quot;&quot;, &quot;name&quot;: &quot;RockForm&quot;, &quot;tracker&#95;url&quot;: &quot;&quot;, &quot;version&quot;: &#91;1, 4, 1&#93;, &quot;warning&quot;: &quot;&quot;}} |
| manifest | rockform/blender&#95;manifest.toml | {&quot;blender&#95;version&#95;min&quot;: &quot;4.5.0&quot;, &quot;id&quot;: &quot;rockform&quot;, &quot;license&quot;: &#91;&quot;SPDX:GPL-2.0-or-later&quot;&#93;, &quot;maintainer&quot;: &quot;Brian Y&quot;, &quot;name&quot;: &quot;RockForm&quot;, &quot;schema&#95;version&quot;: &quot;1.0.0&quot;, &quot;tagline&quot;: &quot;Procedural Rock Formation Designer&quot;, &quot;tags&quot;: &#91;&quot;3D View&quot;&#93;, &quot;type&quot;: &quot;add-on&quot;, &quot;version&quot;: &quot;1.4.1&quot;, &quot;website&quot;: &quot;https://pinto-search-a2e.notion.site/RockForm-v1-0-8b-for-Blender-4-0-4a891ed5556b4639af12082f351c0c6f?pvs=4&quot;} |

## Controles declarados por la referencia

| Identificador | Nombre | Tipo fuente | Default | Mínimo / máximo | Unidad fuente | Conversión revisada |
| --- | --- | --- | --- | --- | --- | --- |
| rockform/&#95;&#95;init&#95;&#95;.py::SNA&#95;OT&#95;Import&#95;Img&#95;3B570::filter&#95;glob | NO DECLARADO | StringProperty | &#42;.png;&#42;.jpg;&#42;.exr | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |
| rockform/&#95;&#95;init&#95;&#95;.py::SNA&#95;OT&#95;Import&#95;Img&#95;3B570::files | Filepaths | CollectionProperty | NO DECLARADO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;curvemesh&#95;object | Curve/Mesh Object | PointerProperty | NO DECLARADO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;displacementtextures | DisplacementTextures | EnumProperty | NO DECLARADO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;spawn&#95;on&#95;faces | Spawn on Faces | IntProperty | 0 | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;texthumbs&#95;list | TexThumbs&#95;List | EnumProperty | NO DECLARADO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;tex1&#95;name | Tex1&#95;Name | StringProperty |  | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;sig&#95;thumbs&#95;icons | SIG&#95;Thumbs&#95;Icons | EnumProperty | NO DECLARADO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;sig&#95;names | SIG&#95;Names | StringProperty |  | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;sig&#95;selected&#95;group | SIG Selected Group | IntProperty | 0 | &#91;0, 19&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;hide&#95;rf&#95;assets | Hide RF Assets | BoolProperty | false | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;isimported | IsImported | BoolProperty | false | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | NO DECLARADO | IMPLEMENTATION REQUIRED |

| ID | Descripción | Opciones declaradas | Subtipo / tamaño | Rango blando | Resolución de factory |
| --- | --- | --- | --- | --- | --- |
| rockform/&#95;&#95;init&#95;&#95;.py::SNA&#95;OT&#95;Import&#95;Img&#95;3B570::filter&#95;glob | NO DECLARADO | NO DECLARADO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |
| rockform/&#95;&#95;init&#95;&#95;.py::SNA&#95;OT&#95;Import&#95;Img&#95;3B570::files | NO DECLARADO | NO DECLARADO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;curvemesh&#95;object |  | NO DECLARADO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;displacementtextures |  | &#91;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;spawn&#95;on&#95;faces |  | NO DECLARADO | &#91;&quot;NONE&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;texthumbs&#95;list |  | NO RESUELTO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;tex1&#95;name |  | NO DECLARADO | &#91;&quot;NONE&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;sig&#95;thumbs&#95;icons |  | NO RESUELTO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;sig&#95;names |  | NO DECLARADO | &#91;&quot;NONE&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;sig&#95;selected&#95;group |  | NO DECLARADO | &#91;&quot;NONE&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;hide&#95;rf&#95;assets |  | NO DECLARADO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |
| rockform/&#95;&#95;init&#95;&#95;.py::register::bpy.types.Scene.sna&#95;isimported |  | NO DECLARADO | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | &#91;&quot;NO DECLARADO&quot;, &quot;NO DECLARADO&quot;&#93; | unresolved |

Las dependencias de callbacks/UI requieren revisión manual; el lector conserva declaraciones y no las evalúa.

## Herramientas propias disponibles

No hay bindings ejecutables para esta referencia. Revisar comportamiento y usar ADDON_TEMPLATE.md.

## Bibliotecas y controles binarios

| Biblioteca | Bytes | SHA-256 | Estado |
| --- | --- | --- | --- |
| rockform/assets/RockForm&#95;v140&#95;B5.blend | 70303281 | sha256:eaf896aaf335fc8e824bbfb8d9f55e8109fb8e79684d19466c984177bf08086c | METADATOS INSPECCIONADOS |

### rockform/assets/RockForm&#95;v140&#95;B5.blend

SIN BINDING: sockets inspeccionados; nombres/identificadores no demuestran comportamiento propio.
Opciones de sockets menú: NO INSPECCIONADAS por el probe actual; un default no define sus alternativas.

Grupo: Mapping

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;1 | Color | &#91;&quot;NodeSocketColor&quot;, null&#93; | OUTPUT | &#91;0, 0, 0, 0&#93; | &#91;null, null&#93; |  |
| Input&#95;9 | Seam Blend Softness | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.10000000149011612 | &#91;0, 1&#93; |  |
| Input&#95;3 | Front/Back (XZ) | &#91;&quot;NodeSocketColor&quot;, null&#93; | INPUT | &#91;1, 0, 0, 1&#93; | &#91;null, null&#93; |  |
| Input&#95;5 | Left/Right (YZ) | &#91;&quot;NodeSocketColor&quot;, null&#93; | INPUT | &#91;0, 1, 0, 1&#93; | &#91;null, null&#93; |  |
| Input&#95;7 | Top/Bottom (XY) | &#91;&quot;NodeSocketColor&quot;, null&#93; | INPUT | &#91;0, 0, 1, 1&#93; | &#91;null, null&#93; |  |

Grupo: RF&#95;Base Rotation

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Socket&#95;3 | Vector | &#91;&quot;NodeSocketRotation&quot;, null&#93; | OUTPUT | &#91;0, 0, 0&#93; | &#91;null, null&#93; |  |
| Socket&#95;0 | Align to Faces | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |
| Socket&#95;1 | Face Normal | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;0 | X Rot | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;1 | Y Rot | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;2 | Z Rot | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |

Grupo: RF&#95;BottomCover

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;0 | BottomCover | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | OUTPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;3 | Color B Smooth Falloff | &#91;&quot;NodeSocketColor&quot;, null&#93; | OUTPUT | &#91;0, 0, 0, 1&#93; | &#91;null, null&#93; |  |
| Socket&#95;5 | Edge Falloff | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;1 | Cover B Direction | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;2 | Cover B Spread | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;4 | AO Influence | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0.75 | &#91;0, 1&#93; |  |

Grupo: RF&#95;DisplaceAngle

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;1 | Vector | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | OUTPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;0 | Vector | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-10000, 10000&#93; |  |
| Input&#95;7 | Int | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;0, 2&#93; |  |

Grupo: RF&#95;DisplaceTex

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;0 | Result | &#91;&quot;NodeSocketColor&quot;, null&#93; | OUTPUT | &#91;0, 0, 0, 0&#93; | &#91;null, null&#93; |  |
| Socket&#95;0 | Triplanar Blending | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;2 | Tex 1 Multiplier | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;4 | Tex 2 Multiplier | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;2 | Image Tex1 | &#91;&quot;NodeSocketImage&quot;, null&#93; | INPUT | null | &#91;null, null&#93; |  |
| Socket&#95;5 | Tex1 Separate Axis Rot | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;6 | Tex2 Separate Axis Rot | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;8 | Image Tex2 | &#91;&quot;NodeSocketImage&quot;, null&#93; | INPUT | null | &#91;null, null&#93; |  |
| Input&#95;6 | Vector | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;3 | Vector | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;5 | Input | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-2147483648, 2147483647&#93; |  |
| Socket&#95;7 | Tex1 H Offset | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;8 | Tex1 V Offset | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;9 | Tex2 H Offset | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;10 | Tex2 V Offset | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;11 | Z-Split Switch | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |
| Socket&#95;12 | Z-Split Bias | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;0, 0&#93; |  |
| Socket&#95;13 | Z-Split Blend | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;0, 0&#93; |  |
| Socket&#95;16 | Socket | &#91;&quot;NodeSocketGeometry&quot;, null&#93; | INPUT | null | &#91;null, null&#93; |  |
| Socket&#95;14 | Invert Tex1 | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |
| Socket&#95;15 | Invert Tex2 | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |

Grupo: RF&#95;InstanceRandomRot

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;2 | Vector | &#91;&quot;NodeSocketRotation&quot;, null&#93; | OUTPUT | &#91;0, 0, 0&#93; | &#91;null, null&#93; |  |
| Input&#95;0 | X Rot Min | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;1 | X Rot Max | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;3 | Y Rot Min | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;2 | Y Rot Max | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;4 | Z Rot Min | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;3 | Z Rot Max | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;1 | Seed | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 1 | &#91;-10000, 10000&#93; |  |

Grupo: RF&#95;Master

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Socket&#95;0 | BSDF | &#91;&quot;NodeSocketShader&quot;, null&#93; | OUTPUT | null | &#91;null, null&#93; |  |
| Socket&#95;8 | Base Color Tint | &#91;&quot;NodeSocketColor&quot;, null&#93; | INPUT | &#91;0.7158145308494568, 0.39155539870262146, 0.171428844332695, 1&#93; | &#91;null, null&#93; | Base |
| Socket&#95;25 | Base Texture Mix | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 1 | &#91;0, 1&#93; | Base |
| Socket&#95;26 | Base Color Texture Scale | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; | Base |
| Socket&#95;22 | Base Roughness | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Base |
| Socket&#95;23 | Base Metallic | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Base |
| Socket&#95;29 | Occlusion Contrast | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Base |
| Socket&#95;1 | Normal Scale | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 1 | &#91;-1, 1&#93; | Base |
| Socket&#95;4 | Normal Strength | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.5 | &#91;0, 1&#93; | Base |
| Socket&#95;5 | Normal XYZ Rotation (Degrees) | &#91;&quot;NodeSocketVector&quot;, &quot;EULER&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; | Base |
| Socket&#95;47 | Base Normal &lt;&gt; Bump | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 1 | &#91;0, 1&#93; | Base |
| Socket&#95;54 | Base Edge Wear Tint | &#91;&quot;NodeSocketColor&quot;, null&#93; | INPUT | &#91;0, 0, 0, 1&#93; | &#91;null, null&#93; | Base - Additional Options |
| Socket&#95;56 | Base Edge Wear Spread | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Base - Additional Options |
| Socket&#95;57 | Base Edge Occlusion | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Base - Additional Options |
| Socket&#95;51 | Cover A Off &lt;&gt; On | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 1 | &#91;0, 1&#93; | Cover A |
| Socket&#95;9 | Cover A Color Tint | &#91;&quot;NodeSocketColor&quot;, null&#93; | INPUT | &#91;0.14705193042755127, 0.29175806045532227, 0.030711302533745766, 1&#93; | &#91;null, null&#93; | Cover A |
| Socket&#95;27 | Cover A Texture Mix | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 1 | &#91;0, 1&#93; | Cover A |
| Socket&#95;7 | Cover A Texture Scale | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.25 | &#91;-1, 1&#93; | Cover A |
| Socket&#95;44 | Cover A Roughness | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.5 | &#91;0, 1&#93; | Cover A |
| Socket&#95;43 | Cover A Metallic | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Cover A |
| Socket&#95;13 | Cover A Direction | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 1 | &#91;-1, 1&#93; | Cover A |
| Socket&#95;14 | Cover A Spread | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.10000000149011612 | &#91;-1, 1&#93; | Cover A |
| Socket&#95;48 | Cover A Edge Falloff | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.75 | &#91;0, 1&#93; | Cover A |
| Socket&#95;15 | Cover A Occlusion Influence | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Cover A |
| Socket&#95;6 | Cover A Fill | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Cover A |
| Socket&#95;24 | Cover A Thickness | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.5 | &#91;0, 1&#93; | Cover A |
| Socket&#95;3 | Cover A Normal Direction | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.25 | &#91;-1, 1&#93; | Cover A |
| Socket&#95;45 | Cover A Normal &lt;&gt; Bump | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Cover A |
| Socket&#95;50 | Cover B Off &lt;&gt; On | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 1 | &#91;0, 1&#93; | Cover B |
| Socket&#95;10 | Cover B Color Tint | &#91;&quot;NodeSocketColor&quot;, null&#93; | INPUT | &#91;0.7158145308494568, 0.39155539870262146, 0.171428844332695, 1&#93; | &#91;null, null&#93; | Cover B |
| Socket&#95;28 | Cover B Texture Mix | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 1 | &#91;0, 1&#93; | Cover B |
| Socket&#95;18 | Cover B Texture Scale | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.25 | &#91;-1, 1&#93; | Cover B |
| Socket&#95;41 | Cover B Roughness | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.5 | &#91;0, 1&#93; | Cover B |
| Socket&#95;42 | Cover B Metallic | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Cover B |
| Socket&#95;19 | Cover B Direction | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | -1 | &#91;-1, 1&#93; | Cover B |
| Socket&#95;49 | Cover B Edge Falloff | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.75 | &#91;0, 1&#93; | Cover B |
| Socket&#95;20 | Cover B Spread | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0.10000000149011612 | &#91;-1, 1&#93; | Cover B |
| Socket&#95;21 | Cover B Occlusion Influence | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Cover B |
| Socket&#95;17 | Cover B Fill | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;0, 1&#93; | Cover B |
| Socket&#95;40 | Cover B Thickness | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.5 | &#91;0, 1&#93; | Cover B |
| Socket&#95;16 | Cover B Normal Direction | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0 | &#91;-1, 1&#93; | Cover B |
| Socket&#95;46 | Cover B Normal &lt;&gt; Bump | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 1 | &#91;0, 1&#93; | Cover B |

Grupo: RF&#95;Position Spread

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;2 | Value | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | OUTPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;0 | Vector | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-10000, 10000&#93; |  |
| Input&#95;1 | Seed | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 1 | &#91;-10000, 10000&#93; |  |

Grupo: RF&#95;RandomScale

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;3 | Value | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | OUTPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;0 | Min | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;1 | Max | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;2 | Seed | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 1 | &#91;-10000, 10000&#93; |  |

Grupo: RF&#95;SetMat

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;1 | Geometry | &#91;&quot;NodeSocketGeometry&quot;, null&#93; | OUTPUT | null | &#91;null, null&#93; |  |
| Input&#95;0 | Geometry | &#91;&quot;NodeSocketGeometry&quot;, null&#93; | INPUT | null | &#91;null, null&#93; |  |
| Input&#95;2 | Material | &#91;&quot;NodeSocketMaterial&quot;, null&#93; | INPUT | {&quot;dataBlockReference&quot;: &quot;RF&#95;MAT&#95;Master&quot;, &quot;rnaType&quot;: &quot;Material&quot;} | &#91;null, null&#93; |  |
| Input&#95;3 | Image Tex1 | &#91;&quot;NodeSocketImage&quot;, null&#93; | INPUT | {&quot;dataBlockReference&quot;: &quot;RF&#95;DRF&#95;V5&#95;33.jpg&quot;, &quot;rnaType&quot;: &quot;Image&quot;} | &#91;null, null&#93; |  |
| Input&#95;20 | Image Tex2 | &#91;&quot;NodeSocketImage&quot;, null&#93; | INPUT | null | &#91;null, null&#93; |  |
| Input&#95;21 | Invert Color | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |
| Input&#95;10 | Displace Tex Type &#91;0-5&#93; | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;0, 5&#93; |  |
| Input&#95;4 | Resolution Levels | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 1 | &#91;0, 6&#93; |  |
| Socket&#95;1 | SubD Switch | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |
| Input&#95;16 | Tex1: Use XYZ, XY, Z &#91;0-2&#93; | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;0, 3&#93; |  |
| Input&#95;17 | Tex2: Use XYZ, XY, Z &#91;0-2&#93; | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;0, 3&#93; |  |
| Input&#95;5 | Displace Strength | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 1 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;7 | Tex 1 Strength | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 1 | &#91;0, 1&#93; |  |
| Socket&#95;8 | Tex 2 Strength | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 1 | &#91;0, 1&#93; |  |
| Input&#95;19 | Z Strength Multiplier | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0.5 | &#91;0, 1&#93; |  |
| Socket&#95;3 | Z Top-Bottom Relation | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |
| Socket&#95;0 | Inflate/Deflate | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0.5 | &#91;-1, 1&#93; |  |
| Socket&#95;4 | Seam Blending | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0.5 | &#91;0.009999999776482582, 1&#93; |  |
| Input&#95;7 | Tex 1 and 2 Scale | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 1 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;12 | Tex 1 Scale | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0.10000000149011612 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;13 | Tex 2 Scale | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0.10000000149011612 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;12 | Tex1 Combined Axis Rot | &#91;&quot;NodeSocketVector&quot;, &quot;EULER&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;9 | Tex1 Separate Axis Rot | &#91;&quot;NodeSocketVector&quot;, &quot;EULER&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;10 | Tex2 Combined Axis Rot | &#91;&quot;NodeSocketVector&quot;, &quot;EULER&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;11 | Tex2 Separate Axis Rot | &#91;&quot;NodeSocketVector&quot;, &quot;EULER&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;2 | Shade Smooth | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | true | &#91;null, null&#93; |  |
| Socket&#95;14 | Tex1 Horizontal Offset | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-1, 1&#93; |  |
| Socket&#95;16 | Tex1 Vertical Offset | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-1, 1&#93; |  |
| Socket&#95;15 | Tex2 Horizontal Offset | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-1, 1&#93; |  |
| Socket&#95;17 | Tex2 Vertical Offset | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-1, 1&#93; |  |
| Socket&#95;18 | Smooth Iterations | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 10 | &#91;0, 100&#93; |  |
| Socket&#95;19 | Smooth Switch | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | true | &#91;null, null&#93; |  |
| Socket&#95;20 | Z-Split Tex Switch | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |
| Socket&#95;21 | Z-Split Bias | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.5 | &#91;0, 1&#93; |  |
| Socket&#95;22 | Z-Blend | &#91;&quot;NodeSocketFloat&quot;, &quot;FACTOR&quot;&#93; | INPUT | 0.10000000149011612 | &#91;0, 1&#93; |  |
| Socket&#95;23 | Invert Tex1 | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |
| Socket&#95;24 | Invert Tex2 | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |

Grupo: RF&#95;SetShape

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;1 | Geometry | &#91;&quot;NodeSocketGeometry&quot;, null&#93; | OUTPUT | null | &#91;null, null&#93; |  |
| Input&#95;0 | Geometry | &#91;&quot;NodeSocketGeometry&quot;, null&#93; | INPUT | null | &#91;null, null&#93; |  |
| Input&#95;3 | Curve/Mesh Object | &#91;&quot;NodeSocketObject&quot;, null&#93; | INPUT | {&quot;dataBlockReference&quot;: &quot;RockForm Guide Object&quot;, &quot;rnaType&quot;: &quot;Object&quot;} | &#91;null, null&#93; |  |
| Input&#95;2 | Shape Collection | &#91;&quot;NodeSocketCollection&quot;, null&#93; | INPUT | null | &#91;null, null&#93; |  |
| Input&#95;20 | Debug Instances | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |
| Input&#95;16 | Spawn Method | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;0, 2&#93; |  |
| Socket&#95;1 | Align to Faces | &#91;&quot;NodeSocketBool&quot;, null&#93; | INPUT | false | &#91;null, null&#93; |  |
| Input&#95;11 | Random Seed | &#91;&quot;NodeSocketInt&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-2147483648, 2147483647&#93; |  |
| Input&#95;17 | Density | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0.25 | &#91;0.05000000074505806, 1&#93; |  |
| Input&#95;19 | Voxel Size | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0.25 | &#91;0.05000000074505806, 1&#93; |  |
| Input&#95;4 | Shape X Rot | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;2 | Shape X Rot Min | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;3 | Shape X Rot Max | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;5 | Shape Y Rot | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;4 | Shape Y Rot Min | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;5 | Shape Y Rot Max | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;6 | Shape Z Rot | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;6 | Shape Z Rot Min | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;7 | Shape Z Rot Max | &#91;&quot;NodeSocketFloat&quot;, &quot;ANGLE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;9 | Shape Scale Min | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0.5 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;10 | Shape Scale Max | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 1 | &#91;0, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;12 | Position Spread | &#91;&quot;NodeSocketVector&quot;, &quot;XYZ&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |

Grupo: RF&#95;TopCover

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;0 | Cover A | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | OUTPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;0 | Cover A Smooth Falloff | &#91;&quot;NodeSocketColor&quot;, null&#93; | OUTPUT | &#91;0, 0, 0, 0&#93; | &#91;null, null&#93; |  |
| Socket&#95;4 | Edge Falloff | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0.75 | &#91;0, 1&#93; |  |
| Socket&#95;1 | Cover A Direction | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 1 | &#91;-500, 500&#93; |  |
| Socket&#95;2 | Cover A Spread | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-1, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;3 | AO Influence | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |

Grupo: RF&#95;Triplanar

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;0 | Result | &#91;&quot;NodeSocketColor&quot;, null&#93; | OUTPUT | &#91;0, 0, 0, 0&#93; | &#91;null, null&#93; |  |
| Socket&#95;0 | Triplanar Blending | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;1 | Separate Axis Rot | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;1 | Images | &#91;&quot;NodeSocketImage&quot;, null&#93; | INPUT | null | &#91;null, null&#93; |  |
| Input&#95;2 | Vector | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;3 | H Offset | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Socket&#95;2 | V Offset | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | INPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |

Grupo: RF&#95;VolumeScaler

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Output&#95;1 | Value | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | OUTPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Output&#95;2 | Value | &#91;&quot;NodeSocketFloat&quot;, &quot;NONE&quot;&#93; | OUTPUT | 0 | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |
| Input&#95;0 | Geometry | &#91;&quot;NodeSocketGeometry&quot;, null&#93; | INPUT | null | &#91;null, null&#93; |  |

Grupo: TriPlanar

| ID | Nombre | Tipo / subtipo | Entrada/salida | Default | Mínimo / máximo | Panel |
| --- | --- | --- | --- | --- | --- | --- |
| Socket&#95;2 | Color | &#91;&quot;NodeSocketColor&quot;, null&#93; | OUTPUT | &#91;0, 0, 0, 0&#93; | &#91;null, null&#93; |  |
| Socket&#95;0 | ObjVector | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-10000, 10000&#93; |  |
| Socket&#95;1 | XYZ Rot | &#91;&quot;NodeSocketVector&quot;, &quot;NONE&quot;&#93; | INPUT | &#91;0, 0, 0&#93; | &#91;-340282346638528859811704183484516925440, 340282346638528859811704183484516925440&#93; |  |

## Diagnóstico e integración

Inventarios de soporte no reproducidos aquí; permanecen en el JSON identificado por su hash:

| Inventario | Registros |
| --- | --- |
| files | 2 |
| classes | 40 |
| imports | 6 |
| uiBindings | 83 |
| libraryHints | 2 |
| groupHints | 2 |
| socketHints | 38 |

Datos adicionales de bibliotecas/grafos (recursos, tipos y cantidades de nodos, enlaces y dependencias) también permanecen en ese JSON.

- {&quot;code&quot;: &quot;blend.not&#95;inspected&quot;, &quot;count&quot;: 1}
- {&quot;code&quot;: &quot;behavior.review&#95;required&quot;, &quot;message&quot;: &quot;Declarations do not establish behavior. Binary libraries and runtime state are not evaluated.&quot;}

INTEGRATION REQUIRED: Editor/ProceduralAuthoring, M10.5, para UI Qt, preview/diff y Apply/Undo/Redo; asset pipeline/M8 para admisión y cook.
Para ampliar: pruebas de comportamiento -> operación propia -> descriptor/registro -> perfil revisado -> conformidad y diez procesos deterministas. Pasos manuales y contrato: ADDON_TEMPLATE.md.
