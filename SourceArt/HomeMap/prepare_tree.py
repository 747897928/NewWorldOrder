import bpy, json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceArt/HomeMap'
master=OUT/'HomeMap_Master.blend'
assert Path(bpy.data.filepath).resolve()==master.resolve()
bpy.ops.wm.save_as_mainfile(filepath=str(master))
if bpy.data.collections.get('Landscape_Source'):
    raise RuntimeError('树木源集合已存在，禁止重复导入')
before=set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=str(OUT/'PolyHaven/island_tree_01/island_tree_01_2k.gltf'))
objects=[o for o in bpy.data.objects if o not in before and o.type=='MESH']
print('SOURCE_OBJECTS',[(o.name,len(o.data.polygons),tuple(o.dimensions)) for o in objects])
# 仅选同一棵树的网格，不复制或重做现有室内家具。
coll=bpy.data.collections.new('Landscape_Source');bpy.context.scene.collection.children.link(coll)
for o in objects:
    for c in list(o.users_collection):c.objects.unlink(o)
    coll.objects.link(o)
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=objects[0]
bpy.ops.object.join()
tree=bpy.context.object;tree.name='SM_HM_IslandTree'
tree.data.name=tree.name
before_tris=sum(len(p.vertices)-2 for p in tree.data.polygons)
if before_tris>180000:
    dec=tree.modifiers.new('Environment budget 180k triangles','DECIMATE');dec.ratio=180000/before_tris
    dec.use_collapse_triangulate=True
    bpy.ops.object.modifier_apply(modifier=dec.name)
tree.location=(0,0,0)
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
after_tris=sum(len(p.vertices)-2 for p in tree.data.polygons)
report={'source':'https://polyhaven.com/a/island_tree_01','license':'CC0','resolution':'2K','source_triangles':before_tris,'export_triangles':after_tris,'slots':[m.name for m in tree.data.materials]}
(OUT/'tree_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
bpy.ops.export_scene.fbx(filepath=str(OUT/'Exports/SM_HM_IslandTree.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE',path_mode='RELATIVE')
# 源树放在建筑外侧，仅在 UE 用实例布置；贴图使用项目内相对路径。
tree.location=(24,24,-1.5)
bpy.ops.file.make_paths_relative()
bpy.ops.wm.save_as_mainfile(filepath=str(master))
print('TREE_REPORT',report)
