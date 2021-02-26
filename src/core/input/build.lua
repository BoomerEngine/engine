-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("core_system")
Dependency("core_memory")
Dependency("core_containers")
Dependency("core_io")
Dependency("core_math")
Dependency("core_reflection")
Dependency("core_object")
Dependency("core_config")
Dependency("core_fibers")

FileFilter("src/inputContextWinApi.cpp", "winapi")
FileFilter("src/inputContextWinApi.h", "winapi")
FileFilter("src/inputContextX11.cpp", "linux")
FileFilter("src/inputContextX11.h", "linux")