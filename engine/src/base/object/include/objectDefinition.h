/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once

namespace base
{
    //--

    struct ObjectDefinitionValue : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ObjectDefinitionValue, IObject);

    public:
        GUID key;
        StringID name; // only named children
        Variant value; // current value, may be NONE (not applied then)

        bool madeFinal = false;
        bool madeDeleted = false;

        Array<RefPtr<ObjectDefinitionValue>> children;
    };

    //--

    class BASE_OBJECT_API ObjectDefinition : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ObjectDefinition, IObject);

    public:
        ObjectDefinition();
            
    private:
        Array<RefPtr<ObjectDefinitionValue>> m_rootProperties;
    };

    ///--

} // base
