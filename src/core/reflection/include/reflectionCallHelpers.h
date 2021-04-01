/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

struct CORE_REFLECTION_API StringRefParam : public NoCopy
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(StringRefParam);

public:
    virtual StringBuf str() const = 0;
    virtual StringID strId() const = 0;
    virtual BaseStringView<wchar_t> view() const = 0;
    virtual void text(StringView txt) = 0;

private:
    void* str = nullptr;
};

//--

END_BOOMER_NAMESPACE()
