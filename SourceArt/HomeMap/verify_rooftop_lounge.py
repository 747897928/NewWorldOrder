"""实际角色穿行屋顶茶几、座位前和花池旁通道；仅起点归位一次。"""
import unreal,time,json,math
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=next(p for p in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlayerController) if p.get_controlled_pawn())
pawn=pc.get_controlled_pawn();movement=pawn.get_component_by_class(unreal.CharacterMovementComponent)
assert 'ShootCharacter' in pawn.get_class().get_name()
movement.stop_movement_immediately();pawn.set_actor_location(unreal.Vector(260,-400,834),False,False)
targets=[[260,-500],[50,-700],[-215,-755],[-360,-755],[-360,-500],[-120,-500],[60,-870],[280,-870],[280,-400],[260,-100]]
closure_result={'targets':targets,'reached':0,'samples':[]};start=time.monotonic();last=0
def closure_tick(dt):
    global last,closure_handle
    now=time.monotonic()-start;p=pawn.get_actor_location()
    if now-last>.2:closure_result['samples'].append({'t':round(now,3),'position':list(p.to_tuple()),'target':closure_result['reached'],'mode':str(movement.movement_mode)});last=now
    if closure_result['reached']==len(targets) or now>90:
        movement.stop_movement_immediately();closure_result['passed']=closure_result['reached']==len(targets) and movement.movement_mode==unreal.MovementMode.MOVE_WALKING;closure_result['end']=list(p.to_tuple())
        (ROOT/'Docs/Tasks/HomeMap/Reports/rooftop_lounge_walk.json').write_text(json.dumps(closure_result,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(closure_handle);return
    x,y=targets[closure_result['reached']];dx,dy=x-p.x,y-p.y;d=math.hypot(dx,dy)
    if d<22:closure_result['reached']+=1;return
    pc.set_control_rotation(unreal.Rotator(pitch=-7,yaw=math.degrees(math.atan2(dy,dx)),roll=0));pawn.add_movement_input(unreal.Vector(dx/d,dy/d,0),min(1,d/150),True)
closure_handle=unreal.register_slate_post_tick_callback(closure_tick)
print('ROOFTOP_LOUNGE_WALK_STARTED')
