/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "uiDataBoxText.h"
#include "uiEditBox.h"
#include "base/object/include/rttiDataView.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDataBoxText);
    RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxText");
RTTI_END_TYPE();

IDataBoxText::IDataBoxText()
{
    auto flags = ui::EditBoxFeatureFlags({ ui::EditBoxFeatureBit::AcceptsEnter, ui::EditBoxFeatureBit::NoBorder });

    m_editBox = createChild<EditBox>(flags);
    m_editBox->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    m_editBox->customVerticalAligment(ui::ElementVerticalLayout::Expand);

    m_editBox->bind(EVENT_TEXT_ACCEPTED) = [this]()
    {
        if (m_editBox->isEnabled())
            write();
    };
}

void IDataBoxText::handleValueChange()
{
    base::StringBuf txt;

    const auto ret = readText(txt);
    if (ret.code == base::DataViewResultCode::OK)
    {
        m_editBox->text(txt);
        m_editBox->enable(!readOnly());
    }
    else if (ret.code == base::DataViewResultCode::ErrorManyValues)
    {
        //m_editBox->text(txt);
        m_editBox->enable(!readOnly());
    }
    else
    {
        // TODO: error display
        m_editBox->prefixText("");
        m_editBox->enable(false);
    }
}

void IDataBoxText::cancelEdit()
{
    m_editBox->clearSelection();
    handleValueChange();
}

void IDataBoxText::enterEdit()
{
    m_editBox->selectWholeText();
    m_editBox->focus();
}

void IDataBoxText::write()
{
    if (!readOnly())
    {
        auto txt = m_editBox->text();
        writeText(txt);
    }
}

//--

/// StringID editor
class DataBoxStringID : public IDataBoxText
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxStringID, IDataBoxText);

public:
    virtual base::DataViewResult readText(base::StringBuf& outText) override
    {
        base::StringID data;
        if (const auto ret = HasError(readValue(data)))
            return ret.result;

        outText = data.c_str();
        return base::DataViewResultCode::OK;
    }

    static bool IsValidNameChar(char ch)
    {
        if (ch >= 'A' && ch <= 'Z') return true;
        if (ch >= 'a' && ch <= 'z') return true;
        if (ch >= '0' && ch <= '9') return true;
        if (ch == '(' || ch == ')' || ch == ' ' || ch == '_') return true;
        return false;
    }

    virtual base::DataViewResult writeText(const base::StringBuf& text) override
    {
        for (const auto ch : text.view())
        {
            if (!IsValidNameChar(ch))
                return base::DataViewResultCode::ErrorInvalidValid;
        }

        base::StringID id(text.c_str());
        return writeValue(id);
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxStringID);
RTTI_END_TYPE();

//--

/// StringBuf editor
class DataBoxStringBuf : public IDataBoxText
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxStringBuf, IDataBoxText);

public:
    virtual base::DataViewResult readText(base::StringBuf& outText) override
    {
        return readValue(outText);
    }

    virtual base::DataViewResult writeText(const base::StringBuf& text) override
    {
        return writeValue(text);
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxStringBuf);
RTTI_END_TYPE();

//--

class DataBoxStringFactory : public IDataBoxFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxStringFactory, IDataBoxFactory);

public:
    virtual DataBoxPtr tryCreate(const base::rtti::DataViewInfo& info) const override
    {
        if (info.dataType == base::reflection::GetTypeObject<base::StringID>())
        {
            return base::RefNew<DataBoxStringID>();
        }
        else if (info.dataType == base::reflection::GetTypeObject<base::StringBuf>())
        {
            return base::RefNew<DataBoxStringBuf>();
        }

        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(DataBoxStringFactory);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE(ui)