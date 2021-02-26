/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles #]
***/

#include "build.h"
#include "uiElement.h"
#include "uiStyleSelector.h"

BEGIN_BOOMER_NAMESPACE_EX(ui::style)

//---

RTTI_BEGIN_TYPE_ENUM(SelectorCombinatorType);
    RTTI_ENUM_OPTION(AnyParent);
    RTTI_ENUM_OPTION(DirectParent);
    RTTI_ENUM_OPTION(AdjecentSibling);
    RTTI_ENUM_OPTION(GeneralSibling);
RTTI_END_TYPE();

//---

SelectorMatchContext::SelectorMatchContext(StringID id, SpecificClassType<IElement> elementClass)
    : m_id(id)
{
    while (elementClass)
    {
        const auto* metaData = elementClass->MetadataContainer::metadata(ElementClassNameMetadata::GetStaticClass());
        if (metaData != nullptr)
        {
            const auto* classNameMetaData = static_cast<const ElementClassNameMetadata*>(metaData);
            m_types.pushBackUnique(StringID(classNameMetaData->name()));
        }

        elementClass = elementClass->baseClass().cast<IElement>();
    }

    updateKey();
}

bool SelectorMatchContext::updateKey()
{
    CRC64 crc;

    if (m_id)
    {
        crc << (uint8_t)1;
        crc << m_id;
    }

    for (auto val : m_types)
    {
        crc << (uint8_t)2;
        crc << val;
    }

    for (auto val : m_classes)
    {
        crc << (uint8_t)3;
        crc << val;
    }

    for (auto val : m_pseudoClasses)
    {
        crc << (uint8_t)4;
        crc << val;
    }

    auto old = m_key;
    m_key = crc;
    return (old != m_key);
}

bool SelectorMatchContext::id(StringID name)
{
    if (m_id != name)
    { 
        m_id = name;
        return updateKey();
    }

    return false;
}

bool SelectorMatchContext::hasClass(StringID name) const
{
    return m_classes.contains(name);
}

bool SelectorMatchContext::addClass(StringID name)
{
    if (m_classes.contains(name))
        return false;

    m_classes.pushBack(name);
    std::sort(m_classes.begin(), m_classes.end());
    return updateKey();
}

bool SelectorMatchContext::removeClass(StringID name)
{
    if (m_classes.remove(name))
        return updateKey();
    return false;
}

bool SelectorMatchContext::hasPseudoClass(StringID name) const
{
    return m_pseudoClasses.contains(name);
}

bool SelectorMatchContext::addPseudoClass(StringID name)
{
    if (m_pseudoClasses.contains(name))
        return false;

    m_pseudoClasses.pushBack(name);
    std::sort(m_pseudoClasses.begin(), m_pseudoClasses.end());
    return updateKey();
}

bool SelectorMatchContext::removePseudoClass(StringID name)
{
    if (m_pseudoClasses.remove(name))
        return updateKey();
    return false;
}

bool SelectorMatchContext::customType(StringID type)
{
    if (customType() != type)
    {
        if (m_hasCustomTypeName)
        {
            m_types.popBack();
            m_hasCustomTypeName = false;
        }

        if (type)
        {
            m_hasCustomTypeName = true;
            m_types.pushBack(type);
        }

        std::sort(m_types.begin(), m_types.end());
        return updateKey();
    }

    return false;
}

StringID SelectorMatchContext::customType() const
{
    return m_hasCustomTypeName ? m_types.back() : StringID::EMPTY();
}

void SelectorMatchContext::print(IFormatStream& f) const
{
    for (auto& val : m_types)
        f << "Type:" << val << " ";
            
    for (auto& val : m_classes)
        f << "Class:" << val << " ";

    for (auto& val : m_pseudoClasses)
        f << "PseudoClass:" << val << " ";

    if (m_id)
        f << "ID:" << m_id;
}

//---

SelectorMatch::SelectorMatch()
    : m_hash(0)
{}

