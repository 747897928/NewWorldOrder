"""通过已安装 Blender MCP 的本地桥执行项目内脚本。"""
import json, socket, sys
from pathlib import Path

script = Path(sys.argv[1]).resolve()
root = Path(__file__).resolve().parents[2]
assert script.is_relative_to(root)
code = f"__file__={str(script)!r}\nexec(compile(open(__file__,encoding='utf-8').read(),__file__,'exec'))"
with socket.create_connection(('127.0.0.1',9876),timeout=20) as client:
    client.settimeout(240)
    client.sendall(json.dumps({'type':'execute_code','params':{'code':code}}).encode('utf-8'))
    data=b''
    while True:
        part=client.recv(65536)
        if not part:break
        data+=part
        try:
            result=json.loads(data.decode('utf-8'))
            break
        except json.JSONDecodeError:continue
print(json.dumps(result,ensure_ascii=False))
