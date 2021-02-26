/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#pragma once

#include "core/containers/include/stringBuf.h"
#include "core/object/include/rttiProperty.h"

BEGIN_BOOMER_NAMESPACE_EX(reflection)

// helper class that can add stuff to the class type
class CORE_REFLECTION_API PropertyBuilder : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_RTTI)

public:
    PropertyBuilder(Type propType, const char* name, const char* category, uint32_t offset);
    ~PropertyBuilder();

    void submit(rtti::IClassType* targetClass);

    INLINE PropertyBuilder& name(const char* name)
    {
        DEBUG_CHECK_EX(name != nullptr && *name, "Custom name cannot be empty");
        m_name = StringID(name);
        return *this;
    }

    INLINE PropertyBuilder& category(const char* category)
    {
        m_category = StringID(category);
        return *this;
    }

    INLINE PropertyBuilder& editable(const char* hint = "")
    {
        m_editable = true;
        m_editorData.m_comment = hint;
        return *this;
    }

    INLINE PropertyBuilder& readonly()
    {
        m_editable = true;
        m_readonly = true;
        return *this;
    }

    INLINE PropertyBuilder& inlined()
    {
        m_inlined = true;
        return *this;
    }

    INLINE PropertyBuilder& transient()
    {
        m_transient = true;
        return *this;
    }

    INLINE PropertyBuilder& noScripts()
    {
        m_scriptHidden = true;
        return *this;
    }

    INLINE PropertyBuilder& scriptReadOnly()
    {
        m_scriptReadOnly = true;
        return *this;
    }

    INLINE PropertyBuilder& overriddable()
    {
        m_overriddable = true;
        return *this;
    }

    INLINE PropertyBuilder& noReset()
    {
        m_noReset = true;
        return *this;
    }

    template< typename T, typename... Args >
    INLINE PropertyBuilder& metadata(Args && ... args)
    {
        auto obj = new T(std::forward< Args >(args)...);
        m_metadata.pushBack(static_cast<rtti::IMetadata*>(obj));
        return *this;
    }

    INLINE PropertyBuilder& editor(StringID id)
    {
        m_editorData.m_customEditor = id;
        return *this;
    }

    INLINE PropertyBuilder& comment(StringView txt)
    {
        m_editorData.m_comment = StringBuf(txt);
        return *this;
    }

    INLINE PropertyBuilder& units(StringView txt)
    {
        m_editorData.m_units = StringBuf(txt);
        return *this;
    }

    INLINE PropertyBuilder& range(double min, double max)
    {
        m_editorData.m_rangeMin = min;
        m_editorData.m_rangeMax = max;
        return *this;
    }

    INLINE PropertyBuilder& digits(uint8_t numDigits)
    {
        m_editorData.m_digits = numDigits;
        return *this;
    }

    INLINE PropertyBuilder& hasAlpha()
    {
        m_editorData.m_colorWithAlpha = true;
        return *this;
    }

    INLINE PropertyBuilder& noDefaultValue()
    {
        m_editorData.m_noDefaultValue = true;
        return *this;
    }

    INLINE PropertyBuilder& widgetDrag(bool wrap=false)
    {
        m_editorData.m_widgetDrag = true;
        m_editorData.m_widgetDragWrap = wrap;
        return *this;
    }

    INLINE PropertyBuilder& widgetSlider()
    {
        m_editorData.m_widgetSlider = true;
        return *this;
    }

private:
    Type m_type;
    StringID m_name;
    StringID m_category;
    uint32_t m_offset = 0;

    bool m_editable = false;
    bool m_inlined = false;
    bool m_readonly = false;
    bool m_scriptHidden = false;
    bool m_scriptReadOnly = false;
    bool m_transient = false;
    bool m_overriddable = false;
    bool m_noReset = false;

    rtti::PropertyEditorData m_editorData;

    Array<rtti::IMetadata*> m_metadata;
};

END_BOOMER_NAMESPACE_EX(reflection)
