"""卫浴壁挂毛巾与小木凳，补生活细节，靠墙布置。"""
import bpy,math,os,json,bmesh
from pathlib import Path
OUT=Path(__file__).resolve().parent
assert not bpy.data.objects.get('HM_SpaTowelStation')
parts=[]
def box(loc,dims,mat,bevel):
    bpy.ops.mesh.primitive_cube_add(size=1,location=(loc[0]/100,-loc[1]/100,loc[2]/100));o=bpy.context.object;o.dimensions=[d/100 for d in dims]
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(bpy.data.materials[mat]);b=o.modifiers.new('Soft edge','BEVEL');b.width=bevel/100;b.segments=3;bpy.ops.object.modifier_apply(modifier=b.name);parts.append(o)
for y in [-38,38]:box((7,y,155),(14,2,2),'Suite_Brass',.6)
box((14,0,155),(2,80,2),'Suite_Brass',.7)
verts=[];faces=[]
profile=[(9,110),(9,130),(9,153)]+[(14-5*math.cos(t),153+5*math.sin(t)) for t in [i*math.pi/8 for i in range(1,9)]]+[(19,135),(19,115),(19,94)]
for j,(x,z) in enumerate(profile):
    for i in range(25):
        y=-26+i*52/24;wr=.55*math.sin(i/24*math.tau*5)*math.sin(j/(len(profile)-1)*math.pi/2)
        verts.append(((x+wr)/100,-y/100,(z+.25*math.cos(i*.8))/100))
for j in range(len(profile)-1):
    for i in range(24):
        k=j*25+i;faces.append((k,k+1,k+26,k+25))
me=bpy.data.meshes.new('Hanging cotton');me.from_pydata(verts,[],faces);me.update()
o=bpy.data.objects.new('Hanging cotton',me);bpy.context.collection.objects.link(o);o.data.materials.append(bpy.data.materials['Suite_Fabric'])
for p in me.polygons:p.use_smooth=True
bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;b=o.modifiers.new('Cotton thickness','SOLIDIFY');b.thickness=.003;bpy.ops.object.modifier_apply(modifier=b.name);parts.append(o)
for y in [-34,34]:
    for x in [13,43]:box((x,y,22),(5,5,44),'Suite_Wood',.6)
for x in [13,23,33,43]:box((x,0,45),(8,84,5),'Suite_Wood',.6)
box((28,0,51),(28,40,7),'Suite_Fabric',2.5);box((28,0,58),(28,38,7),'Suite_Fabric',2.5)
bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name='HM_SpaTowelStation';o.data.name='SM_HM_SpaTowelStation'
bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
for p in o.data.polygons:
    axes=[i for i in range(3) if i!=max(range(3),key=lambda j:abs(p.normal[j]))]
    for li in p.loop_indices:
        co=o.data.vertices[o.data.loops[li].vertex_index].co;uv.data[li].uv=(co[axes[0]],co[axes[1]])
o.location=(-13.65,-11.15,0);o.bfu_export_type='export_self_only';o.bfu_rotate_to_zero_for_export=True;o.bfu_auto_generate_collision=False
bpy.ops.object.exportforunreal();o.data.calc_loop_triangles()
(OUT/'spa_accessories_report.json').write_text(json.dumps({'name':o.data.name,'triangles':len(o.data.loop_triangles),'materials':[m.name for m in o.data.materials]}),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('SPA_TOWELS',len(o.data.loop_triangles))
