"""衣帽收纳与出征准备区的固定木作。复用现有材质，避开交互家具。"""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforeCanopyAndJoinery.blend'
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
palette={'Wood':('M_Wall_Wood',(.22,.13,.075,1)), 'Metal':('M_Metall_Black',(.025,.028,.032,1)), 'Stone':('Hub/M_Limestone',(.42,.39,.33,1)), 'Bronze':('Hub/M_Bronze',(.25,.17,.08,1)), 'Glow':('Hub/M_Warm_Light',(1,.7,.38,1))}
palette['Linen']=('Hub/MI_Suite_Linen',(.5,.47,.4,1))
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

# 西侧原柜体占地内分成抽屉、挂衣和开放格；面向 +X，不向过道扩张。
box((-1381,810,138),(6,310,276),'Wood')
for y in [655,810,965]:box((-1342,y,138),(82,4,276),'Wood')
for z in [10,276]:box((-1342,810,z),(82,310,4),'Wood')
box((-1342,810,6),(68,295,12),'Metal')
for y in [732,888]:
    box((-1342,y,47),(80,148,70),'Wood')
    for z in [28,51,74]:
        box((-1300.5,y,z),(2,143,21),'Wood')
        box((-1298.8,y,z+5),(1.5,48,1.4),'Bronze',.2)
    box((-1342,y,220),(80,148,4),'Wood')
    box((-1342,y,204),(3,141,3),'Bronze',.4)
    box((-1308,y,217.5),(1.2,126,.6),'Glow',.1)
# 右开间上部再分两层收纳，用折叠织物表现尺度，保留左开间挂衣空间。
box((-1342,888,151),(80,148,3),'Wood')
for y in [851,925]:
    for z in [159,168,177]:box((-1329,y,z),(44,48,7),'Linen',2)
# 左开间保留五只实际衣架，衣物为低密度有厚度的展示布面，不接角色换装资产。
from mathutils import Vector

def rod(a,b,r,material):
    av,bv=Vector(p(a)),Vector(p(b));d=bv-av
    bpy.ops.mesh.primitive_cylinder_add(vertices=8,radius=r/100,depth=d.length,location=(av+bv)/2)
    o=bpy.context.object;o.rotation_euler=d.to_track_quat('Z','Y').to_euler();o.data.materials.append(mats[material]);parts.append(o)
for i,y in enumerate([676,702,728,754,780]):
    rod((-1342,y,204),(-1342,y,194),.6,'Bronze')
    rod((-1342,y,194),(-1367,y,180),.65,'Wood')
    rod((-1367,y,180),(-1317,y,180),.65,'Wood')
    rod((-1317,y,180),(-1342,y,194),.65,'Wood')
    # 前后双面带纵向布褶与松弛下摆，避免衣物像直立木板；全部使用织物材质。
    verts=[];faces=[]
    for back in [0,1]:
        for row in range(9):
            t=row/8
            for col in range(9):
                u=(col-4)/4;width=23+2*t
                z=183-64*t-(1-t)*7*max(0,1-abs(u)*2.5)+t*1.2*math.cos(u*8+i)
                fold=1.5*math.cos(u*12+i)*(.3+.7*t)
                verts.append(((-1342+u*width)/100,-(y+fold+(-.5 if back else .5))/100,z/100))
        offset=back*81
        for row in range(8):
            for col in range(8):
                a=offset+row*9+col;f=(a,a+1,a+10,a+9);faces.append(f if not back else tuple(reversed(f)))
    for row in range(8):
        for col in [0,8]:
            a=row*9+col;faces.append((a,a+9,a+90,a+81))
    for col in range(8):
        for row in [0,8]:
            a=row*9+col;faces.append((a,a+1,a+82,a+81))
    mesh=bpy.data.meshes.new('Display linen');mesh.from_pydata(verts,[],faces);mesh.materials.append(mats['Linen']);o=bpy.data.objects.new('Display linen',mesh);bpy.context.collection.objects.link(o);parts.append(o)
export('HM_DressingJoinery',True)
# 出征桌保留原桌面和足部空间，只替换两只方块柜并增加墙面整合。
for x in [-1060,-840]:
    box((x,-890,44),(70,65,80),'Wood',.8)
    box((x,-890,5),(60,53,10),'Metal')
    for z in [22,48,74]:
        box((x,-856, z),(66,2,24),'Wood')
        box((x,-854.2,z+6),(38,1.4,1.4),'Bronze',.2)
box((-950,-987,158),(320,6,300),'Wood',.6)
for x in [-1120,-780]:box((x,-981,159),(8,7,298),'Metal',.4)
box((-950,-875,284),(350,40,5),'Wood')
box((-950,-853,280),(320,1,.8),'Glow',.1)
export('HM_ExpeditionJoinery',True)
# 卧室顶面与阅读墙补完整收边，不改变床前通道。
box((-1050,300,324),(670,560,4),'Wood',.5)
for x in [-1367,-733]:box((x,300,321),(5,552,4),'Metal')
for y in [24,576]:box((-1050,y,321),(634,5,4),'Metal')
for x in [-1358,-742]:box((x,300,323),(1,525,1),'Glow',.1)
box((-1130,582,170),(150,5,112),'Metal',.7)
box((-1130,578.5,170),(142,3,104),'Stone',.8)
for i in range(7):box((-1184+i*18,576,166+i*4),(5,2,48),'Bronze',.7)
export('HM_PrivateCeilingFinish',False)
(OUT/'joinery_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('JOINERY_SAVED',[(r['mesh'],r['triangles']) for r in manifest])
