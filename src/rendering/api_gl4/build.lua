-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("base_system")
Dependency("base_memory")
Dependency("base_containers")
Dependency("base_config")
Dependency("base_object")
Dependency("base_reflection")
Dependency("base_math")
Dependency("base_app")
Dependency("base_fibers")
Dependency("base_input")
Dependency("base_io")

Dependency("rendering_device")
Dependency("rendering_api_common")

FileOption("src/glew.c", "nopch")