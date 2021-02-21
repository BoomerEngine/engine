-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("external")

LibraryInclude("include")


if PlatformName == "windows" then

    if ConfigurationName == "debug" then

        print("PhysX config: Windows.Debug")

        LibraryLink("lib/win/debug/LowLevel_static_64.lib")
        LibraryLink("lib/win/debug/LowLevelAABB_static_64.lib")
        LibraryLink("lib/win/debug/LowLevelDynamics_static_64.lib")
        LibraryLink("lib/win/debug/PhysX_64.lib")
	    LibraryLink("lib/win/debug/PhysXCharacterKinematic_static_64.lib")
	    LibraryLink("lib/win/debug/PhysXCommon_64.lib")
	    LibraryLink("lib/win/debug/PhysXCooking_64.lib")
	    LibraryLink("lib/win/debug/PhysXExtensions_static_64.lib")
	    LibraryLink("lib/win/debug/PhysXFoundation_64.lib")
	    LibraryLink("lib/win/debug/PhysXPvdSDK_static_64.lib")
	    LibraryLink("lib/win/debug/PhysXTask_static_64.lib")
	    LibraryLink("lib/win/debug/PhysXVehicle_static_64.lib")
	
        Deploy("lib/win/debug/PhysX_64.dll")
        Deploy("lib/win/debug/PhysXCommon_64.dll")
        Deploy("lib/win/debug/PhysXCooking_64.dll")
        Deploy("lib/win/debug/PhysXDevice64.dll")
        Deploy("lib/win/debug/PhysXFoundation_64.dll")
	    Deploy("lib/win/debug/PhysXGpu_64.dll")


    elseif ConfigurationName == "checked" then

        print("PhysX config: Windows.Checked")

        LibraryLink("lib/win/checked/LowLevel_static_64.lib")
        LibraryLink("lib/win/checked/LowLevelAABB_static_64.lib")
        LibraryLink("lib/win/checked/LowLevelDynamics_static_64.lib")
        LibraryLink("lib/win/checked/PhysX_64.lib")
	    LibraryLink("lib/win/checked/PhysXCharacterKinematic_static_64.lib")
	    LibraryLink("lib/win/checked/PhysXCommon_64.lib")
	    LibraryLink("lib/win/checked/PhysXCooking_64.lib")
	    LibraryLink("lib/win/checked/PhysXExtensions_static_64.lib")
	    LibraryLink("lib/win/checked/PhysXFoundation_64.lib")
	    LibraryLink("lib/win/checked/PhysXPvdSDK_static_64.lib")
	    LibraryLink("lib/win/checked/PhysXTask_static_64.lib")
	    LibraryLink("lib/win/checked/PhysXVehicle_static_64.lib")
	
        Deploy("lib/win/checked/PhysX_64.dll")
        Deploy("lib/win/checked/PhysXCommon_64.dll")
        Deploy("lib/win/checked/PhysXCooking_64.dll")
        Deploy("lib/win/checked/PhysXDevice64.dll")
        Deploy("lib/win/checked/PhysXFoundation_64.dll")
	    Deploy("lib/win/checked/PhysXGpu_64.dll")      

    else
        
        print("PhysX config: Windows.Release")

        LibraryLink("lib/win/release/LowLevel_static_64.lib")
        LibraryLink("lib/win/release/LowLevelAABB_static_64.lib")
        LibraryLink("lib/win/release/LowLevelDynamics_static_64.lib")
        LibraryLink("lib/win/release/PhysX_64.lib")
	    LibraryLink("lib/win/release/PhysXCharacterKinematic_static_64.lib")
	    LibraryLink("lib/win/release/PhysXCommon_64.lib")
	    LibraryLink("lib/win/release/PhysXCooking_64.lib")
	    LibraryLink("lib/win/release/PhysXExtensions_static_64.lib")
	    LibraryLink("lib/win/release/PhysXFoundation_64.lib")
	    LibraryLink("lib/win/release/PhysXPvdSDK_static_64.lib")
	    LibraryLink("lib/win/release/PhysXTask_static_64.lib")
	    LibraryLink("lib/win/release/PhysXVehicle_static_64.lib")

        Deploy("lib/win/release/PhysX_64.dll")
        Deploy("lib/win/release/PhysXCommon_64.dll")
        Deploy("lib/win/release/PhysXCooking_64.dll")
        Deploy("lib/win/release/PhysXDevice64.dll")
        Deploy("lib/win/release/PhysXFoundation_64.dll")
	    Deploy("lib/win/release/PhysXGpu_64.dll")

    end

