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
Dependency("base_math")
Dependency("base_app")
Dependency("base_fibers")
Dependency("base_input")
Dependency("base_image")
Dependency("base_font")

Dependency("rendering_device")
Dependency("rendering_compiler")
Dependency("rendering_api_gl4")

ProjectOption("devonly")
ProjectOption("main")
