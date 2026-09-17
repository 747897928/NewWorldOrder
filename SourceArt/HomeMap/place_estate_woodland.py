"""导入中远景专用树冠，实例化既定林带；不改变可玩区域与动态灯。"""
import unreal,json,hashlib
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not le.is_in_play_in_editor()
assert le.get_current_level().get_path_name().startswith('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard.')
ml=unreal.MaterialEditingLibrary
# 远景专用材质复用原 2K 色图，省去此距离不可见的法线与 ARM 采样。
for name,texture_name,leaf in [('M_EstateWoodlandLeaf','island_tree_01_leaves_diff_2k',True),('M_EstateWoodlandBark','island_tree_01_diff_2k',False)]:
    path='/Game/Environment/HomeMap/Materials/Hub/'+name
    m=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,'/Game/Environment/HomeMap/Materials/Hub',unreal.Material,unreal.MaterialFactoryNew())
    if unreal.EditorAssetLibrary.get_metadata_tag(m,'HomeMap.WoodlandBuilt')!='2':
        ml.delete_all_material_expressions(m)
        t=ml.create_material_expression(m,unreal.MaterialExpressionTextureSample,-500,0)
        t.texture=unreal.load_asset('/Game/Environment/HomeMap/Landscape/Textures/'+texture_name)
        tint=ml.create_material_expression(m,unreal.MaterialExpressionMultiply,-200,0)
        tint.set_editor_property('const_b',.55 if leaf else .8)
        ml.connect_material_expressions(t,'RGB',tint,'A');ml.connect_material_property(tint,'',unreal.MaterialProperty.MP_BASE_COLOR)
        for prop,value,y in [(unreal.MaterialProperty.MP_ROUGHNESS,.93,200),(unreal.MaterialProperty.MP_SPECULAR,.12,330)]:
            e=ml.create_material_expression(m,unreal.MaterialExpressionConstant,-200,y);e.r=value;ml.connect_material_property(e,'',prop)
        m.set_editor_property('two_sided',leaf);m.set_editor_property('used_with_instanced_static_meshes',True)
        ml.recompile_material(m);unreal.EditorAssetLibrary.set_metadata_tag(m,'HomeMap.WoodlandBuilt','2');unreal.EditorAssetLibrary.save_loaded_asset(m)
        print('MODIFIED_MATERIAL',path)
p=ROOT/'SourceArt/HomeMap/sync_static_kit.py';exec(compile(p.read_text(encoding='utf-8'),str(p),'exec'))
sync_kit('estate_woodland_manifest.json')
text=(ROOT/'SourceArt/HomeMap/estate_woodland_positions.json').read_text(encoding='utf-8');positions=json.loads(text);digest=hashlib.sha256(text.encode()).hexdigest()
result=[]
for i,points in enumerate(positions):
    path='/Game/Environment/HomeMap/Landscape/FT_HM_Woodland_'+str(i)
    meshpath='/Game/Environment/HomeMap/Landscape/Meshes/SM_HM_EstateWoodland_'+str(i)
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.FoliageService.create_foliage_type(meshpath,'/Game/Environment/HomeMap/Landscape',path.rsplit('/',1)[-1],.85,1.55,False,0,75,40000)
    ft=unreal.load_asset(path)
    ft.set_editor_property('cast_shadow',False)
    ft.set_editor_property('cast_dynamic_shadow',False)
    ft.set_editor_property('affect_distance_field_lighting',False)
    ft.set_editor_property('enable_density_scaling',True)
    count=unreal.FoliageService.get_instance_count(path)
    if count<=0:
        print('PLACED',path,unreal.FoliageService.add_foliage_instances(path,[unreal.Vector(*p) for p in points],.85,1.55,False,True,False))
        unreal.EditorAssetLibrary.set_metadata_tag(ft,'HomeMap.LayoutHash',digest)
    else:
        assert unreal.EditorAssetLibrary.get_metadata_tag(ft,'HomeMap.LayoutHash')==digest,'已有林带布局变化，需要显式迁移，禁止重复叠加'
    unreal.EditorAssetLibrary.save_loaded_asset(ft)
    mesh=unreal.load_asset(meshpath)
    result.append({'foliage_type':path,'count':unreal.FoliageService.get_instance_count(path),'mesh_triangles':mesh.get_num_triangles(0),'bounds':str(mesh.get_bounds()),'cast_shadow':ft.cast_shadow})
assert le.save_current_level()
(ROOT/'Docs/Tasks/HomeMap/Reports/estate_woodland_saved_state.json').write_text(json.dumps({'map':'HomeMap_Courtyard','source':'HomeMap_Master.blend','woodland':result,'note':'Geometry and saved-state verification only; screenshots and performance reviewed separately'},indent=2)+'\n',encoding='utf-8')
print('WOODLAND_SAVED',result)
