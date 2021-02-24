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

#include "rendering/device/include/renderingShaderReloadNotifier.h"

BEGIN_BOOMER_NAMESPACE(rendering)

//---

// runtime version of material template - immutable (template itself can be edited and can change)
class RENDERING_MATERIAL_API MaterialTemplateProxy : public base::IReferencable
{
	RTTI_DECLARE_POOL(POOL_MATERIAL_DATA);

public:
	MaterialTemplateProxy(const base::StringBuf& contextName, const base::Array<MaterialTemplateParamInfo>& parameters, const MaterialTemplateMetadata& metadata, const MaterialTemplateDynamicCompilerPtr& compiler, const base::Array<MaterialPrecompiledStaticTechnique>& precompiledTechniques);
    virtual ~MaterialTemplateProxy();

    //--

	// data layout for the material template, compiled from parameters
	INLINE const MaterialDataLayout* layout() const { return m_layout; };

    // get metadata for the material
    INLINE const MaterialTemplateMetadata& metadata() const { return m_metadata; }

    //--

	/// find/compile a rendering technique for given rendering settings
	MaterialTechniquePtr fetchTechnique(const MaterialCompilationSetup& setup) const;

	//--

private:
	//--

	const MaterialDataLayout* m_layout = nullptr;

	MaterialTemplateMetadata m_metadata;

	base::SpinLock m_techniqueMapLock;
	mutable base::HashMap<uint32_t, MaterialTechniquePtr> m_techniqueMap;

	//--

	base::Array<MaterialTemplateParamInfo> m_parameters;
	base::Array<MaterialPrecompiledStaticTechnique> m_precompiledTechniques;

	MaterialTemplateDynamicCompilerPtr m_dynamicCompiler;

	base::StringBuf m_contextName;

	//--

    ShaderReloadNotifier m_reloadNotifier;

	void discardCachedTechniques();

	//--

	void registerLayout();
};

//---

END_BOOMER_NAMESPACE(rendering)