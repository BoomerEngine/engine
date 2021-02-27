/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "runtimeLayout.h"
#include "materialTemplate.h"

#include "gpu/device/include/shaderReloadNotifier.h"

BEGIN_BOOMER_NAMESPACE()

//---

// runtime version of material template - immutable (template itself can be edited and can change)
class ENGINE_MATERIAL_API MaterialTemplateProxy : public IReferencable
{
	RTTI_DECLARE_POOL(POOL_MATERIAL_DATA);

public:
	MaterialTemplateProxy(const StringBuf& contextName, const Array<MaterialTemplateParamInfo>& parameters, const MaterialTemplateMetadata& metadata, const MaterialTemplateDynamicCompilerPtr& compiler, const Array<MaterialPrecompiledStaticTechnique>& precompiledTechniques);
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

	SpinLock m_techniqueMapLock;
	mutable HashMap<uint32_t, MaterialTechniquePtr> m_techniqueMap;

	//--

	Array<MaterialTemplateParamInfo> m_parameters;
	Array<MaterialPrecompiledStaticTechnique> m_precompiledTechniques;

	MaterialTemplateDynamicCompilerPtr m_dynamicCompiler;

	StringBuf m_contextName;

	//--

	gpu::ShaderReloadNotifier m_reloadNotifier;

	void discardCachedTechniques();

	//--

	void registerLayout();
};

//---

END_BOOMER_NAMESPACE()
