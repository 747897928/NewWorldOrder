"""第三人称实际沿客厅座椅、茶几和楼梯走到 Gallery；只注入测试移动输入。"""
import unreal,time,json,math
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
pw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=next(p for p in unreal.GameplayStatics.get_all_actors_of_class(pw,unreal.PlayerController) if p.get_controlled_pawn())
pawn=pc.get_controlled_pawn();movement=pawn.get_component_by_class(unreal.CharacterMovementComponent)
targets=[[0,-200],[-175,-230],[-170,-403],[-300,-403],[-170,-403],[-170,-185],[0,-160],[360,170],[360,-745],[65,-730],[-260,-700]]
result={'map':pw.get_path_name(),'targets':targets,'samples':[],'reached':0};started=time.monotonic();last=0
def pavilion_tick(dt):
    global last,pavilion_handle
    now=time.monotonic()-started;pos=pawn.get_actor_location()
    if now-last>.25:
        result['samples'].append({'t':round(now,3),'position':list(pos.to_tuple()),'target':result['reached'],'mode':str(movement.movement_mode)});last=now
    if result['reached']==len(targets) or now>65:
        movement.stop_movement_immediately();result['passed']=result['reached']==len(targets);result['end']=list(pos.to_tuple());result['final_mode']=str(movement.movement_mode)
        (ROOT/'Docs/Tasks/HomeMap/Reports/pavilion_walk.json').write_text(json.dumps(result,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(pavilion_handle);return
    x,y=targets[result['reached']];dx,dy=x-pos.x,y-pos.y;distance=math.hypot(dx,dy)
    if distance<20:result['reached']+=1;return
    pc.set_control_rotation(unreal.Rotator(pitch=-8,yaw=math.degrees(math.atan2(dy,dx)),roll=0));pawn.add_movement_input(unreal.Vector(dx/distance,dy/distance,0),1,True)
pavilion_handle=unreal.register_slate_post_tick_callback(pavilion_tick)
print('PAVILION_WALK_STARTED')
