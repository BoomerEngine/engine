/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
*
***/

#include "build.h"
#include "worldEntityTemplate.h"
#include "worldEntity.h"

namespace base
{
    namespace world
    {
        //--

        RTTI_BEGIN_TYPE_ENUM(EntityStreamingModel);
        RTTI_ENUM_OPTION(Auto);
        RTTI_ENUM_OPTION(StreamWithParent);
        RTTI_ENUM_OPTION(HierarchicalGrid);
        RTTI_ENUM_OPTION(AlwaysLoaded);
        RTTI_ENUM_OPTION(SeparateSector);
        RTTI_ENUM_OPTION(Discard);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(EntityTemplate);
        RTTI_PROPERTY(m_enabled);
        RTTI_PROPERTY(m_placement);
        RTTI_CATEGORY("Streaming");
        RTTI_PROPERTY(m_streamingModel).editable("How this entity should be streamed").overriddable();
        RTTI_PROPERTY(m_streamingDistanceOverride).editable("Override distance for the streaming range").overriddable();
        RTTI_END_TYPE();

        EntityTemplate::EntityTemplate()
        {}

        EntityPtr EntityTemplate::createEntity() const
        {
            auto entity = base::CreateSharedPtr<Entity>();
            return entity;
        }

        void EntityTemplate::enable(bool flag, bool callEvent /*= true*/)
        {
            if (m_enabled != flag)
            {
                m_enabled = flag;
                if (callEvent)
                    onPropertyChanged("enabled");
            }
        }

        void EntityTemplate::placement(const EulerTransform& placement, bool callEvent/*= true*/)
        {
            if (m_placement != placement)
            {
                m_placement = placement;
                if (callEvent)
                    onPropertyChanged("placement");
            }
        }

        void EntityTemplate::streamingModel(EntityStreamingModel model, bool makeOverride /*= true*/, bool callEvent /*= true*/)
        {
            if (m_streamingModel != model)
            {
                m_streamingModel = model;

                if (makeOverride)
                    markPropertyOverride("streamingModel"_id);

                if (callEvent)
                    onPropertyChanged("streamingModel");
            }
        }

        void EntityTemplate::streamingDistanceOverride(float distance, bool makeOverride /*= true*/, bool callEvent /*= true*/)
        {
            const auto safeDistance = std::clamp<float>(distance, 0.0f, 100000.0f);
            if (m_streamingDistanceOverride != safeDistance)
            {
                m_streamingDistanceOverride = safeDistance;

                if (makeOverride)
                    markPropertyOverride("streamingDistanceOverride"_id);

                if (callEvent)
                    onPropertyChanged("streamingDistanceOverride");
            }
        }

        //--

    } // world
} // base
