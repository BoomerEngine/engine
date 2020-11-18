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
    class IRuntimePool : public base::mem::GlobalPoolObject<POOL_RENDERING_RUNTIME>
    {};

    class IDevice;
    class IDriverResource;

    struct PerformanceStats;

    class IDriverOutputCallback;
    class INativeWindowInterface;
    struct OutputInitInfo;
    struct DriverOutputFrameInfo;

    class IErrorHandler;

    class ObjectList;

    class IObject;
    class ObjectID;

    class ParamID;
    class ParamValueSupplier;

    class BufferView;
    class ImageView;

    class ShaderLibrary;
    typedef base::RefPtr<ShaderLibrary> ShaderLibraryPtr;
    typedef base::res::Ref<ShaderLibrary> ShaderLibraryRef;

    class ShaderLibraryData;
    typedef base::RefPtr<ShaderLibraryData> ShaderLibraryDataPtr;

    class Selector;
    typedef std::initializer_list<const Selector*> SelectorList;

    class DebugCollector;

    class FrameBuffer;

    namespace command
    {
        class CommandWriter;
        class CommandBuffer;
    } // command

    ///---

    // maximum number of color targets bindable to framebuffer
    static const uint32_t MAX_COLOR_TARGETS = 4;

    // maximum number of vertex input streams
    static const uint32_t MAX_VERTEX_INPUT_STREAMS = 16;

    // maximum number of global permutation keys
    static const uint16_t MAX_PERMUTATION_KEYS = 256;

    //----

    /// index of structure in pipeline library
    /// TOD: let's pray this won't have to be 32-bit, it would be a huge waste of memory
    typedef uint16_t PipelineIndex;

    /// index to debug string in pipeline library
    typedef uint32_t PipelineStringIndex;

    /// invalid pipeline index
    static const PipelineIndex INVALID_PIPELINE_INDEX = (PipelineIndex)-1;

    ///---

    class IOutputObject;
    typedef base::RefPtr<IOutputObject> OutputObjectPtr;

    class ShaderObject;
    typedef base::RefPtr<ShaderObject> ShaderObjectPtr;

    class BufferObject;
    typedef base::RefPtr<BufferObject> BufferObjectPtr;

    class ImageObject;
    typedef base::RefPtr<ImageObject> ImageObjectPtr;

    class SamplerObject;
    typedef base::RefPtr<SamplerObject> SamplerObjectPtr;

    ///---
    
} // rendering

