import json, hashlib, urllib.request
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
dest=ROOT/'SourceArt/HomeMap/PolyHaven/island_tree_01'
dest.mkdir(parents=True,exist_ok=True)
files=json.loads((ROOT/'SourceArt/HomeMap/polyhaven_tree_files.json').read_text(encoding='utf-8'))
main=files['gltf']['2k']['gltf']
entries={'island_tree_01_2k.gltf':main,**main['include']}
for name,info in entries.items():
    target=(dest/name).resolve()
    assert target.is_relative_to(dest.resolve())
    target.parent.mkdir(parents=True,exist_ok=True)
    if not target.exists():
        req=urllib.request.Request(info['url'],headers={'User-Agent':'HomeMap-Environment-Production'})
        with urllib.request.urlopen(req,timeout=90) as response:target.write_bytes(response.read())
    assert hashlib.md5(target.read_bytes()).hexdigest()==info['md5'],name
    print('VERIFIED',name,target.stat().st_size,flush=True)
print('COMPLETE')
