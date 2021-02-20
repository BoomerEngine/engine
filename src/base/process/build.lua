-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("base_system")
Dependency("base_memory")
Dependency("base_containers")
Dependency("base_io")
Dependency("base_test")

FileFilter("src/pipeWindows.cpp", "winapi")
FileFilter("src/pipeWindows.h", "winapi")
FileFilter("src/processWindows.cpp", "winapi")
FileFilter("src/processWindows.h", "winapi")

FileFilter("src/pipePOSIX.cpp", "posix")
FileFilter("src/pipePOSIX.h", "posix")
FileFilter("src/processPOSIX.cpp", "posix")
FileFilter("src/processPOSIX.h", "posix")
