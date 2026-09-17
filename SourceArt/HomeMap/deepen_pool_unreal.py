"""将已保存 Blender 池深增量应用到用户验收的 BackUp 关卡；不改角色游泳系统。"""
import unreal,json
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert le.get_current_level().get_path_name().startswith('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard_BackUp.')
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
le.save_current_level()
path='/Game/Environment/HomeMap/Architecture/Hub/SM_HM_SiteTerrain_DeepPool'
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    t=unreal.AssetImportTask();t.filename=str(ROOT/'SourceArt/HomeMap/BFEU_Production/StaticMesh/SM_HM_SiteTerrain_DeepPool.fbx')
    t.destination_path='/Game/Environment/HomeMap/Architecture/Hub';t.destination_name='SM_HM_SiteTerrain_DeepPool';t.automated=True;t.save=True
    opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False
    opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    opt.static_mesh_import_data.auto_generate_collision=False;opt.static_mesh_import_data.generate_lightmap_u_vs=False
    t.options=opt;unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
mesh=unreal.load_asset(path);assert mesh
sm=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem);sm.remove_collisions(mesh)
mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
mesh.set_material(0,actors['HM_Site_Ground'].static_mesh_component.get_material(0))
unreal.EditorAssetLibrary.save_loaded_asset(mesh)
actors['HM_Site_Ground'].static_mesh_component.set_static_mesh(mesh)
records=json.loads((ROOT/'SourceArt/HomeMap/architecture.json').read_text(encoding='utf-8'))
tile=actors['HM_Pool_Base'].static_mesh_component.get_material(0)
for r in records:
    if not r['name'].startswith(('Pool_Base','Pool_Side','Pool_End','Pool_Step')):continue
    name='HM_'+r['name'];a=actors.get(name)
    if a is None:
        a=ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*r['loc']));a.set_actor_label(name);a.set_folder_path('HomeMap/Pool')
        c=a.static_mesh_component;c.set_static_mesh(unreal.load_asset('/Game/Environment/HomeMap/Architecture/Hub/'+r['mesh']))
        c.set_material(0,tile);c.set_mobility(unreal.ComponentMobility.STATIC);c.set_collision_profile_name('BlockAll')
    a.set_actor_location(unreal.Vector(*r['loc']),False,False);a.set_actor_scale3d(unreal.Vector(*r['scale']))
    actors[name]=a
assert actors['HM_Pool_Water'].get_actor_location().z==-18
le.save_current_level()
print('POOL_SAVED',{'water_z':-18,'floor_z':-230,'depth_cm':212,'steps':11})