SelectorMatch::SelectorMatch(Array<SelectorID>&& ids, uint64_t hash)
    : m_matchedSelectors(std::move(ids))
    , m_hash(hash)
{}

static SelectorMatch GEmpty;

const SelectorMatch& SelectorMatch::EMPTY()
{
    return GEmpty;
}

//---

RTTI_BEGIN_TYPE_CLASS(SelectorMatchParams);
    RTTI_PROPERTY(m_type);
    RTTI_PROPERTY(m_class);
    RTTI_PROPERTY(m_class2);
    RTTI_PROPERTY(m_id);
    RTTI_PROPERTY(m_pseudoClass);
    RTTI_PROPERTY(m_pseudoClass2);
RTTI_END_TYPE();

SelectorMatchParams::SelectorMatchParams()
{}

SelectorMatchParams::SelectorMatchParams(const StringID type, const StringID clas, const StringID clas2, const StringID id, const StringID pseudoClass, const StringID pseudoClass2)
    : m_type(type)
    , m_class(clas)
    , m_class2(clas2)
    , m_id(id)
    , m_pseudoClass(pseudoClass)
    , m_pseudoClass2(pseudoClass2)
{}

void SelectorMatchParams::calcHash(CRC64& crc) const
{
    crc << m_type;
    crc << m_class;
    crc << m_class2;
    crc << m_pseudoClass;
    crc << m_pseudoClass2;
    crc << m_id;
}

void SelectorMatchParams::print(IFormatStream& f) const
{
    if (m_type)
        f << m_type;
    else
        f << "*";

    if (m_class)
        f << "." << m_class;

    if (m_class2)
        f << "." << m_class2;

    if (m_pseudoClass)
        f << ":" << m_pseudoClass;

    if (m_pseudoClass2)
        f << ":" << m_pseudoClass2;

    if (m_id)
        f << "#" << m_id;
}

static bool HasStringID(StringID name, const StringID* namePtr, uint32_t count)
{
    if (name)
    {
        auto* endNamePtr = namePtr + count;
        while (namePtr < endNamePtr)
            if (*namePtr++ == name)
                return true;
    }
    return !name;
}

bool SelectorMatchParams::matches(const SelectorMatchContext& ctx) const
{
    bool ret = false;

    if (m_id.empty() || m_id == ctx.id())
    {
        if (HasStringID(m_type, ctx.types(), ctx.numTypes()))
        {
            if (HasStringID(m_class, ctx.classes(), ctx.numClasses()))
            {
                if (HasStringID(m_class2, ctx.classes(), ctx.numClasses()))
                {
                    return HasStringID(m_pseudoClass, ctx.pseudoClasses(), ctx.numPseudoClasses());
                }
            }
        }
    }

    //TRACE_INFO("{} {} to {}", ret ? "MATCHED" : "not matched", *this, ctx);
    return ret;
}

//---

RTTI_BEGIN_TYPE_CLASS(SelectorParam);
    RTTI_PROPERTY(m_paramName);
    RTTI_PROPERTY(m_valueId);
RTTI_END_TYPE();

//---

RTTI_BEGIN_TYPE_CLASS(SelectorNode);
    RTTI_PROPERTY(m_combinator);
    RTTI_PROPERTY(m_parentId);
    RTTI_PROPERTY(m_parameters);
    RTTI_PROPERTY(m_matchParameters);
RTTI_END_TYPE();

SelectorNode::SelectorNode()
    : m_combinator(SelectorCombinatorType::DirectParent)
    , m_parentId(0)
{}

SelectorNode::SelectorNode(SelectorID parentNodeId, const Array<SelectorParam>& params, const SelectorMatchParams& matchParameters, SelectorCombinatorType combinator)
    : m_combinator(combinator)
    , m_parentId(parentNodeId)
    , m_parameters(params)
    , m_matchParameters(matchParameters)
{}

//---

END_BOOMER_NAMESPACE_EX(ui::style)
