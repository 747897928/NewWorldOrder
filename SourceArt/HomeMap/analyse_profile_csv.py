import csv,json,statistics,sys,gzip,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
path=ROOT/'Saved/Profiling/CSV/Profile(20260906_123239).csv'
archive=ROOT/'Docs/Tasks/HomeMap/Reports/HomeMap_Day_1080p_High.csv.gz'
if path.exists():
    with path.open('rb') as source,gzip.open(archive,'wb') as target:shutil.copyfileobj(source,target)
elif archive.exists():
    path.parent.mkdir(parents=True,exist_ok=True)
    with gzip.open(archive,'rb') as source,path.open('wb') as target:shutil.copyfileobj(source,target)
with path.open(encoding='utf-8-sig',newline='') as f:
    reader=csv.DictReader(f);header=reader.fieldnames;rows=[]
    for raw in reader:
        try:frame=float(raw.get('FrameTime',''))
        except (ValueError,TypeError):continue
        row={}
        for k,v in raw.items():
            try:row[k]=float(v)
            except (ValueError,TypeError):pass
        rows.append(row)
elapsed=0;stable=[]
for row in rows:
    elapsed+=row['FrameTime']/1000
    if elapsed>=15:stable.append(row)
def stats(key):
    values=[r[key] for r in stable if key in r]
    if not values:return None
    s=sorted(values)
    return dict(mean=round(statistics.mean(s),3),p50=round(statistics.median(s),3),p95=round(s[int((len(s)-1)*.95)],3),max=round(s[-1],3))
keys=[k for k in header if any(t in k.lower() for t in ['gpu','drawcall','primitivesdrawn','texturestreaming','nanite','frametime','renderthreadtime','gamethreadtime','rhithreadtime'])]
result={'source':str(path.relative_to(ROOT)),'resolution':[1920,1080],'quality':'High / scalability 2','hardware':'RTX 5060 Ti','total_frames':len(rows),'duration_seconds':round(elapsed,2),'warmup_excluded_seconds':15,'stable_frames':len(stable),'metrics':{k:stats(k) for k in keys},'view':{k:stats(k) for k in header if k.startswith('View/Pos')},'limitations':['Standalone editor executable, not packaged shipping build','Default spawn camera; not a complete traversal benchmark','Target GPU models have not been tested','Trace file was not produced by the tracing service; CSV and log are retained']}
(ROOT/'Docs/Tasks/HomeMap/Reports/performance_standalone_high.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k!='metrics'},indent=2))
print(json.dumps({k:v for k,v in result['metrics'].items() if k in ['FrameTime','GameThreadTime','RenderThreadTime','GPUTime'] or k.startswith('GPU/') or 'DrawCalls' in k or k.startswith('TextureStreaming/')},indent=2))
