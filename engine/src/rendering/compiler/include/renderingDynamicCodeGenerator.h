/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\dynamic #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/memory/include/linearAllocator.h"
#include "base/containers/include/inplaceArray.h"

namespace rendering
{
    namespace compiler
    {
        ///---

        enum class DynamicCodeChunkDataType : uint8_t
        {
            None,
            Float,
            Float2,
            Float3,
            Float4,
            Bool,
            Texture2D,
            Texture2DArray,
            TextureCube,
            TextureCubeArray,
        };

        ///---

        /// Swizzle for dynamic code
        enum class DynamicCodeSwizzle : uint8_t
        {
            X,
            Y,
            Z,
            W,
            One,
            Zero,
        };

        ///---

        /// Node type for dynamic compiler
        enum class DynamicCodeNodeOp : uint8_t
        {
            Invalid=0,

            OpExternalParam,
            OpLocalVarAssignment,
            OpInputFunctionCall,
            OpGenericFunctionCall,

            OpSampleTexture,
            OpConstant,
            OpMakeFloat,
            OpSwizzle,
            OpAdd,
            OpMul,
            OpSub,
            OpMin,
            OpMax,
            OpDot,
            OpSaturate,
            OpClamp,
            OpNeg,
            OpFrac,
            OpCeil,
            OpFloor,
            OpRound,
            OpFmod,
            OpSin,
            OpCos,
            OpTan,
            OpAtan,
            OpPow,
            OpExp,
            OpLn,
            OpInv,
            OpOneMinus,
            OpLerp,

            OpCmpEq,
            OpCmpNeq,
            OpCmpLt,
            OpCmpLe,
            OpCmpGt,
            OpCmpGe,

            OpSelect,

            OpLogicAnd,
            OpLogicOr,
            OpLogicXor,
            OpLogicNot,
            OpLogicTrue,
            OpLogicFalse,
        };

        ///---

        /// ID to the code chunk
        typedef uint32_t DynamicCodeChunk;

        ///---

        /// helper class that can be used to build dynamic code
        class RENDERING_COMPILER_API DynamicCompiler : public base::NoCopy
        {
        public:
            DynamicCompiler();
            ~DynamicCompiler();

            //--

            // output code into a text builder
            void generateCode(const base::StringBuf& templateText, base::StringBuilder& ret) const;

            //--

            // get data type of given code chunk, returns DynamicCodeChunkDataType::Invalid if code chunk is not a valid one
            DynamicCodeChunkDataType type(DynamicCodeChunk id) const;

            // get opcode of given code chunk, returns DynamicCodeChunkDataType::Invalid if code chunk is not a valid one
            DynamicCodeNodeOp op(DynamicCodeChunk id) const;

            //--

            /// declare a parameter to be passed to shaders
            DynamicCodeChunk requestParam(const char* name, DynamicCodeChunkDataType dataType);

            //--

            /// run an input function provided by the template code, results are cached
            DynamicCodeChunk runInputFunction(const char* name, DynamicCodeChunkDataType retType);

            /// run a function provided by the template code
            DynamicCodeChunk runFunction(const char* name, DynamicCodeChunkDataType retType, std::initializer_list<DynamicCodeChunk> params = std::initializer_list<DynamicCodeChunk>());

            //--

            /// emit output of the shader
            void emitOutput(const char* name, DynamicCodeChunk id);

            /// emit local variable with initialization, type is carried though
            DynamicCodeChunk emitLocal(DynamicCodeChunk a);

            //--

            /// sample a texture
            DynamicCodeChunk sampleTexture(DynamicCodeChunk tex, DynamicCodeChunk uv);

            ///---

            /// constants
            DynamicCodeChunk constant(float val);
            DynamicCodeChunk constant(const base::Vector2& val);
            DynamicCodeChunk constant(const base::Vector3& val);
            DynamicCodeChunk constant(const base::Vector4& val);

            /// constructions
            DynamicCodeChunk float2(DynamicCodeChunk x, DynamicCodeChunk y);
            DynamicCodeChunk float3(DynamicCodeChunk x, DynamicCodeChunk y, DynamicCodeChunk z);
            DynamicCodeChunk float4(DynamicCodeChunk x, DynamicCodeChunk y, DynamicCodeChunk z, DynamicCodeChunk w);

            /// swizzle vector components
            DynamicCodeChunk swizzle(DynamicCodeChunk val, const char* swz);
            DynamicCodeChunk swizzle(DynamicCodeChunk val, const DynamicCodeSwizzle* swz, uint32_t numComponents);

