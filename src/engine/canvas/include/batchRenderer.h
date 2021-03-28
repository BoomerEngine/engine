/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "gpu/device/include/shaderReloadNotifier.h"

BEGIN_BOOMER_NAMESPACE()

//---

#pragma pack(push)
#pragma pack(4)
struct GPUCanvasImageInfo
{
    Vector2 uvMin;
    Vector2 uvMax;
    Vector2 uvScale;
    Vector2 uvInvScale;
};
#pragma pack(pop)

///---

struct CanvasRenderStates
{
	static const auto MAX_BLEND_OPS = (int)CanvasBlendOp::MAX;

	gpu::GraphicsRenderStatesObjectPtr m_mask = nullptr;
	gpu::GraphicsRenderStatesObjectPtr m_standardFill[MAX_BLEND_OPS];
	gpu::GraphicsRenderStatesObjectPtr m_maskedFill[MAX_BLEND_OPS];
};

///---

/// rendering handler for custom canvas batches
class ENGINE_CANVAS_API ICanvasBatchRenderer : public NoCopy
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ICanvasBatchRenderer);
	RTTI_DECLARE_POOL(POOL_CANVAS);

public:
    virtual ~ICanvasBatchRenderer();

	//--

    /// initialize handler, may allocate resources, load shaders, etc
    virtual bool initialize(const CanvasRenderStates& renderStates, gpu::IDevice* drv) = 0;

	//--

	struct RenderData
	{
		uint32_t width = 0;
		uint32_t height = 0;

		const void* customData = nullptr;

		const gpu::BufferObject* vertexBuffer = nullptr;

		const gpu::ImageSampledView* atlasImage = nullptr;
		const gpu::BufferStructuredView* atlasData = nullptr;

		const gpu::ImageSampledView* glyphImage = nullptr;
		const gpu::BufferStructuredView* glyphData = nullptr;

		CanvasBatchType batchType = CanvasBatchType::FillConvex;
		CanvasBlendOp blendOp = CanvasBlendOp::AlphaPremultiplied;
	};

    /// handle rendering of batches
	virtual void render(gpu::CommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const = 0;
};

//---

class ENGINE_CANVAS_API ICanvasSimpleBatchRenderer : public ICanvasBatchRenderer
{
	RTTI_DECLARE_VIRTUAL_CLASS(ICanvasSimpleBatchRenderer, ICanvasBatchRenderer);

public:
	ICanvasSimpleBatchRenderer();
	virtual ~ICanvasSimpleBatchRenderer();

	// load the default draw shader
	virtual gpu::ShaderObjectPtr loadMainShaderFile() = 0;

protected:
	virtual bool initialize(const CanvasRenderStates& renderStates, gpu::IDevice* drv) override final;
	virtual void render(gpu::CommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const override;

	virtual const gpu::GraphicsPipelineObject* selectShader(gpu::CommandWriter& cmd, const RenderData& data) const;

	void loadShaders();

	static const auto MAX_BLEND_OPS = (int)CanvasBlendOp::MAX;

	gpu::GraphicsPipelineObjectPtr m_mask;
	gpu::GraphicsPipelineObjectPtr m_standardFill[MAX_BLEND_OPS];
	gpu::GraphicsPipelineObjectPtr m_maskedFill[MAX_BLEND_OPS];

	const CanvasRenderStates* m_renderStates = nullptr;

	gpu::ShaderReloadNotifier m_reloadNotifier;
};

//---

/// get the static handler ID
template< typename T >
static uint16_t GetCanvasHandlerIndex()
{
    static const auto id = T::GetStaticClass()->userIndex();
    DEBUG_CHECK_EX(id != INDEX_NONE, "Handled class not properly registered");
    return (uint16_t)id;
}

//--

END_BOOMER_NAMESPACE()
