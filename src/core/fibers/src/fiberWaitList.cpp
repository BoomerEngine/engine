/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: public #]
***/

#include "build.h"
#include "fiberWaitList.h"

BEGIN_BOOMER_NAMESPACE_EX(fibers)

WaitList::WaitList()
{
}

WaitList::~WaitList()
{
}

CAN_YIELD void WaitList::sync()
{
	PC_SCOPE_LVL2(WaitListSync);

	uint32_t index = 0;
	for (;;)
	{
		fibers::WaitCounter fence;

		// as long as there are fences get the next one to wait
		{
			auto lock = CreateLock(m_lock);

			if (index >= m_fences.size())
				break;

			fence = std::move(m_fences[index++]);
		}

		// wait for the collector to finish
		// NOTE: this may add more fences
		Fibers::GetInstance().waitForCounterAndRelease(fence);
	}
}

void WaitList::pushFence(WaitCounter fence)
{
	if (fence.empty())
	{
		auto lock = CreateLock(m_lock);
		m_fences.pushBack(fence);
	}
}

END_BOOMER_NAMESPACE_EX(fibers)
