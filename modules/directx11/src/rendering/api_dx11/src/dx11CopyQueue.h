/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/api_common/include/apiCopyQueue.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			//---

			class CopyQueue : public IBaseCopyQueue
			{
			public:
				CopyQueue(Thread* owner, CopyPool* pool, ObjectRegistry* objects);
				virtual ~CopyQueue();
			};

			//---

		} // dx11
    } // api
} // rendering