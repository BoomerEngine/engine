/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#pragma once


BEGIN_BOOMER_NAMESPACE()

//---

DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_LOADING, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_LOADED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_UNLOADED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_LOADER_FILE_FAILED, StringBuf)

DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_MODIFIED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_RESOURCE_RELOADED) // old resource + new resource

//---

// resource loading service
class CORE_RESOURCE_API LoadingService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(LoadingService, IService);

public:
    LoadingService();

    //--

    /// load resource from specified path and of specified class (can be subclass)
    /// NOTE: this will yield the current job until the resource is loaded
    virtual CAN_YIELD ResourcePtr loadResource(StringView path, ClassType expectedClassType) = 0;

    /// load resource from specified path and of specified class (can be subclass)
    /// NOTE: this will yield the current job until the resource is loaded
    virtual CAN_YIELD ResourcePtr loadResource(const ResourceID& id, ClassType expectedClassType) = 0;
};

//-----

END_BOOMER_NAMESPACE()
