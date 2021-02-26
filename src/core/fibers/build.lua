-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("core_system")
Dependency("core_memory")
Dependency("core_containers")

FileFilter("src/fiberSystemPOSIX.cpp", "posix")
FileFilter("src/fiberSystemPOSIX.h", "posix")

FileFilter("src/fiberSystemWinApi.cpp", "winapi")
FileFilter("src/fiberSystemWinApi.h", "winapi")