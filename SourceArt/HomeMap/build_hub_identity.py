"""Home Hub 的物理标识、任务参考板、收藏柜与 Gallery 书桌；不生成玩法 UI。"""
import bpy,bmesh,math,json,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'SourceArt/HomeMap'
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
assert not bpy.data.objects.get('HM_ExpeditionSign')
parts=[];reports=[]
palette={'Hub_Wood':(.18,.085,.04,1),'Hub_Dark':(.025,.031,.032,1),'Hub_Brass':(.36,.22,.10,1),'Hub_Letter':(.7,.66,.51,1),'Hub_Map':(.085,.13,.125,1),'Hub_Linen':(.42,.39,.33,1)}
mats={}
for name,col in palette.items():
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.diffuse_color=col;mats[name]=m
def p(v):return (v[0]/100,-v[1]/100,v[2]/100)
def box(name,loc,dims,mat='Hub_Wood',bevel=.4):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p(loc));o=bpy.context.object;o.name=name;o.dimensions=[v/100 for v in dims]
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mats[mat])
    if bevel:
        b=o.modifiers.new('Edge highlight','BEVEL');b.width=bevel/100;b.segments=3;bpy.ops.object.modifier_apply(modifier=b.name)
    parts.append(o)
def text(body,loc,size,face='Y',mat='Hub_Letter'):
    bpy.ops.object.text_add();o=bpy.context.object;o.data.body=body;o.data.align_x='CENTER';o.data.size=size/100;o.data.extrude=.0004;o.data.resolution_u=2
    bpy.ops.object.convert(target='MESH')
    for v in o.data.vertices:
        u,w,d=v.co.x*100,v.co.y*100,v.co.z*100
        # 文字水平轴需按观察面确定；UE +X 面向室内时水平右方是 -Y，+Y 面则是 +X。
        co=(loc[0]+d,loc[1]-u,loc[2]+w) if face=='X' else (loc[0]+u,loc[1]+d,loc[2]+w)
        v.co=p(co)
    o.data.materials.append(mats[mat]);parts.append(o)
def line(name,points,mat='Hub_Brass',radius=.12):
    c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.resolution_u=1;c.bevel_depth=radius/100;c.bevel_resolution=1
    s=c.splines.new('POLY');s.points.add(len(points)-1)
    for pt,co in zip(s.points,points):pt.co=(*p(co),1)
    o=bpy.data.objects.new(name,c);bpy.context.collection.objects.link(o);bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH');o=bpy.context.object;o.data.materials.append(mats[mat]);parts.append(o)
def export(name,loc):
    global parts
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;o.data.name='SM_'+name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
    o.data.update();bpy.context.view_layer.update()
    uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
    for poly in o.data.polygons:
        axes=[i for i in range(3) if i!=max(range(3),key=lambda j:abs(poly.normal[j]))]
        for li in poly.loop_indices:
            co=o.data.vertices[o.data.loops[li].vertex_index].co;uv.data[li].uv=(co[axes[0]],co[axes[1]])
    o.location=p(loc);o.bfu_export_type='export_self_only';o.bfu_rotate_to_zero_for_export=True;o.bfu_auto_generate_collision=False
    s=bpy.context.scene;s.bfu_export_static_mesh_file_path=str(OUT/'BFEU_Production/StaticMesh')+os.sep;s.bfu_unreal_import_location='Environment/HomeMap/Architecture/Hub'
    bpy.ops.object.exportforunreal();o.data.calc_loop_triangles()
    reports.append({'name':o.data.name,'location':loc,'triangles':len(o.data.loop_triangles),'materials':[m.name for m in o.data.materials]});parts=[];return o

# 名牌面向室内 +X，位于原门框上方，不改变原 Visual 或交互组件。
box('Inset sign plate',(0,0,0),(4,256,32),'Hub_Dark',1)
for z in [-15,15]:box('Bronze reveal',(2.2,0,z),(1,252,.8),'Hub_Brass',.1)
text('EXPEDITION',(2.4,0,-3),16,'X')
text('FIELD ACCESS',(2.4,0,-11),4,'X')
export('HM_ExpeditionSign',[-1225,-500,299])

