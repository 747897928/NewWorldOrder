"""庭院躺椅：薄木框、弧面织物和靠枕；一份网格用于泳池两侧。"""
import bpy,math,os,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'SourceArt/HomeMap'
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
parts=[]
wood=bpy.data.materials['Console_Wood'];metal=bpy.data.materials['Console_Obsidian']
fabric=bpy.data.materials.get('Lounger_Linen') or bpy.data.materials.new('Lounger_Linen');fabric.diffuse_color=(.67,.62,.51,1)
def box(name,loc,dims,mat,bevel=.008):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=dims
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mat)
    b=o.modifiers.new('Upholstery edge' if mat==fabric else 'Rounded frame edge','BEVEL');b.width=bevel;b.segments=3;bpy.ops.object.modifier_apply(modifier=b.name)
    parts.append(o);return o
# 底面中心为统一 pivot，长轴沿 Y；宽 72 cm、长 200 cm。
for x in [-.31,.31]:
    box('Teak side rail',(x,0,.29),(.055,1.90,.075),wood)
    for y in [-.73,.60]:box('Recessed foot',(x,y,.14),(.065,.12,.28),metal,.014)
for y in [-.79,-.3,.25,.72]:box('Teak cross rail',(0,y,.29),(.64,.05,.055),wood)
box('Foot cushion',(0,-.43,.385),(.66,1.03,.12),fabric,.045)
seat=box('Contoured seat',(0,.12,.402),(.66,.25,.13),fabric,.045)
back=box('Reclined back cushion',(0,.58,.625),(.66,.86,.12),fabric,.045);back.rotation_euler[0]=math.radians(32)
for x in [-.35,.35]:
    side=box('Back support',(x,.59,.59),(.045,.87,.06),wood);side.rotation_euler[0]=math.radians(32)
pillow=box('Head pillow',(0,.78,.86),(.48,.23,.11),fabric,.045);pillow.rotation_euler[0]=math.radians(32)
box('Foot end frame',(0,-.98,.32),(.72,.055,.07),wood)
bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name='HM_PoolLounger';o.data.name='SM_HM_PoolLounger'
bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
for p in o.data.polygons:
    axis=max(range(3),key=lambda i:abs(p.normal[i]));axes=[i for i in range(3) if i!=axis]
    for li in p.loop_indices:
        co=o.data.vertices[o.data.loops[li].vertex_index].co;uv.data[li].uv=(co[axes[0]],co[axes[1]])
o.location=(6.17,-8.8,0)
o.bfu_export_type='export_self_only';o.bfu_rotate_to_zero_for_export=True;o.bfu_auto_generate_collision=True
s=bpy.context.scene;s.bfu_unreal_import_location='Environment/HomeMap/Furniture/Hub';s.bfu_export_static_mesh_file_path=str(OUT/'BFEU_Production/StaticMesh')+os.sep
bpy.ops.object.exportforunreal();o.data.calc_loop_triangles()
(OUT/'lounger_report.json').write_text(json.dumps({'triangles':len(o.data.loop_triangles),'width_cm':72,'length_cm':200,'new_textures':0}),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('LOUNGER',len(o.data.loop_triangles))
