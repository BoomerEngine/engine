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

			class CopyPool : public IBaseStagingPool
			{
			public:
				CopyPool(uint32_t size, uint32_t pageSize);
				virtual ~CopyPool();

			private:
				
			};

			//---

		} // dx11
    } // api
} // rendering