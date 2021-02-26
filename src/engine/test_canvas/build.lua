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

Dependency("gpu_*")

Dependency("engine_font")
Dependency("engine_canvas")

ProjectOption("devonly")
ProjectOption("main")
