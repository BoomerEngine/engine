/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingFrameView_Main.h"
#include "renderingFrameView_Cascades.h"
#include "renderingFrameResources.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

FrameViewCascadesRecorder::CascadeSlice::CascadeSlice()
    : solid(nullptr)
    , masked(nullptr)
{}

FrameViewCascadesRecorder::FrameViewCascadesRecorder(FrameViewRecorder* parentView)
    : FrameViewRecorder(parentView)
{}

//--

FrameViewCascades::FrameViewCascades(const FrameRenderer& frame, const CascadeData& cascades)
    : m_frame(frame)
    , m_cascades(cascades)
{}

FrameViewCascades::~FrameViewCascades()
{}

void FrameViewCascades::render(gpu::CommandWriter& cmd, FrameViewRecorder* parentView)
{
    // initialize the scaffolding of the view
    FrameViewCascadesRecorder rec(parentView);
    initializeCommandStreams(cmd, rec);

    // render scene into cascades, background scene is not casting global shadows
    if (auto* scene = m_frame.frame().scenes.mainScenePtr)
        scene->renderCascadesView(rec, *this, m_frame);

    // wait for recording jobs to finish
    rec.finishRendering();
}

void FrameViewCascades::initializeCommandStreams(gpu::CommandWriter& cmd, FrameViewCascadesRecorder& rec)
{
    cmd.opBeginBlock("Cascades");

    for (uint32_t i = 0; i < m_cascades.numCascades; ++i)
    {
        const auto& cascadeInfo = m_cascades.cascades[i];
        auto& recSlice = rec.slices[i];

        cmd.opBeginBlock(TempString("Cascade{}", i));

        {
            gpu::FrameBuffer fb;
            fb.depth.view(m_frame.resources().cascadesShadowDepthRTV[i]).clearDepth(1.0f).clearStencil(0);

            cmd.opBeingPass(fb);

            recSlice.solid.attachBuffer(cmd.opCreateChildCommandBuffer());
            recSlice.masked.attachBuffer(cmd.opCreateChildCommandBuffer());

            cmd.opEndPass();
        }

        cmd.opEndBlock();
    }

    cmd.opEndBlock();
}


/*void BindShadowsData(gpu::CommandWriter& cmd, const CascadeData& cascades)
{
	struct
	{
		GPUCascadeInfo cascades;
	} consts;

	PackCascadeData(cascades, consts.cascades);

	DescriptorEntry desc[2];
	desc[0].constants(&consts);
	desc[1] = cascades.cascadesAtlasSRV;
	cmd.opBindDescriptor("ShadowParams"_id, desc);
}*/

//---

#if 0
FrameView_CascadeShadows::FrameView_CascadeShadows(const FrameRenderer& frame, const CascadeData& data)
    : FrameView(frame, FrameViewType::GlobalCascades, data.cascadeShadowMap.width(), data.cascadeShadowMap.height())
    , m_cascadeData(data)
{}

void FrameView_CascadeShadows::render(gpu::CommandWriter& parentCmd)
{
    // collect from multiple frustums
    collectCascadeCameras(frame().camera.camera, m_cascadeData);

    // prepare fragments for the visible objects
    {
        gpu::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer(), "CascadeFragments");
        generateFragments(cmd);
    }

    // render into separate buckets
    for (uint32_t i = 0; i < m_cascadeData.numCascades; ++i)
    {
        gpu::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer(), TempString("CascadeSlice{}", i));

        // bind global settings
        BindSingleCamera(cmd, m_cascadeData.cascades[i].camera);

        // render the shadow depth 
        const auto cascadeDepthMapSliceRT = m_cascadeData.cascadeShadowMap.createSingleSliceView(i);
        const auto& depthBiasSettings = m_cascadeData.cascades[i];
        RenderShadowDepthPass(cmd, *this, cascadeDepthMapSliceRT, depthBiasSettings.depthBiasConstant, depthBiasSettings.depthBiasSlope, 0.0f, i);
    }
}

//--
#endif

//--

END_BOOMER_NAMESPACE_EX(rendering)
