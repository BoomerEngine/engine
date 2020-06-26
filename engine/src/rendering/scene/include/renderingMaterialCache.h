/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: materials #]
***/

#pragma once

#include "rendering/mesh/include/renderingMeshFormat.h"

namespace rendering
{
    namespace scene
    {
        ///--

        class MaterialCachedTemplate;

        // technique list for material
        class  MaterialCachedTemplateTechniqueList : public base::NoCopy
        {
        public:
            MaterialCachedTemplateTechniqueList(const MaterialCachedTemplate* parent);

            MaterialTechniquePtr fetchTechnique(MaterialPass pass);

        private:
            base::SpinLock m_techniquesLock;
            MaterialTechniquePtr m_techniques[(int)MaterialPass::MAX];

            const MaterialCachedTemplate* m_parent;
        };
        

        // entry in the material cache
        class MaterialCachedTemplate : public base::IReferencable
        {
        public:
            MaterialCachedTemplate(const MaterialTemplatePtr& material, MeshVertexFormat vertexFormat);
            ~MaterialCachedTemplate();

            INLINE const MaterialTemplatePtr& materialTemplate() const { return m_materialTemplate; }
            INLINE MeshVertexFormat vertexFormat() const { return m_vertexFormat; }

            MaterialTechniquePtr fetchTechnique(MaterialPass pass, bool msaa = false, uint8_t fetchMode = 0);

        private:
            MaterialTemplatePtr m_materialTemplate;
            MeshVertexFormat m_vertexFormat = MeshVertexFormat::Static;
            MaterialCachedTemplateTechniqueList m_techniqueList;
        };

        ///--

        // helper registry that serves as cache for material techniques to use in proxy rendering
        class MaterialCache : public base::app::ILocalService
        {
            RTTI_DECLARE_VIRTUAL_CLASS(MaterialCache, base::app::ILocalService);

        public:
            MaterialCache();
            virtual ~MaterialCache();

            //--

            // map entry for given material template and vertex factory
            MaterialCachedTemplatePtr mapTemplate(const MaterialTemplatePtr& materialTemplate, MeshVertexFormat format);

            //--

        private:
            virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
            virtual void onShutdownService() override final;
            virtual void onSyncUpdate() override final;

            static const uint64_t BuildSearchKey(const MaterialTemplate* materialTemplate, MeshVertexFormat format);

            base::HashMap<uint64_t, base::RefWeakPtr<MaterialCachedTemplate>> m_entryMap;
            base::SpinLock m_entryLock;
        };

        ///--

    } // scene
} // rendering

