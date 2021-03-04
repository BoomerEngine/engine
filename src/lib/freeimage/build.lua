-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("external")

LibraryInclude("include")

if PlatformName == "windows" then

	if UseStaticLibs then
	
		if ConfigurationName == "debug" then
			LibraryLink("lib/win/FreeImageLibd.lib")
		else
			LibraryLink("lib/win/FreeImageLib.lib")
		end
		
	else
	
		if ConfigurationName == "debug" then
			LibraryLink("lib/win/FreeImaged.lib")
			Deploy("lib/win/FreeImaged.dll")
		else
			LibraryLink("lib/win/FreeImage.lib")
			Deploy("lib/win/FreeImage.dll")
		end
		
	end

elseif PlatformName == "linux" then

    LibraryLink("lib/linux/libfreeimage.a")

end
