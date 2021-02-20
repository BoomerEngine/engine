-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("base_system")
Dependency("base_containers")
Dependency("base_object")
Dependency("base_reflection")
Dependency("base_io")
Dependency("base_fibers")
Dependency("base_memory")
Dependency("base_socket")
Dependency("base_process")
Dependency("base_replication")
Dependency("base_config")
Dependency("base_net")

FileFilter("src/posixOutput.h", "posix")
FileFilter("src/posixOutput.cpp", "posix")
FileFilter("src/posixPlatform.h", "posix")
FileFilter("src/posixPlatform.cpp", "posix")

FileFilter("src/winOutput.cpp", "winapi")
FileFilter("src/winOutput.h", "winapi")
FileFilter("src/winPlatform.cpp", "winapi")
FileFilter("src/winPlatform.h", "winapi")
FileFilter("src/winErrorWindow.cpp", "winapi")
FileFilter("src/winErrorWindow.h", "winapi")