/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
*
***/

#include "build.h"
#include "worldPrefab.h"
#include "worldNodeTemplate.h"

#include "base/resource/include/resourceFactory.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace world
    {

        ///----

        ConfigProperty<float> cvPrefabUpdateDelay("Scene.Prefab", "UpdateDelay", 0.5f);

        ///----

        // factory class for the scene template
        class ScenePrefabFactory : public res::IFactory
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ScenePrefabFactory, res::IFactory);

        public:
            virtual res::ResourceHandle createResource() const override final
            {
                return RefNew<Prefab>();
            }
        };

        RTTI_BEGIN_TYPE_CLASS(ScenePrefabFactory);
            RTTI_METADATA(res::FactoryClassMetadata).bindResourceClass<Prefab>();
        RTTI_END_TYPE();

        ///----

	    RTTI_BEGIN_TYPE_CLASS(Prefab);
            RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4prefab");
            RTTI_PROPERTY(m_nodes);
        RTTI_END_TYPE();

        Prefab::Prefab()
        {
        }

        void Prefab::setup(const Array<NodeTemplatePtr>& nodes)
        {
            for (const auto& node : m_nodes)
                node->parent(nullptr);

            m_nodes.reset();

            for (const auto& node : nodes)
            {
                if (node)
                {
                    DEBUG_CHECK(node->parent() == nullptr);
                    node->parent(this);
                    m_nodes.pushBack(node);
                }
            }

            markModified();
        }

        //--

    } // world
} // game
