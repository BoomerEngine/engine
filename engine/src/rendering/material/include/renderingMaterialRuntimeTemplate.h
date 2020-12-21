/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "renderingMaterialRuntimeLayout.h"
#include "renderingMaterialTemplate.h"

namespace rendering
{
    //---

    // runtime version of material template - immutable (template itself can be edited and can change)
    class RENDERING_MATERIAL_API MaterialTemplateProxy : public base::IReferencable
    {
		RTTI_DECLARE_POOL(POOL_MATERIAL_DATA);

    public:
		MaterialTemplateProxy(const base::StringBuf& contextName, const base::Array<MaterialTemplateParamInfo>& parameters, MaterialSortGroup sortGroup, const MaterialTemplateDynamicCompilerPtr& compiler, const base::Array<MaterialPrecompiledStaticTechnique>& precompiledTechniques);
        virtual ~MaterialTemplateProxy();

        //--

		// data layout for the material template, compiled from parameters
		INLINE const MaterialDataLayout* layout() const { return m_layout; };

        // sort group for this material proxy, NOTE: never updated, a new proxy must be created as if we changed the template (usually the template DOES change)
        INLINE MaterialSortGroup sortGroup() const { return m_sortGroup; }

        //--

		/// find/compile a rendering technique for given rendering settings
		MaterialTechniquePtr fetchTechnique(const MaterialCompilationSetup& setup);

		//--

    private:
		//--

		const MaterialDataLayout* m_layout = nullptr;
		MaterialSortGroup m_sortGroup = MaterialSortGroup::Opaque;

		base::SpinLock m_techniqueMapLock;
		base::HashMap<uint32_t, MaterialTechniquePtr> m_techniqueMap;

		//--

		base::Array<MaterialTemplateParamInfo> m_parameters;
		base::Array<MaterialPrecompiledStaticTechnique> m_precompiledTechniques;

		MaterialTemplateDynamicCompilerPtr m_dynamicCompiler;

		base::StringBuf m_contextName;

		//--

		void registerLayout();
    };

    //---

} // rendering