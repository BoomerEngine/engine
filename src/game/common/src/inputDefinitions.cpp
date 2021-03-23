/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
*
***/

#include "build.h"
#include "inputMapping.h"
#include "inputDefinitions.h"

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
        return RefNew<GameInputDefinitions>();
    }
};

RTTI_BEGIN_TYPE_CLASS(InputDefinitionsFactory);
    RTTI_METADATA(ResourceFactoryClassMetadata).bindResourceClass<GameInputDefinitions>();
RTTI_END_TYPE();

///--

RTTI_BEGIN_TYPE_CLASS(GameInputDefinitions);
    RTTI_METADATA(ResourceDescriptionMetadata).description("Input Definitions");
    RTTI_PROPERTY(m_root);
RTTI_END_TYPE();

GameInputDefinitions::GameInputDefinitions()
{}

static InputActionTablePtr FindChildren(const GameInputActionTable* table, StringView name)
{
    for (const auto& child : table->children())
        if (child->name() == name)
            return child;

    return nullptr;
}

InputActionTablePtr GameInputDefinitions::findTable(StringView name) const
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
