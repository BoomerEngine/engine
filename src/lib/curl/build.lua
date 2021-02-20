-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("external")

LibraryInclude("include")

if PlatformName == "windows" then

    if ConfigName == "debug" then
        LibraryLink("lib/win/libcurld.lib")
        Deploy("lib/win/libcurld.dll")
    else
        LibraryLink("lib/win/libcurl.lib")
        Deploy("lib/win/libcurl.dll")
    end

elseif PlatformName == "linux" then

    LibraryLink("lib/linux/libcurl.so")
    Deploy("lib/linux/libcurl.so")

end
