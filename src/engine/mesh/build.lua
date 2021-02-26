-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("core_system")
Dependency("core_memory")
Dependency("core_containers")
Dependency("core_config")
Dependency("core_object")
Dependency("core_reflection")
Dependency("core_resource")
Dependency("core_image")
Dependency("core_math")
Dependency("core_fibers")
Dependency("core_app")

Dependency("gpu_device")

Dependency("engine_texture")
Dependency("engine_material")

FileOption("src/meshopt/indexcodec.cpp", "nopch")
FileOption("src/meshopt/vertexcodec.cpp", "nopch")
