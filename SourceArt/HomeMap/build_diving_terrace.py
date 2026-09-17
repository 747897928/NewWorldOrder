"""阳台直线跳水台；只重建本脚本拥有的三件网格，不覆盖其余建筑。"""
import bpy,bmesh,math,json,os
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforeDivingTerrace.blend'
checkpoint.parent.mkdir(parents=True,exist_ok=True)
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
for name in ['HM_DivingTerrace','HM_DivingTerraceGuard','HM_DivingTerraceInlay']:
    old=bpy.data.objects.get(name)
    if old:bpy.data.objects.remove(old,do_unlink=True)
palette={'Wood':('M_Wall_Wood',(.22,.13,.07,1)),'Metal':('M_Metall_Black',(.035,.038,.04,1)),'Stone':('Hub/M_Limestone',(.5,.46,.4,1)),'Bronze':('Hub/M_Bronze',(.3,.2,.1,1)),'Rubber':('Hub/M_Rubber',(.035,.04,.04,1))}
mats={}
palette['Glass']=('Hub/M_Architectural_Glass',(.2,.3,.35,.12))
for n,(path,col) in palette.items():
    m=bpy.data.materials.get('Terrace_'+n) or bpy.data.materials.new('Terrace_'+n);m.diffuse_color=col;mats[n]=m
parts=[];manifest=[]
def p(v):return (v[0]/100,-v[1]/100,v[2]/100)
def finish(o,m):o.data.materials.append(mats[m]);parts.append(o);return o
def box(loc,dims,mat,bevel=.5):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p(loc));o=bpy.context.object;o.dimensions=[v/100 for v in dims];bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        b=o.modifiers.new('Soft edge','BEVEL');b.width=bevel/100;b.segments=2;bpy.ops.object.modifier_apply(modifier=b.name)
    return finish(o,mat)
def beam(a,b,width,depth,mat):
    av,bv=Vector(p(a)),Vector(p(b));d=bv-av;o=box(((a[0]+b[0])/2,(a[1]+b[1])/2,(a[2]+b[2])/2),(width,depth,d.length*100),mat,.35);o.rotation_euler=d.to_track_quat('Z','Y').to_euler();return o
def export(name,collision=True):
    global parts
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;o.data.name='SM_'+name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
    uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
    for f in o.data.polygons:
        axes=[i for i in range(3) if i!=max(range(3),key=lambda j:abs(f.normal[j]))]
        for li in f.loop_indices:
            v=o.data.vertices[o.data.loops[li].vertex_index].co;uv.data[li].uv=(v[axes[0]],v[axes[1]])
    o.data.update();bpy.context.view_layer.update()
    # 当前用户新安装的 BFEU 缺失内部 fbxio 模块；使用 Blender 内置 FBX，保持同一米制和轴向契约。
    bpy.ops.export_scene.fbx(filepath=str(OUT/'BFEU_Production/StaticMesh'/('SM_'+name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Z',axis_up='Y',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False)
    o.data.calc_loop_triangles()
    (OUT/'BFEU_Production/StaticMesh'/('SM_'+name+'_additional_data.json')).write_text(json.dumps({'exporter':'Blender built-in FBX','reason':'Installed BFEU missing fbxio/io_scene_fbx_4_4','axis_forward':'-Z','axis_up':'Y','units':'meters','actor_transform':'identity','triangles':len(o.data.loop_triangles)},indent=2),encoding='utf-8')
    manifest.append({'mesh':'SM_'+name,'triangles':len(o.data.loop_triangles),'materials':['/Game/Environment/HomeMap/Materials/'+palette[m.name.removeprefix('Terrace_')][0] for m in o.data.materials],'nanite':False,'collision':collision,'instances':[{'label':name,'location_cm':[0,0,0],'rotation':[0,0,0],'scale':[1,1,1]}]});parts=[]
# 顶面与阳台齐平，从开口沿 +Y 连续直跑；池上无折返、横栏或高差。
box((-280,517.5,337),(180,745,26),'Metal',2)
box((-280,517.5,351),(180,745,2),'Wood',.8)
box((-280,835,352.15),(156,100,.3),'Rubber',.1)
# 支撑位于池口西侧，中央浅水台阶及庭院地面路线保持净空。
box((-380,232,164),(26,30,328),'Stone',1.2)
beam((-380,232,242),(-360,695,323),12,18,'Metal')
beam((-380,232,318),(-200,232,318),12,16,'Metal')
export('HM_DivingTerrace')
# 细玻璃必须有可见端柱和底座，玩家在逆光下也能识别边界；末端仍开放。
for x in [-370,-190]:
    box((x,504,406),(2,652,108),'Glass',0)
    box((x,504,461),(3,652,3),'Metal',.4)
    box((x,504,355),(4,652,6),'Metal',.5)
    for y in [180,395,615,828]:
        box((x,y,408),(4.5,4.5,108),'Metal',.35)
        box((x,y,353.5),(10,10,3),'Metal',.35)
        for z in [377,435]:box((x,y,z),(5.5,9,3),'Bronze',.2)
export('HM_DivingTerraceGuard')
# 导向仅靠材质与真实铺装，不创建新交互或新按键。
for y in range(210,781,38):box((-280,y,352.1),(172,.35,.2),'Bronze',.03)
for y in [850,860,870]:box((-280,y,352.4),(140,1.2,.3),'Bronze',.1)
export('HM_DivingTerraceInlay',False)
for row in manifest:row['rebuild_material_slots']=True
(OUT/'diving_terrace_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('DIVING_TERRACE_EXPORTED',[(r['mesh'],r['triangles']) for r in manifest])
