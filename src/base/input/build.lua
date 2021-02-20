-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("base_system")
Dependency("base_memory")
Dependency("base_containers")
Dependency("base_io")
Dependency("base_math")
Dependency("base_reflection")
Dependency("base_object")
Dependency("base_config")
Dependency("base_fibers")

FileFilter("src/inputContextWinApi.cpp", "winapi")
FileFilter("src/inputContextWinApi.h", "winapi")
FileFilter("src/inputContextX11.cpp", "linux")
FileFilter("src/inputContextX11.h", "linux")