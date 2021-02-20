#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string_view>
#include <algorithm>
#include <filesystem>

#include <assert.h>

extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "lua/lstate.h"
}

enum class ToolType : uint8_t {
    SolutionGenerator,
    ReflectionGenerator,
};

enum class PlatformType : uint8_t {
    Any,
    Windows,
    UWP,
    Linux,
};

enum class ConfigurationType : uint8_t {
    Debug,
    Checked,
    Release,
    Final, // release + even more code removed
};

enum class BuildType : uint8_t {
    Development, // dlls + devonly projects, does not work with final    
    Standalone, // standalone executables (all statically linked)
};

enum class GeneratorType : uint8_t {
    VisualStudio,
    CMake,
};

using namespace std;


