"""中央屋顶观景与双高度跳水路线；保留现有二层和低跳台。"""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforeRooftopRoute.blend'
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

def beam(a,b,w,d,mat):
    av,bv=Vector(p(a)),Vector(p(b));delta=bv-av
    o=box(tuple((a[i]+b[i])/2 for i in range(3)),(w,d,delta.length*100),mat,.3)
    o.rotation_euler=delta.to_track_quat('Z','Y').to_euler();return o

def rail(a,b,z,height=108):
    length=math.dist(a,b);count=max(1,math.ceil(length/180))
    for i in range(count+1):
        t=i/count;x=a[0]+(b[0]-a[0])*t;y=a[1]+(b[1]-a[1])*t
        box((x,y,z+height/2),(5,5,height),'Metal',.3)
        box((x,y,z+1.5),(12,12,3),'Metal',.3)
    beam((a[0],a[1],z+height),(b[0],b[1],z+height),4,4,'Metal')
    beam((a[0],a[1],z+48),(b[0],b[1],z+48),2.5,2.5,'Metal')

# 楼梯外侧补真实墙窗；后部留 180 cm 常开侧门接屋顶路线，避免堵住原上楼口。
for lo,hi in [(-1000,-910),(-730,0)]:box((562,(lo+hi)/2,391),(24,hi-lo,78),'Stone',.6)
box((562,-500,671),(24,1000,38),'Stone',.6)
for lo,hi in [(-1000,-910),(-730,-620),(-370,-330),(-80,0)]:
    box((562,(lo+hi)/2,541),(24,hi-lo,222),'Stone',.6)
for lo,hi in [(-620,-370),(-330,-80)]:
    for y in [lo+2,hi-2]:box((550,y,541),(8,4,222),'Metal')
    for z in [432,650]:box((550,(lo+hi)/2,z),(8,hi-lo,4),'Metal')
    box((548,(lo+hi)/2,428),(32,hi-lo+8,4),'Stone')
for y in [-910,-730]:box((562,y,502),(28,7,300),'Metal')
box((562,-820,650),(28,187,7),'Metal')
box((581,-500,679),(10,1018,10),'Wood')
export('HM_StairSideEnvelope',True)
for lo,hi in [(-620,-370),(-330,-80)]:box((562,(lo+hi)/2,541),(2,hi-lo-8,214),'Glass',0)
export('HM_StairSideGlass',True)

# 旧玻璃栏端点逐一补实体端柱、底座；沿边放置，不占通道中央。
for x,y in [(-348,-610),(-348,-10),(-410,-612),(250,-612),(-570,0),(-570,178),(570,0),(570,178),(-370,178),(-190,178)]:
    box((x,y,407),(5,5,110),'Metal',.3);box((x,y,353.5),(12,12,3),'Metal',.3)
export('HM_GalleryGuardEndPosts',True)

# 从 Gallery 侧门跨 22 cm 台阶到外平台，直跑 22 级到中央屋顶。
box((665,-820,362),(230,240,24),'Stone',.7)
for i in range(22):
    top=374+(i+1)*(742-374)/22
    box((670,-645+i*30,top-9),(180,30,18),'Wood',.4)
for x in [585,755]:beam((x,-660,356),(x,0,724),12,20,'Metal')
box((580,75,730),(400,150,24),'Stone',.8)
for y in [-590,-80]:box((735,y,460),(22,22,180),'Metal',.6)
# 屋顶上的支座，不向一层主要通道插入新立柱。
for y in [180,760]:
    box((740,y,552),(24,24,356),'Metal',.8)
    beam((740,y,710),(245,y,710),20,26,'Metal')
    beam((740,y,535),(425,y,698),14,18,'Metal')
export('HM_RooftopStairStructure',True)
for x in [575,765]:
    beam((x,-660,482),(x,0,850),4,4,'Metal')
    for i in [0,4,8,12,16,20,22]:
        y=-660+i*30;z=374+i*(742-374)/22
        box((x,y,z+54),(5,5,108),'Metal',.3)
rail((780,-940),(780,-660),374)
rail((555,-940),(780,-940),374)
rail((780,0),(780,150),742)
rail((380,150),(780,150),742)
export('HM_RooftopStairGuard',True)

# 可走屋顶与独立高跳点：低台在 X=-280，高台在 X=260，落水走廊互不重叠。
box((-10,-480,736),(1080,1000,12),'Wood',.6)
box((260,510,730),(160,980,24),'Metal',1)
box((260,510,743),(160,980,2),'Wood',.5)
for y in range(80,981,45):box((260,y,744.1),(154,.4,.2),'Bronze',.03)
for y in [957,970,983]:box((260,y,744.2),(144,1,.3),'Metal',.1)
export('HM_RooftopDeck',True)
for a,b in [((-535,-975),(525,-975)),((-535,-975),(-535,20)),((525,-975),(525,-90)),((-535,20),(180,20))]:rail(a,b,742)
for x in [180,340]:rail((x,20),(x,940),744)
export('HM_RooftopDeckGuard',True)

# 背面坐凳、收边与低木格栅形成观景停留点；不加新房间和动态灯。
for x in [-360,-100]:
    box((x,-860,766),(220,70,48),'Stone',1)
    box((x,-860,792),(224,74,4),'Wood',.5)
    box((x,-892,816),(224,8,48),'Wood',.5)
box((-235,-715,766),(180,65,48),'Stone',1)
box((-235,-715,792),(184,69,4),'Wood',.5)
export('HM_RooftopLounge',True)
(OUT/'rooftop_route_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('ROOFTOP_ROUTE_SAVED',[(r['mesh'],r['triangles']) for r in manifest])
