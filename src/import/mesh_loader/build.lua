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
Dependency("core_fibers")
Dependency("core_app")

Dependency("engine_mesh")
Dependency("engine_material")
Dependency("engine_texture")

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


