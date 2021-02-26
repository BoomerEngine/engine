/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(config)

///----

// get configuration storage
extern CORE_CONFIG_API Storage& RawStorageData();

///---

// make/find config group with given name
extern CORE_CONFIG_API Group& MakeGroup(StringView name);

// find config group, returns NULL if not found
extern CORE_CONFIG_API const Group* FindGroup(StringView name);

// get config entry, creates an empty new one if not found
extern CORE_CONFIG_API Entry& MakeEntry(StringView groupName, StringView entryName);

// find config entry, return NULL if not found
extern CORE_CONFIG_API const Entry* FindEntry(StringView groupName, StringView entryName);

// find all groups starting with given start string
extern CORE_CONFIG_API Array<const Group*> FindAllGroups(StringView groupNameSubString);

//--

/// get all values
extern CORE_CONFIG_API const Array<StringBuf> Values(StringView groupName, StringView entryName);

/// get value (last value from a list or empty string)
extern CORE_CONFIG_API StringBuf Value(StringView groupName, StringView entryName, const StringBuf& defaultValue = StringBuf::EMPTY());

/// read as integer, returns default value if not parsed correctly
extern CORE_CONFIG_API int ValueInt(StringView groupName, StringView entryName, int defaultValue = 0);

/// read as a float, returns default value if not parsed correctly
extern CORE_CONFIG_API float ValueFloat(StringView groupName, StringView entryName, float defaultValue = 0);

/// read as a boolean, returns default value if not parsed correctly
extern CORE_CONFIG_API bool ValueBool(StringView groupName, StringView entryName, bool defaultValue = 0);

//--

/// write value to config storage
extern CORE_CONFIG_API void Write(StringView groupName, StringView entryName, StringView value);

/// write integer value to config storage
extern CORE_CONFIG_API void WriteInt(StringView groupName, StringView entryName, bool value);

/// write boolean value to config storage
extern CORE_CONFIG_API void WriteBool(StringView groupName, StringView entryName, bool value);

//---

END_BOOMER_NAMESPACE_EX(config)
