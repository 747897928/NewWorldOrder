"""补齐康体房间性能视角；只新增环境评审 CameraActor，不改玩家镜头。"""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not le.is_in_play_in_editor()
assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_name()=='HomeMap_Courtyard'
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
p=root/'SourceArt/HomeMap/review_cameras.json'
rows=json.loads(p.read_text(encoding='utf-8'))
source=actors['HM_Review_Kitchen'].get_component_by_class(unreal.CameraComponent)
for label,location,rotation in [('HM_Review_Gym',[780,80,165],[-5,40,0]),('HM_Review_Yoga',[790,760,165],[-3,47,0])]:
    actor=actors.get(label) or ea.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(*location))
    actor.set_actor_label(label);actor.set_folder_path('HomeMap/Review')
    actor.set_actor_location(unreal.Vector(*location),False,False)
    actor.set_actor_rotation(unreal.Rotator(pitch=rotation[0],yaw=rotation[1],roll=rotation[2]),False)
    component=actor.get_component_by_class(unreal.CameraComponent)
    for key in ['field_of_view','aspect_ratio','constrain_aspect_ratio']:
        component.set_editor_property(key,source.get_editor_property(key))
    row=next((r for r in rows if r['label']==label),None)
    if row is None:row={};rows.append(row)
    row.update(label=label,name=actor.get_name(),location=location,rotation=rotation)
    print('REVIEW_CAMERA',label,actor.get_name())
assert le.save_current_level()
p.write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
