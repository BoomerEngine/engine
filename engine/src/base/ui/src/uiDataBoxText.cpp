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

namespace ui
{

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDataBoxText);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxText");
    RTTI_END_TYPE();

    IDataBoxText::IDataBoxText()
    {
        m_editBox = createChild<TextEditor>();
        m_editBox->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_editBox->customVerticalAligment(ui::ElementVerticalLayout::Expand);

        m_editBox->bind("OnTextAccepted"_id) = [this]()
        {
            write();
        };
    }

    void IDataBoxText::handleValueChange()
    {
        base::StringBuf txt;
        if (readText(txt))
            m_editBox->text(txt);

        m_editBox->enable(!readOnly());

        if (readValid())
            m_editBox->prefixText("");
        else
            m_editBox->prefixText("Multiple values");
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
        virtual bool readText(base::StringBuf& outText) override
        {
            base::StringID data;
            if (!readValue(data))
                return false;

            outText = data.c_str();
            return true;
        }

        static bool IsValidNameChar(char ch)
        {
            if (ch >= 'A' && ch <= 'Z') return true;
            if (ch >= 'a' && ch <= 'z') return true;
            if (ch >= '0' && ch <= '9') return true;
            if (ch == '(' || ch == ')' || ch == ' ' || ch == '_') return true;
            return false;
        }

        virtual bool writeText(const base::StringBuf& text) override
        {
            for (const auto ch : text.view())
            {
                if (!IsValidNameChar(ch))
                    return false;
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
        virtual bool readText(base::StringBuf& outText) override
        {
            return readValue(outText);
        }

        virtual bool writeText(const base::StringBuf& text) override
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
                return base::CreateSharedPtr<DataBoxStringID>();
            }
            else if (info.dataType == base::reflection::GetTypeObject<base::StringBuf>())
            {
                return base::CreateSharedPtr<DataBoxStringBuf>();
            }

            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(DataBoxStringFactory);
    RTTI_END_TYPE();

    //--

} // ui