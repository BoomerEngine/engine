/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiGraphicsPassLayout.h"

namespace rendering
{
    namespace api
    {
		//--

		IBaseGraphicsPassLayout::IBaseGraphicsPassLayout(IBaseThread* owner, const GraphicsPassLayoutSetup& setup)
			: IBaseObject(owner, ObjectType::GraphicsPassLayout)
			, m_setup(setup)
		{
			m_key = setup.key();
		}

		IBaseGraphicsPassLayout::~IBaseGraphicsPassLayout()
		{}

		//--

    } // api
} // rendering
