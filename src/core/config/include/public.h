/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_config_glue.inl"

BEGIN_BOOMER_NAMESPACE()

///----

class ConfigEntry;
class ConfigGroup;
class ConfigStorage;

///----

// get configuration storage
extern CORE_CONFIG_API ConfigStorage& ConfigRawStorageData();

///---

// make/find config group with given name
extern CORE_CONFIG_API ConfigGroup& ConfigMakeGroup(StringView name);

// find config group, returns NULL if not found
extern CORE_CONFIG_API const ConfigGroup* ConfigFindGroup(StringView name);

// get config entry, creates an empty new one if not found
extern CORE_CONFIG_API ConfigEntry& ConfigMakeEntry(StringView groupName, StringView entryName);

// find config entry, return NULL if not found
extern CORE_CONFIG_API const ConfigEntry* ConfigFindEntry(StringView groupName, StringView entryName);

// find all groups starting with given start string
extern CORE_CONFIG_API Array<const ConfigGroup*> ConfigFindAllGroups(StringView groupNameSubString);

//--

/// get all values
extern CORE_CONFIG_API const Array<StringBuf> ConfigValues(StringView groupName, StringView entryName);

/// get value (last value from a list or empty string)
extern CORE_CONFIG_API StringBuf ConfigValue(StringView groupName, StringView entryName, const StringBuf& defaultValue = StringBuf::EMPTY());

/// read as integer, returns default value if not parsed correctly
extern CORE_CONFIG_API int ConfigValueInt(StringView groupName, StringView entryName, int defaultValue = 0);

/// read as a float, returns default value if not parsed correctly
extern CORE_CONFIG_API float ConfigValueFloat(StringView groupName, StringView entryName, float defaultValue = 0);

/// read as a boolean, returns default value if not parsed correctly
extern CORE_CONFIG_API bool ConfigValueBool(StringView groupName, StringView entryName, bool defaultValue = 0);

//--

/// write value to config storage
extern CORE_CONFIG_API void ConfigWrite(StringView groupName, StringView entryName, StringView value);

/// write integer value to config storage
extern CORE_CONFIG_API void ConfigWriteInt(StringView groupName, StringView entryName, bool value);

/// write boolean value to config storage
extern CORE_CONFIG_API void ConfigWriteBool(StringView groupName, StringView entryName, bool value);

//---

END_BOOMER_NAMESPACE()

#include "system.h"
