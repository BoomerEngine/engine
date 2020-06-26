/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure #]
***/

#include "build.h"
#include "sceneEditorStructureLayer.h"
#include "sceneEditorStructureGroup.h"

#include "scene/common/include/sceneWorld.h"
#include "scene/common/include/sceneLayer.h"
#include "base/image/include/image.h"

namespace ed
{
    namespace world
    {

        //---

        RTTI_BEGIN_TYPE_CLASS(ContentGroup);
        RTTI_END_TYPE();

        //---

        ContentGroup::ContentGroup(const base::StringBuf& directoryPath, const base::StringBuf& name)
            : IContentElement(ContentElementType::Group)
            , m_directoryPath(directoryPath)
            , m_name(name)
        {
        }

        ContentGroup::~ContentGroup()
        {
        }

        void ContentGroup::syncInitialStructure(const depot::ManagedDirectoryPtr& existingDirectory)
        {
            // load layers
            for (auto& layerFile : existingDirectory->files())
            {
                if (!layerFile->isDeleted())
                {
                    auto contentLayer = base::CreateSharedPtr<ContentLayer>(layerFile->depotPath());
                    m_layers.pushBack(contentLayer);
                    contentLayer->attachToParent(this);
                    contentLayer->syncNodes(layerFile);
                }
            }

            // load groups
            for (auto& groupDir : existingDirectory->directories())
            {
                if (!groupDir->isDeleted())
                {
                    auto newGroup = base::CreateSharedPtr<ContentGroup>(groupDir->depotPath(), groupDir->name());
                    m_groups.pushBack(newGroup);
                    newGroup->attachToParent(this);

                    newGroup->syncInitialStructure(groupDir);
                }
            }
        }

        ContentElementMask ContentGroup::contentType() const
        {
            auto ret = TBaseClass::contentType();

            ret |= ContentElementBit::Group;
            ret |= ContentElementBit::CanSave;
            ret |= ContentElementBit::LayerContainer;
            ret |= ContentElementBit::GroupContainer;
            return ret;
        }

        const base::StringBuf& ContentGroup::name() const
        {
            return m_name;
        }

        int ContentGroup::compareType() const
        {
            return 1;
        }

        bool ContentGroup::compareForSorting(const IContentElement& other) const
        {
            auto& otherLayer = static_cast<const ContentGroup&>(other);
            return name() < otherLayer.name();
        }

        base::ObjectPtr ContentGroup::resolveObject(base::ClassType expectedClass) const
        {
            // groups have nothing to edit
            return nullptr;
        }
        
        void ContentGroup::render(rendering::scene::FrameInfo& frame, float sqDistanceLimit) const
        {
            if (!mergedVisibility())
                return;

            for (auto& group : m_groups)
                group->render(frame, sqDistanceLimit);

            for (auto& layer : m_layers)
                layer->render(frame, sqDistanceLimit);
        }

        void ContentGroup::visitStructure(const std::function<bool(const base::RefPtr<IContentElement>&)>& func) const
        {
            if (!func(sharedFromThis()))
                return;

            for (auto& group : m_groups)
                group->visitStructure(func);

            for (auto& layer : m_layers)
                layer->visitStructure(func);
        }

        ContentGroupPtr ContentGroup::findGroup(base::StringID name) const
        {
            for (auto& group : m_groups)
                if (group->name() == name.view())
                    return group;

            return nullptr;
        }

        ContentLayerPtr ContentGroup::findLayer(base::StringID name) const
        {
            for (auto& layer : m_layers)
                if (layer->name() == name.view())
                    return layer;

            return nullptr;
        }

        ContentGroupPtr ContentGroup::createGroup(base::StringID name)
        {
            // empty name
            if (!name)
                return nullptr;

            // use existing one
            for (auto& group : m_groups)
                if (group->name() == name.view())
                    return group;

            // create new group
            auto newGroupDirectoryPath = base::StringBuf(base::TempString("{}{}/", m_directoryPath, name));
            auto group = base::CreateSharedPtr<ContentGroup>(newGroupDirectoryPath, base::StringBuf(name.view()));

            // add to tree
            m_groups.pushBack(group);
            group->attachToParent(this);
            //group->syncInitialStructure(); // in case we added group that is already on disk
            return group;
        }

