import unreal, json, os
from pathlib import Path

ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
MAP='/Game/Environment/HomeMap/Maps/HomeMap_Courtyard'
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert w.get_path_name().startswith(MAP+'.')
records=json.loads((ROOT/'SourceArt/HomeMap/architecture.json').read_text(encoding='utf-8'))
assets=unreal.AssetToolsHelpers.get_asset_tools()
sm=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
tasks=[]
for name in sorted({r['mesh'] for r in records if not r.get('asset_path')}):
    path='/Game/Environment/HomeMap/Architecture/Hub/'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):continue
    task=unreal.AssetImportTask()
    task.filename=str(ROOT/'SourceArt/HomeMap/Exports'/f'{name}.fbx')
    task.destination_path='/Game/Environment/HomeMap/Architecture/Hub'
    task.automated=True
    task.save=True
    task.replace_existing=False
    options=unreal.FbxImportUI()
    options.import_mesh=True
    options.import_materials=False
    options.import_textures=False
    options.import_as_skeletal=False
    options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.combine_meshes=False
    options.static_mesh_import_data.auto_generate_collision=True
    options.static_mesh_import_data.generate_lightmap_u_vs=False
    task.options=options
    tasks.append(task)
assets.import_asset_tasks(tasks)
for task in tasks:print('IMPORTED',task.imported_object_paths)
existing={a.get_actor_label():a for a in ea.get_all_level_actors()}
grey=unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial')
report=[]
for r in records:
    mesh=unreal.load_asset(r.get('asset_path') or '/Game/Environment/HomeMap/Architecture/Hub/'+r['mesh'])
    assert mesh, r['mesh']
    b=mesh.get_bounding_box()
    dims=[b.max.x-b.min.x,b.max.y-b.min.y,b.max.z-b.min.z]
    if r['name']=='Site_Ground':
        # 低密度地形用真实三角面碰撞，禁止包围盒封住整栋住宅。
        sm.remove_collisions(mesh)
        mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    else:
        assert all(abs(d-e)<.2 for d,e in zip(dims,r['dims'])),(r['name'],dims,r['dims'])
    if r['name'] not in ['Site_Ground','Gallery_Stair'] and sm.get_simple_collision_count(mesh)==0:
        sm.add_simple_collisions(mesh,unreal.ScriptCollisionShapeType.BOX)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    name='HM_'+r['name']
    actor=existing.get(name)
    if actor is None:actor=ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*r['loc']))
    actor.set_actor_location(unreal.Vector(*r['loc']),False,False)
    rotation=r.get('rotation',[0,0,0])
    actor.set_actor_rotation(unreal.Rotator(pitch=rotation[0],yaw=rotation[1],roll=rotation[2]),False)
    actor.set_actor_scale3d(unreal.Vector(*r.get('scale',[1,1,1])))
    actor.set_actor_label(name)
    actor.set_folder_path('HomeMap/'+r['folder'])
    component=actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_material(0,grey)
    component.set_collision_profile_name('BlockAll' if r['collision'] else 'NoCollision')
    # 灰盒阶段不放玻璃实体，避免把未开材质的玻璃误判成实墙。
    if r['mat']=='Glass':component.set_visibility(False)
    print('CREATED',name)
    report.append(dict(name=name,dimensions=dims,collision_count=sm.get_simple_collision_count(mesh)))
if 'HM_PlayerStart' not in existing:
    a=ea.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(0,-740,100),unreal.Rotator(pitch=0,yaw=90,roll=0))
    a.set_actor_label('HM_PlayerStart')
    a.set_folder_path('HomeMap/Gameplay')
for cls,name,loc in [(unreal.DirectionalLight,'HM_Sun',(0,0,1500)),(unreal.SkyLight,'HM_Sky',(0,0,1600)),(unreal.SkyAtmosphere,'HM_Atmosphere',(0,0,0))]:
    if name not in existing:
        a=ea.spawn_actor_from_class(cls,unreal.Vector(*loc))
        a.set_actor_label(name)
        a.set_folder_path('HomeMap/Lighting')
        if cls==unreal.DirectionalLight:
            a.set_actor_rotation(unreal.Rotator(pitch=-38,yaw=-45,roll=0),False)
            a.light_component.set_intensity(6)
        if cls==unreal.SkyLight:
            a.light_component.set_editor_property('real_time_capture',True)
            a.light_component.set_intensity(1)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
(ROOT/'Docs/Tasks/HomeMap/Reports/import.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('GREYBOX_SAVED',len(records))
