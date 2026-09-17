"""中央挑高与 Gallery 的增量美术套件；已有几何保持，只导出本次新对象。"""
import bpy,bmesh,math,json,os
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
assert not bpy.data.objects.get('HM_PavilionChandelier'), '已建模，后续应局部编辑而非重复运行'
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforePavilionDetails.blend'
checkpoint.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
palette={'Wood':('M_Wall_Wood',(.22,.12,.065,1)), 'Bronze':('Hub/M_Bronze',(.3,.2,.1,1)), 'Stone':('Hub/M_Limestone',(.5,.46,.4,1)), 'Dark':('M_Metall_Black',(.03,.03,.03,1)), 'Glow':('Hub/M_Warm_Light',(1,.8,.55,1))}
mats={}
for n,(path,color) in palette.items():
    m=bpy.data.materials.get('Pavilion_'+n) or bpy.data.materials.new('Pavilion_'+n);m.diffuse_color=color;mats[n]=m
parts=[];manifest=[]
def p(v):return (v[0]/100,-v[1]/100,v[2]/100)
def finish(o,mat):o.data.materials.append(mats[mat]);parts.append(o);return o
def box(loc,dims,mat,bevel=.3):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p(loc));o=bpy.context.object;o.dimensions=[v/100 for v in dims]
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        b=o.modifiers.new('Edge','BEVEL');b.width=bevel/100;b.segments=2;bpy.ops.object.modifier_apply(modifier=b.name)
    return finish(o,mat)
def cylinder(a,b,r,mat,sides=12):
    av,bv=Vector(p(a)),Vector(p(b));d=bv-av
    bpy.ops.mesh.primitive_cylinder_add(vertices=sides,radius=r/100,depth=d.length,location=(av+bv)/2)
    o=bpy.context.object;o.rotation_euler=d.to_track_quat('Z','Y').to_euler()
    for f in o.data.polygons:f.use_smooth=len(f.vertices)==4
    return finish(o,mat)
def band(cx,cy,z,rx,ry,width,height,mat):
    verts=[];faces=[];n=80
    for dz,dr in [(-height/2,0),(height/2,0),(height/2,-width),(-height/2,-width)]:
        for i in range(n):
            t=i*math.tau/n;verts.append(p((cx+(rx+dr)*math.cos(t),cy+(ry+dr)*math.sin(t),z+dz)))
    for j in range(4):
        for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,((j+1)%4)*n+(i+1)%n,((j+1)%4)*n+i))
    me=bpy.data.meshes.new('Elliptical band');me.from_pydata(verts,[],faces);me.update()
    o=bpy.data.objects.new('Elliptical band',me);bpy.context.collection.objects.link(o)
    for f in me.polygons:f.use_smooth=f.index//n in [0,2]
    finish(o,mat)
def export(name,collision=False):
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
            co=o.data.vertices[o.data.loops[li].vertex_index].co;uv.data[li].uv=(co[axes[0]],co[axes[1]])
    o.data.update();bpy.context.view_layer.update();o.bfu_export_type='export_self_only';o.bfu_rotate_to_zero_for_export=True;o.bfu_auto_generate_collision=False
    s=bpy.context.scene;s.bfu_export_static_mesh_file_path=str(OUT/'BFEU_Production/StaticMesh')+os.sep;s.bfu_unreal_import_location='Environment/HomeMap/Architecture/Hub'
    bpy.ops.object.exportforunreal();o.data.calc_loop_triangles()
    manifest.append({'mesh':'SM_'+name,'triangles':len(o.data.loop_triangles),'materials':['/Game/Environment/HomeMap/Materials/'+palette[m.name.removeprefix('Pavilion_')][0] for m in o.data.materials],'nanite':False,'collision':collision,'instances':[{'label':name,'location_cm':[0,0,0],'rotation':[0,0,0],'scale':[1,1,1]}]})
    parts=[]
# 吊灯全部处于挑空区，最低 4.35 m，不越过西侧 Gallery 护栏或楼梯。
box((-80,-330,677),(230,110,4),'Dark',1)
for cx,cy,z,rx,ry in [(-100,-340,438,106,54),(-48,-305,505,86,61),(-93,-364,570,69,42)]:
    band(cx,cy,z,rx,ry,2.3,5,'Bronze')
    band(cx,cy,z-2.8,rx-.35,ry-.35,1.6,.75,'Glow')
    for t in [math.pi/6,math.pi*5/6,math.pi*1.5]:
        x,y=cx+(rx-1)*math.cos(t),cy+(ry-1)*math.sin(t)
        cylinder((x,y,z+2.5),(x,y,675),.19,'Dark',8)
export('HM_PavilionChandelier')
# 后墙局部实面形成 Gallery 背景，保留两侧和书桌之间的玻璃采光。
box((-140,-986,516),(356,6,324),'Wood',.6)
box((-140,-981.8,516),(292,3,306),'Stone',.4)
for x in [-309,29]:
    for dx in [-8,0,8]:box((x+dx,-981,516),(3,5,315),'Wood',.2)
box((-140,-978.8,554),(191,2,149),'Dark',.4)
box((-140,-977.4,554),(184,1,142),'Stone',.3)
# 抽象拱线浮雕直接使用几何，不引入大贴图或文字资源。
for j in range(5):
    rx,rz=28+j*9,34+j*7
    points=[(-159-rx,-975.5,510)]+[(-159+rx*math.cos(t),-975.5,547+rz*math.sin(t)) for t in [math.pi-i*math.pi/36 for i in range(37)]]+[(-159+rx,-975.5,510)]
    for a,b in zip(points,points[1:]):cylinder(a,b,.8,'Bronze',8)
box((-99,-975.4,512),(66,1.8,3),'Wood',.3)
box((285,-987,515),(168,5,320),'Wood',.4)
for z in [472,580]:box((285,-983.5,z),(162,1,1.2),'Bronze',.15)
export('HM_GalleryBackdrop')
# 前檐阴影缝与窄木格栅：保持原结构，避免把侧翼抬高。
box((0,33,681),(1150,18,14),'Dark',.4)
box((0,44,686),(1190,5,8),'Wood',.4)
for side in [-1,1]:
    for x in [488,501,514,527]:box((side*x,-8,518),(5,12,316),'Wood',.35)
export('HM_PavilionEaveDetail')
(OUT/'pavilion_manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('PAVILION_EXPORTED',[(r['mesh'],r['triangles']) for r in manifest])