elseif PlatformName == "linux" then

    if ConfigurationName == "debug" then

        print("PhysX config: Linux.Debug")
    
		LibraryLink("lib/linux/debug/libPhysX_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXExtensions_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXCharacterKinematic_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXCooking_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXPvdSDK_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXFoundation_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXCommon_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXVehicle_static_64.a")
		LibraryLink("lib/linux/debug/libSnippetRender_static_64.a")
		LibraryLink("lib/linux/debug/libSnippetUtils_static_64.a")

		LibraryLink("lib/linux/debug/libPhysX_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXExtensions_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXCharacterKinematic_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXCooking_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXPvdSDK_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXFoundation_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXCommon_static_64.a")
		LibraryLink("lib/linux/debug/libPhysXVehicle_static_64.a")
		LibraryLink("lib/linux/debug/libSnippetRender_static_64.a")
		LibraryLink("lib/linux/debug/libSnippetUtils_static_64.a")

    elseif ConfigurationName == "checked" then

        print("PhysX config: Linux.Checked")

		LibraryLink("lib/linux/checked/libPhysX_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXExtensions_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXCharacterKinematic_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXCooking_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXPvdSDK_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXFoundation_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXCommon_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXVehicle_static_64.a")
		LibraryLink("lib/linux/checked/libSnippetRender_static_64.a")
		LibraryLink("lib/linux/checked/libSnippetUtils_static_64.a")

		LibraryLink("lib/linux/checked/libPhysX_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXExtensions_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXCharacterKinematic_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXCooking_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXPvdSDK_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXFoundation_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXCommon_static_64.a")
		LibraryLink("lib/linux/checked/libPhysXVehicle_static_64.a")
		LibraryLink("lib/linux/checked/libSnippetRender_static_64.a")
		LibraryLink("lib/linux/checked/libSnippetUtils_static_64.a")

    else 

        print("PhysX config: Linux.Release")

		LibraryLink("lib/linux/release/libPhysX_static_64.a")
		LibraryLink("lib/linux/release/libPhysXExtensions_static_64.a")
		LibraryLink("lib/linux/release/libPhysXCharacterKinematic_static_64.a")
		LibraryLink("lib/linux/release/libPhysXCooking_static_64.a")
		LibraryLink("lib/linux/release/libPhysXPvdSDK_static_64.a")
		LibraryLink("lib/linux/release/libPhysXFoundation_static_64.a")
		LibraryLink("lib/linux/release/libPhysXCommon_static_64.a")
		LibraryLink("lib/linux/release/libPhysXVehicle_static_64.a")
		LibraryLink("lib/linux/release/libSnippetRender_static_64.a")
		LibraryLink("lib/linux/release/libSnippetUtils_static_64.a")

		LibraryLink("lib/linux/release/libPhysX_static_64.a")
		LibraryLink("lib/linux/release/libPhysXExtensions_static_64.a")
		LibraryLink("lib/linux/release/libPhysXCharacterKinematic_static_64.a")
		LibraryLink("lib/linux/release/libPhysXCooking_static_64.a")
		LibraryLink("lib/linux/release/libPhysXPvdSDK_static_64.a")
		LibraryLink("lib/linux/release/libPhysXFoundation_static_64.a")
		LibraryLink("lib/linux/release/libPhysXCommon_static_64.a")
		LibraryLink("lib/linux/release/libPhysXVehicle_static_64.a")
		LibraryLink("lib/linux/release/libSnippetRender_static_64.a")
		LibraryLink("lib/linux/release/libSnippetUtils_static_64.a")

    end

end
