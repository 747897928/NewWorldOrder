"""康体区墙面与顶面深化；保留器械和瑜伽通行动线。"""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforeWellnessEnvelope.blend'
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
palette={'Wood':('M_Wall_Wood',(.22,.13,.075,1)), 'Metal':('M_Metall_Black',(.025,.028,.032,1)), 'Stone':('Hub/M_Limestone',(.42,.39,.33,1)), 'Bronze':('Hub/M_Bronze',(.25,.17,.08,1)), 'Glow':('Hub/M_WellnessDiffuser',(1,.7,.38,1))}
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


def cylinder(a,b,r,mat,sides=64):
    av,bv=Vector(p(a)),Vector(p(b));axis=bv-av
    bpy.ops.mesh.primitive_cylinder_add(vertices=sides,radius=r/100,depth=axis.length,location=(av+bv)/2)
    o=bpy.context.object;o.rotation_euler=axis.to_track_quat('Z','Y').to_euler()
    for f in o.data.polygons:f.use_smooth=len(f.vertices)==4
    o.data.materials.append(mats[mat]);parts.append(o);return o

# 北墙格栅内凸约 22 cm，顶部檐口约 36 cm；低处不增加柜台挤占垫后通道。
box((1060,1383,166),(420,10,316),'Stone',.8)
for x in [850,1270]:box((x,1376,166),(1.8,2,316),'Bronze',.2)
for lo,hi in [(720,842),(1280,1384)]:
    box(((lo+hi)/2,1382,166),(hi-lo,8,316),'Metal',.2)
    for x in range(lo+4,hi,12):box((x,1374,166),(6,12,316),'Wood',.7)
# 浅浮雕圆盘与金属底圈；闭合低面数圆柱，不使用大面积透明材质。
cylinder((1060,1375,185),(1060,1378,185),78,'Bronze',96)
cylinder((1060,1371,185),(1060,1375,185),75.5,'Stone',96)
for z in [37,303]:box((1060,1373,z),(384,2,1.5),'Bronze',.2)
# 墙顶遮光檐与嵌入灯带，实际光源由现有 UE 光照体系调节。
box((1050,1370,313),(672,32,12),'Wood',.7)
box((1050,1357,309),(648,3,2),'Glow',.2)
export('HM_YogaFeatureWall',True)

# 两房顶面以浅色吸音板和木边形成层次，最低处 3.08 m，不降低玩家头部净空。
for cx,cy,w,d in [(1080,345,490,530),(1050,1060,470,500)]:
    box((cx,cy,322),(w+8,d+8,8),'Metal',.5)
    box((cx,cy,317),(w,d,8),'Stone',.6)
    for x in [cx-w/2,cx+w/2]:box((x,cy,314),(9,d+20,14),'Wood',.7)
    for y in [cy-d/2,cy+d/2]:box((cx,y,314),(w+9,9,14),'Wood',.7)
# 健身房两条实体线形灯；总照度仍由原 HM_Gym_Light 提供。
for x in [955,1205]:
    box((x,345,309),(10,370,6),'Metal',.5)
    box((x,345,305.8),(6,360,.6),'Glow',.15)
# 瑜伽顶面使用四个嵌入式小灯，不逐个新增动态光源。
for x in [895,1205]:
    for y in [910,1210]:
        cylinder((x,y,311),(x,y,315),6,'Metal',24)
        cylinder((x,y,310.6),(x,y,311),4.4,'Glow',24)
export('HM_WellnessCeilingDetail',False)
(OUT/'wellness_envelope_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('WELLNESS_ENVELOPE_SAVED',[(r['mesh'],r['triangles']) for r in manifest])
