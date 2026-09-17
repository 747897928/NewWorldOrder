"""在已验收 BackUp 关卡中摆放本轮主卧和卫浴套件，保留原交互实例。"""
import unreal,json,math
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert le.get_current_level().get_path_name().startswith('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard_BackUp.')
actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
palette={'Suite_Wood':'M_Wall_Wood','Suite_Fabric':'Hub/MI_Suite_Linen','Suite_Brass':'Hub/M_Bronze','Suite_Glow':'Hub/M_Warm_Light','Suite_Stone':'Hub/M_Courtyard_Stone','Suite_Ceramic':'M_Kitchen_Ceramic','Suite_Dark':'M_Metall_Black'}
report=json.loads((ROOT/'SourceArt/HomeMap/private_suite_report.json').read_text(encoding='utf-8'))
for r in report:
    mesh=unreal.load_asset('/Game/Environment/HomeMap/Architecture/Hub/'+r['name']);assert mesh
    for i,name in enumerate(r['materials']):
        mat=unreal.load_asset('/Game/Environment/HomeMap/Materials/'+palette[name]);assert mat,name
        mesh.set_material(i,mat)
    settings=mesh.get_editor_property('nanite_settings');settings.enabled='Curtain' not in r['name'];mesh.set_editor_property('nanite_settings',settings)
    mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    label=r['name'][3:];a=actors.get(label) or ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*r['location_cm']))
    a.set_actor_label(label);a.set_folder_path('HomeMap/PrivateSuite');a.set_actor_location(unreal.Vector(*r['location_cm']),False,False)
    c=a.static_mesh_component;c.set_static_mesh(mesh);c.set_mobility(unreal.ComponentMobility.STATIC);c.set_collision_profile_name('NoCollision' if 'Curtain' in label else 'BlockAll')
    actors[label]=a
label='HM_BedroomCurtain_Return';a=actors.get(label) or ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(-731,535,0))
a.set_actor_label(label);a.set_folder_path('HomeMap/PrivateSuite');a.static_mesh_component.set_static_mesh(actors['HM_BedroomCurtain'].static_mesh_component.static_mesh);a.static_mesh_component.set_collision_profile_name('NoCollision');a.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)

def furniture(label,loc,yaw=0,scale=1,mesh_path=None):
    a=actors.get(label)
    if a is None:
        a=ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector());a.set_actor_label(label);a.set_folder_path('HomeMap/PrivateSuite');a.static_mesh_component.set_static_mesh(unreal.load_asset(mesh_path));a.static_mesh_component.set_collision_profile_name('NoCollision');a.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC);actors[label]=a
    mesh=a.static_mesh_component.static_mesh;b=mesh.get_bounding_box();cx=(b.min.x+b.max.x)*scale/2;cy=(b.min.y+b.max.y)*scale/2;t=math.radians(yaw)
    a.set_actor_location(unreal.Vector(loc[0]-cx*math.cos(t)+cy*math.sin(t),loc[1]-cx*math.sin(t)-cy*math.cos(t),loc[2]-b.min.z*scale),False,False)
    a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=0),False);a.set_actor_scale3d(unreal.Vector(scale,scale,scale))
    proxy=actors.get(label+'_Collision')
    if proxy:
        h=min((b.max.z-b.min.z)*scale,95);proxy.set_actor_location(unreal.Vector(loc[0],loc[1],loc[2]+h/2),False,False)
        proxy.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=0),False);proxy.set_actor_scale3d(unreal.Vector((b.max.x-b.min.x)*scale*.88/100,(b.max.y-b.min.y)*scale*.88/100,h/100))

furniture('HM_Master_Bed',(-1220,300,2),0,1.05)
furniture('HM_Bedroom_Rug',(-1130,300,1),0,1.5)
for old_y,new_y in [(70,110),(410,490)]:
    furniture('HM_Bedside_'+str(old_y),(-1305,new_y,2))
    furniture('HM_Bed_Lamp_'+str(old_y),(-1305,new_y,54))
furniture('HM_Bedroom_Chair',(-955,480,2),210)
furniture('HM_Plant_5',(-1120,540,0),140,1.2)
furniture('HM_Bath_Vase',(-1350,1280,91))
for label,name,loc,yaw,scale in [
    ('HM_Bedside_Books','SM_Stack_of_Books_1',(-1298,101,54),10,.6),
    ('HM_Bedroom_Reading_Table','SM_Coffe_Table_3',(-1050,470,0),0,.75),
    ('HM_Bedroom_Reading_Book','SM_Opened_Book',(-1050,470,30.8),30,.6),
    ('HM_Bedroom_FloorLamp','SM_Floor_Lamp',(-910,560,0),0,1),
    ('HM_Bath_CeramicBottle','SM_Kitchen_Decor_11_White',(-1240,1300,90),0,.7),
    ('HM_Bath_DarkBottle','SM_Kitchen_Decor_11_Black',(-1225,1304,90),0,.6)]:
    folder='Furniture' if name in ['SM_Coffe_Table_3','SM_Floor_Lamp'] else 'Props'
    furniture(label,loc,yaw,scale,'/Game/Environment/HomeMap/'+folder+'/'+name)
furniture('HM_Bed_Lamp_70',(-1315,118,54))
# 只移除已被套件替换的环境占位 Actor，不删除任何资源包，也不触碰 Gameplay。
for label in ['HM_Bath_Vanity','HM_Bath_Counter','HM_Bath_Basin','HM_Bath_Faucet','HM_Bath_Faucet_Nose']:
    if label in actors:ea.destroy_actor(actors[label]);actors.pop(label)
actors['HM_Bedroom_Light'].light_component.set_intensity(900)
actors['HM_Bathroom_Light'].light_component.set_intensity(1100)
label='HM_Bedroom_Headwall_Wash';a=actors.get(label) or ea.spawn_actor_from_class(unreal.RectLight,unreal.Vector(-1335,300,295))
a.set_actor_label(label);a.set_folder_path('HomeMap/Lighting');a.set_actor_rotation(unreal.Rotator(pitch=-50,yaw=0,roll=0),False)
c=a.light_component;c.set_mobility(unreal.ComponentMobility.STATIONARY);c.set_intensity(350);c.set_temperature(3100);c.set_use_temperature(True);c.set_cast_shadows(False);c.set_attenuation_radius(300);c.set_editor_property('source_width',400);c.set_editor_property('source_height',6)
le.save_current_level();print('PRIVATE_SUITE_SAVED')
