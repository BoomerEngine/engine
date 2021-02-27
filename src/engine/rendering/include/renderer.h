/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "params.h"

#include "gpu/device/include/framebuffer.h"

#include "core/fibers/include/fiberWaitList.h"
#include "core/memory/include/linearAllocator.h"
#include "core/reflection/include/variantTable.h"
#include "core/memory/include/pageCollection.h"
#include "stats.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

///---

struct FragmentRenderContext;
struct FrameCompositionTarget;
class FrameResources;

///---

struct FrameViewRecorder
{
    FrameViewRecorder(FrameViewRecorder* parentView);

    void finishRendering(); // waits for all posted fences
    void postFence(fibers::WaitCounter fence, bool localFence=false);

private:
    FrameViewRecorder* m_parentView = nullptr;

    SpinLock m_fenceListLock;
    Array<fibers::WaitCounter> m_fences;
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
    FrameRenderer(const FrameParams& frame, const FrameCompositionTarget& target, const FrameResources& resources, const FrameHelper& helpers);
    ~FrameRenderer();

    //--

    INLINE mem::LinearAllocator& allocator() { return m_allocator; }

    INLINE const FrameParams& frame() const { return m_frame; }

	INLINE const FrameCompositionTarget& target() const { return m_target; }

    INLINE const FrameResources& resources() const { return m_resources; }
	INLINE const FrameHelper& helpers() const { return m_helpers; }

    INLINE const FrameStats& frameStats() const { return m_frameStats; } // frame only stats
    INLINE const SceneStats& scenesStats() const { return m_mergedSceneStats; } // merged from all scenes

    INLINE uint32_t width() const { return m_frame.resolution.width; }
    INLINE uint32_t height() const { return m_frame.resolution.height; }

    //--

    bool usesMultisamping() const;

    //--

    void prepareFrame(gpu::CommandWriter& cmd);
    void finishFrame();

    //--


    void bindFrameParameters(gpu::CommandWriter& cmd) const;

private:
    bool m_msaa = false;

    mem::LinearAllocator m_allocator;

    const FrameParams& m_frame;
	const FrameCompositionTarget& m_target;
	const FrameResources& m_resources;
	const FrameHelper& m_helpers;

    FrameStats m_frameStats;
    SceneStats m_mergedSceneStats;

    //--
};

///---

END_BOOMER_NAMESPACE_EX(rendering)
