"""外围背景、屋顶休息区和有限树带；不更改主场地或玩法。"""
import unreal,json,hashlib
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
p=ROOT/'SourceArt/HomeMap/sync_static_kit.py';exec(compile(p.read_text(encoding='utf-8'),str(p),'exec'))
sync_kit('landscape_finish_manifest.json')
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);a={x.get_actor_label():x for x in ea.get_all_level_actors()}
a['HM_EstateBackdrop'].set_folder_path('HomeMap/Landscape')
c=a['HM_EstateBackdrop'].static_mesh_component;c.set_cast_shadow(False);c.set_editor_property('affect_distance_field_lighting',False)
for n in ['HM_RooftopLounge','HM_RooftopFinish']:a[n].set_folder_path('HomeMap/Rooftop')
# 中景树带直接引用既有 32,304 面树；远处不用昂贵动态阴影。
ftpath='/Game/Environment/HomeMap/Landscape/FT_HM_EstateTrees'
if not unreal.EditorAssetLibrary.does_asset_exist(ftpath):
 unreal.FoliageService.create_foliage_type('/Game/Environment/HomeMap/Landscape/Meshes/SM_HM_CourtyardCanopyTree','/Game/Environment/HomeMap/Landscape','FT_HM_EstateTrees',1.4,2.2,False,0,65,24000)
ft=unreal.load_asset(ftpath);ft.set_editor_property('cast_dynamic_shadow',False);ft.set_editor_property('enable_density_scaling',True);unreal.EditorAssetLibrary.save_loaded_asset(ft)
points_text=(ROOT/'SourceArt/HomeMap/estate_tree_positions.json').read_text(encoding='utf-8')
layout_hash=hashlib.sha256(points_text.encode('utf-8')).hexdigest()
if unreal.EditorAssetLibrary.get_metadata_tag(ft,'HomeMap.LayoutHash')!=layout_hash or unreal.FoliageService.get_instance_count(ftpath)<=0:
 # 只替换此脚本独占的外围树带，避免地形改形后树根悬空；不触及庭院原树或其他植被类型。
 if unreal.FoliageService.get_instance_count(ftpath)>0:unreal.FoliageService.remove_all_foliage_of_type(ftpath)
 points=json.loads(points_text)
 print('TREE_INSTANCES',unreal.FoliageService.add_foliage_instances(ftpath,[unreal.Vector(*p) for p in points],1.4,2.2,False,True,False))
 unreal.EditorAssetLibrary.set_metadata_tag(ft,'HomeMap.LayoutHash',layout_hash);unreal.EditorAssetLibrary.save_loaded_asset(ft)
# 屋顶角落花池复用植物，避开已验证的跑动路线。
plant=unreal.load_asset('/Game/Environment/HomeMap/Props/SM_Plant_1')
for n,loc in [('HM_Rooftop_Plant_West',(-470,-630,748)),('HM_Rooftop_Plant_East',(430,-904,748))]:
 actor=a.get(n) or ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*loc));actor.set_actor_label(n);actor.set_folder_path('HomeMap/Rooftop');actor.set_actor_location(unreal.Vector(*loc),False,False);actor.set_actor_scale3d(unreal.Vector(.9,.9,.9))
 comp=actor.static_mesh_component;comp.set_static_mesh(plant);comp.set_mobility(unreal.ComponentMobility.STATIC);comp.set_collision_profile_name('NoCollision');comp.set_cast_shadow(False)
# 茶几沿用客厅杯碟与书籍，按真实包围盒底面对齐，不复制造型或引入新材质。
for label,source,loc in [('HM_Rooftop_Books','HM_Living_Books',(-248,-654,778)),('HM_Rooftop_Cup_1','HM_Living_Cup',(-178,-638,778)),('HM_Rooftop_Cup_2','HM_Living_Cup',(-186,-669,778))]:
 src=a[source];actor=a.get(label) or ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*loc));actor.set_actor_label(label);actor.set_folder_path('HomeMap/Rooftop')
 c=actor.static_mesh_component;c.set_static_mesh(src.static_mesh_component.static_mesh)
 for i,m in enumerate(src.static_mesh_component.get_materials()):c.set_material(i,m)
 actor.set_actor_scale3d(src.get_actor_scale3d());actor.set_actor_rotation(src.get_actor_rotation(),False)
 origin,extent=actor.get_actor_bounds(False);actor.set_actor_location(actor.get_actor_location()+unreal.Vector(loc[0]-origin.x,loc[1]-origin.y,loc[2]-(origin.z-extent.z)),False,False)
 c.set_mobility(unreal.ComponentMobility.STATIC);c.set_collision_profile_name('NoCollision');c.set_cast_shadow(False)
print('saved',unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True))
print('ESTATE_LANDSCAPE_AND_LOUNGE_APPLIED')
