/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
*/

#include "build.h"
#include "renderingShaderData.h"
#include "renderingShaderService.h"
#include "renderingShaderMetadata.h"

#include "rendering/device/include/renderingShader.h"

namespace rendering
{
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

    base::app::ServiceInitializationResult ShaderService::onInitializeService(const base::app::CommandLine& cmdLine)
    {
        // find runtime compiler
        if (const auto runtimeCompilerClass = RTTI::GetInstance().findClass("rendering::compiler::ShaderRuntimeCompiler"_id))
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

        return base::app::ServiceInitializationResult::Finished;
    }

    void ShaderService::onShutdownService()
    {
        // nothing
    }

    void ShaderService::onSyncUpdate()
    {
        // TODO: periodically save shader cache
    }

    ShaderDataPtr ShaderService::loadSystemShader(base::StringView path, const ShaderSelector& selector)
    {
        // build key
        base::CRC64 key;
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
            base::HashMap<base::StringID, base::StringBuf> defines;
            selector.defines(defines);

            base::Array<ShaderDependency> dependencies; // not used now
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

    ShaderDataPtr ShaderService::compileCustomShader(uint64_t hash, base::StringView code, base::HashMap<base::StringID, base::StringBuf>* defines)
    {
        DEBUG_CHECK_RETURN_EX_V(m_runtimeCompiler, "No runtime shader compiler", nullptr);

        base::Array<ShaderDependency> dependencies; // not used now
        auto ret = m_runtimeCompiler->compileCode(code, defines, dependencies);
        if (ret)
        {
            //auto lock = CreateLock(m_loadedShadersLock);
            //m_loadedShaders[hash] = ret;
        }
        
        return ret;
    }

    //--

    ShaderDataPtr LoadStaticShader(base::StringView path)
    {
        return LoadStaticShader(path, ShaderSelector());
    }

    ShaderDataPtr LoadStaticShader(base::StringView path, const ShaderSelector& selectors)
    {
        if (auto service = base::GetService<ShaderService>())
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
        static auto ptr = base::RefNew<NullShaderObject>();
        return ptr;
    }

    ShaderObjectPtr LoadStaticShaderDeviceObject(base::StringView path)
    {
        return LoadStaticShaderDeviceObject(path, ShaderSelector());
    }

    ShaderObjectPtr LoadStaticShaderDeviceObject(base::StringView path, const ShaderSelector& selectors)
    {
        if (auto service = base::GetService<ShaderService>())
            if (auto data = service->loadSystemShader(path))
                if (data->deviceShader())
                    return data->deviceShader();

        return GetNullShaderObject();
    }

    //--

} // rendering
