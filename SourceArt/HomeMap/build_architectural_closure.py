"""入口与 Gallery 建筑围合。只重建本清单拥有的网格，门扇单独保留铰链轴心。"""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforeArchitecturalClosure.blend'
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
palette={'Wood':('M_Wall_Wood',(.22,.13,.075,1)), 'Metal':('M_Metall_Black',(.025,.028,.032,1)), 'Stone':('Hub/M_Limestone',(.42,.39,.33,1)), 'Bronze':('Hub/M_Bronze',(.25,.17,.08,1)), 'Glow':('Hub/M_Warm_Light',(1,.7,.38,1))}
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
# 后墙用三处竖窗分隔阅读、沙发和书桌，墙体承接原有木饰面。
rear_windows=[(-510,-345),(40,155),(390,520)]
box((0,-1008,391),(1120,24,78),'Stone',.6)
box((0,-1008,663),(1120,24,36),'Stone',.6)
last=-560
for lo,hi in rear_windows+[(560,560)]:
    if lo>last:box(((last+lo)/2,-1008,537.5),(lo-last,24,215),'Stone',.6)
    last=hi
for lo,hi in rear_windows:
    for x in [lo+2,hi-2]:box((x,-994,537.5),(4,8,215),'Metal')
    for z in [432,643]:box(((lo+hi)/2,-994,z),(hi-lo,8,4),'Metal')
    box(((lo+hi)/2,-989,428),(hi-lo+8,36,4),'Stone')
# 西墙避开两米宽的上层廊道；家具留在后部宽厅，窗台仅凸出八厘米。
west_windows=[(-642,-380),(-350,-65)]
box((-562,-500,396),(24,1000,88),'Stone',.6)
box((-562,-500,663),(24,1000,36),'Stone',.6)
last=-1000
for lo,hi in west_windows+[(0,0)]:
    if lo>last:box((-562,(last+lo)/2,542.5),(24,lo-last,205),'Stone',.6)
    last=hi
for lo,hi in west_windows:
    for y in [lo+2,hi-2]:box((-549,y,542.5),(8,4,205),'Metal')
    for z in [442,643]:box((-549,(lo+hi)/2,z),(8,hi-lo,4),'Metal')
    box((-552,(lo+hi)/2,438),(40,hi-lo+8,4),'Stone')
    box((-549,(lo+hi)/2,542.5),(6,3,205),'Metal')
# 檐下与墙脚收边使立面在池畔也有明确结构层次。
box((-579,-500,677),(10,1020,10),'Wood')
box((0,-1024,677),(1168,10,10),'Wood')
box((-548,-500,358),(2,1000,12),'Metal')
export('HM_GalleryEnvelope',True)
for lo,hi in rear_windows:box(((lo+hi)/2,-1001,537.5),(hi-lo-8,2,207),'Glass',0)
for lo,hi in west_windows:box((-556,(lo+hi)/2,542.5),(2,hi-lo-8,197),'Glass',0)
export('HM_GalleryWindowGlass',True)
# 开放客厅的楼梯入口只保留顶梁，不再叠放四片玻璃与竖框。
box((350,0,326),(420,10,8),'Metal',.5)
box((350,-8,318),(420,6,8),'Wood',.5)
export('HM_StairApproachHeader',False)
# 入户门框、门外雨棚及两侧木石门套。通道无高门槛。
for x in [-133,133]:
    box((x,-1008,128),(14,26,256),'Metal',.7)
    box((x*1.2,-1020,130),(35,30,260),'Wood',.7)
box((0,-1008,254),(280,26,12),'Metal',.7)
box((0,-1100,278),(360,246,14),'Metal',1)
box((0,-1100,268),(342,226,6),'Wood',.6)
box((0,-1010,.5),(252,38,1),'Metal',.2)
for x in [-154,154]:box((x,-1036,169),(3,2,64),'Glow',.2)
export('HM_EntryPortal',True)
# 单扇使用本地 +X 为宽度；门铰链在原点，供既有交互系统后续驱动旋转。
box((62.75,0,123),(125.5,8,246),'Wood',.8)
for x in [2,123.5]:box((x,0,123),(4,9,246),'Metal',.4)
for z in [2,244]:box((62.75,0,z),(121.5,9,4),'Metal',.4)
for x in range(12,111,14):box((x,-4.08,123),(.6,.2,234),'Metal',.06)
for y in [-6,6]:
    box((109,y,126),(2,2,70),'Bronze',.4)
    for z in [96,156]:box((109,y/2,z),(3,6,3),'Bronze',.3)
for z in [32,123,214]:box((1,0,z),(5,12,12),'Metal',.5)
export('HM_EntryDoorLeaf',True)
manifest[-1]['instances']=[{'label':'HM_EntryDoor_Left','location_cm':[-126,-1008,1],'rotation':[0,-100,0],'scale':[1,1,1]}, {'label':'HM_EntryDoor_Right','location_cm':[126,-1008,1],'rotation':[0,280,0],'scale':[1,1,1]}]
leaf=bpy.data.objects['HM_EntryDoorLeaf'];leaf.location=p((-126,-1008,1));leaf.rotation_euler.z=math.radians(100)
old=bpy.data.objects.get('HM_EntryDoor_Right')
if old:bpy.data.objects.remove(old,do_unlink=True)
right=leaf.copy();right.data=leaf.data;bpy.context.collection.objects.link(right);right.name='HM_EntryDoor_Right';right.location=p((126,-1008,1));right.rotation_euler.z=math.radians(-280)
(OUT/'architectural_closure_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('ARCHITECTURAL_CLOSURE_SAVED',[(r['mesh'],r['triangles']) for r in manifest])
