"""现代住宅康体套件；保留现有平面，厘米设计、米制建模、BFEU 本地网格导出。"""
import bpy, bmesh, math, json, os
from mathutils import Vector
from pathlib import Path
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
assert not bpy.data.objects.get('HM_TreadmillSuite'), '已有正式套件，禁止重复建模'
parts=[];manifest=[]
palette={'Rubber':('Hub/M_Rubber',(.025,.03,.032,1)), 'Metal':('M_Metall_Black',(.045,.05,.055,1)), 'Silver':('M_Metall',(.3,.32,.34,1)), 'Wood':('M_Wall_Wood',(.2,.1,.05,1)), 'Linen':('Hub/MI_Suite_Linen',(.5,.46,.39,1)), 'Bronze':('Hub/M_Bronze',(.3,.2,.1,1)), 'Screen':('M_Plastic_Black',(.015,.025,.03,1))}
mats={}
for n,(path,col) in palette.items():
    m=bpy.data.materials.get('Wellness_'+n) or bpy.data.materials.new('Wellness_'+n);m.diffuse_color=col;mats[n]=m
def p(v):return (v[0]/100,-v[1]/100,v[2]/100)
def finish(o,mat):o.data.materials.append(mats[mat]);parts.append(o);return o
def box(loc,dims,mat,bevel=.5):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p(loc));o=bpy.context.object;o.dimensions=[d/100 for d in dims]
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        b=o.modifiers.new('Manufactured edge','BEVEL');b.width=bevel/100;b.segments=3;bpy.ops.object.modifier_apply(modifier=b.name)
    return finish(o,mat)
def cylinder(a,b,r,mat,sides=16):
    av,bv=Vector(p(a)),Vector(p(b));axis=bv-av
    bpy.ops.mesh.primitive_cylinder_add(vertices=sides,radius=r/100,depth=axis.length,location=(av+bv)/2)
    o=bpy.context.object;o.rotation_euler=axis.to_track_quat('Z','Y').to_euler()
    for f in o.data.polygons:f.use_smooth=len(f.vertices)==4
    return finish(o,mat)
def tube(points,r,mat):
    for a,b in zip(points,points[1:]):cylinder(a,b,r,mat,12)
def export(name,loc,instances=None,collision=True):
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
    o.location=p(loc);o.data.update();bpy.context.view_layer.update()
    o.bfu_export_type='export_self_only';o.bfu_rotate_to_zero_for_export=True;o.bfu_auto_generate_collision=False
    s=bpy.context.scene;s.bfu_export_static_mesh_file_path=str(OUT/'BFEU_Production/StaticMesh')+os.sep;s.bfu_unreal_import_location='Environment/HomeMap/Architecture/Hub'
    bpy.ops.object.exportforunreal();o.data.calc_loop_triangles()
    objects=[{'label':name,'location_cm':loc,'rotation':[0,0,0],'scale':[1,1,1]}]
    for label,pos in instances or []:
        dup=o.copy();dup.name=label;dup.location=p(pos);bpy.context.collection.objects.link(dup)
        objects.append({'label':label,'location_cm':pos,'rotation':[0,0,0],'scale':[1,1,1]})
    manifest.append({'mesh':'SM_'+name,'triangles':len(o.data.loop_triangles),'materials':['/Game/Environment/HomeMap/Materials/'+palette[m.name.removeprefix('Wellness_')][0] for m in o.data.materials],'nanite':False,'collision':collision,'instances':objects})
    parts=[]

# 跑步机跑带朝外墙镜面；圆管倾斜扶架、滚筒和独立踏边代替方块占位。
box((0,0,11),(218,96,16),'Metal',6)
box((-9,0,20),(178,60,3),'Rubber',1.8)
for y in [-39,39]:
    box((-6,y,22),(182,14,6),'Silver',2)
    for x in [-72,-44,-16,12,40,68]:box((x,y,25.1),(1,11,.25),'Rubber',.1)
    tube([(76,y,20),(83,y,27),(91,y,96),(82,y,111),(39,y,112)],2.8,'Metal')
    cylinder((39,y,112),(62,y,112),3.4,'Rubber')
