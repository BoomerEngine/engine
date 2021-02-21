-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("external")

LibraryInclude("include")

if PlatformName == "windows" then

    if ConfigurationName == "Debug" then
        LibraryLink("lib/win/freetyped.lib")
        Deploy("lib/win/freetyped.dll")
    else
        LibraryLink("lib/win/freetype.lib")
        Deploy("lib/win/freetype.dll")
    end

elseif PlatformName == "linux" then

    LibraryLink("lib/linux/libfreetype.a")

end
