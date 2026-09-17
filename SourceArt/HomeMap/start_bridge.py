import bpy
import blender_mcp

# 独立环境制作会话，不加载或覆盖用户已有 Blender 文件。
server = blender_mcp.BlenderMCPServer(host='127.0.0.1', port=9876)
server.start()
bpy.app.driver_namespace['homemap_mcp_server'] = server
