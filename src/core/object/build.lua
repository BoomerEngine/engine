-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")

print("Configuration: "..ConfigurationName)
print("Build: "..BuildName)
print("Platform: "..PlatformName)

Dependency("core_system")
Dependency("core_memory")
Dependency("core_containers")
Dependency("core_io")
Dependency("core_fibers")
Dependency("core_test")
Dependency("core_xml")
