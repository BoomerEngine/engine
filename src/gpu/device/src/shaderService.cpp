/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
*/

#include "build.h"
#include "shaderData.h"
#include "shaderService.h"
#include "shaderMetadata.h"

#include "gpu/device/include/shader.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

RTTI_BEGIN_TYPE_STRUCT(ShaderDependency);
    RTTI_PROPERTY(path);
    RTTI_PROPERTY(timestamp);
RTTI_END_TYPE();

ShaderDependency::ShaderDependency()
{}

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IShaderRuntimeCompiler);
RTTI_END_TYPE();

IShaderRuntimeCompiler::~IShaderRuntimeCompiler()
{}

//--

RTTI_BEGIN_TYPE_CLASS(ShaderService);
RTTI_END_TYPE();

ShaderService::ShaderService()
{}

bool ShaderService::onInitializeService(const CommandLine& cmdLine)
{
    // find runtime compiler
    if (const auto runtimeCompilerClass = RTTI::GetInstance().findClass("gpu::compiler::ShaderCompiler"_id))
    {
        TRACE_INFO("Found runtime shader compiler, ad-hoc compilation will be possible");
        m_runtimeCompiler = runtimeCompilerClass->create<IShaderRuntimeCompiler>();
    }
    else
    {
        TRACE_INFO("No runtime shader compiler found, missing shaders won't be compiled");
    }

    // TODO: load global shader cache
    // TODO: load local shader cache

    return true;
}

void ShaderService::onShutdownService()
{
    // nothing
}

void ShaderService::onSyncUpdate()
{
    // TODO: periodically save shader cache
}

ShaderDataPtr ShaderService::loadSystemShader(StringView path, const ShaderSelector& selector)
{
    // build key
    CRC64 key;
    key << "SHADER_";
    key << path;
    selector.hash(key);

    // find in runtime cache
    {
        auto lock = CreateLock(m_loadedShadersLock);

        ShaderDataPtr data;
        if (m_loadedShaders.find(key.crc(), data))
            return data;
    }

    // compile
    ShaderDataPtr ret;
    if (m_runtimeCompiler)
    {
        HashMap<StringID, StringBuf> defines;
        selector.defines(defines);

        Array<ShaderDependency> dependencies; // not used now
        ret = m_runtimeCompiler->compileFile(path, &defines, dependencies);
    }
    else
    {
        TRACE_WARNING("Shader compilation not possible");
    }

    // store in memory cache
    if (ret)
    {
        auto lock = CreateLock(m_loadedShadersLock);
        m_loadedShaders[key.crc()] = ret;
    }

    // TODO: add to persistent local cache

    return ret;
}

ShaderDataPtr ShaderService::loadCustomShader(uint64_t hash)
{
    auto lock = CreateLock(m_loadedShadersLock);

    ShaderDataPtr data;
    m_loadedShaders.find(hash, data);
    return data;
}

ShaderDataPtr ShaderService::compileCustomShader(uint64_t hash, StringView code, HashMap<StringID, StringBuf>* defines)
{
    DEBUG_CHECK_RETURN_EX_V(m_runtimeCompiler, "No runtime shader compiler", nullptr);

    Array<ShaderDependency> dependencies; // not used now
    auto ret = m_runtimeCompiler->compileCode(code, defines, dependencies);
    if (ret)
    {
        //auto lock = CreateLock(m_loadedShadersLock);
        //m_loadedShaders[hash] = ret;
    }
        
    return ret;
}

//--

ShaderDataPtr LoadStaticShader(StringView path)
{
    return LoadStaticShader(path, ShaderSelector());
}

ShaderDataPtr LoadStaticShader(StringView path, const ShaderSelector& selectors)
{
    if (auto service = GetService<ShaderService>())
        return service->loadSystemShader(path, selectors);
    return nullptr;
}

class NullShaderObject : public ShaderObject
{
public:
    NullShaderObject()
        : ShaderObject(0, nullptr, new ShaderMetadata())
    {}

    virtual GraphicsPipelineObjectPtr createGraphicsPipeline(const GraphicsRenderStatesObject* renderStats) override
    {
        return nullptr;
    }

    virtual ComputePipelineObjectPtr createComputePipeline() override
    {
        return nullptr;
    }
};

static ShaderObjectPtr GetNullShaderObject()
{
    static auto ptr = RefNew<NullShaderObject>();
    return ptr;
}

ShaderObjectPtr LoadStaticShaderDeviceObject(StringView path)
{
    return LoadStaticShaderDeviceObject(path, ShaderSelector());
}

ShaderObjectPtr LoadStaticShaderDeviceObject(StringView path, const ShaderSelector& selectors)
{
    if (auto service = GetService<ShaderService>())
        if (auto data = service->loadSystemShader(path, selectors))
            if (data->deviceShader())
                return data->deviceShader();

    return GetNullShaderObject();
}

//--

END_BOOMER_NAMESPACE_EX(gpu)
