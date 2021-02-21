-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("external")

LibraryInclude("include")

if PlatformName == "windows" then

    if ConfigurationName == "Debug" then
        LibraryLink("lib/win/crnlibD_DLL_x64_VC9.lib")
    else
        LibraryLink("lib/win/crnlib_DLL_x64_VC9.lib")
    end

elseif PlatformName == "linux" then

    LibraryLink("lib/linux/libcrunch.a")

end
