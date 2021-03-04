/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
*
***/

#include "build.h"
#include "gameInputMapping.h"
#include "gameInputDefinitions.h"

#include "core/resource/include/factory.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE()

///---

// factory class for the scene world
class InputDefinitionsFactory : public IResourceFactory
{
    RTTI_DECLARE_VIRTUAL_CLASS(InputDefinitionsFactory, IResourceFactory);

public:
    virtual ResourcePtr createResource() const override final
    {
        return RefNew<InputDefinitions>();
    }
};

RTTI_BEGIN_TYPE_CLASS(InputDefinitionsFactory);
    RTTI_METADATA(ResourceFactoryClassMetadata).bindResourceClass<InputDefinitions>();
RTTI_END_TYPE();

///--

RTTI_BEGIN_TYPE_CLASS(InputDefinitions);
    RTTI_METADATA(ResourceExtensionMetadata).extension("v4input");
    RTTI_METADATA(ResourceDescriptionMetadata).description("Input Definitions");
    RTTI_PROPERTY(m_root);
RTTI_END_TYPE();

InputDefinitions::InputDefinitions()
{}

static InputActionTablePtr FindChildren(const InputActionTable* table, StringView name)
{
    for (const auto& child : table->children())
        if (child->name() == name)
            return child;

    return nullptr;
}

InputActionTablePtr InputDefinitions::findTable(StringView name) const
{
    auto table = m_root;
    if (table)
    {
        InplaceArray<StringView, 5> nameParts;
        name.slice(".", false, nameParts);

        for (const auto& partName : nameParts)
        {
            table = FindChildren(table, partName);
            if (!table)
                break;
        }
    }

    return table;
}

//---
    
END_BOOMER_NAMESPACE()
