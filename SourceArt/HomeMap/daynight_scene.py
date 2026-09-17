import unreal

# 沿用 Niagara Gallery 的环境序列；控制面板走项目已有 IA_Interact，不复制演示角色或 HUD。
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
dest='/Game/Environment/HomeMap/Cinematics/LS_HomeMap_DayNight'
seq=unreal.load_asset(dest) if unreal.EditorAssetLibrary.does_asset_exist(dest) else unreal.EditorAssetLibrary.duplicate_asset('/Game/NiagaraExamples/Gallery/LevelSequences/LS_DayNight',dest)
moon=actors.get('HM_Moon')
if not moon:
    moon=ea.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,900))
    moon.set_actor_label('HM_Moon');moon.set_folder_path('HomeMap/Lighting')
moon.set_actor_rotation(unreal.Rotator(pitch=-32,yaw=135,roll=0),False)
mc=moon.light_component;mc.set_intensity(0);mc.set_cast_shadows(True)
mc.set_editor_property('use_temperature',True);mc.set_editor_property('temperature',8500)
mc.set_editor_property('atmosphere_sun_light',True);mc.set_editor_property('atmosphere_sun_light_index',1)

def keys(channel,values):
    for k in channel.get_keys():channel.remove_key(k)
    for frame,value in values:
        channel.add_key(unreal.FrameNumber(frame),value,interpolation=unreal.MovieSceneKeyInterpolation.LINEAR)

for b in seq.get_bindings():
    parent=b.get_parent().get_name()
    for t in b.get_tracks():
        prop=str(t.get_property_name()) if hasattr(t,'get_property_name') else ''
        for s in t.get_sections():
            s.set_completion_mode(unreal.MovieSceneCompletionMode.KEEP_STATE)
            channels=s.get_all_channels()
            if prop=='Intensity':
                values={'SunLight':[(0,8000),(60,600),(120,0)],'MoonLight':[(0,0),(60,0),(120,8)],'SkyLight':[(0,1.1),(60,1.0),(120,.6)]}
                keys(channels[0],values[parent])
            elif prop=='bHiddenInGame':
                # 不切换可见性，避免日夜端点产生硬切；光源强度负责淡入淡出。
                for c in channels:
                    for k in c.get_keys():c.remove_key(k)
                    c.set_default(True)
            elif prop=='FogInscatteringLuminance':
                for c,day,night in zip(channels,[.35,.42,.53,1],[.018,.027,.06,1]):keys(c,[(0,day),(60,day*.5),(120,night)])
            elif prop=='Transform':
                for c in channels:
                    name=str(c.channel_name)
                    if name=='Rotation.Y':keys(c,[(0,-30),(60,-6),(120,10)])
                    elif name=='Rotation.Z':c.set_default(-62)
seq.set_playback_end(121)
sequence_actor=actors.get('HM_DayNight')
if not sequence_actor:
    sequence_actor=ea.spawn_actor_from_class(unreal.LevelSequenceActor,unreal.Vector(0,0,0))
    sequence_actor.set_actor_label('HM_DayNight');sequence_actor.set_folder_path('HomeMap/Lighting')
sequence_actor.set_sequence(seq)
mapping={'SunLight':actors['HM_Sun'],'MoonLight':moon,'SkyLight':actors['HM_Sky'],'ExponentialHeightFog':actors['HM_Landscape_Fog']}
for b in seq.get_bindings():
    if b.get_name() in mapping:sequence_actor.set_binding(seq.get_binding_id(b),[mapping[b.get_name()]],False)
ps=sequence_actor.playback_settings;ps.set_editor_property('pause_at_end',True);sequence_actor.set_editor_property('playback_settings',ps)
unreal.EditorAssetLibrary.save_loaded_asset(seq)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('DAYNIGHT_ASSETS_SAVED',sequence_actor.get_path_name())
