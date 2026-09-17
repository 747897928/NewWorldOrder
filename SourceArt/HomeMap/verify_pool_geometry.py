"""运行中角色实走池阶；仅验证几何，不把在池底行走当成游泳功能。"""
import unreal,json,time
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
pw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=next(p for p in unreal.GameplayStatics.get_all_actors_of_class(pw,unreal.PlayerController) if p.get_controlled_pawn())
pawn=pc.get_controlled_pawn();movement=pawn.get_component_by_class(unreal.CharacterMovementComponent)
result={'water_z':-18,'floor_design_z':-230,'depth_cm':212,'floor_traces':[],'samples':[],'swimming_implemented':False}
for x,y in [(0,900),(-300,900),(300,900),(0,1220)]:
    hit=unreal.SystemLibrary.line_trace_single(pw,unreal.Vector(x,y,10),unreal.Vector(x,y,-400),unreal.TraceTypeQuery.ECC_VISIBILITY,False,[pawn],unreal.DrawDebugTrace.NONE)
    d=hit.to_dict();result['floor_traces'].append({'xy':[x,y],'z':d['impact_point'].z,'actor':d['hit_actor'].get_actor_label()})
pawn.set_actor_location(unreal.Vector(0,220,90),False,True)
pc.set_control_rotation(unreal.Rotator(pitch=-10,yaw=90,roll=0))
pool_start=time.monotonic();pool_last=0;pool_phase='down'
def pool_tick(dt):
    global pool_last,pool_phase,pool_handle
    elapsed=time.monotonic()-pool_start;pos=pawn.get_actor_location()
    if elapsed-pool_last>.2:
        result['samples'].append({'time':round(elapsed,3),'phase':pool_phase,'position':list(pos.to_tuple()),'mode':str(movement.movement_mode)})
        pool_last=elapsed
    if pool_phase=='down' and pos.y>760:
        result['bottom_reached']=list(pos.to_tuple());pool_phase='up'
        pc.set_control_rotation(unreal.Rotator(pitch=-5,yaw=-90,roll=0))
    if (pool_phase=='up' and pos.y<220) or elapsed>20:
        movement.stop_movement_immediately()
        result['end']=list(pos.to_tuple());result['passed']=pool_phase=='up' and pos.y<220 and pos.z>85 and 'bottom_reached' in result
        (ROOT/'Docs/Tasks/HomeMap/Reports/pool_depth_geometry.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(pool_handle)
    else:pawn.add_movement_input(unreal.Vector(0,1 if pool_phase=='down' else -1,0),1,True)
pool_handle=unreal.register_slate_post_tick_callback(pool_tick)
print('POOL_WALK_STARTED')
