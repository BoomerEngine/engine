/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiGraphicsRenderStates.h"

namespace rendering
{
    namespace api
    {
		//--

		IBaseGraphicsRenderStates::IBaseGraphicsRenderStates(IBaseThread* owner, const StaticRenderStatesSetup& setup)
			: IBaseObject(owner, ObjectType::GraphicsRenderStates)
			, m_setup(setup)
		{
			m_key = setup.key();
		}

		IBaseGraphicsRenderStates::~IBaseGraphicsRenderStates()
		{}

		//--

    } // api
} // rendering
