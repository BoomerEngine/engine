/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "apiObject.h"

#include "rendering/device/include/renderingGraphicsStates.h"

BEGIN_BOOMER_NAMESPACE(rendering::api)

//---

/// general object for complete static render states setup for graphics pipeline
class RENDERING_API_COMMON_API IBaseGraphicsRenderStates : public IBaseObject
{
public:
	IBaseGraphicsRenderStates(IBaseThread* owner, const rendering::GraphicsRenderStatesSetup& setup);
	virtual ~IBaseGraphicsRenderStates();

	static const auto STATIC_TYPE = ObjectType::GraphicsRenderStates;

	//--

	INLINE const uint64_t key() const { return m_key; }
	INLINE const GraphicsRenderStatesSetup& setup() const { return m_setup; }

	//--

private:
	uint64_t m_key = 0;
	GraphicsRenderStatesSetup m_setup;
};

//---

END_BOOMER_NAMESPACE(rendering::api)