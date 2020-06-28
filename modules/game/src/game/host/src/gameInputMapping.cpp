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

#include "base/resources/include/resourceFactory.h"

namespace game
{
    ///----

    // factory class for the scene world
    class InputDefinitionsFactory : public base::res::IFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(InputDefinitionsFactory, base::res::IFactory);

    public:
        virtual base::res::ResourceHandle createResource() const override final
        {
            return base::CreateSharedPtr<InputDefinitions>();
        }
    };

    RTTI_BEGIN_TYPE_CLASS(InputDefinitionsFactory);
        RTTI_METADATA(base::res::FactoryClassMetadata).bindResourceClass<InputDefinitions>();
    RTTI_END_TYPE();

    ///--

    RTTI_BEGIN_TYPE_ENUM(InputActionType);
        RTTI_ENUM_OPTION(Button);
        RTTI_ENUM_OPTION(Axis);
    RTTI_END_TYPE();

    ///--

    RTTI_BEGIN_TYPE_CLASS(InputAction);
        RTTI_PROPERTY(name).editable();
        RTTI_PROPERTY(type).editable();
        RTTI_PROPERTY(invert).editable();
        RTTI_PROPERTY(defaultKey).editable();
        RTTI_PROPERTY(defaultAxis).editable();
        RTTI_PROPERTY(mappingName).editable();
        RTTI_PROPERTY(mappingGroup).editable();
    RTTI_END_TYPE();

    InputAction::InputAction()
    {}

    ///--

    RTTI_BEGIN_TYPE_CLASS(InputActionTable);
        RTTI_PROPERTY(m_name).editable();
        RTTI_PROPERTY(m_actions).editable();
        RTTI_PROPERTY(m_children).editable();
    RTTI_END_TYPE();

    InputActionTable::InputActionTable()
    {}

    ///--

	RTTI_BEGIN_TYPE_CLASS(InputDefinitions);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4input");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Input Definitions");
        RTTI_PROPERTY(m_root);
    RTTI_END_TYPE();

    InputDefinitions::InputDefinitions()
    {}

    static InputActionTablePtr FindChildren(const InputActionTable* table, base::StringView<char> name)
    {
        for (const auto& child : table->children())
            if (child->name() == name)
                return child;

        return nullptr;
    }

    InputActionTablePtr InputDefinitions::findTable(base::StringView<char> name) const
    {
        auto table = m_root;
        if (table)
        {
            base::InplaceArray<base::StringView<char>, 5> nameParts;
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

    //--
    
} // game
