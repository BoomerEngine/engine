-- Boomer Engine v4 Build script
-- Written by Tomasz Jonarski (RexDex)

ProjectType("library")
ProjectOption("devonly")

Dependency("core_system")
Dependency("core_memory")
    
FileOption("src/gtest/gtest.cpp", "nopch")
FileOption("src/gtest/gtest-death-test.cpp", "nopch")
FileOption("src/gtest/gtest-filepath.cpp", "nopch")
FileOption("src/gtest/gtest-matchers.cpp", "nopch")
FileOption("src/gtest/gtest-port.cpp", "nopch")
FileOption("src/gtest/gtest-printers.cpp", "nopch")
FileOption("src/gtest/gtest-test-part.cpp", "nopch")
FileOption("src/gtest/gtest-typed-test.cpp", "nopch")