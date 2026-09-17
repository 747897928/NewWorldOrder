import unreal,json
# 历史迁移记录：当前制作以 BackUp 关卡为准，不得重复执行本脚本覆盖已验收配置。
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
MAP='/Game/Environment/HomeMap/Maps/HomeMap_Courtyard'
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().startswith(MAP+'.')
actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
records=json.loads((ROOT/'SourceArt/HomeMap/architecture.json').read_text(encoding='utf-8'))
by_name={r['name']:r for r in records}
pilot=json.loads((ROOT/'Docs/Tasks/HomeMap/Reports/verified_stair_migration.json').read_text(encoding='utf-8'))
for r in pilot:
    a=actors[r['label']]
    a.set_actor_location(unreal.Vector(*r['loc']),False,False)
    a.set_actor_rotation(unreal.Rotator(pitch=r['rot'][0],yaw=r['rot'][1],roll=r['rot'][2]),False)
    a.set_actor_scale3d(unreal.Vector(*r['scale']))
    if r['mesh']: a.static_mesh_component.set_static_mesh(unreal.load_asset(r['mesh']))
    source=by_name.get(r['label'][3:])
    if source:
        source.update(loc=r['loc'],rotation=r['rot'],scale=r['scale'],asset_path=r['mesh'])
        if 'BFEU_Pilot' in (r['mesh'] or ''):source['pipeline']='BFEU local mesh + actor transform'

# 右侧玻璃收拢在端部，保留约 310 cm 的庭院开口；楼梯中心线不再撞玻璃。
glass=actors['HM_Living_Glass_350']
glass.set_actor_location(unreal.Vector(505,0,162.5),False,False)
glass.set_actor_scale3d(unreal.Vector(110/420,1,1))
by_name['Living_Glass_350'].update(loc=[505,0,162.5],scale=[110/420,1,1])
for i in range(1,4):
    name='HM_Living_Sliding_Glass_'+str(i)
    a=actors.get(name) or ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(505,-i*8,162.5))
    a.set_actor_label(name);a.set_folder_path('HomeMap/Glazing')
    a.set_actor_scale3d(unreal.Vector(110/420,1,1))
    a.static_mesh_component.set_static_mesh(glass.static_mesh_component.static_mesh)
    a.static_mesh_component.set_material(0,glass.static_mesh_component.get_material(0))
    a.static_mesh_component.set_collision_profile_name('BlockAll')
    rec=dict(by_name['Living_Glass_350']);rec.update(name=name[3:],loc=[505,-i*8,162.5])
    if rec['name'] in by_name:by_name[rec['name']].update(rec)
    else:records.append(rec)

# 家具以组为单位移开，连同原有隐藏碰撞代理一起移动，不修改角色或镜头。
for prefix,delta in [('HM_Window_Chair',(-120,-170,0)),('HM_Window_Ottoman',(-140,-170,0)),('HM_Window_Table',(-140,-410,0)),('HM_Window_Vase',(-140,-410,0))]:
    for name,a in actors.items():
        if name==prefix or name==prefix+'_Collision':
            # 以旧位置阈值防止再次执行时重复平移。
            if a.get_actor_location().x>150:a.set_actor_location(a.get_actor_location()+unreal.Vector(*delta),False,False)

control=actors.get('HM_DayNight_Control')
if control is None:
    control=ea.spawn_actor_from_class(unreal.load_class(None,'/Game/Environment/HomeMap/Blueprints/BP_HomeMap_EnvironmentControl.BP_HomeMap_EnvironmentControl_C'),unreal.Vector(170,-690,0))
    control.set_actor_label('HM_DayNight_Control');control.set_folder_path('HomeMap/Gameplay')
control.set_editor_property('day_night_sequence_actor',actors['HM_DayNight'])
control.set_editor_property('initial_time_of_day',unreal.ShootEnvironmentTimeOfDay.DUSK)

# 复用备份中用户验收的衣柜子类。保留原实例位置，引用检查后才移除旧实例。
old=actors['HM_Vanity_Interactive']
if old.get_class().get_name()!='BP_HomeMap_Dressing_Table_Set_C':
    replacement=ea.spawn_actor_from_class(unreal.load_class(None,'/Game/Environment/HomeMap/Blueprints/BP_HomeMap_Dressing_Table_Set.BP_HomeMap_Dressing_Table_Set_C'),old.get_actor_location(),old.get_actor_rotation())
    replacement.set_actor_scale3d(old.get_actor_scale3d())
    replacement.set_folder_path(old.get_folder_path())
    ea.destroy_actor(old)
    replacement.set_actor_label('HM_Vanity_Interactive')
gate=actors['HM_Expedition_Gate']
prompt=gate.get_component_by_class(unreal.ShootLocalInteractionPrompt)
prompt.set_editor_property('display_distance',256)
prompt.set_editor_property('widget_class',unreal.load_class(None,'/Game/UI/FrontEnd/W_Icon_Interact.W_Icon_Interact_C'))
prompt.set_relative_location(unreal.Vector(0,0,90),False,False)
(ROOT/'SourceArt/HomeMap/architecture.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('PILOT_INTEGRATED',len(pilot))
