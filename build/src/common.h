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
    Windows,
    UWP,
    Linux,

    MAX,
};

enum class ConfigurationType : uint8_t {
    Debug,
    Checked,
    Release,
    Final, // release + even more code removed

    MAX,
};

enum class BuildType : uint8_t {
    Development, // includes development projects - editor, asset import, etc
    Shipment, // only final shipable executable (just game)

    MAX,
};

enum class LibraryType : uint8_t {
    Shared, // dlls
    Static, // static libs

    MAX,
};

enum class GeneratorType : uint8_t {
    VisualStudio,
    CMake,

    MAX,
};

namespace fs = std::filesystem;