        ContentLayerPtr ContentGroup::createLayer(base::StringID name)
        {
            // use existing one
            for (auto& layer : m_layers)
            {
                if (layer->name() == name.view())
                {
                    TRACE_INFO("Layer '{}' already exists in directory", name)
                    return layer;
                }
            }

            // append extension
            auto resourceClass  = scene::Layer::GetStaticClass();
            auto extension = resourceClass->findMetadataRef<base::res::ResourceExtensionMetadata>().extension();
            base::StringBuf fullLayerPath = base::TempString("{}{}.{}", m_directoryPath, name, extension);

            // create wrapper
            auto layer = base::CreateSharedPtr<ContentLayer>(fullLayerPath);
            m_layers.pushBack(layer);
            layer->attachToParent(this);

            return layer;
        }

        bool ContentGroup::removeGroup(const ContentGroupPtr& childGroup, bool force)
        {
            if (!childGroup)
                return true;

            if (!m_groups.contains(childGroup))
            {
                TRACE_ERROR("Group '{}' is not a child of '{}'", childGroup->name(), path());
                return false;
            }

            if (!force && childGroup->hasModifiedContent())
            {
                TRACE_ERROR("Group '{}' cannot be removed because it contains unsaved content", childGroup->name());
                return false;
            }

            // remove content nodes
            childGroup->detachAllChildren();
            childGroup->detachFromParent();
            m_groups.remove(childGroup);

            // notify
            return true;
        }

        bool ContentGroup::removeLayer(const ContentLayerPtr& childLayer, bool force)
        {
            if (!childLayer)
                return true;

            if (!m_layers.contains(childLayer))
            {
                TRACE_ERROR("Layer '{}' is not a child of '{}'", childLayer->name(), path());
                return false;
            }

            if (!force && childLayer->isModified())
            {
                TRACE_ERROR("Layer '{}' cannot be removed because it contains unsaved content", childLayer->name());
                return false;
            }

            // unlink layer
            childLayer->detachAllChildren();
            childLayer->detachFromParent();
            m_layers.remove(childLayer);

            // notify
            return true;
        }

        void ContentGroup::collectLayersToSave(bool recrusive, base::Array<ContentLayer*>& outLayers) const
        {
            for (auto& layer : m_layers)
            {
                if (layer->isModified())
                {
                    outLayers.pushBack(layer.get());
                }
            }

            if (recrusive)
            {
                for (auto& group : m_groups)
                    group->collectLayersToSave(true, outLayers);
            }
        }

        bool ContentGroup::hasModifiedContent() const
        {
            for (auto& layer : m_layers)
                if (layer->isModified())
                    return true;

            for (auto& group : m_groups)
                if (group->hasModifiedContent())
                    return true;

            return false;
        }

        static base::res::StaticResource<base::image::Image> resFolder("engine/ui/styles/icons/folder.png");

        void ContentGroup::buildViewContent(ui::ElementPtr& content)
        {
            auto image = base::CreateSharedPtr<ui::StaticContent>();
            image->customImage(resFolder.loadAndGet());
            image->name("NodeIcon");

            auto caption = base::CreateSharedPtr<ui::StaticContent>();
            caption->text(name());
            caption->name("NodeName");
            caption->customMargins(ui::Offsets(5, 0, 0, 0));

            auto leftSide = ui::LayoutHorizontally({ image, caption });

            auto klass = base::CreateSharedPtr<ui::StaticContent>();
            klass->text("Group");
            klass->name("NodeClass");
            klass->styleClasses("italic");
            klass->customMargins(ui::Offsets(5, 0, 0, 0));

            content = ui::LayoutHorizontally({ leftSide, klass });
            content->layoutMode(ui::LayoutMode::Columns);
        }

        void ContentGroup::refreshVisualizationVisibility()
        {
            for (auto& layer : m_layers)
                layer->refreshMergedVisibility();

            for (auto& group : m_groups)
                group->refreshMergedVisibility();
        }

        //---

    } // world
} // ed