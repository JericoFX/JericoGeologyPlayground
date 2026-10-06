"""Original localized Boolean cuts. Only a rendered copy is changed."""
import math
import bpy,bmesh
from mathutils import Vector


def _bounds(obj):
 return [[min(v.co[i] for v in obj.data.vertices) for i in range(3)],[max(v.co[i] for v in obj.data.vertices) for i in range(3)]]


def _overlap(a,b):return all(min(a[1][i],b[1][i])-max(a[0][i],b[0][i])>1e-8 for i in range(3))

def _inside(p,cut,eps=1e-6):return all(cut['minimum'][i]-eps<=p[i]<=cut['maximum'][i]+eps for i in range(3))

def _volume(mesh):
 mesh.calc_loop_triangles()
 return sum(mesh.vertices[t.vertices[0]].co.dot(mesh.vertices[t.vertices[1]].co.cross(mesh.vertices[t.vertices[2]].co))/6 for t in mesh.loop_triangles)


def _interior():
 m=bpy.data.materials.get('Damage.fresh_cut')
 if m:return m
 m=bpy.data.materials.new('Damage.fresh_cut');m.use_nodes=True
 bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(.062,.032,.014,1);bs.inputs['Roughness'].default_value=.95
 return m


def apply_frame_cuts(objects,cuts):
 reports=[];cuts=[c for c in cuts if c['body']=='frame']
 if not cuts:return reports
 for pid,obj in objects.items():
  if obj.type!='MESH' or obj.get('role')!='Frame' or not obj.get('material_role','').startswith('wood'):continue
  relevant=[c for c in cuts if _overlap(_bounds(obj),[c['minimum'],c['maximum']])]
  if not relevant:continue
  original=[tuple(v.co) for v in obj.data.vertices];volume_before=_volume(obj.data)
  for cut in relevant:
   lo=Vector(cut['minimum']);hi=Vector(cut['maximum'])
   bpy.ops.mesh.primitive_cube_add(size=1)
   cutter=bpy.context.object;cutter.name='Temporary.local_slot';cutter.parent=obj.parent
   cutter.location=(lo+hi)*.5;cutter.dimensions=hi-lo
   bpy.context.view_layer.objects.active=cutter;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
   bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
   boolean=obj.modifiers.new('Own localized volume subtraction','BOOLEAN');boolean.operation='DIFFERENCE';boolean.solver='EXACT';boolean.object=cutter
   while obj.modifiers.find(boolean.name)>0:bpy.ops.object.modifier_move_up(modifier=boolean.name)
   bpy.ops.object.modifier_apply(modifier=boolean.name)
   bpy.data.objects.remove(cutter,do_unlink=True)
  mesh=obj.data;mesh.update();mesh.calc_loop_triangles();mat_index=len(mesh.materials);mesh.materials.append(_interior());interior_faces=0
  uv=mesh.uv_layers.active or mesh.uv_layers.new(name='MetricUV')
  for poly in mesh.polygons:
   pts=[mesh.vertices[i].co for i in poly.vertices]
   newly_cut=any(any(all(abs(p[axis]-plane)<2e-6 for p in pts) for axis in range(3) for plane in [cut['minimum'][axis],cut['maximum'][axis]]) and _inside(poly.center,cut,2e-6) for cut in relevant)
   if newly_cut:
    poly.material_index=mat_index;interior_faces+=1
    axis=max(range(3),key=lambda i:abs(poly.normal[i]));u,v=[i for i in range(3) if i!=axis]
    for loop in poly.loop_indices:
     p=mesh.vertices[mesh.loops[loop].vertex_index].co;uv.data[loop].uv=(p[u],p[v])
  existing={tuple(round(x,6) for x in v.co) for v in mesh.vertices}
  outside=[p for p in original if not any(_inside(p,c) for c in relevant)]
  missing=sum(tuple(round(x,6) for x in p) not in existing for p in outside)
  bm=bmesh.new();bm.from_mesh(mesh);nonmanifold=sum(not e.is_manifold for e in bm.edges);bm.free()
  mesh.calc_loop_triangles();zero=0
  for tri in mesh.loop_triangles:
   a,b,c=[mesh.vertices[i].co for i in tri.vertices]
   zero+=(b-a).cross(c-a).length_squared<=1e-18
  reports.append({'part_id':pid,'cuts':len(relevant),'removed_volume_m3':volume_before-_volume(mesh),'outside_vertices_missing':missing,'nonmanifold_edges':nonmanifold,'zero_area_triangles':zero,'interior_faces':interior_faces,'finite_uvs':all(math.isfinite(x) for corner in uv.data for x in corner.uv),'source_geometry_modified':False})
 return reports
