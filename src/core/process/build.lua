-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("core_system")
Dependency("core_memory")
Dependency("core_containers")
Dependency("core_io")
Dependency("core_test")

FileFilter("src/pipeWindows.cpp", "winapi")
FileFilter("src/pipeWindows.h", "winapi")
FileFilter("src/processWindows.cpp", "winapi")
FileFilter("src/processWindows.h", "winapi")

FileFilter("src/pipePOSIX.cpp", "posix")
FileFilter("src/pipePOSIX.h", "posix")
FileFilter("src/processPOSIX.cpp", "posix")
FileFilter("src/processPOSIX.h", "posix")
