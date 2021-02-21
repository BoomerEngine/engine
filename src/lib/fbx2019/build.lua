-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("external")

LibraryInclude("include")

if PlatformName == "windows" then

    if ConfigurationName == "debug" then
        LibraryLink("lib/win/debug/libfbxsdk.lib")
        Deploy("lib/win/debug/libfbxsdk.dll")
        Deploy("lib/win/debug/libfbxsdk.pdb")
    else
        LibraryLink("lib/win/release/libfbxsdk.lib")
        Deploy("lib/win/release/libfbxsdk.dll")
    end

elseif PlatformName == "linux" then

    LibraryLink("lib/linux/libfbxsdk.a")

end
