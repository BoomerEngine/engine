/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_device_glue.inl"

#include "renderingObject.h"
#include "renderingPredefinedObjects.h"
#include "renderingStates.h"
#include "renderingImageFormat.h"

namespace rendering
{

    ///---

    namespace command
    {
        class CommandWriter;
        class CommandBuffer;
    } // command

    ///---

    class IDevice;
    struct PerformanceStats;

    ///---

    class ObjectID;

    class IDeviceObject;
    typedef base::RefPtr<IDeviceObject> DeviceObjectPtr;

    //--

    struct OutputInitInfo;

    class IOutputObject;
    typedef base::RefPtr<IOutputObject> OutputObjectPtr;

    class INativeWindowInterface;

    class FrameBuffer;

    //----

    /// index of structure in pipeline library
    /// TOD: let's pray this won't have to be 32-bit, it would be a huge waste of memory
    typedef uint16_t PipelineIndex;

    /// index to debug string in pipeline library
    typedef uint32_t PipelineStringIndex;

    /// invalid pipeline index
    static const PipelineIndex INVALID_PIPELINE_INDEX = (PipelineIndex)-1;

    class ShaderLibrary;
    typedef base::RefPtr<ShaderLibrary> ShaderLibraryPtr;
    typedef base::res::Ref<ShaderLibrary> ShaderLibraryRef;

    class ShaderLibraryData;
    typedef base::RefPtr<ShaderLibraryData> ShaderLibraryDataPtr;

    class ShaderObject;
    typedef base::RefPtr<ShaderObject> ShaderObjectPtr;

    //--

    class BufferObject;
    typedef base::RefPtr<BufferObject> BufferObjectPtr;

    class BufferView;
    struct BufferCreationInfo;

    //--

    class ImageObject;
    typedef base::RefPtr<ImageObject> ImageObjectPtr;

    class ImageView;

    struct ImageCreationInfo;

    //---

    class SamplerObject;
    typedef base::RefPtr<SamplerObject> SamplerObjectPtr;

    struct SamplerState;

    ///---

} // rendering

