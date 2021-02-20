-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("external")

LibraryInclude("include")

if PlatformName == "windows" then

    LibraryLink("lib/win/glew32.lib")
    Deploy("lib/win/glew32.dll")
    Deploy("lib/win/glewinfo.exe")
    Deploy("lib/win/visualinfo.exe")

elseif PlatformName == "linux" then

    LibraryLink("lib/linux/libGLEW.a")

end
