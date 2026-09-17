import bpy,math,os,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'SourceArt/HomeMap'
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
parts=[];mat=bpy.data.materials['Console_Obsidian']
def piece(name,loc,dims):
    bpy.ops.mesh.primitive_cube_add(size=1,location=(loc[0]/100,-loc[1]/100,loc[2]/100))
    o=bpy.context.object;o.name=name;o.dimensions=[v/100 for v in dims]
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    o.data.materials.append(mat)
    b=o.modifiers.new('Soft metal edges','BEVEL');b.width=.001;b.segments=2;bpy.ops.object.modifier_apply(modifier=b.name)
    parts.append(o)
# 常开折叠玻璃的四条齐平轨道，净开口到第一踏步之间没有凸起门槛。
for i in range(4):
    piece('Floor track '+str(i),(350,-i*8,.25),(420,1,.5))
    piece('Head track '+str(i),(350,-i*8,326),(420,2,3))
    for x in [450,560]:piece('Stacked leaf stile',(x,-i*8,162.5),(2.2,5,325))
    for z in [2,323]:piece('Stacked leaf rail',(505,-i*8,z),(110,5,3))
piece('Door pull',(446,-25,112),(2,2,44))
bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object
o.name='HM_LivingSlidingDoorDetail';o.data.name='SM_HM_LivingSlidingDoorDetail'
bpy.context.scene.cursor.location=(3.5,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
o.bfu_export_type='export_self_only';o.bfu_rotate_to_zero_for_export=True;o.bfu_auto_generate_collision=False
scene=bpy.context.scene;scene.bfu_unreal_import_location='Environment/HomeMap/Architecture/Hub'
scene.bfu_export_static_mesh_file_path=str(OUT/'BFEU_Production/StaticMesh')+os.sep
bpy.ops.object.exportforunreal()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('DOOR_DETAIL_EXPORTED')
