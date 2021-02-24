/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "renderingFrameParams.h"

#include "rendering/scene/include/renderingFrameCamera.h"
#include "rendering/device/include/renderingFramebuffer.h"

#include "base/fibers/include/fiberWaitList.h"
#include "base/memory/include/linearAllocator.h"
#include "base/reflection/include/variantTable.h"
#include "base/memory/include/pageCollection.h"
#include "renderingSceneStats.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

///---

struct FragmentRenderContext;
struct FrameCompositionTarget;
class FrameResources;

///---

struct FrameViewRecorder
{
    FrameViewRecorder(FrameViewRecorder* parentView);

    void finishRendering(); // waits for all posted fences
    void postFence(base::fibers::WaitCounter fence, bool localFence=false);

private:
    FrameViewRecorder* m_parentView = nullptr;

    base::SpinLock m_fenceListLock;
    base::Array<base::fibers::WaitCounter> m_fences;
};

///---

class FrameHelperDebug;
class FrameHelperCompose;
class FrameHelperOutline;

class FrameHelper : public base::NoCopy
{
public:
	FrameHelper(IDevice* dev);
	~FrameHelper();

	const FrameHelperDebug* debug = nullptr;
	const FrameHelperCompose* compose = nullptr;
    const FrameHelperOutline* outline = nullptr;
};

///---

/// a stack-based renderer created to facilitate rendering of a single frame
class RENDERING_SCENE_API FrameRenderer : public base::NoCopy
{
public:
    FrameRenderer(const FrameParams& frame, const FrameCompositionTarget& target, const FrameResources& resources, const FrameHelper& helpers);
    ~FrameRenderer();

    //--

    INLINE base::mem::LinearAllocator& allocator() { return m_allocator; }

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

    void prepareFrame(GPUCommandWriter& cmd);
    void finishFrame();

    //--


    void bindFrameParameters(GPUCommandWriter& cmd) const;

private:
    bool m_msaa = false;

    base::mem::LinearAllocator m_allocator;

    const FrameParams& m_frame;
	const FrameCompositionTarget& m_target;
	const FrameResources& m_resources;
	const FrameHelper& m_helpers;

    FrameStats m_frameStats;
    SceneStats m_mergedSceneStats;

    //--
};

///---

END_BOOMER_NAMESPACE(rendering::scene)