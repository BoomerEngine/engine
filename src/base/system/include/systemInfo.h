/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

// Get name of the user
#undef GetUserName
extern BASE_SYSTEM_API const char* GetUserName();

// Get computer name
extern BASE_SYSTEM_API const char* GetHostName();

// Get string that describes the system
extern BASE_SYSTEM_API const char* GetSystemName();

// Get value of the environment variable
extern BASE_SYSTEM_API const char* GetEnv(const char* key);

// Get value from the registry
extern BASE_SYSTEM_API bool GetRegistryKey(const char* path, const char* key, char* outBuffer, uint32_t& outBufferSize);

END_BOOMER_NAMESPACE(base)