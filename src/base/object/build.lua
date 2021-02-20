-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

print("Configuration: "..ConfigurationName)
print("Build: "..BuildName)
print("Platform: "..PlatformName)

Dependency("base_system")
Dependency("base_memory")
Dependency("base_containers")
Dependency("base_io")
Dependency("base_fibers")
Dependency("base_test")
Dependency("base_xml")
