-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

FileFilter("src/debugPOSIX.cpp", "posix")
FileFilter("src/mutexPOSIX.cpp", "posix")
FileFilter("src/eventPOSIX.cpp", "posix")
FileFilter("src/multiQueuePOSIX.cpp", "posix")
FileFilter("src/semaphorePOSIX.cpp", "posix")
FileFilter("src/threadPOSIX.cpp", "posix")
FileFilter("src/systemInfoPOSIX.cpp", "posix")

FileFilter("src/debugWin.cpp", "winapi")
FileFilter("src/eventWindows.cpp", "winapi")
FileFilter("src/multiQueueWindows.cpp", "winapi")
FileFilter("src/mutexWindows.cpp", "winapi")
FileFilter("src/orderedQueueWindows.cpp", "winapi")
FileFilter("src/semaphoreWindows.cpp", "winapi")
FileFilter("src/threadWindows.cpp", "winapi")
FileFilter("src/systemInfoWindows.cpp", "winapi")

--AdditionalCMakeFind("uuid");