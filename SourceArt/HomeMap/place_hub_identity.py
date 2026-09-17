"""布置用户保留的实体参考板、出征标识、收藏柜和 Gallery 书桌。无任务玩法。"""
import unreal,json
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert le.get_current_level().get_path_name().startswith('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard.')
actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
assert 'HM_ArchiveCabinet' not in actors, '增量已摆放，禁止再次移动陈列摆件'
report=json.loads((ROOT/'SourceArt/HomeMap/hub_identity_report.json').read_text(encoding='utf-8'))
palette={'Hub_Wood':'M_Wall_Wood','Hub_Dark':'M_Metall_Black','Hub_Brass':'Hub/M_Bronze','Hub_Letter':'Hub/M_Courtyard_Stone','Hub_Map':'M_Metall_Black','Hub_Linen':'Hub/MI_Suite_Linen'}
for r in report:
    path='/Game/Environment/HomeMap/Architecture/Hub/'+r['name']
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        t=unreal.AssetImportTask();t.filename=str(ROOT/'SourceArt/HomeMap/BFEU_Production/StaticMesh'/(r['name']+'.fbx'));t.destination_path='/Game/Environment/HomeMap/Architecture/Hub';t.destination_name=r['name'];t.automated=True;t.save=True
        opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
        opt.static_mesh_import_data.auto_generate_collision=False;opt.static_mesh_import_data.generate_lightmap_u_vs=False;t.options=opt
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    mesh=unreal.load_asset(path);assert mesh
    for i,name in enumerate(r['materials']):mesh.set_material(i,unreal.load_asset('/Game/Environment/HomeMap/Materials/'+palette[name]))
    mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    name=r['name'][3:];a=actors.get(name) or ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*r['location']))
    a.set_actor_label(name);a.set_folder_path('HomeMap/GalleryAndExpedition');a.set_actor_location(unreal.Vector(*r['location']),False,False)
    c=a.static_mesh_component;c.set_static_mesh(mesh);c.set_mobility(unreal.ComponentMobility.STATIC);c.set_collision_profile_name('BlockAll' if 'Cabinet' in name or 'Desk' in name else 'NoCollision');actors[name]=a
a=actors.get('HM_GalleryArchiveCabinet') or ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(-535,-805,353))
a.set_actor_label('HM_GalleryArchiveCabinet');a.set_folder_path('HomeMap/GalleryAndExpedition');a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=90,roll=0),False);a.set_actor_scale3d(unreal.Vector(290/336,.55,1))
c=a.static_mesh_component;c.set_static_mesh(actors['HM_ArchiveCabinet'].static_mesh_component.static_mesh);c.set_mobility(unreal.ComponentMobility.STATIC);c.set_collision_profile_name('BlockAll')
# 只替换环境占位 Actor，原收藏摆件和副本交互实例保留。
for n,a in list(actors.items()):
    if n.startswith(('HM_Trophy_Shelf_','HM_Gallery_Collection_Shelf_','HM_Gallery_Desk_Leg_')) or n=='HM_Gallery_Desk':ea.destroy_actor(a)
    elif n.startswith('HM_Trophy_') and n[-1].isdigit():
        pos=a.get_actor_location();pos.z-=1.8;a.set_actor_location(pos,False,False)
    elif n.startswith('HM_Gallery_Collection_') and n[-1].isdigit():
        pos=a.get_actor_location();pos.z-=3.8 if int(n[-1])%2==0 else 8.8;a.set_actor_location(pos,False,False)
le.save_current_level();print('HUB_IDENTITY_SAVED',len(report),'meshes; no gameplay changes')
