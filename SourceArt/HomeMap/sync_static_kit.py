"""HomeMap 静态套件的小型导入器。读取显式清单；不扫描全场景、不删除、不改 Gameplay。"""
import unreal,json,hashlib
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
def sync_kit(manifest_name):
    source=ROOT/'SourceArt/HomeMap';manifest_path=(source/manifest_name).resolve()
    assert manifest_path.is_relative_to(source.resolve())
    le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert le.get_current_level().get_path_name().startswith('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard.')
    ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
    rows=json.loads(manifest_path.read_text(encoding='utf-8'));result=[]
    for r in rows:
        assert r['mesh'].startswith('SM_HM_') and '/' not in r['mesh']
        filename=source/'BFEU_Production/StaticMesh'/(r['mesh']+'.fbx');digest=hashlib.sha256(filename.read_bytes()).hexdigest()
        # 显式清单可把背景地形放 Landscape；既有清单维持原路径，不全库扫描或迁移。
        destination=r.get('destination','/Game/Environment/HomeMap/Architecture/Hub')
        assert destination.startswith('/Game/Environment/HomeMap/') and '..' not in destination
        path=destination+'/'+r['mesh']
        mesh=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        imported=mesh is None or unreal.EditorAssetLibrary.get_metadata_tag(mesh,'HomeMap.SourceSHA256')!=digest
        if imported:
            t=unreal.AssetImportTask();t.filename=str(filename);t.destination_path=destination;t.destination_name=r['mesh'];t.automated=True;t.save=True;t.replace_existing=True
            opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
            opt.static_mesh_import_data.auto_generate_collision=False;opt.static_mesh_import_data.generate_lightmap_u_vs=False;t.options=opt
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);assert t.imported_object_paths
            mesh=unreal.load_asset(path);assert mesh
            unreal.EditorAssetLibrary.set_metadata_tag(mesh,'HomeMap.SourceSHA256',digest)
        if imported and r.get('rebuild_material_slots',False):
            # 跳台改型时 UE 会保留已移除的旧 FBX 槽；仅显式清单允许按新 FBX 分段顺序重建。
            assert mesh.get_num_sections(0)==len(r['materials']), '新 FBX 分段数量不符，停止重绑'
            mesh.set_editor_property('static_materials',[unreal.StaticMaterial(material_interface=unreal.load_asset(p),material_slot_name=unreal.Name(p.split('/')[-1])) for p in r['materials']])
            sm=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
            for i in range(len(r['materials'])):sm.set_lod_material_slot(mesh,material_slot_index=i,lod_index=0,section_index=i)
        assert len(mesh.static_materials)==len(r['materials']), '材质槽数不匹配，禁止静默错绑'
        for i,p in enumerate(r['materials']):
            mat=unreal.load_asset(p);assert mat,p
            if mesh.get_material(i)!=mat:mesh.set_material(i,mat)
            if r['nanite']:
                base=mat
                while isinstance(base,unreal.MaterialInstance):base=base.parent
                # 引擎／插件资产只读。需要该父材质时先建立项目内副本，再在清单绑定。
                assert base.get_path_name().startswith('/Game/'), 'Nanite 父材质在项目外，先使用项目内材质副本'
                if not base.get_editor_property('used_with_nanite'):
                    base.set_editor_property('used_with_nanite',True);unreal.MaterialEditingLibrary.recompile_material(base)
                unreal.EditorAssetLibrary.save_loaded_asset(base,only_if_is_dirty=True)
        settings=mesh.get_editor_property('nanite_settings')
        if settings.enabled!=r['nanite']:settings.enabled=r['nanite'];mesh.set_editor_property('nanite_settings',settings)
        # 开口家具采用逐面碰撞，防止自动凸包封住器械间的行走空间。
        body=mesh.get_editor_property('body_setup')
        if body.get_editor_property('collision_trace_flag')!=unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=True)
        for ins in r['instances']:
            assert ins['label'].startswith('HM_')
            a=actors.get(ins['label'])
            assert a is None or isinstance(a,unreal.StaticMeshActor), '同名 Actor 不是静态模型，停止以保护 Gameplay'
            if a is None:a=ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*ins['location_cm']));a.set_actor_label(ins['label']);actors[ins['label']]=a
            a.set_folder_path('HomeMap/Wellness');c=a.static_mesh_component;c.set_static_mesh(mesh);c.set_mobility(unreal.ComponentMobility.STATIC);c.set_collision_profile_name('BlockAll' if r['collision'] else 'NoCollision')
            a.set_actor_location(unreal.Vector(*ins['location_cm']),False,False)
            pitch,yaw,roll=ins['rotation'];a.set_actor_rotation(unreal.Rotator(pitch=pitch,yaw=yaw,roll=roll),False);a.set_actor_scale3d(unreal.Vector(*ins['scale']))
        result.append({'mesh':path,'imported':imported,'instances':len(r['instances'])})
        print('SYNCED',r['mesh'],'imported' if imported else 'unchanged FBX',len(r['instances']),'instances')
    le.save_current_level()
    return result
