/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
***/

#pragma once

#include "renderingShaderSelector.h"

namespace rendering
{

	//----

	/// static shader permutation
	class RENDERING_DEVICE_API StaticShaderPermutation : public base::NoCopy
	{
	public:
		StaticShaderPermutation(base::StringView path, const ShaderSelector& selector = ShaderSelector());
		~StaticShaderPermutation();
		
		GraphicsPipelineObjectPtr loadGraphicsPSO() const;
		ComputePipelineObjectPtr loadComputePSO() const;

	private:
		base::StringBuf m_path;	
		ShaderSelector m_selector;

		base::SpinLock m_lock;

		mutable GraphicsPipelineObjectPtr m_lastValidGraphicsPSO;
		mutable ComputePipelineObjectPtr m_lastValidComputePSO;
	};

	//----

} // rendering
