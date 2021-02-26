/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiGraphicsRenderStates.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

IBaseGraphicsRenderStates::IBaseGraphicsRenderStates(IBaseThread* owner, const GraphicsRenderStatesSetup& setup)
	: IBaseObject(owner, ObjectType::GraphicsRenderStates)
	, m_setup(setup)
{
	m_key = setup.key();
}

IBaseGraphicsRenderStates::~IBaseGraphicsRenderStates()
{}

//--

END_BOOMER_NAMESPACE_EX(gpu::api)
