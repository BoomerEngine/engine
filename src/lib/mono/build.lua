-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("external")

LibraryInclude("include")
SharedDeployDir("shared")

if PlatformName == "windows" then

	-- if ConfigurationName == "debug" then

	if UseStaticLibs then	

		LibraryLink("lib/libmono-static-sgen.lib")

	else 

		LibraryLink("lib/mono-2.0-sgen.lib")
		Deploy("bin/mono-2.0-sgen.dll")
		Deploy("bin/mono-2.0-sgen.pdb")

		LibraryLink("lib/MonoPosixHelper.lib")
		Deploy("bin/MonoPosixHelper.dll")
		Deploy("bin/MonoPosixHelper.pdb")
		
	end


elseif PlatformName == "linux" then

    

end
