-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("base_system")
Dependency("base_memory")
Dependency("base_containers")

FileFilter("src/fiberSystemPOSIX.cpp", "posix")
FileFilter("src/fiberSystemPOSIX.h", "posix")

FileFilter("src/fiberSystemWinApi.cpp", "winapi")
FileFilter("src/fiberSystemWinApi.h", "winapi")