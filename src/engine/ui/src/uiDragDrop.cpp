/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\dragdrop #]
***/

#include "build.h"
#include "uiDragDrop.h"
#include "uiElement.h"
#include "uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

RTTI_BEGIN_TYPE_ENUM(DragDropStatus);
    RTTI_ENUM_OPTION(Ignore);
    RTTI_ENUM_OPTION(Invalid);
    RTTI_ENUM_OPTION(Valid);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDragDropData);
RTTI_END_TYPE();

IDragDropData::IDragDropData()
{}

IDragDropData::~IDragDropData()
{}

ElementPtr IDragDropData::createPreview() const
{
    return nullptr;
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(DragDropData_String);
    RTTI_PROPERTY(m_data);
RTTI_END_TYPE();

DragDropData_String::DragDropData_String(const StringBuf& data)
    : m_data(data)
{}


ElementPtr DragDropData_String::createPreview() const
{
    return RefNew<TextLabel>(m_data);
}


//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDragDropHandler);
RTTI_END_TYPE();

IDragDropHandler::IDragDropHandler(const DragDropDataPtr& data, IElement* target, const Position& initialPosition)
    : m_data(data)
    , m_target(target)
    , m_initialPosition(initialPosition)
    , m_lastPosition(initialPosition)
    , m_lastValidPosition(initialPosition)
{}

IDragDropHandler::~IDragDropHandler()
{}

void IDragDropHandler::handleCancel()
{}

bool IDragDropHandler::canHandleDataAt(const Position& absolutePos) const
{
    return true;
}

bool IDragDropHandler::canHideDefaultDataPreview() const
{
    return false;
}

void IDragDropHandler::handleMouseWheel(const input::MouseMovementEvent &evt, float delta)
{
}

IDragDropHandler::UpdateResult IDragDropHandler::processMouseMovement(const input::MouseMovementEvent& evt)
{
    // if we lost the target we must cancel
    auto target = m_target.lock();
    if (!target)
        return UpdateResult::Cancel;

    if (!target->renderer())
        return UpdateResult::Cancel;

    // update the last tested position
    m_lastPosition = evt.absolutePosition().toVector();

    // test the position
    if (canHandleDataAt(m_lastPosition))
    {
        m_lastValidPosition = m_lastPosition;
        return UpdateResult::Valid;
    }
    else
    {
        return UpdateResult::Invalid;
    }
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(DragDropHandlerGeneric);
RTTI_END_TYPE();

DragDropHandlerGeneric::DragDropHandlerGeneric(const DragDropDataPtr& data, IElement* target, const Position& initialPosition)
    : IDragDropHandler(data, target, initialPosition)
{}

void DragDropHandlerGeneric::handleCancel()
{
    // TODO: maybe the generic interface should have cancel as well ?
}

void DragDropHandlerGeneric::handleData(const Position& absolutePos) const
{
    target()->handleDragDropGenericCompletion(data(), absolutePos);
}

//--

END_BOOMER_NAMESPACE_EX(ui)
