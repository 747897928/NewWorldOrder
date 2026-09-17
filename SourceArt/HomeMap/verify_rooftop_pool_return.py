"""接跳台测试的入水位置，实际下潜并游回浅水台阶；不修改游泳属性。"""
import unreal,time,json,math
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
pw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=next(p for p in unreal.GameplayStatics.get_all_actors_of_class(pw,unreal.PlayerController) if p.get_controlled_pawn())
pawn=pc.get_controlled_pawn();movement=pawn.get_component_by_class(unreal.CharacterMovementComponent)
targets=[[0,1300,-400],[0,1000,-65],[0,220,90]]
pool_result={'targets':targets,'samples':[],'reached':0,'floor_traces':[]}
for x,y in [(0,625),(0,790),(0,1100),(-280,1400),(300,1800)]:
    hit=unreal.SystemLibrary.line_trace_single(pw,unreal.Vector(x,y,-25),unreal.Vector(x,y,-600),unreal.TraceTypeQuery.ECC_VISIBILITY,True,[pawn],unreal.DrawDebugTrace.NONE).to_dict()
    pool_result['floor_traces'].append({'xy':[x,y],'z':hit['impact_point'].z,'actor':hit['hit_actor'].get_actor_label() if hit['hit_actor'] else None})
start=time.monotonic();last=0
def deep_return_tick(dt):
    global last,deep_return_handle
    now=time.monotonic()-start;p=pawn.get_actor_location()
    if now-last>.15:pool_result['samples'].append({'t':round(now,3),'position':list(p.to_tuple()),'mode':str(movement.movement_mode),'target':pool_result['reached']});last=now
    if pool_result['reached']==len(targets) or now>60:
        movement.stop_movement_immediately();pool_result['passed']=pool_result['reached']==len(targets) and movement.movement_mode==unreal.MovementMode.MOVE_WALKING
        pool_result['end']=list(p.to_tuple());pool_result['final_mode']=str(movement.movement_mode)
        (ROOT/'Docs/Tasks/HomeMap/Reports/rooftop_pool_return.json').write_text(json.dumps(pool_result,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(deep_return_handle);return
    x,y,z=targets[pool_result['reached']];dx,dy,dz=x-p.x,y-p.y,z-p.z
    if pool_result['reached']>=2:dz=0
    d=math.sqrt(dx*dx+dy*dy+dz*dz)
    if d<25:pool_result['reached']+=1;return
    pc.set_control_rotation(unreal.Rotator(pitch=math.degrees(math.atan2(dz,max(1,math.hypot(dx,dy)))),yaw=math.degrees(math.atan2(dy,dx)),roll=0));pawn.add_movement_input(unreal.Vector(dx/d,dy/d,dz/d),1,True)
deep_return_handle=unreal.register_slate_post_tick_callback(deep_return_tick)
print('ROOFTOP_POOL_RETURN_STARTED')
