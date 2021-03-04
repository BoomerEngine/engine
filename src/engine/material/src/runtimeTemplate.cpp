/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "runtimeTechnique.h"
#include "runtimeTemplate.h"
#include "runtimeService.h"
#include "materialTemplate.h"

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_CLASS(MaterialRenderState);
	RTTI_PROPERTY(hasVertexAnimation);
	RTTI_PROPERTY(hasPixelDiscard);
	RTTI_PROPERTY(hasTransparency);
	RTTI_PROPERTY(hasLighting);
	RTTI_PROPERTY(hasPixelReadback);
RTTI_END_TYPE();

MaterialRenderState::MaterialRenderState()
{}
 
///---

MaterialTemplateProxy::MaterialTemplateProxy(const StringBuf& contextName, const Array<MaterialTemplateParamInfo>& parameters, const MaterialTemplateDynamicCompilerPtr& compiler, const Array<MaterialPrecompiledStaticTechnique>& precompiledTechniques)
	: m_parameters(parameters)
	, m_precompiledTechniques(precompiledTechniques)
	, m_dynamicCompiler(compiler)
	, m_contextName(contextName)
{
	m_reloadNotifier = [this]() { discardCachedTechniques(); };

	m_techniqueMap.reserve(32);
	registerLayout();
}

MaterialTemplateProxy::~MaterialTemplateProxy()
{
	m_techniqueMap.clear();
	m_dynamicCompiler.reset();
}

void MaterialTemplateProxy::discardCachedTechniques()
{
	if (m_dynamicCompiler)
	{
		auto lock = CreateLock(m_techniqueMapLock);
		m_techniqueMap.clear();
	}
}

void MaterialTemplateProxy::registerLayout()
{
	Array<MaterialDataLayoutEntry> layoutEntries;
	layoutEntries.reserve(m_parameters.size());

	for (const auto& param : m_parameters)
	{
		auto& entry = layoutEntries.emplaceBack();
		entry.name = param.name;
		entry.type = param.parameterType;
	}

	// TODO: optimize layout/order

	std::sort(layoutEntries.begin(), layoutEntries.end(), [](const MaterialDataLayoutEntry& a, MaterialDataLayoutEntry& b) { return a.name.view() < b.name.view(); });

	m_layout = GetService<MaterialService>()->registerDataLayout(std::move(layoutEntries));
}

void MaterialTemplateProxy::evalRenderStates(const IMaterial& setup, MaterialRenderState& outRenderStates) const
{
	if (m_dynamicCompiler)
		m_dynamicCompiler->evalRenderStates(setup, outRenderStates);
}

MaterialTechniquePtr MaterialTemplateProxy::fetchTechnique(const MaterialCompilationSetup& setup) const
{
	const auto key = setup.key();

	// look in dynamic list
	auto lock = CreateLock(m_techniqueMapLock);
	auto& ret = m_techniqueMap[key];
	if (!ret)
	{
		ret = RefNew<MaterialTechnique>(setup);

		/*// lookup in precompiled list
		for (const auto& techniqe : m_precompiledTechniques)
		{
			const auto techniqueKey = techniqe.setup.key();
			if (key == techniqueKey)
			{
				auto compiledTechnique = new MaterialCompiledTechnique;
				compiledTechnique->shader = techniqe.shader;
				ret->pushData(compiledTechnique);
				valid = true;
				break;
			}
		}*/

		// try to compile dynamically
		if (m_dynamicCompiler)
		{
			m_dynamicCompiler->requestTechniqueComplation(m_contextName, ret);
		}
		else
		{
			TRACE_STREAM_ERROR().appendf("Missing material '{}' permutation '{}'. Key {}.", m_contextName, setup, key);
		}
	}

	return ret;
}

///---

END_BOOMER_NAMESPACE()
