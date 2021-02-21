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
Dependency("base_fibers")
Dependency("base_app")

Dependency("rendering_mesh")
Dependency("rendering_material")
Dependency("rendering_texture")

ProjectOption("devonly")

FileOption("src/meshopt/allocator.cpp", "nopch")
FileOption("src/meshopt/clusterizer.cpp", "nopch")
FileOption("src/meshopt/indexcodec.cpp", "nopch")
FileOption("src/meshopt/indexgenerator.cpp", "nopch")
FileOption("src/meshopt/overdrawanalyzer.cpp", "nopch")
FileOption("src/meshopt/overdrawoptimizer.cpp", "nopch")
FileOption("src/meshopt/simplifier.cpp", "nopch")
FileOption("src/meshopt/spatialorder.cpp", "nopch")
FileOption("src/meshopt/stripifier.cpp", "nopch")
FileOption("src/meshopt/vcacheanalyzer.cpp", "nopch")
FileOption("src/meshopt/vcacheoptimizer.cpp", "nopch")
FileOption("src/meshopt/vertexcodec.cpp", "nopch")
FileOption("src/meshopt/vertexfilter.cpp", "nopch")
FileOption("src/meshopt/vfetchanalyzer.cpp", "nopch")
FileOption("src/meshopt/vfetchoptimizer.cpp", "nopch")

FileOption("src/mikktspace/mikktspace.c", "nopch")


