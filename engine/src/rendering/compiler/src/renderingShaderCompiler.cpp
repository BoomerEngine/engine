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
#include "renderingShaderFunctionFolder.h"
#include "renderingShaderFileParser.h"
#include "renderingShaderCompiler.h"
#include "renderingShaderExporter.h"

#include "base/containers/include/stringBuilder.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/parser/include/textFilePreprocessor.h"

#include "rendering/device/include/renderingShaderData.h"

namespace rendering
{
    namespace compiler
    {

        //---

        class CommandErrorForwarder : public base::parser::IErrorReporter
        {
        public:
            CommandErrorForwarder(base::parser::IErrorReporter& err)
                : m_hasErrors(false)
                , m_errorHandler(err)
            {}

            virtual void reportError(const base::parser::Location& loc, base::StringView message) override
            {
                m_errorHandler.reportError(loc, message);
            }

            virtual void reportWarning(const base::parser::Location& loc, base::StringView message) override
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

		ShaderDataPtr CompileShader(
			base::StringView code, // code to compile
			base::StringView contextPath, // context path to print with errors (Z:/projects/awesome/shaders/box.fx)
			base::StringView sourceContentDebugPath,
			base::parser::IIncludeHandler* includeHandler, // handler for any files we want to include (ShaderCompilerDefaultIncludeHandler in cooking)
			base::parser::IErrorReporter& err, // where to report any errors (ShaderCompilerDefaultErrorReporter in cooking)
			base::HashMap<base::StringID, base::StringBuf>* defines)  // defines and their values
		{
            base::mem::LinearAllocator mem(POOL_SHADER_COMPILATION);
            base::ScopeTimer timer;

            // setup preprocessor
            base::parser::TextFilePreprocessor parser(mem, *includeHandler, err, base::parser::ICommentEater::StandardComments(), parser::GetShaderLanguageDefinition());

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
			// NOTE: may fail if undefined symbols are encountered
            if (!parser.processContent(code, contextPath))
                return false;

            // parse and compile code
            CommandErrorForwarder errForwarder(err);
            TypeLibrary typeLibrary(mem);
            CodeLibrary codeLibrary(mem, typeLibrary);
            if (!codeLibrary.parseContent(parser.tokens(), err))
                return false;

			// assemble final data
			AssembledShader data;
			if (!AssembleShaderStubs(mem, codeLibrary, data, sourceContentDebugPath, err))
				return false;
			
            // prints stats
			TRACE_INFO("Compiled shader file '{}' in {}, used {} ({} allocs). Generated {} of data",
				contextPath, TimeInterval(timer.timeElapsed()), MemSize(mem.totalUsedMemory()), mem.numAllocations(), MemSize(data.blob.size()));

			// create the data object
            return base::RefNew<ShaderData>(data.blob, data.metadata);
        }
       
        //--

    } // compiler
} // rendering

