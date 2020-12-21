/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "renderingMaterialRuntimeTechnique.h"
#include "renderingMaterialRuntimeTemplate.h"
#include "renderingMaterialRuntimeService.h"
#include "renderingMaterialTemplate.h"

namespace rendering
{
 
    ///---

	MaterialTemplateProxy::MaterialTemplateProxy(const base::StringBuf& contextName, const base::Array<MaterialTemplateParamInfo>& parameters, MaterialSortGroup sortGroup, const MaterialTemplateDynamicCompilerPtr& compiler, const base::Array<MaterialPrecompiledStaticTechnique>& precompiledTechniques)
		: m_parameters(parameters)
		, m_sortGroup(sortGroup)
		, m_precompiledTechniques(precompiledTechniques)
		, m_dynamicCompiler(compiler)
		, m_contextName(contextName)
	{
		m_techniqueMap.reserve(32);
		registerLayout();
	}

	MaterialTemplateProxy::~MaterialTemplateProxy()
	{
		m_techniqueMap.clear();
		m_dynamicCompiler.reset();
	}

	void MaterialTemplateProxy::registerLayout()
	{
		base::Array<MaterialDataLayoutEntry> layoutEntries;
		layoutEntries.reserve(m_parameters.size());

		for (const auto& param : m_parameters)
		{
			auto& entry = layoutEntries.emplaceBack();
			entry.name = param.name;
			entry.type = param.parameterType;
		}

		// TODO: optimize layout/order

		std::sort(layoutEntries.begin(), layoutEntries.end(), [](const MaterialDataLayoutEntry& a, MaterialDataLayoutEntry& b) { return a.name.view() < b.name.view(); });

		m_layout = base::GetService<MaterialService>()->registerDataLayout(std::move(layoutEntries));
	}


	MaterialTechniquePtr MaterialTemplateProxy::fetchTechnique(const MaterialCompilationSetup& setup)
	{
		const auto key = setup.key();

		// look in dynamic list
		auto lock = base::CreateLock(m_techniqueMapLock);
		auto& ret = m_techniqueMap[key];
		if (!ret)
		{
			ret = base::RefNew<MaterialTechnique>(setup);

			// lookup in precompiled list
			bool valid = false;
			for (const auto& techniqe : m_precompiledTechniques)
			{
				const auto techniqueKey = techniqe.setup.key();
				if (key == techniqueKey)
				{
					auto compiledTechnique = new MaterialCompiledTechnique;
					compiledTechnique->shader = techniqe.shader;
					compiledTechnique->dataLayout = m_layout;
					compiledTechnique->renderStates = techniqe.renderStates;
					ret->pushData(compiledTechnique);
					valid = true;
					break;
				}
			}

			// try to compile dynamically
			if (!valid)
			{
				if (m_dynamicCompiler)
				{
					m_dynamicCompiler->requestTechniqueComplation(m_contextName, ret);
				}
				else
				{
					TRACE_STREAM_ERROR().appendf("Missing material '{}' permutation '{}'. Key {}.", m_contextName, setup, key);
				}
			}
		}

		return ret;
	}

    ///---

} // rendering