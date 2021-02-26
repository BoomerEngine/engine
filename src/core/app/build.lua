-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("core_system")
Dependency("core_containers")
Dependency("core_object")
Dependency("core_reflection")
Dependency("core_io")
Dependency("core_fibers")
Dependency("core_memory")
Dependency("core_socket")
Dependency("core_process")
Dependency("core_replication")
Dependency("core_config")
Dependency("core_net")

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