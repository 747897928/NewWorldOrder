"""屋顶桌灯增量：复用暖光材质，仅增加一盏无阴影局部灯，不改昼夜业务。"""
import unreal
from pathlib import Path

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
p = root / 'SourceArt/HomeMap/sync_static_kit.py'
exec(compile(p.read_text(encoding='utf-8'), str(p), 'exec'))
sync_kit('rooftop_lantern_manifest.json')
ea = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = {a.get_actor_label(): a for a in ea.get_all_level_actors()}
actors['HM_RooftopLantern'].set_folder_path('HomeMap/Rooftop')
light = actors.get('HM_Rooftop_LanternLight') or ea.spawn_actor_from_class(unreal.PointLight, unreal.Vector(-287, -666, 793))
light.set_actor_label('HM_Rooftop_LanternLight')
light.set_folder_path('HomeMap/Rooftop')
light.set_actor_location(unreal.Vector(-287, -666, 793), False, False)
component = light.get_component_by_class(unreal.PointLightComponent)
component.set_mobility(unreal.ComponentMobility.STATIONARY)
for key, value in {
    'intensity_units': unreal.LightUnits.LUMENS, 'intensity': 240,
    'attenuation_radius': 600, 'cast_shadows': False,
    'use_temperature': True, 'temperature': 3000,
    'source_radius': 4, 'volumetric_scattering_intensity': 0,
}.items():
    component.set_editor_property(key, value)
print('ROOFTOP_LANTERN_SAVED', unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True))
