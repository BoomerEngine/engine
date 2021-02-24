/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"
#include "renderingTestScene.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"

BEGIN_BOOMER_NAMESPACE(rendering::test)

/// test of rendering to cubemap
class RenderingTest_RenderToCube : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_RenderToCube, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

	virtual void queryInitialCamera(base::Vector3& outPosition, base::Angles& outRotation) override
	{
		outRotation = base::Angles(40.0f, 0.0f, 0);
		outPosition = base::Vector3(0.8f, 0.8f, 0.0f) - outRotation.forward() * 1.2f;
	}

	virtual void describeSubtest(base::IFormatStream& f) override
	{
		switch (subTestIndex())
		{
			case 0: f << "ClearOnly"; break;
			case 1: f << "SeparateFaces"; break;
			case 2: f << "GeometryShaderLayer"; break;
		}
	}

private:
    SimpleScenePtr m_scene;
	SimpleScenePtr m_sceneSphere;

	ImageObjectPtr m_shadowMap;
	RenderTargetViewPtr m_shadowMapRTV;
	ImageSampledViewPtr m_shadowMapSRV;

	ImageObjectPtr m_cubeMap;
	RenderTargetViewPtr m_cubeMapSeparateRTV[6];
	RenderTargetViewPtr m_cubeMapArrayRTV;
	ImageSampledViewPtr m_cubeMapSRV;

	ImageObjectPtr m_cubeMapDepth;
	RenderTargetViewPtr m_cubeMapDepthSeparateRTV[6];
	RenderTargetViewPtr m_cubeMapDepthArrayRTV;

    GraphicsPipelineObjectPtr m_shaderDrawToShadowMap;
	GraphicsPipelineObjectPtr m_shaderDrawToReflectionCube;
	GraphicsPipelineObjectPtr m_shaderDrawToReflectionCubeGS;
	GraphicsPipelineObjectPtr m_shaderReflection;
	GraphicsPipelineObjectPtr m_shaderScene;

	base::Vector3 m_reflectionCenter;
	base::Vector3 m_initialReflectionCenter;

	int m_cubeRenderMode = 1;

	//--
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_RenderToCube);
    RTTI_METADATA(RenderingTestOrderMetadata).order(3010);
    RTTI_METADATA(RenderingTestSubtestCountMetadata).count(3);
RTTI_END_TYPE();

///---

void RenderingTest_RenderToCube::initialize()
{
	m_initialReflectionCenter = base::Vector3(0.8f, 0.8f, 0.3f);
	m_reflectionCenter = m_initialReflectionCenter;

    m_scene = CreateManyTeapotsScene(*this, 3);
	m_sceneSphere = CreateSphereScene(*this, m_reflectionCenter, 0.3f);

	{
		GraphicsRenderStatesSetup render;
		render.depth(true);
		render.depthWrite(true);
		render.depthFunc(CompareOp::Less);
		render.cull(false);
		render.depthBias(true);
		render.depthBiasValue(500);
		render.depthBiasSlope(1.0f);
		m_shaderDrawToShadowMap = loadGraphicsShader("GenericScene.csl", &render);
	}

	{
		GraphicsRenderStatesSetup render;
		render.depth(true);
		render.depthWrite(true);
		render.depthFunc(CompareOp::Less);
		render.cull(false);
		m_shaderDrawToReflectionCube = loadGraphicsShader("GenericScenePoisonShadows.csl", &render);
		m_shaderDrawToReflectionCubeGS = loadGraphicsShader("GenericScenePoisonShadowsGS.csl", &render);
	}

	{
		GraphicsRenderStatesSetup render;
		render.depth(true);
		render.depthWrite(true);
		render.depthFunc(CompareOp::LessEqual);
		m_shaderScene = loadGraphicsShader("GenericScenePoisonShadows.csl", &render);
		//m_shaderScene = loadGraphicsShader("GenericScene.csl", m_shadowMapPassLayout, &render);
	}

	{
		GraphicsRenderStatesSetup render;
		render.depth(true);
		render.depthWrite(true);
		render.depthFunc(CompareOp::LessEqual);
		m_shaderReflection = loadGraphicsShader("GenericSceneReflection.csl", &render);
		//m_shaderScene = loadGraphicsShader("GenericScene.csl", m_shadowMapPassLayout, &render);
	}

	// create shadowmap
	{
		ImageCreationInfo info;
		info.allowRenderTarget = true;
		info.allowShaderReads = true;
		info.format = rendering::ImageFormat::D32;
		info.width = 1024;
		info.height = 1024;
		m_shadowMap = createImage(info);
		m_shadowMapRTV = m_shadowMap->createRenderTargetView();
		m_shadowMapSRV = m_shadowMap->createSampledView();
	}

	// cubemap
	{
		ImageCreationInfo info;
		info.allowRenderTarget = true;
		info.allowShaderReads = true;
		info.format = rendering::ImageFormat::RGBA8_UNORM;
		info.view = ImageViewType::ViewCube;
		info.width = 512;
		info.height = 512;
		info.numSlices = 6;
		m_cubeMap = createImage(info);
		m_cubeMapArrayRTV = m_cubeMap->createRenderTargetView(0, 0, 6);

		for (uint32_t i=0; i<6; ++i)
			m_cubeMapSeparateRTV[i] = m_cubeMap->createRenderTargetView(0, i, 1);

		m_cubeMapSRV = m_cubeMap->createSampledView();
	}

	// cubemap depth
	{
		ImageCreationInfo info;
		info.allowRenderTarget = true;
		info.allowShaderReads = true;
		info.format = rendering::ImageFormat::D32;
		info.view = ImageViewType::ViewCube;
		info.width = 512;
		info.height = 512;
		info.numSlices = 6;
		m_cubeMapDepth = createImage(info);
		m_cubeMapDepthArrayRTV = m_cubeMapDepth->createRenderTargetView(0, 0, 6);

		for (uint32_t i = 0; i < 6; ++i)
			m_cubeMapDepthSeparateRTV[i] = m_cubeMapDepth->createRenderTargetView(0, i, 1);
	}

	m_cubeRenderMode = subTestIndex();
}

