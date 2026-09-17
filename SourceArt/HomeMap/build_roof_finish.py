"""两翼屋顶碎石保护层和矮女儿墙；保持原屋檐及标高。"""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforeArchitecturalClosure.blend'
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
palette={'Wood':('M_Wall_Wood',(.22,.13,.075,1)), 'Metal':('M_Metall_Black',(.025,.028,.032,1)), 'Stone':('Hub/M_Limestone',(.42,.39,.33,1)), 'Bronze':('Hub/M_Bronze',(.25,.17,.08,1)), 'Glow':('Hub/M_Warm_Light',(1,.7,.38,1))}
palette['Gravel']=('Hub/M_RoofAggregate',(.16,.16,.14,1))
palette['Glass']=('Hub/M_Architectural_Glass',(.2,.3,.35,.12))
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
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
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

# 顶面原标高 370 cm。内退保护层留下白色檐口边带；屋顶不新建可玩房间。
for x in [-1050,1050]:
    box((x,200,371.5),(638,2328,3),'Gravel',.4)
    for dx in [-339,339]:box((x+dx,200,385),(10,2390,30),'Stone',.5)
    for y in [-990,1390]:box((x,y,385),(688,10,30),'Stone',.5)
    # 从上层眼高看也能分辨的深色压顶，与浅色檐口形成边界。
    for dx in [-339,339]:box((x+dx,200,401),(14,2394,2),'Metal',.3)
    for y in [-990,1390]:box((x,y,401),(692,14,2),'Metal',.3)
    inner=x+265 if x<0 else x-265
    # 靠中央 Pavilion 的低种植床，中央留检修间隔，屋面仍是低矮背景。
    for y,length in [(-570,580),(240,600),(1080,400)]:
        for dx in [-43,43]:box((inner+dx,y,393),(6,length,40),'Stone',.5)
        for dy in [-length/2+3,length/2-3]:box((inner,y+dy,393),(80,6,40),'Stone',.5)
        box((inner,y,404),(80,length-12,4),'Metal',.4)
    for y in [-190,650]:box((x,y,374),(620,65,2),'Stone',.5)
export('HM_WingRoofFinish',True)
(OUT/'roof_finish_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('ROOF_FINISH_SAVED',manifest[0]['triangles'])
