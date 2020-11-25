/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\pipeline #]
***/

#pragma once

#include "renderingObject.h"
#include "renderingResources.h"
#include "renderingStates.h"

namespace rendering
{
    ///---

    // graphics states wrapper, object represents baked basic render states
    class RENDERING_DEVICE_API GraphicsRenderStatesObject : public IDeviceObject
    {
		RTTI_DECLARE_VIRTUAL_CLASS(GraphicsRenderStatesObject, IDeviceObject);

    public:
		GraphicsRenderStatesObject(ObjectID id, IDeviceObjectHandler* impl, const StaticRenderStatesSetup& states, uint64_t key);
        virtual ~GraphicsRenderStatesObject();

		//--

		// rendering states we used to generate this from
		INLINE const StaticRenderStatesSetup& states() const { return m_states; }

		// merged "key" (64-bit hash) that represents the graphical render stats
		INLINE uint64_t key() const { return m_key; }

		//--
	
	protected:
		uint64_t m_key = 0;
		StaticRenderStatesSetup m_states;
    };

	///---

	// graphic pass layout settings - render target formats and multisampling
	class RENDERING_DEVICE_API GraphicsPassLayoutObject : public IDeviceObject
	{
		RTTI_DECLARE_VIRTUAL_CLASS(GraphicsPassLayoutObject, IDeviceObject);

	public:
		GraphicsPassLayoutObject(ObjectID id, IDeviceObjectHandler* impl, const GraphicsPassLayoutSetup& states, uint64_t key);
		virtual ~GraphicsPassLayoutObject();

		//--

		// rendering states we used to generate this from
		INLINE const GraphicsPassLayoutSetup& layout() const { return m_layout; }

		// merged "key" (64-bit hash) that represents the graphical render stats
		INLINE uint64_t key() const { return m_key; }

		// do we have depth buffer ?
		INLINE bool hasDepth() const { return m_layout.depth; }

		// do we have depth only ?
		INLINE bool depthOnly() const { return !m_layout.color[0]; }

		// are we multisampled ?
		INLINE bool multisampled() const { return m_layout.samples > 1; }

		//--

	protected:
		uint64_t m_key = 0;
		GraphicsPassLayoutSetup m_layout;
	};

	///---

	/// compiled graphics pipeline - all render settings required to render with shaders into render targets
	class RENDERING_DEVICE_API GraphicsPipelineObject : public IDeviceObject
	{
		RTTI_DECLARE_VIRTUAL_CLASS(GraphicsPipelineObject, IDeviceObject);

	public:
		GraphicsPipelineObject(ObjectID id, IDeviceObjectHandler* impl, const GraphicsPassLayoutObject* passLayout, const GraphicsRenderStatesObject* renderStats, const ShaderObject* shaders);
		virtual ~GraphicsPipelineObject();

		// layout of the render targets
		INLINE const GraphicsPassLayoutObjectPtr& passLayout() const { return m_passLayout; }

		// render states
		INLINE const GraphicsRenderStatesObjectPtr& renderStats() const { return m_renderStats; }

		// source shaders
		INLINE const ShaderObjectPtr& shaders() const { return m_shaders; }

	public:
		GraphicsPassLayoutObjectPtr m_passLayout;
		GraphicsRenderStatesObjectPtr m_renderStats;
		ShaderObjectPtr m_shaders;
	};

	///---

	/// compiled compute pipeline
	class RENDERING_DEVICE_API ComputePipelineObject : public IDeviceObject
	{
		RTTI_DECLARE_VIRTUAL_CLASS(ComputePipelineObject, IDeviceObject);

	public:
		ComputePipelineObject(ObjectID id, IDeviceObjectHandler* impl, const ShaderObject* shaders);
		virtual ~ComputePipelineObject();

		// source shaders
		INLINE const ShaderObjectPtr& shaders() const { return m_shaders; }

	public:
		ShaderObjectPtr m_shaders;
	};

	///---

} // rendering