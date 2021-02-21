-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("app")

Dependency("base_system")
Dependency("base_memory")
Dependency("base_containers")
Dependency("base_config")
Dependency("base_object")
Dependency("base_reflection")
Dependency("base_resource")
Dependency("base_image")
Dependency("base_math")
Dependency("base_fibers")
Dependency("base_app")
Dependency("base_canvas")
Dependency("base_ui")

Dependency("rendering_device")
Dependency("rendering_canvas")
Dependency("rendering_scene")
Dependency("rendering_compiler")
Dependency("rendering_ui_host")
Dependency("rendering_api_gl4")

ProjectOption("devonly")
ProjectOption("main")