            /// math operations
            DynamicCodeChunk add(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk mul(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk sub(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk min(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk max(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk dot4(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk dot3(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk dot2(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk saturate(DynamicCodeChunk a);
            DynamicCodeChunk clamp(DynamicCodeChunk a, DynamicCodeChunk vmin, DynamicCodeChunk vmax);
            DynamicCodeChunk neg(DynamicCodeChunk a);
            DynamicCodeChunk frac(DynamicCodeChunk a);
            DynamicCodeChunk floor(DynamicCodeChunk a);
            DynamicCodeChunk ceil(DynamicCodeChunk a);
            DynamicCodeChunk fmod(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk sin(DynamicCodeChunk a);
            DynamicCodeChunk cos(DynamicCodeChunk a);
            DynamicCodeChunk tan(DynamicCodeChunk a);
            DynamicCodeChunk atan(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk pow(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk exp(DynamicCodeChunk a);
            DynamicCodeChunk ln(DynamicCodeChunk a);
            DynamicCodeChunk inv(DynamicCodeChunk a);
            DynamicCodeChunk oneMinus(DynamicCodeChunk a);
            DynamicCodeChunk lerp(DynamicCodeChunk a, DynamicCodeChunk b, DynamicCodeChunk s);

            /// compare, returns boolean or boolean vector
            DynamicCodeChunk cmpEq(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk cmpNeq(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk cmpLt(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk cmpLe(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk cmpGt(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk cmpGe(DynamicCodeChunk a, DynamicCodeChunk b);

            /// select based on mask, per component
            DynamicCodeChunk select(DynamicCodeChunk a, DynamicCodeChunk b, DynamicCodeChunk mask);

            /// boolean
            DynamicCodeChunk logicAnd(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk logicOr(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk logicXor(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunk logicNot(DynamicCodeChunk a);
            DynamicCodeChunk logicTrue();
            DynamicCodeChunk logicFalse();

            ///---

        private:
            base::mem::LinearAllocator m_mem;

            DynamicCodeChunk convertType(DynamicCodeChunk node, DynamicCodeChunkDataType type);
            bool changeTypeInplace(DynamicCodeChunk& node, DynamicCodeChunkDataType type);

            bool matchNumericalType(DynamicCodeChunk& a, DynamicCodeChunk& b);
            bool matchNumericalType(DynamicCodeChunk& a, DynamicCodeChunk& b, DynamicCodeChunk& c);

            DynamicCodeChunkDataType returnNumericalType(DynamicCodeChunk a, DynamicCodeChunk b);
            DynamicCodeChunkDataType returnNumericalType(DynamicCodeChunk a, DynamicCodeChunk b, DynamicCodeChunk c);

            DynamicCodeChunk printError(DynamicCodeChunk id, const char* txt) const;
            mutable bool m_hasError;

            struct DynamicCodeNode;

            struct DynamicCodeBlock
            {
                base::Array<DynamicCodeNode*> m_code;
            };

            struct DynamicParam
            {
                base::StringBuf m_name;
                DynamicCodeChunkDataType m_type;
                DynamicCodeNode* m_node;
            };

            struct DynamicInput
            {
                base::StringBuf m_paramName;
                base::StringBuf m_funcName;
                DynamicCodeChunkDataType m_type;
                DynamicCodeNode* m_node;
            };

            struct DynamicOutput
            {
                base::StringBuf m_name;
                DynamicCodeNode* m_node;
            };

            struct DynamicTempVar
            {
                base::StringBuf m_name;
                DynamicCodeChunkDataType m_type;
            };

            /// Code node for dynamic shader
            struct DynamicCodeNode
            {
                DynamicCodeChunk m_id;
                DynamicCodeNodeOp m_op;
                DynamicCodeChunkDataType m_type;
                base::InplaceArray<DynamicCodeNode*, 4> m_children;
                base::InplaceArray<DynamicCodeSwizzle, 4> m_swizzle;

                struct Value
                {
                    base::StringBuf m_name;
                    base::Vector4 m_const;
                } m_value;

                INLINE DynamicCodeNode()
                    : m_id(0)
                    , m_op(DynamicCodeNodeOp::Invalid)
                    , m_type(DynamicCodeChunkDataType::None)
                {}
            };


            base::Array<DynamicCodeNode*> m_nodes;
            base::Array<DynamicParam> m_params;
            base::Array<DynamicInput> m_inputs;
            base::Array<DynamicOutput> m_outputs;
            base::Array<DynamicTempVar> m_vars;

            static void GenerateCodeNode(base::StringBuilder& ret, const DynamicCodeNode* node);
            static void GenerateCodeBlock(base::StringBuilder& ret, const DynamicCodeBlock* block);
            static void GenerateCodeForchildren(base::StringBuilder& ret, const DynamicCodeNode* node);

            bool validateType(DynamicCodeChunk id, DynamicCodeChunkDataType type) const;

            INLINE DynamicCodeNode* node(DynamicCodeChunk id) const
            {
                return (id > 0 || id < m_nodes.size()) ? m_nodes[id] : nullptr;
            }

            INLINE DynamicCodeNode* allocNode(DynamicCodeNodeOp op, DynamicCodeChunkDataType type)
            {
                auto node  = m_mem.create<DynamicCodeNode>();
                node->m_id = range_cast<DynamicCodeChunk>(m_nodes.size());
                node->m_op = op;
                node->m_type = type;
                m_nodes.pushBack(node);
                return node;
            }

            DynamicCodeBlock* m_block;

            uint32_t m_varIdAllocator;
        };

        ///---

    } // compiler
} // rendering