/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\pass  #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneUtils.h"
#include "renderingSceneFragment.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameSurfaceCache.h"
#include "renderingFrameParams.h"
#include "renderingFrameView.h"

namespace rendering
{
    namespace scene
    {
        //---

        base::ConfigProperty<base::Color> cvRenderingDefaultClearColor("Rendering.Scene", "DefaultClearColor", base::Color(50, 50, 50, 255));

        //---

        PassBracket::PassBracket(command::CommandWriter& cmd, const FrameView& view, base::StringView<char> name)
            : m_cmd(cmd)
            , m_view(view)
        {
            cmd.opBeginBlock(name);

            m_viewport.minDepthRange = 0.0f;
            m_viewport.maxDepthRange = 1.0f;
            m_viewport.viewportRect = base::Rect(0, 0, view.frame().resolution.width, view.frame().resolution.height);
        }

        void PassBracket::depthClear(const ImageView& rt)
        {
            DEBUG_CHECK(!rt.empty());
            DEBUG_CHECK(rt.renderTargetDepth());

            m_fb.depth.rt = rt;
            m_fb.depth.loadOp = LoadOp::Clear;
            m_fb.depth.clearDepth(1.0f);
            m_fb.depth.clearStencil(0);
        }

        void PassBracket::colorClear(uint8_t index, const ImageView& rt, const base::Vector4& clearValues)
        {
            DEBUG_CHECK(!rt.empty());
            DEBUG_CHECK(rt.renderTarget());

            m_fb.color[index].rt = rt;
            m_fb.color[index].loadOp = LoadOp::Clear;
            m_fb.color[index].clearColorValues[0] = clearValues.x;
            m_fb.color[index].clearColorValues[1] = clearValues.y;
            m_fb.color[index].clearColorValues[2] = clearValues.z;
            m_fb.color[index].clearColorValues[3] = clearValues.w;
        }

        void PassBracket::colorClear(uint8_t index, const ImageView& rt)
        {
            const auto clearColor = (index == 0) ? cvRenderingDefaultClearColor.get().toVectorSRGB() : base::Vector4(0, 0, 0, 0);
            colorClear(index, rt, clearColor);
        }

        void PassBracket::begin()
        {
            DEBUG_CHECK(!m_hasStartedPass);
            m_cmd.opBeingPass(m_fb, 1, &m_viewport);
            m_hasStartedPass = true;
        }

        PassBracket::~PassBracket()
        {
            if (m_hasStartedPass)
                m_cmd.opEndPass();

            m_cmd.opEndBlock();
        }

        //---

    } // scene
} // rendering

