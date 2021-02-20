-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("external")

if PlatformName == "windows" then

    Tool("bison", "win/win_bison.exe")

elseif PlatformName == "linux" then

    Tool("bison", "linux/bin/bison")

end