box((90,0,25),(29,87,18),'Metal',5)
for x in [-98,68]:cylinder((x,-30,18),(x,30,18),5,'Metal',20)
console=box((81,0,123),(24,85,10),'Metal',3);console.rotation_euler[1]=math.radians(-18)
screen=box((70,0,132),(1.5,52,19),'Screen',1);screen.rotation_euler[1]=math.radians(-18)
for y in [-35,35]:
    cylinder((81,y,129),(81,y,131),4,'Rubber',20)
box((65,0,122),(2,11,3),'Bronze',.5)
for x in [-90,92]:
    for y in [-36,36]:box((x,y,3),(18,13,6),'Rubber',2)
export('HM_TreadmillSuite',[1200,235,0],[('HM_TreadmillSuite_Second',[1200,495,0])])

# 双层哑铃架，六角橡胶包胶配重；实际尺寸与握把比例保持一致。
for x in [-132,132]:
    tube([(x,-29,5),(x,-18,75),(x,20,80)],3,'Metal')
    box((x,0,3),(17,64,6),'Rubber',2)
for z,y in [(42,-13),(79,9)]:
    box((0,y,z),(290,30,5),'Metal',1.2)
    for x in [-117,-70,-23,24,71,118]:
        rad=8+(x+117)/235*2.5
        cylinder((x,y-17,z+rad+3),(x,y+17,z+rad+3),1.4,'Silver',12)
        for yy in [y-14,y+14]:cylinder((x,yy-4,z+rad+3),(x,yy+4,z+rad+3),rad,'Rubber',6)
export('HM_DumbbellRack',[1170,65,0])

# 调节式卧推凳占地约 150×52 cm；收在两台机器后的实墙旁。
box((0,0,29),(128,10,8),'Metal',2)
for x in [-52,53]:
    tube([(x,-24,4),(x,0,29),(x,24,4)],3,'Metal')
    for y in [-23,23]:box((x,y,3),(16,10,6),'Rubber',1.5)
box((-44,0,47),(46,44,12),'Rubber',5)
box((29,0,49),(95,44,12),'Rubber',5)
cylinder((-12,-23,40),(-12,23,40),3,'Silver',16)
box((21,0,35),(42,5,7),'Bronze',1)
export('HM_TrainingBench',[1175,625,0])

# 瑜伽垫、圆柱抱枕与软木砖组合，保留原两组练习位。
box((0,0,.7),(192,72,1.4),'Rubber',.65)
for y in [-34,34]:box((0,y,1.43),(182,.35,.08),'Bronze',.02)
box((-73,49,5),(22,14,10),'Wood',1.1)
cylinder((59,-22,11),(59,22,11),9.5,'Linen',24)
for y in [-22.5,22.5]:cylinder((59,y-.3,11),(59,y+.3,11),7.8,'Linen',24)
export('HM_YogaPracticeSet',[1090,950,0],[('HM_YogaPracticeSet_Second',[1090,1240,0])],False)

# 瑜伽侧墙窄收纳：开放格、木条背板、毛巾和卷垫；面向庭院 -X。
box((9,0,135),(4,222,270),'Metal')
for y in range(-105,106,10):box((5,y,138),(5,4,260),'Wood',.4)
box((-15,0,39),(43,214,68),'Wood',1)
for y in [-72,0,72]:
    box((-37,y,39),(2,69,63),'Wood',.5)
    box((-38.5,y,67),(1,28,1),'Bronze',.2)
for z in [79,155,226]:box((-15,0,z),(50,224,4),'Wood',.7)
for y in [-109,0,109]:box((-15,y,158),(47,2.5,158),'Bronze',.4)
for y in [-69,-36,42,75]:
    for z in [85,93]:box((-13,y,z),(30,28,7),'Linen',2)
for y in [-70,-38,50,80]:cylinder((-13,y,158),(-13,y,217),10,'Rubber',20)
export('HM_WellnessStorage',[1367,1120,0])
(OUT/'wellness_manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('WELLNESS_EXPORTED',[(r['mesh'],r['triangles']) for r in manifest])
