/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*/

#include "build.h"

#include "renderingShaderProgram.h"
#include "renderingShaderFunction.h"
#include "renderingShaderStaticExecution.h"
#include "renderingShaderProgramInstance.h"
#include "renderingShaderProgram.h"
#include "renderingShaderCodeLibrary.h"
#include "renderingShaderLibraryDataBuilder.h"
#include "renderingShaderProgramLinker.h"
#include "renderingShaderFunctionFolder.h"
#include "renderingShaderOpcodeGenerator.h"
#include "renderingShaderFileParser.h"
#include "renderingShaderLibraryCompiler.h"

#include "base/io/include/absolutePath.h"
#include "base/containers/include/stringBuilder.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"

#include "rendering/compiler/include/renderingShaderOpcodeGenerator.h"
#include "base/parser/include/textFilePreprocessor.h"

namespace rendering
{
    namespace compiler
    {

        //---

        static base::mem::PoolID POOL_SHADER_COMPILATION("Rendering.ShaderCompilation");

        //--

        class CommandErrorForwarder : public base::parser::IErrorReporter
        {
        public:
            CommandErrorForwarder(base::parser::IErrorReporter& err)
                : m_hasErrors(false)
                , m_errorHandler(err)
            {}

            virtual void reportError(const base::parser::Location& loc, base::StringView<char> message) override
            {
                m_errorHandler.reportError(loc, message);
            }

            virtual void reportWarning(const base::parser::Location& loc, base::StringView<char> message) override
            {
                m_errorHandler.reportWarning(loc, message);
            }

            INLINE bool hasErrors() const
            {
                return m_hasErrors;
            }

        private:
            base::parser::IErrorReporter& m_errorHandler;
            bool m_hasErrors;
        };

        //--

        static bool ExtractShaderBundle(const ExportedShaders& exports, CodeLibrary& code, ShaderBunderSetup& outBundleSetup, base::parser::IErrorReporter& err)
        {
            bool valid = true;

            for (int i = 1; i < (int)ShaderType::MAX; ++i)
            {
                if (const auto* program = exports.exports[i])
                {
                    ProgramConstants constants; // injects ?

                    if (const auto* main = program->findFunction("main"_id))
                    {
                        if (auto* pi = code.createProgramInstance(program->location(), program, constants, err))
                        {
                            outBundleSetup.location = program->location();
                            outBundleSetup.stages[i].pi = pi;
                            outBundleSetup.stages[i].func = main;
                        }
                        else
                        {
                            err.reportError(program->location(), base::TempString("Failed to create instance of shader '{}' required for exporting", program->name()));
                            valid = false;
                        }
                    }
                    else
                    {
                        err.reportError(program->location(), base::TempString("Exported shader '{}' should have main() function :)", program->name()));
                        valid = false;
                    }
                }
            }

            return valid;
        }

        //--

        ShaderLibraryPtr CompileShaderLibrary(
            base::StringView<char> code, // code to compile
            base::StringView<char> contextPath, // name of the shader, 
            base::parser::IIncludeHandler* includeHandler, // handler for any files we want to include
            base::parser::IErrorReporter& err, // where to report any errors
            base::StringID nativeGeneratorName,
            base::HashMap<base::StringID, base::StringBuf>* defines /*=nullptr*/
        )
        {
            base::mem::LinearAllocator mem(POOL_SHADER_COMPILATION);
            base::ScopeTimer timer;

            // find shader generator
            auto opcodeGeneratorClass = RTTI::GetInstance().findClass("GLSLOpcodeGenerator"_id).cast<opcodes::IShaderOpcodeGenerator>();
            auto opcodeGenerator = opcodeGeneratorClass ? opcodeGeneratorClass->create<opcodes::IShaderOpcodeGenerator>() : nullptr;
            if (!opcodeGenerator)
            {
                TRACE_ERROR("No code generator to generate final shader code found");
                return nullptr;
            }

            // preprocess code
            base::parser::TextFilePreprocessor parser(mem, *includeHandler, err, base::parser::ICommentEater::StandardComments(), parser::GetShaderLanguageDefinition());

            // inject system defines from opcode generator (ie. GLSL, )
            if (nativeGeneratorName == "GLSL"_id)
                parser.defineSymbol("FLIP_Y", "1");
            else
                parser.defineSymbol("FLIP_Y", "0");

            // inject given defines
            if (nullptr != defines)
            {
                for (uint32_t i = 0; i < defines->size(); ++i)
                {
                    parser.defineSymbol(defines->keys()[i].view(), defines->values()[i]);
                    TRACE_DEEP("Defined static shader permutation '{}' = '{}'", defines->keys()[i], defines->values()[i]);
                }
            }

            // process the code into tokens
            if (!parser.processContent(code, contextPath))
                return false;

            // parse and compile code
            CommandErrorForwarder errForwarder(err);
            TypeLibrary typeLibrary(mem);
            CodeLibrary codeLibrary(mem, typeLibrary);
            if (!codeLibrary.parseContent(parser.tokens(), err))
                return false;

            // see if we need to compile a shader bundle for this program
            ShaderBunderSetup bundleSetup;
            if (!ExtractShaderBundle(codeLibrary.exports(), codeLibrary, bundleSetup, err))
                return false;

            // generate final data
            ShaderLibraryBuilder dataBuilder;
            LinkerCache linkerCache(mem, codeLibrary);
            const auto index = AssembleShaderBundle(mem, dataBuilder, codeLibrary, linkerCache, opcodeGenerator.get(), contextPath, bundleSetup, err);
            if (index == INVALID_PIPELINE_INDEX)
                return false;

            // extract data from the builder to form final data for the shader library
            auto structureData = dataBuilder.extractStructureData();
            auto shaderData = dataBuilder.extractShaderData();

            // prints stats
            TRACE_INFO("Compiled shader file '{}' in {}, used {} ({} allocs), {} bundles, {} shaders. Generated {} structure and {} shader data", 
                contextPath, TimeInterval(timer.timeElapsed()), MemSize(mem.totalUsedMemory()), mem.numAllocations(),
                dataBuilder.shaderBundles().size(), dataBuilder.shaderBlobs().size(),
                MemSize(structureData.size()), MemSize(shaderData.size()));

            // save the library
            auto data = base::CreateSharedPtr<ShaderLibraryData>(structureData, shaderData);
            return base::CreateSharedPtr<ShaderLibrary>(data);
        }

        //--

        /*class ShaderCacheRuntimeCompilerDefault : public IShaderCacheRuntimeCompiler
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ShaderCacheRuntimeCompilerDefault, IShaderCacheRuntimeCompiler);

        public:
            virtual ShaderLibraryPtr compile(
                base::StringView<char> code, // code to compile
                base::StringView<char> contextPath, // context path to print with errors 
                base::parser::IIncludeHandler* includeHandler, // handler for any files we want to include
                base::parser::IErrorReporter& err, // where to report any errors
                base::StringID nativeGeneratorName, // GLSL, HLSL, NULL etc
                base::HashMap<base::StringID, base::StringBuf>* forceSinglePermutation) override final
            {
                return CompileShaderLibrary(code, contextPath, includeHandler, err, nativeGeneratorName, forceSinglePermutation);
            }
        };

        RTTI_BEGIN _TYPE_CLASS(ShaderCacheRuntimeCompilerDefault);
        RTTI_END_TYP E();*/

        //--

    } // compiler
} // rendering

