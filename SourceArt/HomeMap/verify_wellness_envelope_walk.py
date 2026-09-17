"""现有 CharacterMovement 连续通过器械区、瑜伽位和柜前走道。"""
import unreal,time,json,math
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
pw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=next(p for p in unreal.GameplayStatics.get_all_actors_of_class(pw,unreal.PlayerController) if p.get_controlled_pawn());pawn=pc.get_controlled_pawn();movement=pawn.get_component_by_class(unreal.CharacterMovementComponent)
targets=[[990,160],[995,365],[995,585],[810,660],[810,1100],[1200,1100],[1210,1320],[810,1320],[810,740],[795,140]]
result={'map':pw.get_path_name(),'targets':targets,'samples':[],'reached':0}
started=time.monotonic();last=0
def wellness_tick(dt):
    global last,wellness_handle
    now=time.monotonic()-started;pos=pawn.get_actor_location()
    if now-last>.3:
        result['samples'].append({'t':round(now,3),'position':list(pos.to_tuple()),'target':result['reached'],'mode':str(movement.movement_mode)});last=now
    if result['reached']==len(targets) or now>50:
        movement.stop_movement_immediately();result['passed']=result['reached']==len(targets);result['end']=list(pos.to_tuple())
        (ROOT/'Docs/Tasks/HomeMap/Reports/wellness_envelope_walk.json').write_text(json.dumps(result,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(wellness_handle);return
    x,y=targets[result['reached']];dx,dy=x-pos.x,y-pos.y;distance=math.hypot(dx,dy)
    if distance<24:result['reached']+=1;return
    pc.set_control_rotation(unreal.Rotator(pitch=-8,yaw=math.degrees(math.atan2(dy,dx)),roll=0));pawn.add_movement_input(unreal.Vector(dx/distance,dy/distance,0),min(1,distance/150),True)
wellness_handle=unreal.register_slate_post_tick_callback(wellness_tick)
print('WELLNESS_ENVELOPE_WALK_STARTED')
