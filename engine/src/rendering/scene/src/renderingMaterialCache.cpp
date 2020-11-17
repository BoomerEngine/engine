/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: materials #]
***/

#include "build.h"
#include "renderingMaterialCache.h"
#include "rendering/material/include/renderingMaterialTemplate.h"

namespace rendering
{
    namespace scene
    {

        //--

        MaterialCachedTemplateTechniqueList::MaterialCachedTemplateTechniqueList(const MaterialCachedTemplate* parent)
            : m_parent(parent)
        {
        }

        MaterialTechniquePtr MaterialCachedTemplateTechniqueList::fetchTechnique(MaterialPass pass)
        {
            auto& entry = m_techniques[(int)pass];

            auto lock = base::CreateLock(m_techniquesLock);

            if (!entry)
            {
                MaterialCompilationSetup setup;
                setup.vertexFormat = (uint8_t)m_parent->vertexFormat();
                setup.pass = pass;
                setup.msaa = false;
                setup.vertexFetchMode = 0;
                entry = m_parent->materialTemplate()->fetchTechnique(setup);
            }

            return entry;
        }

        //---

        MaterialCachedTemplate::MaterialCachedTemplate(const MaterialTemplatePtr& material, MeshVertexFormat vertexFormat)
            : m_materialTemplate(material)
            , m_vertexFormat(vertexFormat)
            , m_techniqueList(this)
        {}

        MaterialCachedTemplate::~MaterialCachedTemplate()
        {}

        MaterialTechniquePtr MaterialCachedTemplate::fetchTechnique(MaterialPass pass, bool msaa, uint8_t fetchMode)
        {
            // TODO: msaa/fetchmode etc
            return m_techniqueList.fetchTechnique(pass);
        }

        //---

        RTTI_BEGIN_TYPE_CLASS(MaterialCache);
        RTTI_END_TYPE();

        MaterialCache::MaterialCache()
        {}

        MaterialCache::~MaterialCache()
        {}

        MaterialCachedTemplatePtr MaterialCache::mapTemplate(const MaterialTemplatePtr& materialTemplate, MeshVertexFormat format)
        {
            // empty material
            if (!materialTemplate)
                return 0;

            const auto key = BuildSearchKey(materialTemplate, format);

            auto lock = base::CreateLock(m_entryLock);

            // find existing and reuse
            if (const auto* existingPtr = m_entryMap.find(key))
                if (auto cacheEntry = existingPtr->lock())
                    return cacheEntry;

            // create new one
            auto entry = base::RefNew<MaterialCachedTemplate>(materialTemplate, format);
            m_entryMap[key] = entry;
            return entry;
        }

        //--

        base::app::ServiceInitializationResult MaterialCache::onInitializeService(const base::app::CommandLine& cmdLine)
        {
            return base::app::ServiceInitializationResult::Finished;
        }

        void MaterialCache::onShutdownService()
        {
            m_entryMap.clear();
        }

        void MaterialCache::onSyncUpdate()
        {

        }

        //--

        const uint64_t MaterialCache::BuildSearchKey(const MaterialTemplate* materialTemplate, MeshVertexFormat format)
        {
            uint64_t key = 0;;
            key |= ((uint64_t)format);
            key |= ((uint64_t)materialTemplate->runtimeUniqueId()) << 4;
            return key;
        }

        //---

    } // scene
} // rendering