struct ShadowMapParams
{
    base::Matrix WorldToShadowmap;
};

void RenderingTest_RenderToCube::render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepth)
{
	// rotate the teapots
	for (uint32_t i=1; i< m_scene->m_objects.size(); ++i)
	{
		auto& teapot = m_scene->m_objects[i];
		const auto rot = teapot.m_initialYaw + time * 20.0f;
		teapot.m_params.LocalToWorld = base::Matrix::BuildRotation(base::Angles(0.0f, rot, 0.0f));
		teapot.m_params.LocalToWorld.translation(teapot.m_initialPos);
	}

	// move the reflection cube
	{
		const auto phaseLength = 10.0f;
		const auto phaseIndex = (int)std::floor(time / phaseLength);
		const auto phaseFrac = fmodf(time / phaseLength, 1.0f);

		const auto displacementLength = 2.0f;
		const auto displacementDelta = (1.0f - abs(cosf(PI * phaseFrac))) * displacementLength;

		base::Vector3 displacement(0, 0, 0);
		switch (phaseIndex % 4)
		{
		case 0: displacement.x = displacementDelta; break;
		case 1: displacement.y = displacementDelta; break;
		case 2: displacement.x = -displacementDelta; break;
		case 3: displacement.y = -displacementDelta; break;
		}

		m_reflectionCenter = m_initialReflectionCenter + displacement;

		auto& sphere = m_sceneSphere->m_objects[0];
		sphere.m_params.LocalToWorld.identity();
		sphere.m_params.LocalToWorld.translation(m_reflectionCenter);
	}

    // animate the light
    const float angle = 200.0f + 30.0f * std::sinf(time * 0.2f);
    m_scene->m_lightPosition = base::Angles(-40.0f, angle, 0.0f).forward();

    // render shadowmap
    {
        SceneCamera camera;
		camera.setupDirectionalShadowmap(base::Vector3::ZERO(), m_scene->m_lightPosition);
		camera.calcMatrices();

        {
            FrameBuffer fb;
            fb.depth.view(m_shadowMapRTV).clearDepth(1.0f).clearStencil(0);

            cmd.opBeingPass(fb);
            m_scene->draw(cmd, m_shaderDrawToShadowMap, camera);
			m_sceneSphere->draw(cmd, m_shaderDrawToShadowMap, camera);
            cmd.opEndPass();
        }

		cmd.opTransitionLayout(m_shadowMap, ResourceLayout::DepthWrite, ResourceLayout::ShaderResource);

        auto biasMatrix = base::Matrix(
            0.5f, 0.0f, 0.0f, 0.5f,
            0.0f, 0.5f, 0.0f, 0.5f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );

        ShadowMapParams params;
        params.WorldToShadowmap = camera.m_WorldToScreen * biasMatrix;

		DescriptorEntry desc[3];
		desc[0].constants(params);
		desc[1] = Globals().SamplerBiLinearDepthLE;
		desc[2] = m_shadowMapSRV;
        cmd.opBindDescriptor("SceneShadowMapParams"_id, desc);
    }

	static const base::Vector4 CLEAR_COLORS[6] = {
		base::Vector4(1.0f, 0.5f, 0.5f, 1.0f),
		base::Vector4(0.0f, 0.5f, 0.5f, 1.0f),
		base::Vector4(0.5f, 1.0f, 0.5f, 1.0f),
		base::Vector4(0.5f, 0.0f, 0.5f, 1.0f),
		base::Vector4(0.5f, 0.5f, 1.0f, 1.0f),
		base::Vector4(0.5f, 0.5f, 0.0f, 1.0f)
	};

	// render reflections
	if (m_cubeRenderMode == 0)
	{
		for (uint32_t i = 0; i < 6; ++i)
		{
			cmd.opClearRenderTarget(m_cubeMapSeparateRTV[i], CLEAR_COLORS[i]);
			cmd.opClearDepthStencil(m_cubeMapDepthSeparateRTV[i], true, true, 1.0f, 0);
		}
	}
	else if (m_cubeRenderMode == 1)
	{
		// mode 1 - render each face separately
		for (uint32_t i = 0; i < 6; ++i)
		{					
			FrameBuffer fb;
			fb.color[0].view(m_cubeMapSeparateRTV[i]).clear(0.1f, 0.1f, 0.1f, 1.0f);
			fb.depth.view(m_cubeMapDepthSeparateRTV[i]).clearDepth(1.0f).clearStencil(0);

			{
				cmd.opBeingPass(fb);

				SceneCamera camera;
				camera.setupCubemap(m_reflectionCenter, i);
				camera.calcMatrices();

				m_scene->draw(cmd, m_shaderDrawToReflectionCube, camera);

				cmd.opEndPass();
			}
		}
	}
	else if (m_cubeRenderMode == 2)
	{
		// mode 2 - render to all faces at the same time
		FrameBuffer fb;
		fb.color[0].view(m_cubeMapArrayRTV).clear(0.1f, 0.1f, 0.1f, 1.0f);
		fb.depth.view(m_cubeMapDepthArrayRTV).clearDepth(1.0f).clearStencil(0);

		{
			cmd.opBeingPass(fb);

			SceneCamera cameras[6];
			for (uint32_t i = 0; i < 6; ++i)
			{
				cameras[i].setupCubemap(m_reflectionCenter, i);
				cameras[i].calcMatrices();
			}

			m_scene->draw(cmd, m_shaderDrawToReflectionCubeGS, cameras, 6);

			cmd.opEndPass();
		}
	}

	// bind reflections
	DescriptorEntry desc[2];
	desc[0].constants(m_reflectionCenter);
	desc[1] = m_cubeMapSRV;
	cmd.opBindDescriptor("ReflectionParams"_id, desc);

	// transition for rendering
	cmd.opTransitionLayout(m_cubeMap, ResourceLayout::RenderTarget, ResourceLayout::ShaderResource);

	// main rendering
	{
		SceneCamera camera;
		camera.aspect = backBufferView->width() / (float)backBufferView->height();
		camera.position = m_cameraPosition;
		camera.rotation = m_cameraAngles.toQuat();
		camera.calcMatrices(backBufferView->flipped());

		FrameBuffer fb;
		fb.color[0].view(backBufferView).clear(0.0f, 0.0f, 0.2f, 1.0f);
		fb.depth.view(backBufferDepth).clearDepth(1.0f).clearStencil(0);

		cmd.opBeingPass(fb);

		// render scene
		m_scene->draw(cmd, m_shaderScene, camera);

		// render reflection sphere
		m_sceneSphere->draw(cmd, m_shaderReflection, camera);

		cmd.opEndPass();
	}
}

END_BOOMER_NAMESPACE(rendering::test)