"""康体区地面、镜框及墙面收口；复用现有帘模型。"""
import bpy,bmesh,json,os
from pathlib import Path
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
assert not bpy.data.objects.get('HM_GymFinish')
parts=[]
def box(loc,dims,material,bevel=.35):
    bpy.ops.mesh.primitive_cube_add(size=1,location=(loc[0]/100,-loc[1]/100,loc[2]/100));o=bpy.context.object;o.dimensions=[v/100 for v in dims]
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(bpy.data.materials['Wellness_'+material])
    b=o.modifiers.new('Edge','BEVEL');b.width=bevel/100;b.segments=2;bpy.ops.object.modifier_apply(modifier=b.name);parts.append(o)
# 嵌入式运动地面，边缘 1.5 cm 高，不挡第三人称角色进出。
box((1110,350,.65),(550,618,1.3),'Rubber',.25)
for y in [41,659]:box((1110,y,.8),(552,1,1.6),'Bronze',.15)
box((834.5,350,.8),(1,618,1.6),'Bronze',.15)
# 保留现有镜面 Actor；这里只做框、底部木饰面和隐藏灯槽。
for y in [63,657]:box((1378,y,170),(10,4,202),'Wood',.6)
for z in [69,271]:box((1378,360,z),(10,598,4),'Wood',.6)
box((1380,360,32),(6,594,58),'Wood',.6)
for y in range(82,641,20):box((1375,y,32),(5,2,54),'Bronze',.25)
box((1370,360,275),(22,604,4),'Wood',.6)
bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name='HM_GymFinish';o.data.name='SM_HM_GymFinish'
bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
for f in o.data.polygons:
    axes=[i for i in range(3) if i!=max(range(3),key=lambda j:abs(f.normal[j]))]
    for li in f.loop_indices:
        co=o.data.vertices[o.data.loops[li].vertex_index].co;uv.data[li].uv=(co[axes[0]],co[axes[1]])
o.data.update();bpy.context.view_layer.update();o.bfu_export_type='export_self_only';o.bfu_rotate_to_zero_for_export=True;o.bfu_auto_generate_collision=False
s=bpy.context.scene;s.bfu_export_static_mesh_file_path=str(OUT/'BFEU_Production/StaticMesh')+os.sep;s.bfu_unreal_import_location='Environment/HomeMap/Architecture/Hub';bpy.ops.object.exportforunreal()
o.data.calc_loop_triangles();palette={'Rubber':'Hub/M_Rubber','Bronze':'Hub/M_Bronze','Wood':'M_Wall_Wood'}
rows=json.loads((OUT/'wellness_manifest.json').read_text(encoding='utf-8'))
rows.append({'mesh':'SM_HM_GymFinish','triangles':len(o.data.loop_triangles),'materials':['/Game/Environment/HomeMap/Materials/'+palette[m.name.removeprefix('Wellness_')] for m in o.data.materials],'nanite':False,'collision':True,'instances':[{'label':'HM_GymFinish','location_cm':[0,0,0],'rotation':[0,0,0],'scale':[1,1,1]}]})
(OUT/'wellness_manifest.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
for n,y in [('HM_YogaCurtain_Entry',805),('HM_YogaCurtain_End',1290)]:
    o=bpy.data.objects['HM_BedroomCurtain'].copy();o.name=n;o.location=(7.31,-y/100,0);bpy.context.collection.objects.link(o)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'));print('GYM_FINISH_EXPORTED')
