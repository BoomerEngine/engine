/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_app_glue.inl"
#include "core/system/include/timing.h"

BEGIN_BOOMER_NAMESPACE()

//--

class App;

class CommandLine;

class IService;
class ServiceContainer;

//--

class ICommand;
typedef RefPtr<ICommand> CommandPtr;

class CommandHost;
typedef RefPtr<CommandHost> CommandHostPtr;

//--

class IProjectSettings;
typedef RefPtr<IProjectSettings> ProjectSettingsPtr;

class ProjectSettingsFile;
typedef RefPtr<ProjectSettingsFile> ProjectSettingsFilePtr;

//--

/// get service
extern CORE_APP_API void* GetServicePtr(int serviceIndex);

/// get the local service by class
template< typename T >
INLINE T* GetService()
{
    static auto ptr  = (T*)GetServicePtr(ClassID<T>()->userIndex());
    return ptr;
}

//--

END_BOOMER_NAMESPACE()

//--

#include "configProperty.h"