# 南墙任务参考板：手绘式线路和分区网格只作为环境陈设，不标注伪造奖励或实时数据。
box('Oak board backing',(0,0,0),(294,7,164),'Hub_Wood',1)
box('Dark board',(0,4,0),(280,1.5,150),'Hub_Map',.4)
for x in [-144,144]:box('Board border',(x,5,0),(1,2,159),'Hub_Brass',.1)
for z in [-79,79]:box('Board border',(0,5,z),(287,2,1),'Hub_Brass',.1)
text('FIELD NOTES',(0,5.1,55),12)
text('ROUTE STUDY / HOME BASE',(0,5.1,-64),4)
for i in range(-5,6):line('Survey grid',[(i*23,5.0,-49),(i*23,5.0,42)],'Hub_Linen',.05)
for j in range(-2,3):line('Survey grid',[(-125,5.0,j*19),(125,5.0,j*19)],'Hub_Linen',.05)
for r in range(4):
    line('Topographic contour',[(44+(27+r*8)*math.cos(i*math.tau/44)+5*math.sin(i*.42),5.2,-8+(17+r*7)*math.sin(i*math.tau/44)) for i in range(45)],'Hub_Linen',.09)
route=[(-105,5.4,-30),(-81,5.4,-25),(-68,5.4,8),(-28,5.4,10),(-10,5.4,28),(28,5.4,31),(49,5.4,18)]
line('Field route',route,'Hub_Brass',.5)
for x,y,z in [route[0],route[3],route[-1]]:
    line('Pinned waypoint',[(x+3*math.cos(i*math.tau/24),y+.1,z+3*math.sin(i*math.tau/24)) for i in range(25)],'Hub_Letter',.4)
text('HOME',(-105,5.5,-42),4);text('N',(117,5.1,29),7)
line('North arrow',[(117,5.2,8),(117,5.2,24),(114,5.2,19),(117,5.2,24),(120,5.2,19)],'Hub_Letter',.25)
export('HM_FieldNotesBoard',[-950,-979,205])

# 柜背遮挡卧室与公共区直视；三个陈列开间复用原有装饰物，低柜提供生活收纳感。
box('Archive back',(0,27,138),(334,4,276),'Hub_Dark')
box('Recessed plinth',(0,0,6),(322,45,12),'Hub_Dark')
box('Lower cabinet',(0,0,46),(330,52,68),'Hub_Wood',.8)
for x in [-110,0,110]:
    box('Cabinet door',(x,-27,45),(108,2,62),'Hub_Wood')
    box('Integrated pull',(x,-28.5,71),(45,1,1),'Hub_Brass',.15)
for z in [90,160,230,277]:box('Display shelf',(0,0,z),(336,58,4),'Hub_Wood')
for x in [-166,-55,55,166]:box('Display upright',(x,0,179),(3,56,198),'Hub_Brass',.3)
for z in [158,228,275]:box('Display light trim',(0,-19,z),(320,1,1),'Hub_Letter',.1)
case=export('HM_ArchiveCabinet',[-1030,-60,0])
o=case.copy();o.name='HM_GalleryArchiveCabinet';o.location=p((-535,-805,353));o.rotation_euler[2]=-math.pi/2;o.scale=(290/336,.55,1);bpy.context.collection.objects.link(o)

# 书桌和底座形成一件家具，保持原 190×62 cm 占地与 78 cm 桌面高度。
box('Solid oak desktop',(0,0,77),(190,62,4),'Hub_Wood',1.4)
box('Dark writing inset',(15,-2,79.1),(98,45,.5),'Hub_Dark',.8)
for x in [-76,76]:
    box('Recessed foot',(x,0,2),(10,50,4),'Hub_Dark',.8)
    box('Tapered upright',(x,0,37),(5,45,68),'Hub_Brass',.6)
box('Slim drawer',(-53,0,68),(59,51,12),'Hub_Wood',.7)
box('Drawer pull',(-53,27,70),(33,1.5,1),'Hub_Brass',.3)
box('Cable trough',(0,-24,70),(140,8,5),'Hub_Dark',.5)
export('HM_GalleryWritingDesk',[260,-960,353])
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
(OUT/'hub_identity_report.json').write_text(json.dumps(reports,indent=2),encoding='utf-8')
print('HUB_IDENTITY',reports)
