-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("app")

Dependency("core_system")
Dependency("core_memory")
Dependency("core_containers")
Dependency("core_config")
Dependency("core_object")
Dependency("core_reflection")
Dependency("core_resource")
Dependency("core_math")
Dependency("core_app")
Dependency("core_fibers")
Dependency("core_input")
Dependency("core_image")

Dependency("gpu_device")
Dependency("gpu_shader_compiler")
Dependency("gpu_api_gl4")

ProjectOption("devonly")
ProjectOption("main")
