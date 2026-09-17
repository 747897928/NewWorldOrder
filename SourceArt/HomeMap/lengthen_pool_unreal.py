"""应用已保存的 16 m 泳池源增量。先验收并重命名正式地图，再运行一次。"""
import unreal, json
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert le.get_current_level().get_path_name().startswith('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard.')
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
assert actors['HM_Pool_Water'].get_actor_location().y == 790, '已经延长或基准不匹配'
le.save_current_level()
records=json.loads((ROOT/'SourceArt/HomeMap/architecture.json').read_text(encoding='utf-8'))
changed=json.loads((ROOT/'SourceArt/HomeMap/pool_extension_records.json').read_text(encoding='utf-8'))
old=actors['HM_Site_Ground'].static_mesh_component.static_mesh
terrain_material=old.get_material(0)
t=unreal.AssetImportTask(); t.filename=str(ROOT/'SourceArt/HomeMap/BFEU_Production/StaticMesh/SM_HM_SiteTerrain_DeepPool.fbx')
t.destination_path='/Game/Environment/HomeMap/Architecture/Hub'; t.destination_name='SM_HM_SiteTerrain_DeepPool'; t.automated=True; t.save=True; t.replace_existing=True
opt=unreal.FbxImportUI(); opt.import_mesh=True; opt.import_materials=False; opt.import_textures=False; opt.import_as_skeletal=False; opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
opt.static_mesh_import_data.auto_generate_collision=False; opt.static_mesh_import_data.generate_lightmap_u_vs=False; t.options=opt
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
mesh=unreal.load_asset('/Game/Environment/HomeMap/Architecture/Hub/SM_HM_SiteTerrain_DeepPool')
sm=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem); sm.remove_collisions(mesh)
mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
mesh.set_material(0,terrain_material); unreal.EditorAssetLibrary.save_loaded_asset(mesh)
for r in records:
    if r['name'] not in changed: continue
    a=actors['HM_'+r['name']]
    a.set_actor_location(unreal.Vector(*r['loc']),False,False); a.set_actor_scale3d(unreal.Vector(*r['scale']))
water=actors['HM_Pool_Water']; water.set_actor_location(unreal.Vector(0,1090,-18),False,False); water.set_actor_scale3d(unreal.Vector(8.5,16,.01))
moved=[]
for n,a in actors.items():
    if n.startswith('HM_Garden_Plant_') or n in ['HM_Plant_12','HM_Plant_13'] or ('1370' in n and ('Lantern' in n or 'CourtyardWarm' in n)):
        pos=a.get_actor_location(); pos.y+=600; a.set_actor_location(pos,False,False); moved.append(n)
    if n.startswith('HM_Pool_Light'):
        pos=a.get_actor_location(); pos.y=1100; a.set_actor_location(pos,False,False)
        if 'Strip' in n: a.set_actor_scale3d(unreal.Vector(.03,8,.04))
# 保留原两盏水下灯，不因为水面变大成倍新增动态灯。
le.save_current_level()
print('POOL_EXTENSION_SAVED',{'length_cm':1600,'depth_cm':212,'moved_decor':moved})
