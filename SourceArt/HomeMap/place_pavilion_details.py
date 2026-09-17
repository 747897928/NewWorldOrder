"""本轮客厅配置：仅环境 Actor，位置按实际包围盒落地，不修改共享家具资产。"""
import unreal,json
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert le.get_current_level().get_path_name().startswith('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard.')
actors={a.get_actor_label():a for a in ea.get_all_level_actors()};changed=[]
def note(a):
    p=a.get_actor_location();r=a.get_actor_rotation();s=a.get_actor_scale3d();o,e=a.get_actor_bounds(False)
    changed.append({'label':a.get_actor_label(),'location':list(p.to_tuple()),'rotation':[r.pitch,r.yaw,r.roll],'scale':list(s.to_tuple()),'bounds_center':list(o.to_tuple()),'bounds_extent':list(e.to_tuple())})
    print('MODIFIED',a.get_actor_label())
def align(a,x,y,bottom):
    o,e=a.get_actor_bounds(False);p=a.get_actor_location()
    a.set_actor_location(p+unreal.Vector(x-o.x,y-o.y,bottom-(o.z-e.z)),False,False);note(a)
for label,pos in [('HM_GalleryCurtain_West',[-435,-979,353]),('HM_GalleryCurtain_East',[460,-979,353]),('HM_LivingCurtain_Front',[-506,-20,0])]:
    a=actors.get(label)
    if not a:a=ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*pos));a.set_actor_label(label)
    c=a.static_mesh_component;source=actors['HM_BedroomCurtain'].static_mesh_component;c.set_static_mesh(source.static_mesh)
    for i,m in enumerate(source.get_materials()):c.set_material(i,m)
    c.set_mobility(unreal.ComponentMobility.STATIC);c.set_collision_profile_name('NoCollision')
    a.set_actor_location(unreal.Vector(*pos),False,False);a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=-90,roll=0),False);a.set_actor_scale3d(unreal.Vector(1,1,1.05));a.set_folder_path('HomeMap/Pavilion');note(a)
for label,scale,xy in [('HM_Living_Table',[2.3,2.3,.83],[-300,-292]),('HM_Gallery_Table',[1.28,1.28,.8],[-140,-805])]:
    a=actors[label];a.set_actor_scale3d(unreal.Vector(*scale));align(a,*xy,2 if label=='HM_Living_Table' else 353)
    o,e=a.get_actor_bounds(False);proxy=actors[label+'_Collision']
    proxy.set_actor_location(o,False,False);proxy.set_actor_scale3d(unreal.Vector(e.x*.88/50,e.y*.88/50,e.z/50));note(proxy)
table=actors['HM_Living_Table'];o,e=table.get_actor_bounds(False);top=o.z+e.z
align(actors['HM_Living_Books'],-315,-298,top+.1);align(actors['HM_Living_Cup'],-274,-282,top+.1)
# 茶几放大后，窗边扶手椅前移 35 cm，沙发与茶几之间保持约 1.07 m 净空。
align(actors['HM_Living_Armchair'],-300,-85,2)
a=actors['HM_Living_Armchair'];o,e=a.get_actor_bounds(False);proxy=actors['HM_Living_Armchair_Collision'];proxy.set_actor_location(o,False,False);note(proxy)
a=actors['HM_Living_Rug'];extent=a.static_mesh_component.static_mesh.get_bounds().box_extent
a.set_actor_scale3d(unreal.Vector(510/(extent.x*2),490/(extent.y*2),a.get_actor_scale3d().z));align(a,-280,-320,1)
for prop,table in [('HM_Window_Vase','HM_Window_Table'),('HM_Gallery_Books','HM_Gallery_Table'),('HM_Gallery_Study_Vase','HM_GalleryWritingDesk')]:
    a=actors[prop];o,e=actors[table].get_actor_bounds(False);po,pe=a.get_actor_bounds(False);align(a,po.x,po.y,o.z+e.z+.1)
# 将原有主灯移至新灯具附近，不叠加新的动态光源。
a=actors['HM_Living_Key'];a.set_actor_location(unreal.Vector(-80,-330,431),False,False)
c=a.get_component_by_class(unreal.LightComponent);c.set_editor_property('intensity',1900);c.set_editor_property('use_temperature',True);c.set_editor_property('temperature',3200);c.set_editor_property('cast_shadows',False);note(a)
for label in ['HM_PavilionChandelier','HM_GalleryBackdrop','HM_PavilionEaveDetail']:actors[label].set_folder_path('HomeMap/Pavilion');note(actors[label])
assert le.save_current_level()
(ROOT/'Docs/Tasks/HomeMap/Reports/pavilion_placement.json').write_text(json.dumps(changed,indent=2),encoding='utf-8')
