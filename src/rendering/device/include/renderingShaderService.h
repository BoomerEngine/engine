/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
***/

#pragma once

#include "renderingShaderSelector.h"
#include "base/io/include/timestamp.h"

namespace rendering
{
    //----

    /// shader code dependency
    struct RENDERING_DEVICE_API ShaderDependency
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(ShaderDependency);

        ShaderDependency();

        base::StringBuf path;
        base::io::TimeStamp timestamp;
    };

    //----

    /// shader runtime compiler
    class RENDERING_DEVICE_API IShaderRuntimeCompiler : public base::IReferencable
    {
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IShaderRuntimeCompiler);

    public:
        virtual ~IShaderRuntimeCompiler();

        // compile file
        virtual ShaderDataPtr compileFile(
            base::StringView filePath, // source shader path 
            base::HashMap<base::StringID, base::StringBuf>* defines,
            base::Array<ShaderDependency>& outDependencies) = 0;  // defines and their values

        // compile arbitrary shader code
        virtual ShaderDataPtr compileCode(
            base::StringView code, // code to compile
            base::HashMap<base::StringID, base::StringBuf>* defines,
            base::Array<ShaderDependency>& outDependencies) = 0;  // defines and their values
    };

    //----

    /// service for loading/compiling shaders
    class RENDERING_DEVICE_API ShaderService : public base::app::ILocalService
    {
		RTTI_DECLARE_VIRTUAL_CLASS(ShaderService, base::app::ILocalService);

    public:
		ShaderService();

		//--

		// query system shader (from /engine/shaders) compiled with given defines
		ShaderDataPtr loadSystemShader(base::StringView path, const ShaderSelector& selector = ShaderSelector());

		//--

		// load cached custom shader, valid only if we have material cache
		ShaderDataPtr loadCustomShader(uint64_t hash);

        // compile custom shader
        ShaderDataPtr compileCustomShader(uint64_t hash, base::StringView code, base::HashMap<base::StringID, base::StringBuf>* defines = nullptr);

        //--

    private:
        base::RefPtr<IShaderRuntimeCompiler> m_runtimeCompiler;

        //--

        base::SpinLock m_loadedShadersLock;
        base::HashMap<uint64_t, ShaderDataPtr> m_loadedShaders;

        //--

        virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
        virtual void onShutdownService() override final;
        virtual void onSyncUpdate() override final;
    };

    //----

} // rendering
