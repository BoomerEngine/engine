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
#include "base/resource/include/objectIndirectTemplate.h"

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
                const auto seed = rand(); // TODO!

                auto prefab = RefNew<Prefab>(seed);
                prefab->setup(nullptr);

                return prefab;
            }
        };

        RTTI_BEGIN_TYPE_CLASS(ScenePrefabFactory);
            RTTI_METADATA(res::FactoryClassMetadata).bindResourceClass<Prefab>();
        RTTI_END_TYPE();

        ///----

        RTTI_BEGIN_TYPE_CLASS(PrefabAppearance);
            RTTI_PROPERTY(name);
            RTTI_PROPERTY(randomWeight);
        RTTI_END_TYPE();

        PrefabAppearance::PrefabAppearance()
        {}

        ///----

	    RTTI_BEGIN_TYPE_CLASS(Prefab);
            RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4prefab");
            RTTI_METADATA(res::ResourceDescriptionMetadata).description("Prefab");
            RTTI_METADATA(res::ResourceTagColorMetadata).color(0x86, 0x6b, 0xed);
            RTTI_PROPERTY(m_root);
            RTTI_PROPERTY(m_appearances);
            RTTI_PROPERTY(m_internalSeed);
        RTTI_END_TYPE();

        Prefab::Prefab(uint64_t seed)
            : m_internalSeed(seed)
        {
        }

        void Prefab::setup(NodeTemplate* root)
        {
            if (root)
            {
                m_root = AddRef(root);
                m_root->parent(this);
            }
            else
            {
                m_root = RefNew<NodeTemplate>();
                m_root->m_name = "default"_id;
                m_root->parent(this);

                m_root->m_entityTemplate = RefNew<ObjectIndirectTemplate>();
                m_root->m_entityTemplate->parent(m_root);
            }

            markModified();
        }

        void Prefab::onPostLoad()
        {
            TBaseClass::onPostLoad();

            if (!m_root)
                setup(nullptr);
        }

        //--

    } // world
} // game
