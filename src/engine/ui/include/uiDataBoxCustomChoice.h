/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#pragma once

#include "uiDataBox.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

///---

/// Enum with custom options
class ENGINE_UI_API DataBoxCustomChoice : public IDataBox
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataBoxCustomChoice, IDataBox);

public:
    DataBoxCustomChoice();

    virtual DataViewResult readOptionValue(StringBuf& outText) = 0;

    virtual DataViewResult writeOptionValue(int optionIndex, StringView optionCaption) = 0;

    virtual void queryOptions(Array<StringBuf>& outOptions) const = 0;

protected:
    virtual void enterEdit();
    virtual void cancelEdit();
    virtual void handleValueChange();
    virtual bool canExpandChildren() const;

    DynamicChoiceBoxPtr m_box;
};

///---

END_BOOMER_NAMESPACE_EX(ui)
