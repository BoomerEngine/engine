/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_app_glue.inl"
#include "base/system/include/timing.h"

//--

BEGIN_BOOMER_NAMESPACE(base::app)

class ILocalService;
class IRemoteService;

class App;
class LocalServiceContainer;

class IFramework;

class CommandLine;

class ICommand;
typedef RefPtr<ICommand> CommandPtr;

class CommandHost;
typedef RefPtr<CommandHost> CommandHostPtr;

END_BOOMER_NAMESPACE(base::app);

//--

BEGIN_BOOMER_NAMESPACE(base)

/// get service
extern BASE_APP_API void* GetServicePtr(int serviceIndex);

/// get the local service by class
template< typename T >
INLINE T* GetService()
{
    static auto ptr  = (T*)GetServicePtr(reflection::ClassID<T>()->userIndex());
    return ptr;
}

END_BOOMER_NAMESPACE(base)

//--

#include "configProperty.h"