"""现有角色连续穿行卧室、衣帽走道、卫浴并返回床前。"""
import unreal,time,json,math
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
pw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=next(p for p in unreal.GameplayStatics.get_all_actors_of_class(pw,unreal.PlayerController) if p.get_controlled_pawn())
pawn=pc.get_controlled_pawn();movement=pawn.get_component_by_class(unreal.CharacterMovementComponent)
targets=[[-810,1120],[-1180,1120],[-1180,1050],[-810,1050],[-810,350],[-1020,350]]
walk={'targets':targets,'samples':[],'reached':0,'start':list(pawn.get_actor_location().to_tuple())}
started=time.monotonic();last_sample=0
def private_tick(dt):
    global last_sample,private_handle
    now=time.monotonic()-started;pos=pawn.get_actor_location()
    if now-last_sample>.3:
        walk['samples'].append({'t':round(now,3),'position':list(pos.to_tuple()),'target':walk['reached'],'mode':str(movement.movement_mode)});last_sample=now
    if walk['reached']>=len(targets) or now>40:
        movement.stop_movement_immediately();walk['passed']=walk['reached']==len(targets);walk['end']=list(pos.to_tuple())
        (ROOT/'Docs/Tasks/HomeMap/Reports/private_suite_walk.json').write_text(json.dumps(walk,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(private_handle);return
    x,y=targets[walk['reached']];dx=x-pos.x;dy=y-pos.y;length=math.hypot(dx,dy)
    if length<24:walk['reached']+=1;return
    pc.set_control_rotation(unreal.Rotator(pitch=-8,yaw=math.degrees(math.atan2(dy,dx)),roll=0))
    pawn.add_movement_input(unreal.Vector(dx/length,dy/length,0),1,True)
private_handle=unreal.register_slate_post_tick_callback(private_tick)
print('PRIVATE_WALK_STARTED')
