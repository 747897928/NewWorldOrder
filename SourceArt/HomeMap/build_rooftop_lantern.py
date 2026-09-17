"""屋顶茶几提灯增量，仅更新此灯具并保存 Master。"""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforeLandscapeFinish.blend'
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
palette={'Wood':('M_Wall_Wood',(.22,.13,.075,1)), 'Metal':('M_Metall_Black',(.025,.028,.032,1)), 'Stone':('Hub/M_Limestone',(.42,.39,.33,1)), 'Bronze':('Hub/M_Bronze',(.25,.17,.08,1)), 'Glow':('Hub/M_Warm_Light',(1,.7,.38,1))}
palette['Glass']=('Hub/M_Architectural_Glass',(.2,.3,.35,.12))
palette['Linen']=('Hub/MI_Suite_Linen',(.5,.47,.4,1))
palette['Terrain']=('Hub/M_EstateTerrain',(.12,.16,.07,1))
mats={}
for n,(path,col) in palette.items():
    m=bpy.data.materials.get('Closure_'+n) or bpy.data.materials.new('Closure_'+n);m.diffuse_color=col;mats[n]=m
parts=[];manifest=[]
def p(v):return (v[0]/100,-v[1]/100,v[2]/100)
def box(loc,dims,mat,bevel=.4):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p(loc));o=bpy.context.object;o.dimensions=[v/100 for v in dims];bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Finished edges','BEVEL');mod.width=bevel/100;mod.segments=2;bpy.ops.object.modifier_apply(modifier=mod.name)
    o.data.materials.append(mats[mat]);parts.append(o);return o
def export(name,collision):
    global parts
    old=bpy.data.objects.get(name)
    if old:bpy.data.objects.remove(old,do_unlink=True)
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;o.data.name='SM_'+name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    # 开放地形没有封闭体积，重算法线可能整体朝下；必须验证朝上，不能用双面材质遮掩。
    if name=='HM_EstateBackdrop' and sum(f.normal.z for f in bm.faces)<0:bmesh.ops.reverse_faces(bm,faces=list(bm.faces))
    bm.to_mesh(o.data);bm.free()
    uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
    for f in o.data.polygons:
        axes=[i for i in range(3) if i!=max(range(3),key=lambda j:abs(f.normal[j]))]
        for li in f.loop_indices:
            co=o.data.vertices[o.data.loops[li].vertex_index].co;uv.data[li].uv=(co[axes[0]],co[axes[1]])
    o.data.update();bpy.context.view_layer.update()
    bpy.ops.export_scene.fbx(filepath=str(OUT/'BFEU_Production/StaticMesh'/('SM_'+name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Z',axis_up='Y',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False)
    o.data.calc_loop_triangles()
    manifest.append({'mesh':'SM_'+name,'triangles':len(o.data.loop_triangles),'materials':['/Game/Environment/HomeMap/Materials/'+palette[m.name.removeprefix('Closure_')][0] for m in o.data.materials],'nanite':False,'collision':collision,'rebuild_material_slots':True,'instances':[{'label':name,'location_cm':[0,0,0],'rotation':[0,0,0],'scale':[1,1,1]}]})
    parts=[]


# 台灯使用真实灯罩、边框和提手；发光面不代替唯一的小范围实际光源。
x,y,z=-287,-666,778
box((x,y,z+1),(16,16,2),'Metal',.4)
box((x,y,z+12),(10,10,18),'Glow',.7)
for dx in [-7,7]:
    for dy in [-7,7]:box((x+dx,y+dy,z+13),(1.2,1.2,24),'Metal',.25)
box((x,y,z+25),(16,16,2),'Metal',.5)
for dx in [-5,5]:box((x+dx,y,z+31),(1.1,1.1,10),'Bronze',.25)
box((x,y,z+36),(11,1.1,1.1),'Bronze',.25)
export('HM_RooftopLantern',False)
(OUT/'rooftop_lantern_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('ROOFTOP_LANTERN_SAVED',manifest[0]['triangles'])
