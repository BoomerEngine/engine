/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: actions #]
***/

#include "build.h"
#include "action.h"
#include "rttiNativeClassType.h"
#include "rttiTypeSystem.h"

BEGIN_BOOMER_NAMESPACE()

//----

static std::atomic<uint32_t> GActionSequenceNumber(1);

//----

IAction::IAction()
    : m_sequenceNumber(GActionSequenceNumber.load())
{}

IAction::~IAction()
{}

//--

void IAction::BumpSequenceNumber()
{
    GActionSequenceNumber++;
}

//----

END_BOOMER_NAMESPACE()

