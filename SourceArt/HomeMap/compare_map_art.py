import unreal,json
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
def snapshot_art():
    result={}
    for a in ea.get_all_level_actors():
        name=a.get_actor_label()
        if not name.startswith('HM_'):continue
        # 只比较环境表现。蓝图、序列 Actor、PlayerStart 和 WorldSettings 全部保留新基准。
        if not isinstance(a,(unreal.StaticMeshActor,unreal.Light,unreal.PostProcessVolume,unreal.SkyAtmosphere,unreal.ExponentialHeightFog,unreal.VolumetricCloud)):continue
        data={'class':a.get_class(),'transform':a.get_actor_transform(),'folder':a.get_folder_path(),'hidden':a.get_editor_property('hidden'),'components':[]}
        for c in a.get_components_by_class(unreal.SceneComponent):
            if isinstance(c,unreal.BillboardComponent):continue
            props={}
            names=['mobility','relative_location','relative_rotation','relative_scale3d','visible','hidden_in_game']
            if isinstance(c,unreal.StaticMeshComponent):
                props['static_mesh']=c.static_mesh
                props['override_materials']=list(c.get_materials())
                names+=['cast_shadow','ld_max_draw_distance']
                data['collision']=c.get_collision_profile_name()
            if isinstance(c,unreal.LightComponentBase):
                names+=['intensity','light_color','cast_shadows','cast_volumetric_shadow','indirect_lighting_intensity','volumetric_scattering_intensity']
            if isinstance(c,unreal.LightComponent):
                names+=['use_temperature','temperature','specular_scale','affect_translucent_lighting']
            if isinstance(c,unreal.LocalLightComponent):names+=['intensity_units','attenuation_radius']
            if isinstance(c,unreal.RectLightComponent):names+=['source_width','source_height','barn_door_angle','barn_door_length']
            if isinstance(c,unreal.DirectionalLightComponent):names+=['light_source_angle','atmosphere_sun_light','atmosphere_sun_light_index']
            if isinstance(c,unreal.SkyLightComponent):names+=['real_time_capture','lower_hemisphere_is_black','source_type','cubemap']
            if isinstance(c,unreal.ExponentialHeightFogComponent):names+=['fog_density','fog_height_falloff','start_distance','fog_max_opacity','enable_volumetric_fog']
            if isinstance(c,unreal.VolumetricCloudComponent):names+=['material','layer_bottom_altitude','layer_height']
            for p in names:
                try:
                    value=c.get_editor_property(p)
                    # 属性结构体可能借用 UObject 内存，切换地图前必须复制。
                    props[p]=value.copy() if isinstance(value,unreal.StructBase) else value
                except Exception:pass
            data['components'].append({'class':c.get_class(),'props':props})
        if isinstance(a,unreal.PostProcessVolume):data['settings']=a.settings.copy();data['unbound']=a.get_editor_property('unbound')
        result[name]=data
    return result
def readable(value):
    if isinstance(value,unreal.Object):return value.get_path_name()
    if isinstance(value,dict):return {k:readable(v) for k,v in value.items()}
    if isinstance(value,(list,tuple)):return [readable(v) for v in value]
    if isinstance(value,(str,int,float,bool)) or value is None:return value
    if isinstance(value,unreal.Transform):return {'loc':list(value.translation.to_tuple()),'rot':list(value.rotation.euler().to_tuple()),'scale':list(value.scale3d.to_tuple())}
    # 去掉 Python wrapper 地址，只比较结构内容。
    import re
    return re.sub(r' \(0x[0-9A-Fa-f]+\)','',str(value))
assert le.load_level('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard')
art_main=snapshot_art()
print('ART_MAIN_CAPTURED',len(art_main))
assert le.load_level('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard_BackUp')
art_base=snapshot_art()
delta={}
for name,data in art_main.items():
    if name not in art_base:delta[name]={'new_art':True}
    else:
        a,b=readable(data),readable(art_base[name])
        changed={k:{'main':a[k],'backup':b[k]} for k in a if a[k]!=b[k]}
        if changed:delta[name]=changed
(ROOT/'Docs/Tasks/HomeMap/Reports/map_art_diff.json').write_text(json.dumps(delta,ensure_ascii=False,indent=2),encoding='utf-8')
print('BACKUP_ART',len(art_base),'DIFFERENCES',len(delta))
print([(k,list(v)) for k,v in delta.items()])
