/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "params.h"
#include "stats.h"

#include "gpu/device/include/framebuffer.h"
#include "gpu/device/include/commandWriter.h"

#include "core/memory/include/linearAllocator.h"
#include "core/reflection/include/variantTable.h"
#include "core/memory/include/pageCollection.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

///---

struct FragmentRenderContext;
class FrameResources;

///---

struct FrameViewRecorder
{
    FrameViewRecorder(FrameViewRecorder* parentView);

    void finishRendering(); // waits for all posted fences
    void postFence(FiberSemaphore fence, bool localFence=false);

private:
    FrameViewRecorder* m_parentView = nullptr;

    SpinLock m_fenceListLock;
    Array<FiberSemaphore> m_fences;
};

///---

class FrameHelperDebug;
class FrameHelperCompose;
class FrameHelperOutline;

class FrameHelper : public NoCopy
{
public:
	FrameHelper(gpu::IDevice* dev);
	~FrameHelper();

	const FrameHelperDebug* debug = nullptr;
	const FrameHelperCompose* compose = nullptr;
    const FrameHelperOutline* outline = nullptr;
};

///---

/// a stack-based renderer created to facilitate rendering of a single frame
class ENGINE_RENDERING_API FrameRenderer : public NoCopy
{
public:
    FrameRenderer(const FrameParams& frame, const gpu::AcquiredOutput& output, const FrameResources& resources, const FrameHelper& helpers, const Scene* scene);
    ~FrameRenderer();

    //--

    INLINE LinearAllocator& allocator() { return m_allocator; }

    INLINE const FrameParams& frame() const { return m_frame; }

	INLINE const gpu::AcquiredOutput& output() const { return m_output; }

    INLINE const FrameResources& resources() const { return m_resources; }
	INLINE const FrameHelper& helpers() const { return m_helpers; }

    INLINE uint32_t width() const { return m_frame.resolution.width; }
    INLINE uint32_t height() const { return m_frame.resolution.height; }

    INLINE const Scene* scene() const { return m_scene; }

    //--

    bool usesMultisamping() const;

    //--

    void prepare(gpu::CommandWriter& cmd);
    void finish(gpu::CommandWriter& cmd, FrameStats& outStats);

    //--


    void bindFrameParameters(gpu::CommandWriter& cmd) const;

private:
    bool m_msaa = false;

    const Scene* m_scene = nullptr;

    LinearAllocator m_allocator;

    const gpu::AcquiredOutput& m_output;

    const FrameParams& m_frame;
	const FrameResources& m_resources;
	const FrameHelper& m_helpers;

    //--
};

///---

END_BOOMER_NAMESPACE_EX(rendering)
