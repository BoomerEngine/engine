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

ActionInplace::ActionInplace(StringView caption, StringID id)
    : m_caption(caption)
    , m_id(id)
{}

ActionInplace::~ActionInplace()
{}

//--

void IAction::BumpSequenceNumber()
{
    GActionSequenceNumber++;
}

//----

END_BOOMER_NAMESPACE()

