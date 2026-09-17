import unreal
from pathlib import Path

# UE 单轮 Python 有 30 秒时限；导入分批，成功的包立即保存。
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source=(root/'SourceArt/HomeMap/import_greybox.py').read_text(encoding='utf-8')
exec(compile(source.split('assets.import_asset_tasks(tasks)')[0],'import_preparation','exec'))
assets.import_asset_tasks(tasks[:10])
print('IMPORTED_BATCH',len(tasks[:10]),'REMAINING',max(0,len(tasks)-10))
