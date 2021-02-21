-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

Dependency("base_system")
Dependency("base_memory")
Dependency("base_containers")
Dependency("base_config")
Dependency("base_object")
Dependency("base_reflection")
Dependency("base_resource")
Dependency("base_resource_compiler")
Dependency("base_math")
Dependency("base_image")
Dependency("base_fibers")
Dependency("base_app")

Dependency("rendering_device")
Dependency("rendering_texture")

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