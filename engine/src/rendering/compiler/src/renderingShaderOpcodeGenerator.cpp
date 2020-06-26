/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\opcodes #]
***/

#include "build.h"
#include "renderingShaderOpcodeGenerator.h"

namespace rendering
{
    namespace compiler
    {
        namespace opcodes
        {

            ///--

            RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IShaderOpcodeGenerator);
            RTTI_END_TYPE();

            IShaderOpcodeGenerator::~IShaderOpcodeGenerator()
            {}

            ///--

            /// NULL implementation of the opcode generator, used when we just want to "dry compile" the permutations without doing much work
            class RENDERING_COMPILER_API NullShaderOpcodeGenerator : public IShaderOpcodeGenerator
            {
                RTTI_DECLARE_VIRTUAL_CLASS(NullShaderOpcodeGenerator, IShaderOpcodeGenerator);

            public:
                virtual ~NullShaderOpcodeGenerator()
                {}

                virtual bool generateOpcodes(const ShaderOpcodeGenerationContext& context, base::Buffer& outData, ShaderStageDependencies& outDependencies, base::parser::IErrorReporter& err) const
                {
                    return false;
                }

                virtual bool buildResourceBinding(const base::Array<const ResourceTable*>& resourceTables, ShaderResourceBindings& outBinding, base::parser::IErrorReporter& err) const
                {
                    return false;
                }
            };

            RTTI_BEGIN_TYPE_CLASS(NullShaderOpcodeGenerator);
            RTTI_END_TYPE();

            ///--

        } // opcodes
    } // shader
} // rendering