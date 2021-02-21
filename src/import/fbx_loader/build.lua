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
Dependency("rendering_material")
Dependency("rendering_mesh")

Dependency("import_mesh_loader")

Dependency("lib_fbx2019")

ProjectOption("devonly")
