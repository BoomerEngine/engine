-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("core_system")
Dependency("core_memory")
Dependency("core_containers")
Dependency("core_config")
Dependency("core_object")
Dependency("core_reflection")
Dependency("core_resource")
Dependency("core_resource_compiler")
Dependency("core_math")
Dependency("core_image")
Dependency("core_fibers")
Dependency("core_app")

--Dependency("gpu_device")

Dependency("engine_texture")

ProjectOption("devonly")

FileOption("src/bc6h/half/half.cpp", "nopch")
FileOption("src/bc6h/BC6H_Decode.cpp", "nopch")
FileOption("src/bc6h/BC6H_Definitions.cpp", "nopch")
FileOption("src/bc6h/BC6H_Encode.cpp", "nopch")
FileOption("src/bc6h/BC6H_utils.cpp", "nopch")
FileOption("src/bc6h/HDR_Encode.cpp", "nopch")

FileOption("src/bc7/3dquant_vpc.cpp", "nopch")
FileOption("src/bc7/BC7_Decode.cpp", "nopch")
FileOption("src/bc7/BC7_Definitions.cpp", "nopch")
FileOption("src/bc7/BC7_Encode.cpp", "nopch")
FileOption("src/bc7/BC7_Partitions.cpp", "nopch")
FileOption("src/bc7/BC7_utils.cpp", "nopch")
FileOption("src/bc7/reconstruct.cpp", "nopch")
FileOption("src/bc7/shake.cpp", "nopch")

FileOption("src/squish/alpha.cpp", "nopch")
FileOption("src/squish/clusterfit.cpp", "nopch")
FileOption("src/squish/colourblock.cpp", "nopch")
FileOption("src/squish/colourfit.cpp", "nopch")
FileOption("src/squish/colourset.cpp", "nopch")
FileOption("src/squish/maths.cpp", "nopch")
FileOption("src/squish/rangefit.cpp", "nopch")
FileOption("src/squish/singlecolourfit.cpp", "nopch")
FileOption("src/squish/squish.cpp", "nopch")