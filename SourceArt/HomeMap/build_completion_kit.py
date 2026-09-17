"""整场景收尾套件：厨房收纳、餐厅顶面、庭院凉亭和立面。只建本清单拥有的几何。"""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforeCompletion.blend'
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
palette={'Wood':('M_Wall_Wood',(.22,.13,.075,1)), 'Metal':('M_Metall_Black',(.025,.028,.032,1)), 'Stone':('Hub/M_Limestone',(.42,.39,.33,1)), 'Bronze':('Hub/M_Bronze',(.25,.17,.08,1)), 'Glow':('Hub/M_Warm_Light',(1,.7,.38,1))}
mats={}
for n,(path,col) in palette.items():
    m=bpy.data.materials.get('Completion_'+n) or bpy.data.materials.new('Completion_'+n);m.diffuse_color=col;mats[n]=m
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
    manifest.append({'mesh':'SM_'+name,'triangles':len(o.data.loop_triangles),'materials':['/Game/Environment/HomeMap/Materials/'+palette[m.name.removeprefix('Completion_')][0] for m in o.data.materials],'nanite':False,'collision':collision,'rebuild_material_slots':True,'instances':[{'label':name,'location_cm':[0,0,0],'rotation':[0,0,0],'scale':[1,1,1]}]})
    parts=[]
# 厨房东侧的浅柜与开放层板：中岛东端仍留约 190 cm 操作通路。
box((1358,-680,46),(48,280,76),'Wood',1)
box((1365,-680,8),(30,264,16),'Metal')
box((1355,-680,87),(54,286,6),'Stone',.8)
box((1380,-680,179),(4,286,178),'Wood')
for y in [-823,-537]:box((1360,y,178),(44,5,178),'Wood')
for z in [133,191,267]:
    box((1360,-680,z),(44,282,4),'Wood')
    if z!=267:box((1341,-680,z-2.3),(2,246,.6),'Glow',.1)
for y in [-751,-680,-609]:box((1333.8,y,48),(.5,1.2,65),'Metal',.08)
for y in [-790,-720,-650,-580]:box((1333.2,y,71),(1,29,1.5),'Bronze',.15)
# 中岛拆分板缝、踢脚与嵌入式灶面，用建模交代用途与尺度。
box((990,-377.2,9),(283,1,14),'Metal',.2)
for x in [915,990,1065]:box((x,-377.2,51),(1,1,68),'Metal',.1)
for x in [877,952,1027,1102]:box((x,-376.4,84),(31,1.5,1.3),'Bronze',.15)
box((920,-445,100.8),(72,53,.6),'Metal',.25)
for x in [903,937]:
    for y in [-460,-430]:
        bpy.ops.mesh.primitive_torus_add(major_radius=.105,minor_radius=.0025,major_segments=24,minor_segments=4,location=p((x,y,101.3)))
        o=bpy.context.object;o.data.materials.append(mats['Bronze']);parts.append(o)
export('HM_KitchenJoineryFinish',True)
# 餐桌上方悬浮木顶与细阴影边；灯光继续复用现有房间灯。
box((1040,-135,324),(450,305,7),'Metal',1)
box((1040,-135,319),(436,291,4),'Wood',1)
for x in [842,1238]:box((x,-135,316.8),(1.2,250,.5),'Glow',.1)
box((1383,-160,169),(5,250,208),'Wood',.6)
for y in range(-276,-39,12):box((1378,y,169),(4,3,196),'Wood',.5)
box((1372,-160,162),(9,142,110),'Stone',1.4)
for z in [120,204]:box((1366.7,-160,z),(1,112,1),'Bronze',.1)
export('HM_DiningFinish',False)
# 小尺度花园凉亭，木地台与住宅齐平；柱只在边角，中央保持可穿行。
box((1170,1740,0),(660,460,8),'Wood',1)
for x in [858,1482]:
    for y in [1530,1950]:box((x,y,142),(12,12,284),'Metal',.8)
for y in [1530,1950]:box((1170,y,286),(660,16,20),'Wood',1)
for x in [858,1482]:box((x,1740,280),(14,434,15),'Metal',.6)
for y in range(1530,1960,35):box((1170,y,300),(680,7,10),'Wood',.6)
for y in range(1535,1960,22):box((1170,y,4.08),(620,.25,.16),'Metal',.02)
export('HM_GardenLoungePavilion',True)
# 两翼立面连续的深色檐口与石材窄柱，避开 188 cm 池沿通行带。
for side in [-1,1]:
    box((side*704,720,326),(7,1380,7),'Metal',.4)
    box((side*708,720,318),(9,1360,8),'Wood',.6)
    for y in [55,720,1385]:box((side*723,y,161),(24,28,322),'Stone',.8)
export('HM_WingFacadeFinish',True)
# 西侧静园的分缝踏石与围墙收边，保持室内向花园的视线。
for y in [1490,1600,1710,1820,1930,2040]:box((-1050,y,1),(168,82,6),'Stone',2)
for side in [-1,1]:box((side*1725,600,183),(40,3590,8),'Stone',1.2)
box((0,2350,204),(3470,40,8),'Stone',1.2)
export('HM_GardenPavingFinish',True)
(OUT/'completion_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('COMPLETION_KIT_SAVED',[(r['mesh'],r['triangles']) for r in manifest])
