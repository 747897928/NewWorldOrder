"""主卧床头、窗帘和卫浴细化。厘米设计转 Blender 米制，BFEU 仅导出新套件。"""
import bpy,math,os,json,bmesh
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'SourceArt/HomeMap'
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
assert not bpy.data.objects.get('HM_BedroomHeadwall'),'已有本轮套件，先检查再局部编辑，禁止盲目重建。'
palette={'Suite_Wood':(.22,.105,.048,1),'Suite_Fabric':(.45,.40,.32,1),'Suite_Brass':(.36,.22,.10,1),'Suite_Glow':(1,.7,.4,1),'Suite_Stone':(.27,.26,.23,1),'Suite_Ceramic':(.7,.68,.63,1),'Suite_Dark':(.025,.025,.022,1)}
mats={}
for name,color in palette.items():
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.diffuse_color=color;mats[name]=m
parts=[];reports=[]
def point(p):return (p[0]/100,-p[1]/100,p[2]/100)
def box(name,loc,dims,mat,bevel=.4):
    bpy.ops.mesh.primitive_cube_add(size=1,location=point(loc));o=bpy.context.object;o.name=name;o.dimensions=[d/100 for d in dims]
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mats[mat])
    if bevel:
        b=o.modifiers.new('Edge detail','BEVEL');b.width=bevel/100;b.segments=3;bpy.ops.object.modifier_apply(modifier=b.name)
    parts.append(o);return o
def tube(name,points,r,mat):
    c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.resolution_u=2;c.bevel_depth=r/100;c.bevel_resolution=2
    s=c.splines.new('POLY');s.points.add(len(points)-1)
    for p,co in zip(s.points,points):p.co=(*point(co),1)
    o=bpy.data.objects.new(name,c);bpy.context.collection.objects.link(o);bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    bpy.ops.object.convert(target='MESH');o=bpy.context.object;o.data.materials.append(mats[mat]);parts.append(o)
def export(name,loc):
    global parts
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;o.data.name='SM_'+name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
    uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
    for p in o.data.polygons:
        axes=[i for i in range(3) if i!=max(range(3),key=lambda j:abs(p.normal[j]))]
        for li in p.loop_indices:
            co=o.data.vertices[o.data.loops[li].vertex_index].co;uv.data[li].uv=(co[axes[0]],co[axes[1]])
    o.location=point(loc);o.bfu_export_type='export_self_only';o.bfu_rotate_to_zero_for_export=True;o.bfu_auto_generate_collision=False
    s=bpy.context.scene;s.bfu_export_static_mesh_file_path=str(OUT/'BFEU_Production/StaticMesh')+os.sep;s.bfu_unreal_import_location='Environment/HomeMap/Architecture/Hub'
    bpy.ops.object.exportforunreal();o.data.calc_loop_triangles()
    reports.append({'name':'SM_'+name,'location_cm':loc,'triangles':len(o.data.loop_triangles),'materials':[m.name for m in o.data.materials]})
    parts=[];return o

# 靠西侧外墙，深度压在床头后方，中央织物与侧边竖向木条形成层次。
box('Shadow backing',(-2,0,157),(7,550,314),'Suite_Dark')
box('Upholstered head panel',(3,0,120),(7,338,190),'Suite_Fabric',2.2)
box('Upper oak field',(1,0,262),(5,340,90),'Suite_Wood')
for side in [-1,1]:
    for i in range(16):box('Oak batten',(3,side*(181+i*5.7),157),(6,3.4,307),'Suite_Wood',.3)
    box('Brass seam',(7,side*173,157),(1.0,1.2,305),'Suite_Brass',.1)
box('Concealed warm reveal',(6,0,308),(2,508,1),'Suite_Glow',.1)
box('Fine oak cornice',(7,0,313),(12,550,4),'Suite_Wood')
export('HM_BedroomHeadwall',[-1373,300,0])

# 收拢帘只占 70 cm 宽，不封堵庭院开口；几何褶皱无需透明贴图。
verts=[];faces=[];nx=48;nz=12
for j in range(nz+1):
    z=3+j*301/nz
    for i in range(nx+1):
        y=-35+i*70/nx;x=4.8*math.cos(i/nx*math.tau*6)+.6*math.sin(j/nz*math.pi)
        verts.append(point((x,y,z+.35*math.sin(i/nx*math.tau*6))))
