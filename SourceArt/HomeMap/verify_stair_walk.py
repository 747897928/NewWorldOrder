import unreal,json,time
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
pw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=next(p for p in unreal.GameplayStatics.get_all_actors_of_class(pw,unreal.PlayerController) if p.get_controlled_pawn())
pawn=pc.get_controlled_pawn();movement=pawn.get_component_by_class(unreal.CharacterMovementComponent)
cap=pawn.get_component_by_class(unreal.CapsuleComponent)
pawn.set_actor_location(unreal.Vector(360,170,90),False,True)
pc.set_control_rotation(unreal.Rotator(pitch=-8,yaw=-90,roll=0))
walk_result={'capsule_radius':cap.get_scaled_capsule_radius(),'capsule_half_height':cap.get_scaled_capsule_half_height(),'start':[360,170,90],'samples':[]}
walk_start=time.monotonic();walk_last=0
def stair_tick(dt):
    global walk_last,walk_handle
    elapsed=time.monotonic()-walk_start
    pos=pawn.get_actor_location()
    if elapsed-walk_last>.2:
        walk_result['samples'].append({'time':round(elapsed,3),'position':list(pos.to_tuple()),'mode':str(movement.movement_mode)})
        walk_last=elapsed
    if pos.y>-740 and elapsed<12:
        pawn.add_movement_input(unreal.Vector(0,-1,0),1,True)
    else:
        movement.stop_movement_immediately()
        walk_result['passed']=pos.y<-620 and pos.z>430 and movement.movement_mode==unreal.MovementMode.MOVE_WALKING
        walk_result['end']=list(pos.to_tuple())
        (ROOT/'Docs/Tasks/HomeMap/Reports/stair_walk_final.json').write_text(json.dumps(walk_result,indent=2),encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(walk_handle)
walk_handle=unreal.register_slate_post_tick_callback(stair_tick)
print('WALK_STARTED')
