/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: resources #]
*
***/

#include "build.h"
#include "game.h"
#include "gameResourceDefinitions.h"
#include "gameDefinitionFile.h"

#include "base/resource/include/resourceFactory.h"
#include "base/resource/include/resourceTags.h"

namespace game
{
    ///---

    // factory class for the scene world
    class DefinitionFileFactory : public base::res::IFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DefinitionFileFactory, base::res::IFactory);

    public:
        virtual base::res::ResourceHandle createResource() const override final
        {
            return base::CreateSharedPtr<DefinitionFile>();
        }
    };

    RTTI_BEGIN_TYPE_CLASS(DefinitionFileFactory);
        RTTI_METADATA(base::res::FactoryClassMetadata).bindResourceClass<DefinitionFile>();
    RTTI_END_TYPE();

    ///--

	RTTI_BEGIN_TYPE_CLASS(DefinitionFile);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4game");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Game Definition");
        RTTI_CATEGORY("Resources");
        RTTI_PROPERTY(m_resources).editable("Resource lists");
        RTTI_CATEGORY("Launcher");
        RTTI_PROPERTY(m_standaloneLauncher).editable("Launcher for use in standalone mode").inlined();
        RTTI_PROPERTY(m_editorLauncher).editable("Launcher for use in editor mode").inlined();
    RTTI_END_TYPE();

    DefinitionFile::DefinitionFile()
    {}

    //---
    
} // game