for j in range(nz):
    for i in range(nx):
        k=j*(nx+1)+i;faces.append((k,k+1,k+nx+2,k+nx+1))
me=bpy.data.meshes.new('Pleated linen');me.from_pydata(verts,[],faces);me.update()
o=bpy.data.objects.new('Pleated linen',me);bpy.context.collection.objects.link(o);o.data.materials.append(mats['Suite_Fabric'])
for p in me.polygons:p.use_smooth=True
bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
mod=o.modifiers.new('Fabric thickness','SOLIDIFY');mod.thickness=.0015;bpy.ops.object.modifier_apply(modifier=mod.name);parts.append(o)
box('Curtain rail',(0,0,310),(4,79,3),'Suite_Dark')
curtain=export('HM_BedroomCurtain',[-731,65,0])
o=curtain.copy();o.name='HM_BedroomCurtain_Return';o.location=point((-731,535,0));bpy.context.collection.objects.link(o)

# 悬浮双台盆：真正的盆腔、薄石材台面、细圆管龙头，不用实心方块代替。
box('Floating carcass',(0,0,54),(270,70,58),'Suite_Wood',.8)
for x in [-101.5,-34,34,101.5]:
    box('Drawer front',(x,-35.5,55),(65.5,2.4,52),'Suite_Wood',.45)
    box('Recessed bronze pull',(x,-37.1,74),(40,.9,1.2),'Suite_Brass',.2)
box('Stone counter',(0,0,87),(280,80,6),'Suite_Stone',.8)
box('Undercabinet light',(0,-29,25),(242,1,1),'Suite_Glow',.1)
for cx in [-65,65]:
    rings=[(25,17,90),(29,20,100),(28.5,19.5,104),(26.5,17.5,104),(24.7,15.7,98),(16,9,93.2),(2.2,2.2,93)]
    vertices=[];faces=[];n=64
    for rx,ry,z in rings:
        for i in range(n):
            t=i*math.tau/n;vertices.append(point((cx+rx*math.cos(t),-3+ry*math.sin(t),z)))
    for j in range(len(rings)-1):
        for i in range(n):
            a=j*n+i;b=j*n+(i+1)%n;faces.append((a,b,b+n,a+n))
    faces.append(tuple(reversed(range(n))))
    mesh=bpy.data.meshes.new('Ceramic basin');mesh.from_pydata(vertices,[],faces);mesh.update()
    ob=bpy.data.objects.new('Ceramic basin',mesh);bpy.context.collection.objects.link(ob);ob.data.materials.append(mats['Suite_Ceramic'])
    for p in mesh.polygons:p.use_smooth=True
    parts.append(ob)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24,radius=.022,depth=.002,location=point((cx,-3,93)))
    ob=bpy.context.object;ob.data.materials.append(mats['Suite_Brass']);parts.append(ob)
    points=[(cx,28,90),(cx,28,117)]+[(cx,23+5*math.cos(t),117+5*math.sin(t)) for t in [i*math.pi/2/8 for i in range(1,9)]]+[(cx,1,122),(cx,-3,118)]
    tube('Swan neck mixer',points,1.1,'Suite_Brass')
    box('Mixer lever',(cx+4,28,99),(7,1.5,1.1),'Suite_Brass',.3)
# 镜面仍复用已有独立 Actor；这里只增加外围细框与暖色背光边。
for x in [-133,133]:
    box('Mirror bronze border',(x,95,180),(1.8,3,131),'Suite_Brass',.2)
    box('Mirror warm reveal',(x*1.012,96,180),(1,1,125),'Suite_Glow',.1)
for z in [115,245]:box('Mirror bronze border',(0,95,z),(268,3,1.8),'Suite_Brass',.2)
box('Spa floor',(180,-80,.6),(650,380,1.2),'Suite_Stone',.1)
box('Spa north stone wall',(180,104,155),(650,2,308),'Suite_Stone',.1)
export('HM_BathVanitySuite',[-1240,1280,0])
for name in ['Bath_Vanity','Bath_Counter']:
    old=bpy.data.objects.get(name)
    if old:bpy.data.objects.remove(old,do_unlink=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
(OUT/'private_suite_report.json').write_text(json.dumps(reports,ensure_ascii=False,indent=2),encoding='utf-8')
print('PRIVATE_SUITE',reports)
