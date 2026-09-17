"""按实际记录的相机位置分类采样，防止控制台切换失败却误报多视角通过。"""
import csv,json,re,statistics,gzip,sys,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
csv.field_size_limit(16*1024*1024)
label=sys.argv[2] if len(sys.argv)>2 else 'High'
assert label in ['High','Medium','LegacyLow']
log=ROOT/sys.argv[1];s=log.read_text(encoding='utf-8')
rx=int(re.findall(r'systemresolution.resx="(\d+)"',s)[-1]);ry=int(re.findall(r'systemresolution.resy="(\d+)"',s)[-1])
assert (rx,ry)==(1920,1080), f'实际分辨率 {rx}×{ry} 不符合 1080p，禁止作为目标采样提交'
tier={'High':2,'Medium':1,'LegacyLow':0}[label]
actual_scalability={}
for group in ['ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage']:
    matches=re.findall(r'sg\.'+group+r'Quality = "(\d+)"',s)
    assert matches and int(matches[-1])==tier, '日志未证明实际画质档位: '+group
    actual_scalability[group]=int(matches[-1])
path=Path(re.findall(r'Writing CSV to file : (.+)',s)[-1].strip())
archive=ROOT/('Docs/Tasks/HomeMap/Reports/HomeMap_Pavilion_'+label+'.csv.gz')
with path.open('rb') as inp,gzip.open(archive,'wb') as out:shutil.copyfileobj(inp,out)
with path.open(encoding='utf-8-sig',newline='') as f:
    reader=csv.DictReader(f);keys=reader.fieldnames;rows=[]
    for raw in reader:
        row={}
        for k,v in raw.items():
            try:row[k]=float(v)
            except (ValueError,TypeError):pass
        if 'FrameTime' in row:rows.append(row)
views=json.loads((ROOT/'SourceArt/HomeMap/review_cameras.json').read_text(encoding='utf-8'))
def stats(seq,key):
    vals=sorted(r[key] for r in seq if key in r)
    return {'mean':round(statistics.mean(vals),3),'p50':round(statistics.median(vals),3),'p95':round(vals[int((len(vals)-1)*.95)],3),'max':round(max(vals),3)} if vals else None
metrics=[k for k in keys if k in ['FrameTime','GPUTime','GameThreadTime','RenderThreadTime','RHIThreadTime','RHI/DrawCalls'] or k.startswith(('GPU/','DrawCall/','TextureStreaming/','NaniteStreaming/'))]
result={'log':str(log.relative_to(ROOT)),'archive':str(archive.relative_to(ROOT)),'hardware':'RTX 5060 Ti / i5-10400F','resolution':[rx,ry],'quality':'High / scalability 2 / TSR / 100%','mode':'Independent UnrealEditor game process, RenderOffscreen','total_frames':len(rows),'views':[],'limitations':['Not a packaged Shipping build or target GPU validation','Editor remains open with realtime disabled','Fixed daytime cameras; not a full traversal or night benchmark']}
view_keys=[k for k in keys if k.startswith('View/')]
for i,view in enumerate(views):
    start=600+i*1200;end=start+1200;seq=rows[start+180:end]
    pos={k:stats(seq,k) for k in view_keys}
    assert len(seq)==1020, '采样未完整刷新，不能冒充完整视角统计'
    for axis,expected in zip('XYZ',view['location']):
        assert abs(pos['View/Pos'+axis]['mean']-expected)<1 and abs(pos['View/Pos'+axis]['max']-expected)<1, '相机位置不符，禁止统计错误视角'
    result['views'].append({'label':view['label'],'expected_position':view['location'],'frames':len(seq),'discarded_after_switch_frames':180,'recorded_view':pos,'metrics':{k:stats(seq,k) for k in metrics}})
result['quality']=label+' / scalability '+str(tier)+(' / TAA / Nanite,VSM,Lumen disabled / 100%' if label=='LegacyLow' else ' / TSR / 100%')
if label=='LegacyLow':
    result['limitations'].append('Legacy fallback cost measured on current D3D12 workstation; not D3D11/SM5 or GTX compatibility validation')
    for cvar,value in [('r.Nanite',0),('r.Shadow.Virtual.Enable',0),('r.Lumen.DiffuseIndirect.Allow',0),('r.Lumen.Reflections.Allow',0),('r.AntiAliasingMethod',2)]:
        values=re.findall(re.escape(cvar)+r' = "(\d+)"',s)
        assert values and int(values[-1])==value, '回退设置未实际生效: '+cvar
result['actual_scalability_from_log']=actual_scalability
(ROOT/('Docs/Tasks/HomeMap/Reports/performance_pavilion_'+label.lower()+'.json')).write_text(json.dumps(result,indent=2),encoding='utf-8')
print(label,'frames',len(rows),'(view, frame mean, frame p95, GPU mean)',[(v['label'],v['metrics']['FrameTime']['mean'],v['metrics']['FrameTime']['p95'],v['metrics']['GPUTime']['mean']) for v in result['views']])
