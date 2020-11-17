/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: resources #]
*
***/

#include "build.h"
#include "gameInputMapping.h"
#include "gameInputDefinitions.h"

#include "base/resource/include/resourceFactory.h"
#include "base/resource/include/resourceTags.h"

namespace game
{
    ///---

    // factory class for the scene world
    class InputDefinitionsFactory : public base::res::IFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(InputDefinitionsFactory, base::res::IFactory);

    public:
        virtual base::res::ResourceHandle createResource() const override final
        {
            return base::RefNew<InputDefinitions>();
        }
    };

    RTTI_BEGIN_TYPE_CLASS(InputDefinitionsFactory);
        RTTI_METADATA(base::res::FactoryClassMetadata).bindResourceClass<InputDefinitions>();
    RTTI_END_TYPE();

    ///--

	RTTI_BEGIN_TYPE_CLASS(InputDefinitions);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4input");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Input Definitions");
        RTTI_PROPERTY(m_root);
    RTTI_END_TYPE();

    InputDefinitions::InputDefinitions()
    {}

    static InputActionTablePtr FindChildren(const InputActionTable* table, base::StringView name)
    {
        for (const auto& child : table->children())
            if (child->name() == name)
                return child;

        return nullptr;
    }

    InputActionTablePtr InputDefinitions::findTable(base::StringView name) const
    {
        auto table = m_root;
        if (table)
        {
            base::InplaceArray<base::StringView, 5> nameParts;
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
    
} // game
