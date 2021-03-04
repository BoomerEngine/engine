/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
***/

#pragma once

#include "shaderSelector.h"
#include "core/io/include/timestamp.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//----

/// shader code dependency
struct GPU_DEVICE_API ShaderDependency
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ShaderDependency);

    ShaderDependency();

    StringBuf path;
    TimeStamp timestamp;
};

//----

/// shader runtime compiler
class GPU_DEVICE_API IShaderRuntimeCompiler : public IReferencable
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IShaderRuntimeCompiler);

public:
    virtual ~IShaderRuntimeCompiler();

    // compile file
    virtual ShaderDataPtr compileFile(
        StringView filePath, // source shader path
        HashMap<StringID, StringBuf>* defines,
        Array<ShaderDependency>& outDependencies) = 0;  // defines and their values

    // compile arbitrary shader code
    virtual ShaderDataPtr compileCode(
        StringView code, // code to compile
        HashMap<StringID, StringBuf>* defines,
        Array<ShaderDependency>& outDependencies) = 0;  // defines and their values
};

//----

/// service for loading/compiling shaders
class GPU_DEVICE_API ShaderService : public app::ILocalService
{
	RTTI_DECLARE_VIRTUAL_CLASS(ShaderService, app::ILocalService);

public:
	ShaderService();

	//--

	// query system shader (from /engine/shaders) compiled with given defines
	ShaderDataPtr loadSystemShader(StringView path, const ShaderSelector& selector = ShaderSelector());

	//--

	// load cached custom shader, valid only if we have material cache
	ShaderDataPtr loadCustomShader(uint64_t hash);

    // compile custom shader
    ShaderDataPtr compileCustomShader(uint64_t hash, StringView code, HashMap<StringID, StringBuf>* defines = nullptr);

    //--

private:
    RefPtr<IShaderRuntimeCompiler> m_runtimeCompiler;

    //--

    SpinLock m_loadedShadersLock;
    HashMap<uint64_t, ShaderDataPtr> m_loadedShaders;

    //--

    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;
};

//----

END_BOOMER_NAMESPACE_EX(gpu)
