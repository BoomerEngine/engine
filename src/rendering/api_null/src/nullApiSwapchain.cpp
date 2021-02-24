/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiThread.h"
#include "nullApiSwapchain.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::nul)

//--

Swapchain::Swapchain(OutputClass cls, const WindowSetup& setup)
	: IBaseWindowedSwapchain(cls, setup)
{}

Swapchain::~Swapchain()
{
	DEBUG_CHECK_EX(m_bound == false, "Destroying swapchain while in use");
}

bool Swapchain::acquire()
{
	DEBUG_CHECK_RETURN_EX_V(m_bound == false, "Swapchain not swapped", false);
	m_bound = true;
	return true;
}

void Swapchain::present(bool forReal)
{
	DEBUG_CHECK_RETURN_EX(m_bound == true, "Swapping swapchain that was not bound");
	m_bound = false;
}

//--

END_BOOMER_NAMESPACE(rendering::api::nul)