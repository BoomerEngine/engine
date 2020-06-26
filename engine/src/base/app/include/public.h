/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_app_glue.inl"
#include "base/system/include/timing.h"

#define RESPONSE_FUNC const base::http::RequestResult& result

#define REQUEST_FUNC const base::http::IncomingRequestPtr& request

namespace base
{
    namespace app
    {
        class ILocalService;
        class IRemoteService;

        class App;
        class LocalServiceContainer;

        class IFramework;

        class CommandLine;

    } // app

    //--

    /// get service
    extern BASE_APP_API void* GetServicePtr(int serviceIndex);

    /// get the local service by class
    template< typename T >
    INLINE T* GetService()
    {
        static auto ptr  = (T*)GetServicePtr(reflection::ClassID<T>()->userIndex());
        return ptr;
    }

    //--

} // base

#include "configProperty.h"