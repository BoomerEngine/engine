/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#include "build.h"
#include "renderingShaderCodeNode.h"
#include "renderingShaderNativeFunction.h"
#include "renderingShaderFunction.h"

#include "base/containers/include/stringBuilder.h"

namespace rendering
{
    namespace compiler
    {

        //---

        RTTI_BEGIN_TYPE_ENUM(OpCode);
            RTTI_ENUM_OPTION(Nop);
            RTTI_ENUM_OPTION(Scope);
            RTTI_ENUM_OPTION(Load);
            RTTI_ENUM_OPTION(Store);
            RTTI_ENUM_OPTION(Call);
            RTTI_ENUM_OPTION(NativeCall);
            RTTI_ENUM_OPTION(Const);
            RTTI_ENUM_OPTION(Cast);
            RTTI_ENUM_OPTION(Ident);
            RTTI_ENUM_OPTION(FuncRef);
            RTTI_ENUM_OPTION(ParamRef);
            RTTI_ENUM_OPTION(AccessArray);
            RTTI_ENUM_OPTION(AccessMember);
            RTTI_ENUM_OPTION(ReadSwizzle);
            RTTI_ENUM_OPTION(ProgramInstance);
            RTTI_ENUM_OPTION(ProgramInstanceParam);
            RTTI_ENUM_OPTION(VariableDecl);

            RTTI_ENUM_OPTION(First);
            RTTI_ENUM_OPTION(Loop);
            RTTI_ENUM_OPTION(Break);
            RTTI_ENUM_OPTION(Continue);
            RTTI_ENUM_OPTION(Exit);
            RTTI_ENUM_OPTION(Return);
            RTTI_ENUM_OPTION(IfElse);
        RTTI_END_TYPE();

        //---

        CodeNode::CodeNode()
            : m_op(OpCode::Nop)
        {}

        CodeNode::~CodeNode()
        {}

        CodeNode::CodeNode(const base::parser::Location& loc, const DataType& dataType, const DataValue& dataValue)
            : m_loc(loc)
            , m_op(OpCode::Const)
            , m_dataType(dataType)
            , m_dataValue(dataValue)
        {}

        CodeNode::CodeNode(const base::parser::Location& loc, OpCode op)
            : m_op(op)
            , m_loc(loc)
        {
        }

        CodeNode::CodeNode(const base::parser::Location& loc, OpCode op, const CodeNode* a)
            : m_op(op)
            , m_loc(loc)
        {
            m_children.pushBack(a);
        }

        CodeNode::CodeNode(const base::parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b)
            : m_op(op)
            , m_loc(loc)
        {
            m_children.pushBack(a);
            m_children.pushBack(b);
        }

        CodeNode::CodeNode(const base::parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b, const CodeNode* c)
            : m_op(op)
            , m_loc(loc)
        {
            m_children.pushBack(a);
            m_children.pushBack(b);
            m_children.pushBack(c);
        }

        CodeNode::CodeNode(const base::parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b, const CodeNode* c, const CodeNode* d)
            : m_op(op)
            , m_loc(loc)
        {
            m_children.pushBack(a);
            m_children.pushBack(b);
            m_children.pushBack(c);
            m_children.pushBack(d);
        }

        CodeNode::CodeNode(const base::parser::Location& loc, OpCode op, const CodeNode* a, const CodeNode* b, const CodeNode* c, const CodeNode* d, const CodeNode* e)
            : m_op(op)
            , m_loc(loc)
        {
            m_children.pushBack(a);
            m_children.pushBack(b);
            m_children.pushBack(c);
            m_children.pushBack(d);
            m_children.pushBack(e);
        }

        CodeNode::CodeNode(const base::parser::Location& loc, OpCode op, const DataType& dataType)
            : m_op(op)
            , m_loc(loc)
        {
            m_dataType = dataType;
        }

        void CodeNode::addChild(const CodeNode* child)
        {
            m_children.pushBack(child);
        }

        void CodeNode::moveChildren(CodeNode* otherNode)
        {
            if (otherNode)
            {
                if (otherNode->m_op == OpCode::First)
                {
                    auto children = std::move(otherNode->m_children);
                    for (auto child : children)
                        m_children.pushBack(child);
                }
                /*else if (otherNode->op == OpCode::Scope && op == OpCode::Scope)
                {
                    auto children = std::move(otherNode->children);
                    for (auto child : children)
                        children.pushBack(child);
                }*/
                else
                {
                    m_children.pushBack(otherNode);
                }
            }
        }

        CodeNode* CodeNode::clone(base::mem::LinearAllocator& mem) const
        {
            // create the copy of the node
            auto copy  = mem.create<CodeNode>();
            copy->m_dataType = m_dataType;
            copy->m_dataValue = m_dataValue;
            copy->m_extraData = m_extraData;
            copy->m_loc = m_loc;
            copy->m_op = m_op;
            copy->m_typesResolved = m_typesResolved;

            // copy the children
            copy->m_children.reserve(m_children.size());
            for (auto child  : m_children)
                copy->m_children.pushBack(child->clone(mem));

            return copy;
        }

        void CodeNode::print(base::IFormatStream& builder, uint32_t level, uint32_t childIndex) const
        {
            builder.appendPadding(' ', level*2);

            auto opCodeNode  = base::reflection::GetEnumValueName(m_op);
            builder.appendf("[{}]: {} ", childIndex, opCodeNode);

            if (m_dataType.valid())
                builder << "type='" << m_dataType << "' ";

            // value
            if (m_op == OpCode::ReadSwizzle)
            {
                builder.appendf("swizzle='{}' ", m_extraData.m_mask);
            }
            else if (m_op == OpCode::Ident)
            {
                builder.appendf("ident='{}' ", m_extraData.m_name);
            }
            else if (m_op == OpCode::NativeCall)
            {
               builder.appendf("nativeFunction='{}' ", m_extraData.m_nativeFunctionRef->cls()->findMetadataRef<FunctionNameMetadata>().name());
            }
            else if (m_op == OpCode::ParamRef)
            {
                builder.appendf("param='{}' ", *m_extraData.m_paramRef);
            }
            else if (m_op == OpCode::FuncRef)
            {
                builder.appendf("func='{}' ", m_extraData.m_name);
            }
            else if (m_op == OpCode::VariableDecl)
            {
                builder.appendf("name='{}' type='{}'", m_extraData.m_name, m_extraData.m_castType);
            }
            else if (m_op == OpCode::AccessMember)
            {
                builder.appendf("member='{}' ", m_extraData.m_name);
            }

            if (m_op == OpCode::Const)
            {
                builder.append("value='");
                builder << m_dataValue;
                builder.append("' ");
            }

            if (!m_children.empty())
            {
                builder.append("\n");

                // children
                for (uint32_t i = 0; i < m_children.size(); ++i)
                    if (m_children[i])
                        m_children[i]->print(builder, level+1, i);
            }
            else
            {
                builder.append("\n");
            }
        }

    } // shader
} // rendering
