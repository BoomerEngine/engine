/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure #]
***/

#include "build.h"
#include "sceneEditorStructureWorldRoot.h"
#include "sceneEditorStructureLayer.h"
#include "scene/common/include/sceneWorld.h"
#include "ui/widgets/include/uiWindowMessage.h"
#include "base/image/include/image.h"

namespace ed
{
    namespace world
    {

        RTTI_BEGIN_TYPE_CLASS(ContentWorldRoot);
        RTTI_END_TYPE();

        ContentWorldRoot::ContentWorldRoot(const depot::ManagedDirectoryPtr& dirPtr, const depot::ManagedFilePtr& file)
            : ContentGroup(dirPtr->depotPath(), "World")
            , m_fileHandler(file)
        {
            m_elementType = ContentElementType::World;
            m_world = file->loadEditableContent<scene::World>();
        }

        ContentWorldRoot::~ContentWorldRoot()
        {
        }

        const base::StringBuf& ContentWorldRoot::name() const
        {
            static base::StringBuf theName("world");
            return theName;
        }

        scene::NodePath ContentWorldRoot::path() const
        {
            return scene::NodePath();
        }

        ContentElementMask ContentWorldRoot::contentType() const
        {
            auto ret = TBaseClass::contentType();
            ret |= ContentElementBit::World;
            ret |= ContentElementBit::Root;
            return ret;
        }

        base::edit::DocumentObjectID ContentWorldRoot::CreateRootDocumentID(const depot::ManagedFilePtr& file)
        {
            ASSERT_EX(file != nullptr, "World root requires a valid file");

            //auto filePath = file->depotPath();
            //return base::edit::DocumentObjectID::CreateFromPath(filePath.c_str(), "WorldRoot"_id);
            return 0;
        }

        static base::res::StaticResource<base::image::Image> resWorld("engine/ui/styles/icons/world.png");

        void ContentWorldRoot::buildViewContent(ui::ElementPtr& content)
        {
            auto image = base::CreateSharedPtr<ui::StaticContent>();
            image->customImage(resWorld.loadAndGet());
            image->name("NodeIcon");

            auto caption = base::CreateSharedPtr<ui::StaticContent>();
            caption->text(name());
            caption->name("NodeName");
            caption->customMargins(ui::Offsets(5, 0, 0, 0));

            auto leftSide = ui::LayoutHorizontally({ image, caption });

            auto klass = base::CreateSharedPtr<ui::StaticContent>();
            klass->text("World");
            klass->name("NodeClass");
            klass->styleClasses("italic");
            klass->customMargins(ui::Offsets(5, 0, 0, 0));

            content = ui::LayoutHorizontally({ leftSide, klass });
            content->layoutMode(ui::LayoutMode::Columns);
        }

    } // world
} // ed