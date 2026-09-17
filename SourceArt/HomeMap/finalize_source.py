import bpy, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceArt/HomeMap'
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
tree=bpy.data.objects['SM_HM_IslandTree']
bpy.ops.object.select_all(action='DESELECT');tree.select_set(True);bpy.context.view_layer.objects.active=tree
before=sum(len(p.vertices)-2 for p in tree.data.polygons)
if before>18000:
    dec=tree.modifiers.new('Background tree 18k maximum','DECIMATE');dec.ratio=18000/before;dec.use_collapse_triangulate=True
    bpy.ops.object.modifier_apply(modifier=dec.name)
loc=tree.location.copy();tree.location=(0,0,0)
bpy.ops.export_scene.fbx(filepath=str(OUT/'Exports/SM_HM_IslandTree.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE',path_mode='RELATIVE')
tree.location=loc
report=json.loads((OUT/'tree_report.json').read_text(encoding='utf-8'))
report['intermediate_triangles']=180000;report['export_triangles']=sum(len(p.vertices)-2 for p in tree.data.polygons)
(OUT/'tree_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
records=json.loads((OUT/'architecture.json').read_text(encoding='utf-8'))
# 护栏旋转来自 BFEU Copy Transform。这里不再写入旧的反向 Roll。
if not bpy.data.objects.get('Gallery_Stair_Landing'):
    bpy.ops.mesh.primitive_cube_add(size=1,location=(4.3,6.1,3.4))
    ob=bpy.context.object;ob.name='Gallery_Stair_Landing';ob.dimensions=(1.8,.2,.24)
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    for c in list(ob.users_collection):c.objects.unlink(ob)
    bpy.data.collections['HomeMap_Architecture'].objects.link(ob)
    ob.data.materials.append(bpy.data.materials['Preview_Oak'])
(OUT/'architecture.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('SOURCE_FINALIZED',report)
