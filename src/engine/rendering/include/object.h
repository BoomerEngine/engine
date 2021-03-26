/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

#include "scene.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// manager for rendering objects
class ENGINE_RENDERING_API IRenderingObjectManager : public NoCopy
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IRenderingObjectManager);
    RTTI_DECLARE_POOL(POOL_RENDERING_RUNTIME)

public:
    virtual ~IRenderingObjectManager();

    void renderLock();
    void renderUnlock();

    virtual void initialize(RenderingScene* scene, gpu::IDevice* dev) = 0;
    virtual void shutdown() = 0;

    //--- VALID ANYWHERE, BUFFERED IN RENDERING PHASE

    virtual void commandAttachProxy(IRenderingObject* object) = 0;
    virtual void commandDetachProxy(IRenderingObject* object) = 0;
    virtual void commandMoveProxy(IRenderingObject* object, Matrix newLocation) = 0;
    virtual void commandUpdateProxyFlag(IRenderingObject* object, RenderingObjectFlags clearFlags, RenderingObjectFlags setFlags) = 0;

    //--- VALID ONLY IN RENDER PHASE

    virtual void prepare(gpu::CommandWriter& cmd, gpu::IDevice* dev, const FrameRenderer& frame) = 0;
    virtual void finish(gpu::CommandWriter& cmd, gpu::IDevice* dev, const FrameRenderer& frame, FrameStats& outStats) = 0;

    virtual void render(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame) = 0;
    virtual void render(FrameViewCascadesRecorder& cmd, const FrameViewCascades& view, const FrameRenderer& frame) = 0;
    virtual void render(FrameViewWireframeRecorder& cmd, const FrameViewWireframe& view, const FrameRenderer& frame) = 0;
    virtual void render(FrameViewCaptureSelectionRecorder& cmd, const FrameViewCaptureSelection& view, const FrameRenderer& frame) = 0;
    virtual void render(FrameViewCaptureDepthRecorder& cmd, const FrameViewCaptureDepth& view, const FrameRenderer& frame) = 0;
    
protected:
    // run given function now if we are not locked or buffer it
    void runNowOrBuffer(std::function<void(void)> func);

private:
    volatile bool m_locked = false;

    struct BufferedCommand
    {
        RTTI_DECLARE_POOL(POOL_RENDERING_RUNTIME);
    public:
        std::function<void(void)> func;
    };

    SpinLock m_commandLock;
    Array<BufferedCommand*> m_bufferedCommands;
};

//--

template< typename T >
struct RenderingObjectMemoryAllocator
{
    uint32_t size = 0;

    uint8_t* memPtr = nullptr;
    uint8_t* memEndPtr = nullptr;

    inline RenderingObjectMemoryAllocator()
    {
        size = sizeof(T);
    }

    template< typename W >
    void add(uint32_t count = 1)
    {
        size += sizeof(W) * count;
    }

    T* allocate()
    {
        memPtr = (uint8_t*)GlobalPool<POOL_RENDERING_RUNTIME>::Alloc(size, alignof(T));
        memzero(memPtr, size);
        memEndPtr = memPtr + size;

        auto* ret = new (memPtr) T();
        memPtr += sizeof(T);

        return ret;
    }

    template< typename W >
    W* retive(uint32_t count = 1)
    {
        auto* ret = (W*)memPtr;
        memPtr = ret + (sizeof(T) * count);
        ASSERT(memPtr <= memEndPtr);
        return ret;
    }
};

//--

END_BOOMER_NAMESPACE()

