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
		namespace nul
		{

			//---

			class CopyQueue : public IBaseCopyQueue
			{
			public:
				CopyQueue(Thread* owner, ObjectRegistry* objects);
				virtual ~CopyQueue();

				virtual bool schedule(IBaseCopiableObject* ptr, const ISourceDataProvider* sourceData) override;
				virtual void update(uint64_t frameIndex) override;
			};

			//---

		} // nul
    } // api
} // rendering