/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptFunctionCode.h"
#include "scriptLibrary.h"
#include "scriptTypeCasting.h"
#include "scriptOpcodeGenerator.h"

#include "base/containers/include/inplaceArray.h"
#include "base/containers/include/hashSet.h"

namespace base
{
    namespace script
    {

        ///---

        ConfigProperty<bool> cvDumpOpcodes("Script.Compiler", "DumpOpcodes", false);
        ConfigProperty<StringBuf> cvDumpOpcodesForFunction("Script.Compiler", "DumpOpcodesForFunction", StringBuf::EMPTY());

        ///---

        RTTI_BEGIN_TYPE_ENUM(FunctionNodeOp);
            RTTI_ENUM_OPTION(Nop);
            RTTI_ENUM_OPTION(Statement);
            RTTI_ENUM_OPTION(Scope);
            RTTI_ENUM_OPTION(StatementList);
            RTTI_ENUM_OPTION(ExpressionList);
            RTTI_ENUM_OPTION(IfThenElse);
            RTTI_ENUM_OPTION(Switch);
            RTTI_ENUM_OPTION(Case);
            RTTI_ENUM_OPTION(DefaultCase);
            RTTI_ENUM_OPTION(For);
            RTTI_ENUM_OPTION(While);
            RTTI_ENUM_OPTION(DoWhile);
            RTTI_ENUM_OPTION(Assign);
            RTTI_ENUM_OPTION(Operator);
            RTTI_ENUM_OPTION(Call);
            RTTI_ENUM_OPTION(Type);
            RTTI_ENUM_OPTION(New);
            RTTI_ENUM_OPTION(Ident);
            RTTI_ENUM_OPTION(AccessMember);
            RTTI_ENUM_OPTION(AccessIndex);
            RTTI_ENUM_OPTION(Var);
            RTTI_ENUM_OPTION(Const);
            RTTI_ENUM_OPTION(Return);
            RTTI_ENUM_OPTION(Break);
            RTTI_ENUM_OPTION(Continue);
            RTTI_ENUM_OPTION(Conditional);
            RTTI_ENUM_OPTION(This);

            RTTI_ENUM_OPTION(VarArg);
            RTTI_ENUM_OPTION(VarClass);
            RTTI_ENUM_OPTION(VarLocal);
            RTTI_ENUM_OPTION(FunctionVirtual);
            RTTI_ENUM_OPTION(FunctionStatic);
            RTTI_ENUM_OPTION(EnumConst);
            RTTI_ENUM_OPTION(CallFinal);
            RTTI_ENUM_OPTION(CallVirtual);
            RTTI_ENUM_OPTION(CallStatic);
            RTTI_ENUM_OPTION(MakeValueFromRef);
            RTTI_ENUM_OPTION(Context);
            RTTI_ENUM_OPTION(ContextRef);
            RTTI_ENUM_OPTION(MemberOffset);
            RTTI_ENUM_OPTION(MemberOffsetRef);
            RTTI_ENUM_OPTION(Construct);

            RTTI_ENUM_OPTION(GeneralEqual);
            RTTI_ENUM_OPTION(GeneralNotEqual);
            RTTI_ENUM_OPTION(PointerEqual);
            RTTI_ENUM_OPTION(PointerNotEqual);

            RTTI_ENUM_OPTION(CastWeakToStrong);
            RTTI_ENUM_OPTION(CastStrongToWeak);
            RTTI_ENUM_OPTION(CastDownStrong);
            RTTI_ENUM_OPTION(CastDownWeak);
            RTTI_ENUM_OPTION(CastEnumToInt64);
            RTTI_ENUM_OPTION(CastEnumToInt32);
            RTTI_ENUM_OPTION(CastEnumToName);
            RTTI_ENUM_OPTION(CastEnumToString);
            RTTI_ENUM_OPTION(CastInt64ToEnum);
            RTTI_ENUM_OPTION(CastInt32ToEnum);
            RTTI_ENUM_OPTION(CastNameToEnum);
            RTTI_ENUM_OPTION(CastStrongPtrToBool);
            RTTI_ENUM_OPTION(CastWeakPtrToBool);
            RTTI_ENUM_OPTION(CastTypeToVariant);
            RTTI_ENUM_OPTION(CastVariantToType);
            RTTI_ENUM_OPTION(CastClassMetaDownCast);
            RTTI_ENUM_OPTION(CastClassToBool);
            RTTI_ENUM_OPTION(CastClassToName);
            RTTI_ENUM_OPTION(CastClassToString);
        RTTI_END_TYPE();

        ///---

        FunctionTypeInfo FunctionTypeInfo::removeRef() const
        {
            FunctionTypeInfo ret(*this);
            ret.reference = false;
            return ret;
        }

        FunctionTypeInfo FunctionTypeInfo::makeRef() const
        {
            FunctionTypeInfo ret(*this);
            ret.reference = true;
            return ret;
        }

        FunctionTypeInfo FunctionTypeInfo::removeConst() const
        {
            FunctionTypeInfo ret(*this);
            ret.constant = false;
            return ret;
        }

        FunctionTypeInfo FunctionTypeInfo::makeConst() const
        {
            FunctionTypeInfo ret(*this);
            ret.constant = true;
            return ret;
        }

        bool FunctionTypeInfo::isStruct() const
        {
            if (type != nullptr && type->metaType == StubTypeType::Simple)
                return (type->referencedType->resolvedStub->stubType == StubType::Class);

            return false;
        }

        bool FunctionTypeInfo::isClassType() const
        {
            if (type != nullptr && type->metaType == StubTypeType::ClassType)
                return (type->referencedType->resolvedStub->stubType == StubType::Class);

            return false;
        }

        bool FunctionTypeInfo::isEnumType() const
        {
            if (type != nullptr && type->metaType == StubTypeType::Simple)
                return (type->referencedType->resolvedStub->stubType == StubType::Enum);

            return false;
        }

        bool FunctionTypeInfo::isSharedPtr() const
        {
            if (type)
                return type->metaType == StubTypeType::PtrType;
            return false;
        }

        bool FunctionTypeInfo::isWeakPtr() const
        {
            if (type)
                return type->metaType == StubTypeType::WeakPtrType;
            return false;
        }

        const StubEnum* FunctionTypeInfo::typeEnum() const
        {
            if (type && type->metaType == StubTypeType::Simple)
                return type->referencedType->resolvedStub->asEnum();

            return nullptr;
        }

        const StubClass* FunctionTypeInfo::typeClass() const
        {
            if (type)
            {
                if (type->metaType == StubTypeType::WeakPtrType || type->metaType == StubTypeType::PtrType || type->metaType == StubTypeType::ClassType || type->metaType == StubTypeType::Simple)
                    return type->referencedType->resolvedStub->asClass();
            }

            return nullptr;
        }

        bool FunctionTypeInfo::isAnyPtr() const
        {
            if (type)
                return (type->metaType == StubTypeType::PtrType) || (type->metaType == StubTypeType::WeakPtrType);
            return false;
        }

        ///---

        FunctionNumber::FunctionNumber(FunctionNumberType t)
            : type(t)
        {
            value.u = 0;
        }

        FunctionNumber::FunctionNumber(uint64_t u)
            : type(FunctionNumberType::Unsigned)
        {
            value.u = u;
        }

        FunctionNumber::FunctionNumber(int64_t i)
            : type(FunctionNumberType::Integer)
        {
            value.i = i;
        }

        FunctionNumber::FunctionNumber(double d)
            : type(FunctionNumberType::FloatingPoint)
        {
            value.f = d;
        }

        bool FunctionNumber::isNegative() const
        {
            if (type == FunctionNumberType::Integer)
                return value.i < 0;
            if (type == FunctionNumberType::FloatingPoint)
                return value.f < 0.0f;
            return false;
        }

        uint64_t FunctionNumber::asUnsigned() const
        {
            if (type == FunctionNumberType::FloatingPoint)
                return (uint64_t)value.f;
            return value.u;
        }

        int64_t FunctionNumber::asInteger() const
        {
            if (type == FunctionNumberType::FloatingPoint)
                return (int64_t)value.f;
            return value.i;
        }

        double FunctionNumber::asFloat() const
        {
            if (type == FunctionNumberType::Integer)
                return (double)value.i;
            else if (type == FunctionNumberType::Unsigned)
                return (double)value.u;
            return value.f;
        }

        bool FunctionNumber::fitsUint8() const
        {
            if (isNegative())
                return false;

            if (type == FunctionNumberType::FloatingPoint)
                return value.f <= (double)std::numeric_limits<uint8_t>::max();
            else
                return value.u <= std::numeric_limits<uint8_t>::max();
        }

        bool FunctionNumber::fitsUint16() const
        {
            if (isNegative())
                return false;

            if (type == FunctionNumberType::FloatingPoint)
                return value.f <= (double)std::numeric_limits<uint16_t>::max();
            else
                return value.u <= std::numeric_limits<uint16_t>::max();
        }

        bool FunctionNumber::fitsUint32() const
        {
            if (isNegative())
                return false;

            if (type == FunctionNumberType::FloatingPoint)
                return value.f <= (double)std::numeric_limits<uint32_t>::max();
            else
                return value.u <= std::numeric_limits<uint32_t>::max();
        }

        bool FunctionNumber::fitsUint64() const
        {
            if (isNegative())
                return false;

            if (type == FunctionNumberType::FloatingPoint)
                return value.f <= (double)std::numeric_limits<uint64_t>::max();
            else
                return true;
        }

        bool FunctionNumber::fitsInt8() const
        {
            if (type == FunctionNumberType::FloatingPoint)
                return value.f <= (double)std::numeric_limits<char>::max() && value.f >= (double)std::numeric_limits<char>::min();
            else
                return value.i <= std::numeric_limits<char>::max() && value.i >= std::numeric_limits<char>::min();
        }

        bool FunctionNumber::fitsInt16() const
        {
            if (type == FunctionNumberType::FloatingPoint)
                return value.f <= (double)std::numeric_limits<short>::max() && value.f >= (double)std::numeric_limits<short>::min();
            else
                return value.i <= std::numeric_limits<short>::max() && value.i >= std::numeric_limits<short>::min();
        }

        bool FunctionNumber::fitsInt32() const
        {
            if (type == FunctionNumberType::FloatingPoint)
                return value.f <= (double)std::numeric_limits<int>::max() && value.f >= (double)std::numeric_limits<int>::min();
            else
                return value.i <= std::numeric_limits<int>::max() && value.i >= std::numeric_limits<int>::min();
        }

        bool FunctionNumber::fitsInt64() const
        {
            if (type == FunctionNumberType::FloatingPoint)
                return value.f <= (double)std::numeric_limits<int64_t>::max() && value.f >= (double)std::numeric_limits<int64_t>::min();

            return true;
        }

        void FunctionNumber::print(IFormatStream& f) const
        {
            if (type == FunctionNumberType::Integer)
            {
                f << "integer: ";
                f << value.i;
            }
            else if (type == FunctionNumberType::Unsigned)
            {
                f << "unsigned: ";
                f << value.u;
            }
            else if (type == FunctionNumberType::FloatingPoint)
            {
                f << "float: ";
                f << value.f;
            }
        }

        ///---

        void FunctionNode::add(FunctionNode* node)
        {
            children.pushBack(node);
        }

        void FunctionNode::addFromStatementList(FunctionNode* node)
        {
            if (node != nullptr)
            {
                if (node->op == FunctionNodeOp::StatementList)
                {
                    for (auto child  : node->children)
                        addFromStatementList(child);
                }
                else
                {
                    add(node);
                }
            }
        }

        void FunctionNode::addFromExpressionList(FunctionNode* node)
        {
            if (node != nullptr)
            {
                if (node->op == FunctionNodeOp::ExpressionList)
                {
                    for (auto child  : node->children)
                        addFromExpressionList(child);
                }
                else
                {
                    add(node);
                }
            }
        }

        const FunctionNode* FunctionNode::child(uint32_t index) const
        {
            if (index >= children.size())
                return nullptr;
            return children[index];
        }

        void FunctionNode::print(uint32_t indent, IFormatStream& f) const
        {
            f << op;

            if (type)
            {
                f.append(" : ");
                f << type;
            }

            if (op == FunctionNodeOp::Const)
            {
                if (type.isType<char>() || type.isType<short>() || type.isType<int>() || type.isType<int64_t>())
                {
                    f.appendf(", valueInt = '{}'", data.number);
                }
                else if (type.isType<uint8_t>() || type.isType<uint16_t>() || type.isType<uint32_t>() || type.isType<uint64_t>())
                {
                    f.appendf(", valueUint = '{}'", data.number);
                }
                else if (type.isType<float>() || type.isType<double>())
                {
                    f.appendf(", valueFloat = '{}'", data.number);
                }
                else if (type.isType<bool>())
                {
                    f.appendf(", valueBool = '{}'", data.number.asInteger() != 0);
                }
                else if (type.isType<StringID>())
                {
                    f.appendf(", valueName = '{}'", data.name);
                }
                else if (type.isType<StringBuf>())
                {
                    f.appendf(", valueString = \"{}\"", data.text);
                }
            }
            else if (op == FunctionNodeOp::Type)
            {
                f.appendf(", dataType = '{}'", data.type);
            }
            else if (op == FunctionNodeOp::Var)
            {
                f.appendf(", varName = '{}', varType = '{}'", data.name, data.type);
            }
            else if (op == FunctionNodeOp::Ident)
            {
                f.appendf(", ident = '{}'", data.name);
            }
            else if (op == FunctionNodeOp::AccessMember)
            {
                f.appendf(", member = '{}'", data.name);
            }
            else if (op == FunctionNodeOp::MakeValueFromRef)
            {
                f.appendf(", type = '{}'", data.type);
            }
            else if (op == FunctionNodeOp::Operator)
            {
                f.appendf(", operator = '{}'", data.name);
            }
            else if (op == FunctionNodeOp::FunctionStatic || op == FunctionNodeOp::FunctionVirtual)
            {
                if (data.function)
                {
                    f.appendf(", func = '{}'", data.function->name);

                    if (op == FunctionNodeOp::FunctionStatic)
                    {
                        if (data.function->owner && data.function->owner->stubType == StubType::Class)
                            f.appendf(", class = '{}'", data.function->owner->name);
                    }
                }
            }
            else if (op == FunctionNodeOp::CallStatic || op == FunctionNodeOp::CallFinal || op == FunctionNodeOp::CallVirtual)
            {
                if (data.function)
                {
                    f.appendf(", func = '{}'", data.function->name);

                    if (op == FunctionNodeOp::CallStatic || op == FunctionNodeOp::CallFinal)
                    {
                        if (data.function->owner && data.function->owner->stubType == StubType::Class)
                            f.appendf(", class = '{}'", data.function->owner->name);
                    }
                }
            }
            else if (op == FunctionNodeOp::VarClass)
            {
                f.appendf(", var = '{}'", data.classVar ? data.classVar->name : "None"_id);
            }
            else if (op == FunctionNodeOp::VarArg)
            {
                f.appendf(", var = '{}'", data.argVar ? data.argVar->name : "None"_id);
            }
            else if (op == FunctionNodeOp::VarLocal)
            {
                f.appendf(", var = '{}'", data.localVar ? data.localVar->name : "None"_id);
            }
            else if (op == FunctionNodeOp::Statement)
            {
                f.append(", location = ");
                f << location;
            }

            f.append("\n");

            for (uint32_t i=0; i < children.size(); ++i)
            {
                f.appendPadding(' ', (indent+1)*2);
                f.appendf("{}: ", i);
                if (children[i])
                {
                    children[i]->print(indent + 1, f);
                }
                else
                {
                    f.append("null\n");
                }
            }
        }

        //---

        const FunctionVar* FunctionScope::findLocalVar(StringID name) const
        {
            const FunctionVar* ret = nullptr;
            m_localVars.find(name, ret);
            return ret;
        }

        const FunctionVar* FunctionScope::findVar(StringID name) const
        {
            if (auto ret  = findLocalVar(name))
                return ret;

            if (parent)
                return parent->findVar(name);

            return nullptr;
        }

        //---

        FunctionCode::FunctionCode(mem::LinearAllocator& mem, StubLibrary& lib, const StubFunction* function)
            : m_mem(mem)
            , m_lib(lib)
            , m_function(function)
            , m_class(nullptr)
            , m_static(true)
        {
            if (function->owner && function->owner->stubType == StubType::Class)
            {
                m_class = static_cast<const StubClass *>(function->owner);
                m_static = function->flags.test(StubFlag::Static);
            }
        }

        FunctionCode::~FunctionCode()
        {}

        void FunctionCode::rootNode(FunctionNode* rootNode)
        {
            ASSERT(m_rootNode == nullptr)
            m_rootNode = rootNode;
        }

        void FunctionCode::printTree()
        {
            if (m_rootNode)
                m_rootNode->print(0, TRACE_STREAM_INFO());
        }

        bool FunctionCode::compile(IErrorHandler& err)
        {
            OpcodeGenerator gen(m_mem);

            // generate opcodes for function
            OpcodeList opcodes;
            if (m_function->flags.test(StubFlag::Constructor))
            {
                if (!generateClassConstructor(err, gen, opcodes))
                    return false;
            }
            else if (m_function->flags.test(StubFlag::Destructor))
            {
                if (!generateClassDestructor(err, gen, opcodes))
                    return false;
            }
            else
            {
                if (!generateFunctionCodeOpcodes(err, gen, opcodes))
                    return false;
            }

            // extract opcodes
            opcodes.extract(const_cast<StubFunction*>(m_function)->opcodes);

            // dump
            if (cvDumpOpcodes.get() || (!cvDumpOpcodesForFunction.get().empty() && cvDumpOpcodesForFunction.get() == m_function->fullName()))
            {
                TRACE_INFO("Opcodes for function '{}': ", m_function->name);
                opcodes.print(TRACE_STREAM_INFO());
            }
            return true;
        }

        bool FunctionCode::generateFunctionCodeOpcodes(IErrorHandler& err, OpcodeGenerator& gen, OpcodeList& opcodes)
        {
            // we need to have the code
            if (!m_rootNode)
                return false;

            // connect all nodes into a scope structure
            if (!connectScopes(err, m_rootNode, nullptr))
                return false;

            // create local function variables
            if (!resolveVars(err, m_rootNode))
                return false;

            // determine node types, insert casts
            InplaceArray<FunctionNode*, 100> nodeStack;
            nodeStack.pushBack(nullptr); // so we can always get the parent by just doing nodeStack.back()
            if (!resolveTypes(err, m_rootNode, nodeStack))
                return false;

            // generate opcodes from code tree
            InplaceArray<const FunctionScope*, 64> activeScopes;
            opcodes = gen.generateOpcodes(err, m_rootNode, activeScopes);
            return true;
        }

        bool FunctionCode::generateClassConstructor(IErrorHandler& err, OpcodeGenerator& gen, OpcodeList& opcodes)
        {
            // collect all properties that can be initialized
            InplaceArray<const StubProperty*, 60> props;
            {
                auto currentClass  = m_class;
                while (currentClass && !currentClass->flags.test(StubFlag::Import))
                {
                    for (auto it  : m_class->stubs)
                        if (auto prop  = it->asProperty())
                            props.pushBack(prop);

                    currentClass = currentClass->baseClass;
                }
            }

            // generate member initialization ops
            for (auto prop  : props)
            {
                auto init  = gen.makeOpcode(prop->location, Opcode::ContextCtor);
                init->stub = prop;
                OpcodeList::Glue(opcodes, init);
            }

            return true;
        }

        bool FunctionCode::generateClassDestructor(IErrorHandler& err, OpcodeGenerator& gen, OpcodeList& opcodes)
        {
            // collect all properties that can be initialized
            InplaceArray<const StubProperty*, 60> props;
            {
                auto currentClass  = m_class;
                while (currentClass && !currentClass->flags.test(StubFlag::Import))
                {
                    for (auto it  : m_class->stubs)
                        if (auto prop = it->asProperty())
                            props.pushBack(prop);

                    currentClass = currentClass->baseClass;
                }
            }

            // generate member initialization ops
            for (auto prop  : props)
            {
                auto init = gen.makeOpcode(prop->location, Opcode::ContextDtor);
                init->stub = prop;
                OpcodeList::Glue(opcodes, init);
            }

            return true;
        }

        bool FunctionCode::reportError(IErrorHandler& err, const StubLocation& location, StringView<char> txt)
        {
            err.reportError(location.file->absolutePath, location.line, txt);
            return false;
        }

        void FunctionCode::reportWarning(IErrorHandler& err, const StubLocation& location, StringView<char> txt)
        {
            err.reportWarning(location.file->absolutePath, location.line, txt);
        }

        bool FunctionCode::reportError(IErrorHandler& err, const FunctionNode* node, StringView<char> txt)
        {
            return reportError(err, node->location, txt);
        }

        void FunctionCode::reportWarning(IErrorHandler& err, const FunctionNode* node, StringView<char> txt)
        {
            reportWarning(err, node->location, txt);
        }

        bool FunctionCode::resolveVars(IErrorHandler& err, FunctionNode* node)
        {
            // empty node
            if (!node)
                return true;

            // process variable declaration node
            if (node->op == FunctionNodeOp::Var)
            {
                // nameless variable
                auto varName = node->data.name;
                if (varName.empty())
                    return reportError(err, node, "Internal Compler Error: Nameless variable");

                // typeless variable
                if (varName.empty())
                    return reportError(err, node, TempString("Variable '{}' uses invalid type", varName));

                // we can't redefine the function arguments
                if (auto arg  = m_function->findFunctionArg(varName))
                    return reportError(err, node, TempString("Variable '{}' hides function argument, this is not allowed", varName));

                // we can't redefine variable in the same scope
                if (auto existingVar  = node->scope->findLocalVar(node->data.name))
                    return reportError(err, node, TempString("Variable '{}' was already defined in current scope", varName));

                // we can redefine variable in a child scope but it's not recommended
                if (auto existingVar  = node->scope->findVar(node->data.name))
                    reportWarning(err, node, TempString("Variable '{}' hides previous declaration in parent scope", varName));

                // create variable entry
                auto var  = m_mem.create<FunctionVar>();
                var->location = node->location;
                var->name = varName;
                var->type = node->data.type;
                var->scope = node->scope;
                var->index = m_localVarIndex++;

                // add to scope
                node->scope->m_localVars[varName] = var;
                m_allVars.pushBack(var);

                // kill the var node
                if (node->child(0) == nullptr)
                {
                    node->op = FunctionNodeOp::Nop;
                }
                else
                {
                    node->op = FunctionNodeOp::Assign;

                    auto varRefNode  = m_mem.create<FunctionNode>();
                    varRefNode->op = FunctionNodeOp::Ident;
                    varRefNode->scope = node->scope;
                    varRefNode->location = node->location;
                    varRefNode->data.name = varName;

                    node->children.insert(0, varRefNode);
                }

                return true;
            }

            // recurse
            bool valid = true;
            for (auto child  : node->children)
                valid &= resolveVars(err, child);
            return valid;
        }

        bool FunctionCode::connectScopes(IErrorHandler& err, FunctionNode* node, FunctionScope* parentScope)
        {
            // empty node
            if (!node)
                return true;

            // scope already connected
            if (node->scope)
                return true;

            // if this is a scope node create a new scope
            if (node->op == FunctionNodeOp::Scope)
            {
                node->scope = m_mem.create<FunctionScope>();
                node->scope->parent = parentScope;
                node->scope->ownerNode = node;
            }
            else
            {
                node->scope = parentScope;
            }
            // recurse
            bool valid = true;
            for (auto child  : node->children)
                valid &= connectScopes(err, child, node->scope);
            return valid;
        }

        bool FunctionCode::resolveTypes(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            // null node is valid for now
            if (!node)
                return true;

            // we already have type determined
            if (node->type)
                return true;

            // check children first, if that fails usually there's no point checking more
            {
                bool valid = true;
                parentStack.pushBack(node);
                for (uint32_t i=0; i<node->children.size(); ++i)
                    valid &= resolveTypes(err, node->children[i], parentStack);
                parentStack.popBack();
                if (!valid)
                    return false;
            }

            // each node requires different checking
            switch (node->op)
            {
                case FunctionNodeOp::Scope:
                case FunctionNodeOp::StatementList:
                case FunctionNodeOp::Nop:
                    return true;

                case FunctionNodeOp::ExpressionList:
                    if (node->children.empty())
                        return reportError(err, node, "Internal Compiler Error: empty expression list");
                    node->type = node->children.back()->type; // we pass through the value of the last expression
                    return true;

                case FunctionNodeOp::Type:
                    return resolveType(err, node, parentStack);

                case FunctionNodeOp::Ident:
                    return resolveIdent(err, node, parentStack);

                case FunctionNodeOp::This:
                    return resolveThis(err, node, parentStack);

                case FunctionNodeOp::Null:
                    return resolveNull(err, node, parentStack);

                case FunctionNodeOp::AccessMember:
                    return resolveMember(err, node, parentStack);

                case FunctionNodeOp::Operator:
                    return resolveOperator(err, node, parentStack);

                case FunctionNodeOp::Call:
                    return resolveCall(err, node, parentStack);

                case FunctionNodeOp::Assign:
                    return resolveAssign(err, node, parentStack);

                case FunctionNodeOp::IfThenElse:
                    return resolveIfElse(err, node, parentStack);

                case FunctionNodeOp::For:
                case FunctionNodeOp::DoWhile:
                case FunctionNodeOp::While:
                    return resolveLoop(err, node, parentStack);

                case FunctionNodeOp::Return:
                    return resolveReturn(err, node, parentStack);

                case FunctionNodeOp::New:
                    return resolveNew(err, node, parentStack);

                case FunctionNodeOp::Const:
                    if (!node->type)
                        return reportError(err, node, "Internal Compiler Error: constant without determined type");
                    return true;


            }

            // unsupported node
            //return reportError(err, node, "Internal Compiler Error: Unsupported node");
            return true;
        }

        bool FunctionCode::makeIntoConstantNode(IErrorHandler& err, FunctionNode*& node, const StubConstant* value)
        {
            auto valueType = FunctionTypeInfo(value->typeDecl).makeConst(); // NOTE: simpel constants are passed by value
            return makeIntoConstantNode(err, node, value->value, valueType);
        }

        bool FunctionCode::makeIntoConstantNode(IErrorHandler& err, FunctionNode*& node, const StubConstantValue* value, const FunctionTypeInfo& valueType)
        {
            node->op = FunctionNodeOp::Const;
            node->type = valueType.makeConst().removeRef();
            node->children.reset();

            if (value->m_valueType == StubConstValueType::Integer)
                node->data.number = FunctionNumber(value->value.i);
            else if (value->m_valueType == StubConstValueType::Unsigned)
                node->data.number = FunctionNumber(value->value.u);
            else if (value->m_valueType == StubConstValueType::Bool)
                node->data.number = FunctionNumber(value->value.i);
            else if (value->m_valueType == StubConstValueType::Float)
                node->data.number = FunctionNumber(value->value.f);
            else if (value->m_valueType == StubConstValueType::Name)
                node->data.name = value->name;
            else if (value->m_valueType == StubConstValueType::String)
            {
                node->data.text = value->text;
                node->type = node->type.makeRef();
            }

            return true;
        }

        static bool IsFloatingPointType(const FunctionTypeInfo& t)
        {
            return t.isType<float>() || t.isType<double>();
        }

        static bool IsUnsignedType(const FunctionTypeInfo& t)
        {
            return t.isType<uint8_t>() || t.isType<uint16_t>() || t.isType<uint32_t>() || t.isType<uint64_t>();
        }

        static bool IsIntegerType(const FunctionTypeInfo& t)
        {
            return t.isType<char>() || t.isType<short>() || t.isType<int>() || t.isType<int64_t>();
        }

        static bool IsNumberType(const FunctionTypeInfo& t)
        {
            return IsFloatingPointType(t) || IsUnsignedType(t) || IsIntegerType(t);
        }

        FunctionCode::ConstantMatchResult FunctionCode::tryMakeIntoMatchingConstantValue(IErrorHandler& err, FunctionNode*& node, const FunctionTypeInfo& requiredType)
        {
            // we must be of a proper node type
            if (node->op != FunctionNodeOp::Const)
                return ConstantMatchResult::NotANumber;

            // we can't have ref
            auto currentType = node->type;
            if (!currentType.constant || currentType.reference)
                return ConstantMatchResult::NotANumber;

            // type matches, do nothing
            if (StubTypeDecl::Match(currentType.type, requiredType.type))
                return ConstantMatchResult::OK;

            // we can only match numbers
            if (IsNumberType(currentType) && IsNumberType(requiredType))
            {
                // validate current type
                if (node->data.number.isFloat() && !IsFloatingPointType(currentType))
                {
                    reportError(err, node, "Internal Compiler Error: floating point number without compatible type");
                    return ConstantMatchResult::TypeToSmall;
                }
                else if (node->data.number.isUnsigned() && !IsUnsignedType(currentType))
                {
                    reportError(err, node, "Internal Compiler Error: unsigned number without compatible type");
                    return ConstantMatchResult::TypeToSmall;
                }
                else if (node->data.number.isInteger() && !IsIntegerType(currentType))
                {
                    reportError(err, node, "Internal Compiler Error: integer number without compatible type");
                    return ConstantMatchResult::TypeToSmall;
                }

                // check if we can convert
                if (requiredType.isType<char>())
                {
                    if (!node->data.number.fitsInt8())
                        return ConstantMatchResult::TypeToSmall;
                }
                else if (requiredType.isType<short>())
                {
                    if (!node->data.number.fitsInt16())
                        return ConstantMatchResult::TypeToSmall;
                }
                else if (requiredType.isType<int>())
                {
                    if (!node->data.number.fitsInt32())
                        return ConstantMatchResult::TypeToSmall;
                }
                else if (requiredType.isType<int64_t>())
                {
                    if (!node->data.number.fitsInt64())
                        return ConstantMatchResult::TypeToSmall;
                }
                else if (requiredType.isType<uint8_t>())
                {
                    if (!node->data.number.fitsUint8())
                        return ConstantMatchResult::TypeToSmall;
                }
                else if (requiredType.isType<uint16_t>())
                {
                    if (!node->data.number.fitsUint16())
                        return ConstantMatchResult::TypeToSmall;
                }
                else if (requiredType.isType<uint32_t>())
                {
                    if (!node->data.number.fitsUint32())
                        return ConstantMatchResult::TypeToSmall;
                }
                else if (requiredType.isType<uint64_t>())
                {
                    if (!node->data.number.fitsUint64())
                        return ConstantMatchResult::TypeToSmall;
                }

                // set new value
                if (IsFloatingPointType(requiredType))
                    node->data.number = FunctionNumber(node->data.number.asFloat());
                else if (IsUnsignedType(requiredType))
                    node->data.number = FunctionNumber(node->data.number.asUnsigned());
                else if (IsIntegerType(requiredType))
                    node->data.number = FunctionNumber(node->data.number.asInteger());

                // change type
                node->type = requiredType;
                return ConstantMatchResult::OK;
            }

            // not convertible
            return ConstantMatchResult::NotANumber;
        }

        bool FunctionCode::resolveNull(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            auto objectClass = m_lib.findStubInContext("Core.Object"_id, nullptr);
            if (!objectClass || objectClass->asClass())
                return reportError(err, node, "Internal Compiler Error: object class not resolved, import Core plz");

            auto classSelfPtrType = m_lib.createSharedPointerType(node->location, objectClass->asClass());
            if (!classSelfPtrType)
                return reportError(err, node, "Internal Compiler Error: can't create null reference type");

            node->type = classSelfPtrType;
            return true;
        }

        bool FunctionCode::resolveThis(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            if (!m_class)
                return reportError(err, node, "'this' only makes sense inside class or struct methods");

            if (m_static)
                return reportError(err, node, "Can't use 'this' inside static function");

            if (m_class->flags.test(StubFlag::Struct))
            {
                auto classSelfPtrType = m_lib.createSimpleType(node->location, m_class);
                if (!classSelfPtrType)
                    return reportError(err, node, "Internal Compiler Error: can't create self reference type");

                node->type = classSelfPtrType;
                node->type = node->type.makeRef(); // this is a ref
            }
            else
            {
                auto classSelfPtrType = m_lib.createSharedPointerType(node->location, m_class);
                if (!classSelfPtrType)
                    return reportError(err, node, "Internal Compiler Error: can't create self reference type");

                node->type = classSelfPtrType;
            }

            return true;
        }

        bool FunctionCode::resolveType(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            // nameless ident, WTF
            if (!node->data.type)
                return reportError(err, node, "Internal Compiler Error: Type node with invalid type");

            // just set the type
            node->type = node->data.type.makeConst().removeRef();
            return true;
        }

        bool FunctionCode::resolveIdent(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            auto name = node->data.name;

            // nameless ident, WTF
            if (name.empty())
                return reportError(err, node, "Internal Compiler Error: Nameless identifier");

            // lookup as local scope var
            if (auto localVar  = node->scope->findVar(name))
            {
                node->op = FunctionNodeOp::VarLocal;
                node->data.localVar = localVar;
                node->type = localVar->type.makeRef(); // we have an address of the data in local vars so we can read/write to it
                return true;
            }

            // lookup as function argument
            if (auto argVar  = m_function->findFunctionArg(name))
            {
                node->op = FunctionNodeOp::VarArg;
                node->data.argVar = argVar;
                node->type = FunctionTypeInfo(argVar->typeDecl).makeRef();

                if (!argVar->flags.test(StubFlag::Out))
                    node->type = node->type.makeConst();

                return true;
            }

            // lookup inside the class
            const Stub* stub = nullptr;
            if (m_class)
            {
				stub = m_class->findStub(name);
                if (stub)
                {
                    // can we access it ?
                    if (!m_lib.canAccess(stub, m_function))
                        return reportError(err, node, TempString("'{}' is not accessible from current function", name));

                    // a property, yay
                    if (stub->stubType == StubType::Property)
                    {
                        // if we are static method we can't access properties ;(
                        if (m_static)
                            return reportError(err, node, TempString("Class property '{}' cannot be accessed in static method", name));

                        node->op = FunctionNodeOp::VarClass;
                        node->data.classVar = static_cast<const StubProperty*>(stub);
                        node->type = FunctionTypeInfo(node->data.classVar->typeDecl).makeRef();

                        // TODO: read-only class properties

                        return true;
                    }
                    else if (stub->stubType == StubType::Function)
                    {
                        auto funcStub  = static_cast<const StubFunction*>(stub);
                        if (funcStub->flags.test(StubFlag::Static))
                        {
                            node->op = FunctionNodeOp::FunctionStatic;
                            node->data.function = funcStub;
                        }
                        else
                        {
                            // if we are static method we can't call non-static function
                            if (m_static)
                                return reportError(err, node, TempString("Can't call non static function '{}' from static context", name));

                            if (funcStub->flags.test(StubFlag::Final) || m_class->flags.test(StubFlag::Struct))
                            {
                                node->op = FunctionNodeOp::FunctionFinal;
                                node->data.function = funcStub;
                            }
                            else
                            {
                                node->op = FunctionNodeOp::FunctionVirtual;
                                node->data.function = funcStub;
                            }
                        }

                        return true;
                    }
                }
            }

            // try global
            if (!stub)
            {
				stub = m_lib.findStubInContext(name, m_function);
                if (stub)
                {
                    // can we access it ?
                    if (!m_lib.canAccess(stub, m_function))
                        return reportError(err, node, TempString("'{}' is not accessible from current function", name));

                    if (stub->stubType == StubType::Function)
                    {
                        node->op = FunctionNodeOp::FunctionStatic;
                        node->data.function = static_cast<const StubFunction *>(stub);
                        return true;
                    }
                }
            }

            // common cases
            if (stub && stub->stubType == StubType::Constant)
                return makeIntoConstantNode(err, node, static_cast<const StubConstant*>(stub));

            // strange stub
            if (stub)
                return reportError(err, node, TempString("Unrecognized meaning of '{}'", name));

            // try function alias but only if we are part of a call
            // TODO: this is expensive, do it last
            if (auto parentOp  = parentStack.back())
            {
                if (parentOp->op == FunctionNodeOp::Call)
                {
                    // only filter functions that match the argument count
                    auto argCount = parentOp->children.size() - 1;

                    InplaceArray<const StubFunction *, 16> aliasedFunctions;

                    // look for function aliases in the class
                    if (m_class)
                    {
                        HashSet<const StubFunction*> mutedFunctions;
                        m_class->findAliasedFunctions(name, argCount, mutedFunctions, aliasedFunctions);
                    }

                    // try the global aliases after local aliases
                    m_lib.findAliasedFunctions(name, argCount, aliasedFunctions);

                    // we have some functions, create a speculative node
                    if (!aliasedFunctions.empty())
                    {
                        node->op = FunctionNodeOp::FunctionAlias;
                        node->data.aliasFunctions = aliasedFunctions;
                        return true;
                    }
                }
            }

            // unrecognized member
            return reportError(err, node, TempString("Unrecognized '{}'", name));
        }

        static FunctionTypeInfo TypeFromFuncArg(const StubFunctionArg* funcArgStub)
        {
            // invalid arg
            if (!funcArgStub || !funcArgStub->typeDecl)
                return FunctionTypeInfo();

            // get the type required for the argument
            auto argType = FunctionTypeInfo(funcArgStub->typeDecl);
            if (funcArgStub->flags.test(StubFlag::Ref))
                argType = argType.makeConst().makeRef();
            else if (funcArgStub->flags.test(StubFlag::Out))
                argType = argType.makeRef();
            else
                argType = argType.makeConst();


            return argType;
        }

        bool FunctionCode::resolveReturn(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            if (m_function->returnTypeDecl == nullptr)
            {
                if (node->child(0) != nullptr)
                    return reportError(err, node, TempString("Function '{}' does not return any value", m_function->name));
                return true;
            }
            else
            {
                if (node->child(0) == nullptr)
                    return reportError(err, node, TempString("Function '{}' should return value", m_function->name));
            }

            auto isChildTypeRef = node->children[0]->type.reference;
            auto returnType = FunctionTypeInfo(m_function->returnTypeDecl).makeConst();
            if (isChildTypeRef)
                returnType = returnType.makeRef(); // we allow return to process the references

            if (!makeIntoMatchingType(err, node->children[0], returnType, true))
                return false;

            return true;
        }

        bool FunctionCode::resolveNew(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            makeIntoValue(err, node->children[0]);

            auto typeChild  = node->child(0);
            if (!typeChild || !typeChild->type.isClassType())
                return reportError(err, node, TempString("New requires class type"));

            auto newClass  = typeChild->type.typeClass();
            if (newClass->flags.test(StubFlag::Struct))
                return reportError(err, node, TempString("Can't new a struct '{}'", newClass->name));

            if (typeChild->op == FunctionNodeOp::Const && newClass->flags.test(StubFlag::Abstract))
                return reportError(err, node, TempString("Can't new an abstract class '{}'", newClass->name));

            node->type = m_lib.createSharedPointerType(node->location, newClass);
            return true;
        }

        bool FunctionCode::resolveOperator(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            // we need one or two arguments
            if (node->children.size() != 1 && node->children.size() != 2)
                return reportError(err, node, TempString("Internal Compiler Error: invalid operator argument count"));

            // get left and right types
            auto leftType = node->children[0]->type;
            auto leftTypeDecl  = leftType.type;
            auto leftAssignable = leftType.reference && !leftType.constant;

            auto rightType = (node->children.size() == 2) ? node->children[1]->type : FunctionTypeInfo();
            auto rightTypeDecl  = rightType.type;

            // negate constants
            auto op = node->data.name;
            if (op  == "opNeg"_id)
            {
                if (node->children[0]->op == FunctionNodeOp::Const)
                {
                    // TODO
                }
            }

            // find best operator, first without casting
            auto operatorFunc  = m_lib.typeCastingMatrix().findOperator(op, leftTypeDecl, leftAssignable, rightTypeDecl, false);
            if (!operatorFunc)
            {
                // try general type comparison
                if (StubTypeDecl::Match(leftTypeDecl, rightTypeDecl))
                {
                    if (op == "opEqual"_id)
                    {
                        node->op = FunctionNodeOp::GeneralEqual;
                        node->type = FunctionTypeInfo(m_lib.createEngineType(node->location, "bool"_id));

                        if (!makeIntoValue(err, node->children[0]))
                            return false;
                        if (!makeIntoValue(err, node->children[1]))
                            return false;

                        return true;
                    }
                    else if (op == "opNotEqual"_id)
                    {
                        node->op = FunctionNodeOp::GeneralNotEqual;
                        node->type = FunctionTypeInfo(m_lib.createEngineType(node->location, "bool"_id));

                        if (!makeIntoValue(err, node->children[0]))
                            return false;
                        if (!makeIntoValue(err, node->children[1]))
                            return false;

                        return true;
                    }
                }

                // try pointer comparison
                if (leftTypeDecl->isPointerType() && rightTypeDecl->isPointerType())
                {
                    if (op == "opEqual"_id || op == "opNotEqual"_id)
                    {
                        auto leftClass = leftTypeDecl->classType();
                        auto rightClass = rightTypeDecl->classType();
                        if (!leftClass->is(rightClass) && !rightClass->is(leftClass))
                            return reportError(err, node, TempString("Pointer types '{}' and '{}' are not related, cannot compare them", leftType, rightType));

                        if (!makeIntoValue(err, node->children[0]))
                            return false;
                        if (!makeIntoValue(err, node->children[1]))
                            return false;

                        if (op == "opEqual"_id)
                        {
                            node->op = FunctionNodeOp::PointerEqual;
                            node->type = FunctionTypeInfo(m_lib.createEngineType(node->location, "bool"_id));
                        }
                        else
                        {
                            node->op = FunctionNodeOp::PointerNotEqual;
                            node->type = FunctionTypeInfo(m_lib.createEngineType(node->location, "bool"_id));
                        }

                        return true;
                    }
                }

                // try again to match operator by now allowing casting
				operatorFunc = m_lib.typeCastingMatrix().findOperator(op, leftTypeDecl, leftAssignable, rightTypeDecl, true);
                if (operatorFunc)
                {
                    bool canConvert = false;
                    if (operatorFunc->args.size() == 2)
                    {
                        auto leftArgType = TypeFromFuncArg(operatorFunc->args[0]);
                        if (tryMakeIntoMatchingConstantValue(err, node->children[0], leftArgType) == ConstantMatchResult::OK)
                        {
                            auto rightArgType = TypeFromFuncArg(operatorFunc->args[1]);
                            if (tryMakeIntoMatchingConstantValue(err, node->children[1], rightArgType) == ConstantMatchResult::OK)
                            {
                                canConvert = true;
                            }
                        }
                    }
                    else if (operatorFunc != nullptr && operatorFunc->args.size() == 1)
                    {
                        auto leftArgType = TypeFromFuncArg(operatorFunc->args[0]);
                        if (tryMakeIntoMatchingConstantValue(err, node->children[0], leftArgType) == ConstantMatchResult::OK)
                        {
                            canConvert = true;
                        }
                    }

                    /*// if we can't do an easy convert
                    if (!canConvert)
                        operatorFunc = nullptr;*/
                }
            }

            // Nope, no operator
            if (!operatorFunc)
            {
                if (node->children.size() == 2)
                    return reportError(err, node, TempString("Unable to find binary operator '{}' for given pair of types ({}, {})", op, leftType, rightType));
                else
                    return reportError(err, node, TempString("Unable to find unary operator '{}' for given type ({})", op, leftType));
            }

            auto orgNode  = node;

            // make a function call node
            node = m_mem.create<FunctionNode>();
            node->op = FunctionNodeOp::CallStatic;
            node->location = orgNode->location;
            node->scope = orgNode->scope;
            node->type = FunctionTypeInfo(operatorFunc->returnTypeDecl).makeConst();
            node->data.function = operatorFunc;
            node->data.name = op;
            node->children = std::move(orgNode->children);

            // if an operator is a opcode we must call it directly
            if (operatorFunc->opcodeName.empty())
            {
                // cast first argument (should not be needed)
                auto leftArgType = TypeFromFuncArg(operatorFunc->args[0]);
                if (!makeIntoMatchingTypeOperatorCall(err, node->children[0], leftArgType, false))
                    return false;

                // cast second argument (should not be needed)
                if (node->children.size() == 2)
                {
                    auto rightArgType = TypeFromFuncArg(operatorFunc->args[1]);
                    if (!makeIntoMatchingTypeOperatorCall(err, node->children[1], rightArgType, false))
                        return false;
                }
            }
            else
            {
                // cast first argument (should not be needed)
                if (!makeIntoMatchingTypeOpcodeCall(err, node->children[0], TypeFromFuncArg(operatorFunc->args[0]), false))
                    return false;

                // cast second argument (should not be needed)
                if (node->children.size() == 2)
                {
                    if (!makeIntoMatchingTypeOpcodeCall(err, node->children[1], TypeFromFuncArg(operatorFunc->args[1]), false))
                        return false;
                }
            }

            // add fake context
            node->children.insert(0, nullptr);
            return true;
        }

        bool FunctionCode::makeIntoMatchingTypeNoRefChange(IErrorHandler& err, FunctionNode*& node, const FunctionTypeInfo& requiredType, bool explicitCast)
        {
            // if the node is a constant than try to convert right away
            auto constMatchResult = tryMakeIntoMatchingConstantValue(err, node, requiredType);
            if (constMatchResult == ConstantMatchResult::OK)
                return true;
            else if (constMatchResult == ConstantMatchResult::TypeToSmall)
                return reportError(err, node, TempString("Numerical value '{}' does not fit into the target type '{}'", node->data.number, requiredType));

            // get types
            auto& sourceType = node->type;

            // get info about potential cast
            TypeCast typeCast;
            typeCast.m_sourceType = node->type.type;
            typeCast.m_destType = requiredType.type;
            if (!m_lib.typeCastingMatrix().findBestCast(typeCast))
                return reportError(err, node, TempString("Unable to convert from '{}' to '{}'", sourceType, requiredType));

            // if the target type requires non const shit and we are const than fail
            if (!requiredType.constant && node->type.constant)
                return reportError(err, node, TempString("Can't convert '{}' into a non-const type '{}'", sourceType, requiredType));

            // no cast (data type matches)
            if (typeCast.m_castType == TypeCastMethod::Passthrough)
                return true;

            // null can be casted to any pointer, always
            if (requiredType.isAnyPtr() && node->op == FunctionNodeOp::Null)
                return true;

            // some cast are explicit
            if (typeCast.m_explicit && !explicitCast)
                return reportError(err, node, TempString("Casting from '{}' to '{}' requires explicit cast", sourceType, requiredType));

            // we cannot cast into a non-const "ref" type
            // NOTE: we can cast value into a const reference by creating a copy
            if (requiredType.reference && !requiredType.constant)
                return reportError(err, node, TempString("Can't cast '{}' into a reference '{}'", sourceType, requiredType));

            // sometimes data is comaptible but cannot be passed as reference directly (typical case is a ptr<Derived> should not be passes as a refence to ptr<Base> because we can't hold the guarantee in case it's changed)
            if (typeCast.m_castType == TypeCastMethod::PassthroughNoRef)
                return true;

            // function Test(ref v : Vector3);
            // var a, b : Vector3;
            // Test(a + b);
            //
            // Call Test ?
            //  Operator+ : Vector3
            //    Ident a : ref Vector3
            //    Ident b : ref Vector3

            // void* args[];
            // if (ref(v))
            //   args[0] = v;
            //   step(.., &args[0]);
            // else
            //   args[0] = allocTemp();
            //   step(.., args[0]);

            // OpRefFromValue(, .. void* result)
            //   void* add = allocTemp<T>();
            //   step(, add);
            //   *(void**)result = add;

            // OpValueFromRef(, .. void* result)
            //   void* ptr = nullptr;
            //   step(, &ptr);
            //   *(T*)result = *(const T*)ptr;
            //   T->copy(result, ptr);

            // void* args[];
            // if (ref(0))
            //   step(.., &args[0]);
            // else
            //   step(.., &args[0]);
            //   step(.., args[0]);

            // int a,b;
            // step(, &a);
            // step(, &b);
            // OpAdd:
            //   OpValueFromRef int->copy(result, addr);
            //     OpLocalVar -> *(void**)result = &local;

            // create a casting function call
            if (typeCast.m_castFunction)
            {
                auto sourceNode  = node;

                node = m_mem.create<FunctionNode>();
                node->op = FunctionNodeOp::CallStatic;
                node->scope = sourceNode->scope;
                node->location = sourceNode->location;
                node->type = requiredType.makeConst().removeRef();
                node->data.function = typeCast.m_castFunction;

                if (!makeIntoValue(err, sourceNode))
                    return false;

                node->children.pushBack(nullptr);
                node->children.pushBack(sourceNode);
                return true;
            }

            // create wrapper nodes for specialized casting
            if (typeCast.m_castType == TypeCastMethod::OpCode)
            {
                auto sourceNode = node;

                node = m_mem.create<FunctionNode>();
                node->op = typeCast.m_castingOp;
                node->scope = sourceNode->scope;
                node->location = sourceNode->location;
                node->type = requiredType.makeConst().removeRef();

                if (!makeIntoValue(err, sourceNode))
                    return false;

                node->children.pushBack(sourceNode);

                return true;
            }

            return reportError(err, node, TempString("No way to cast from '{}' to '{}", sourceType, requiredType));
        }

        bool FunctionCode::makeIntoMatchingType(IErrorHandler& err, FunctionNode*& node, const FunctionTypeInfo& requiredType, bool explicitCast)
        {
            // convert into basic shit
            if (!makeIntoMatchingTypeNoRefChange(err, node, requiredType, explicitCast))
                return false;

            // convert back/from reference
            auto& currentType = node->type;
            if (requiredType.reference && !currentType.reference)
            {
                return reportError(err, node, TempString("Cannot take reference of a temporary value of type '{}'", currentType));
            }
            else if (!requiredType.reference && currentType.reference)
            {
                if (!makeIntoValue(err, node))
                    return false;
            }

            return true;
        }

        bool FunctionCode::makeIntoMatchingTypeFuncCall(base::script::IErrorHandler &err, base::script::FunctionNode *&node, const base::script::FunctionTypeInfo &requiredType, bool explicitCast)
        {
            // convert into basic shit
            if (!makeIntoMatchingTypeNoRefChange(err, node, requiredType, explicitCast))
                return false;

            // always convert to value when required
            auto& currentType = node->type;
            if (!requiredType.reference && currentType.reference)
            {
                if (!makeIntoValue(err, node))
                    return false;
            }

            // when dealing with parameters conversions for function call we can relax the typing a little bit because we can handle the conversions AT RUNTIME while calling
            return true;
        }

        bool FunctionCode::makeIntoMatchingTypeOperatorCall(IErrorHandler& err, FunctionNode*& node, const FunctionTypeInfo& requiredType, bool explicitCast)
        {
            return makeIntoMatchingTypeFuncCall(err, node, requiredType, explicitCast);
        }

        bool FunctionCode::makeIntoMatchingTypeOpcodeCall(IErrorHandler& err, FunctionNode*& node, const FunctionTypeInfo& requiredType, bool explicitCast)
        {
            // convert into basic shit
            if (!makeIntoMatchingTypeNoRefChange(err, node, requiredType, explicitCast))
                return false;

            // we must must the ref type
            if (requiredType.reference && !node->type.reference)
            {
                return reportError(err, node, TempString("Cannot convert to reference type '{}' when calling function implemented as opcode", requiredType));
            }
            else if (!requiredType.reference && node->type.reference)
            {
                if (!makeIntoValue(err, node))
                    return false;
            }

            return true;
        }

        bool FunctionCode::resolveStructConstruct(IErrorHandler& err, FunctionNode*& node, const StubClass* structStub, TNodeStack& parentStack)
        {
            // get the structure to construct
            if (!structStub)
                return reportError(err, node, TempString("Internal Compiler Error: struct construct without struct"));

            // count struct properties
            InplaceArray<const StubProperty*, 10> initProperties;
            for (auto stub  : structStub->stubs)
                if (stub->asProperty())
                    initProperties.pushBack(stub->asProperty());

            // to many arguments ?
            auto argCount = node->children.size() - 1;
            if (argCount > initProperties.size())
                return reportError(err, node, TempString("To many arguments for construction of struct '{}' ({} required, {} passed)", structStub->name, initProperties.size(), argCount));

            // set the node type as VALUE structure (all constructed structures are passed on as values)
            node->op = FunctionNodeOp::Construct;
            node->type = node->children[0]->data.type.removeRef();
            node->children.erase(0); // remove the type node

            // match parameters
            for (uint32_t i=0; i<argCount; ++i)
            {
                auto stubProp  = initProperties[i];
                auto requiredType = FunctionTypeInfo(stubProp->typeDecl).removeRef().makeConst();

                if (!makeIntoMatchingType(err, node->children[i], requiredType, true))
                    return reportError(err, node, TempString("Unable to initalize property '{}' of type '{}' from type '{}'", stubProp->name, requiredType, node->children[i]->type));
            }

            // done
            return true;
        }

        bool FunctionCode::resolveExplicitCast(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            // we should have one more node
            if (node->children.size() != 2)
                return reportError(err, node, TempString("Internal Compiler Error: invalid node count for a cast"));

            // get the type we want to cast to
            auto targetType = node->children[0]->type.makeConst().removeRef();

            // remove the cast node
            auto valueNode  = node->children[1];
            node->children.clear();
            node = valueNode;

            // put the cast
            return makeIntoMatchingType(err, node, targetType, true);
        }

        int FunctionCode::calcAliasCastingConst(const FunctionNode* callNode, const StubFunction* func, bool allowExplicitCasting)
        {
            // check argument count
            auto numArgs = callNode->children.size() - 1;
            if (func->args.size() != numArgs)
                return -1;

            // make sure all arguments can be converted
            int totalCost = 0;
            bool usedExplicitCasting = false;
            for (uint32_t i=0; i<numArgs; ++i)
            {
                const auto& existingType = callNode->children[i + 1]->type;
                const auto& requiredType = FunctionTypeInfo(func->args[i]->typeDecl);

                auto castInfo = m_lib.typeCastingMatrix().findBestCast(existingType.type, requiredType.type);
                if (castInfo.m_cost == -1)
                    return -1;

                totalCost += castInfo.m_cost;

                if (castInfo.m_explicit)
                {
                    if (!allowExplicitCasting)
                        return -1;

                    usedExplicitCasting = true;
                }
            }

            if (usedExplicitCasting)
                totalCost += 100;

            return totalCost;
        }

        static void BuildPrintableSignature(const FunctionNode* callNode, IFormatStream& f)
        {
            f << "(";

            auto numArgs = callNode->children.size() - 1;
            for (uint32_t i=0; i<numArgs; ++i)
            {
                if (i > 0)
                    f << ", ";

                auto& existingType = callNode->children[i + 1]->type;
                f << existingType;
            }

            f << ")";
        }

        bool FunctionCode::resolveFunctionAlias(IErrorHandler& err, const FunctionNode* node, FunctionNode* funcNode, TNodeStack& parentStack)
        {
            // we need to have at least one member
            if (node->children.size() < 1)
                return reportError(err, node, TempString("Internal Compiler Error: call without function node"));

            // it must be an alias
            if (funcNode->data.aliasFunctions.empty())
                return reportError(err, node, TempString("Internal Compiler Error: alias function with no candidates"));

            // name of the alias we are looking up
            auto aliasName = funcNode->data.aliasFunctions[0]->asFunction()->aliasName;

            // score all possible functions
            struct FunctionScore
            {
                const StubFunction *m_func;
                int m_castScore;
            };

            InplaceArray<FunctionScore, 16> scoredFunctions;
            for (auto func  : funcNode->data.aliasFunctions)
            {
                auto score = calcAliasCastingConst(node, func, false);
                if (score != -1)
                {
                    auto& entry = scoredFunctions.emplaceBack();
                    entry.m_func = func;
                    entry.m_castScore = score;
                    continue;
                }

                auto scoreWithHardCasting = calcAliasCastingConst(node, func, true);
                if (scoreWithHardCasting != -1)
                {
                    auto& entry = scoredFunctions.emplaceBack();
                    entry.m_func = func;
                    entry.m_castScore = scoreWithHardCasting;
                    continue;
                }
            }

            StringBuilder signature;

            if (scoredFunctions.empty())
            {
                BuildPrintableSignature(node, signature);
                return reportError(err, node, TempString("No version of function '{}' found that matches call arguments {}", aliasName, signature.c_str()));
            }

            int bestScore = scoredFunctions[0].m_castScore;
            for (auto& entry : scoredFunctions)
                if (entry.m_castScore < bestScore)
                    bestScore = entry.m_castScore;

            InplaceArray<FunctionScore, 16> bestScoredFunctions;
            for (auto& entry : scoredFunctions)
                if (entry.m_castScore == bestScore)
                    bestScoredFunctions.emplaceBack(entry);

            if (bestScoredFunctions.size() > 1)
            {
                BuildPrintableSignature(node, signature);
                reportError(err, node, TempString("Multiple versions of function '{}' found that match the call arguments {}", aliasName, signature.c_str()));

                for (auto& entry : bestScoredFunctions)
                    reportError(err, entry.m_func->location, TempString("Function '{}' matches alias '{}' for {}", entry.m_func->name, aliasName, signature.c_str()));

                return false;
            }

            auto bestFunction  = bestScoredFunctions[0].m_func;
            funcNode->data.aliasFunctions.clear();
            funcNode->data.function = bestFunction;

            if (bestFunction->flags.test(StubFlag::Static) || (bestFunction->owner->asClass() == nullptr))
                funcNode->op = FunctionNodeOp::FunctionStatic;
            else if (bestFunction->flags.test(StubFlag::Final) || bestFunction->owner->asClass()->flags.test(StubFlag::Struct))
                funcNode->op = FunctionNodeOp::FunctionFinal;
            else
                funcNode->op = FunctionNodeOp::FunctionVirtual;

            return true;
        }

        bool FunctionCode::resolveCall(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            // we need to have at least one member
            if (node->children.size() < 1)
                return reportError(err, node, TempString("Internal Compiler Error: call without function node"));

            // cast node ?
            auto funcNode  = node->children[0];
            if (funcNode->op == FunctionNodeOp::Type)
            {
                auto typeRef = funcNode->data.type.type;
                if (!typeRef)
                    return reportError(err, node, TempString("Internal Compiler Error: type call without type"));

                if (typeRef->isSimpleType() && typeRef->classType() && typeRef->classType()->flags.test(StubFlag::Struct))
                    return resolveStructConstruct(err, node, typeRef->classType(),parentStack);

                if (typeRef->isSimpleType() || typeRef->isPointerType() || typeRef->isClassType())
                    return resolveExplicitCast(err, node, parentStack);

                return reportError(err, node, TempString("Unsupported cast to '{}'", funcNode->data.type));
            }

            // it's a function alias, look our our now converted params to find the best alias
            if (funcNode->op == FunctionNodeOp::FunctionAlias)
                if (!resolveFunctionAlias(err, node, funcNode, parentStack))
                    return false;

            // get the function node and remove it - whatever happens we don't need it, instead substitute it for the context node
            if (!funcNode || (funcNode->op != FunctionNodeOp::FunctionVirtual && funcNode->op != FunctionNodeOp::FunctionStatic && funcNode->op != FunctionNodeOp::FunctionFinal))
                return reportError(err, node, TempString("Internal Compiler Error: function call without function node"));

            // remove the context nodes, if present
            auto funcStub  = funcNode->data.function;
            if (!funcStub)
                return reportError(err, node, TempString("Internal Compiler Error: function call without function object"));

            // select the calling "convention"
            // NOTE: the calling "convention" may be refined at the byte code generation time
            if (funcNode->op == FunctionNodeOp::FunctionStatic)
            {
                if (!funcStub->flags.test(StubFlag::Static))
                    return reportError(err, node, TempString("Internal Compiler Error: calling static function that is not static"));

                node->children[0] = nullptr;
                node->op = FunctionNodeOp::CallStatic;
            }
            else if (funcNode->op == FunctionNodeOp::FunctionFinal)
            {
                node->children[0] = funcNode->children.empty() ? nullptr : funcNode->children[0]; // NOTE: we may not have a context given, use the implicit context if that happens
                node->op = FunctionNodeOp::CallFinal;
            }
            else if (funcNode->op == FunctionNodeOp::FunctionVirtual)
            {
                node->children[0] = funcNode->children.empty() ? nullptr : funcNode->children[0]; // NOTE: we may not have a context given, use the implicit context if that happens
                node->op = FunctionNodeOp::CallVirtual;
            }

            // keep the function reference
            node->data.function = funcStub;

            // check argument count
            auto numArgs = node->children.size() - 1;
            if (numArgs > funcStub->args.size())
            {
                return reportError(err, node, TempString("Function '{}' expects {} argument(s), {} provided", funcStub->name, funcStub->args.size(), numArgs));
            }
            else if (numArgs < funcStub->args.size())
            {
                node->children.resizeWith(1 + funcStub->args.size(), nullptr);
            }

            // check arguments
            bool validArgs = true;
            for (uint32_t i=0; i<funcStub->args.size(); ++i)
            {
                auto funcArgStub  = funcStub->args[i];

                // get the input node
                // NOTE: the pointer gets modified
                FunctionNode*& argNode = node->children[1 + i];

                // if argument is not provided in a call it must have a default value in the function
                if (argNode == nullptr)
                {
                    // we don't have a default value for this shit
                    if (funcArgStub->defaultValue == nullptr)
                    {
                        reportError(err, node, TempString("Missing value for argument '{}'", funcArgStub->name));
                        validArgs = false;
                        continue;
                    }

                    // create dummy node
                    auto dummyNode  = m_mem.create<FunctionNode>();
                    dummyNode->op = FunctionNodeOp::Nop;
                    dummyNode->location = node->location;
                    dummyNode->scope = node->scope;
                    argNode = dummyNode;

                    // insert constant value
                    auto defaultValueArgType = FunctionTypeInfo(funcArgStub->typeDecl);
                    if (!makeIntoConstantNode(err, argNode, funcArgStub->defaultValue, defaultValueArgType))
                    {
                        reportError(err, node, TempString("Unable to match type for argument '{}'", funcArgStub->name));
                        validArgs = false;
                        continue;
                    }
                }

                // get the argument type we NEED
                auto argType = TypeFromFuncArg(funcArgStub);

                // match type
                auto allowCasting = !funcArgStub->flags.test(StubFlag::Explicit);
                if (funcStub->opcodeName.empty())
                {
                    if (!makeIntoMatchingTypeFuncCall(err, argNode, argType, allowCasting))
                    {
                        validArgs = false;
                        continue;
                    }
                }
                else
                {
                    if (!makeIntoMatchingTypeOpcodeCall(err, argNode, argType, allowCasting))
                    {
                        validArgs = false;
                        continue;
                    }
                }
            }

            // argument validation failed
            if (!validArgs)
                return false;

            // set the return type
            node->type = FunctionTypeInfo(funcStub->returnTypeDecl).makeConst();
            return true;
        }

        bool FunctionCode::makeIntoValue(IErrorHandler& err, FunctionNode*& node)
        {
            if (!node->type)
                return reportError(err, node, TempString("Internal Compiler Error: invalid type"));

            // TODO: check if we can make value, usually we can

            if (node->type.reference)
            {
                auto wrapperNode  = m_mem.create<FunctionNode>();
                wrapperNode->type = node->type.removeRef();

                if (node->type.constant)
                    wrapperNode->type = wrapperNode->type.makeConst();

                wrapperNode->op = FunctionNodeOp::MakeValueFromRef;
                wrapperNode->location = node->location;
                wrapperNode->scope = node->scope;
                wrapperNode->children.pushBack(node);
                wrapperNode->data.type = wrapperNode->type.removeRef().removeConst();
                node = wrapperNode;
            }

            return true;
        }

        bool FunctionCode::resolveMember(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            // no node to be member of
            if (node->children.size() < 1)
                return reportError(err, node, TempString("Internal Compiler Error: no context member"));

            // get the member name
            auto memberName = node->data.name;
            if (memberName.empty())
                return reportError(err, node, TempString("Internal Compiler Error: no member name"));

            // if the child is a function call then oh-well
            auto contextNode  = node->children[0];
            if (contextNode->op == FunctionNodeOp::FunctionVirtual || contextNode->op == FunctionNodeOp::FunctionStatic)
                return reportError(err, node, TempString("Function does not provide any member access"));

            // context node has no type (should fail resolving)
            auto contextType = node->children[0]->type;
            if (!contextType)
                return reportError(err, node, TempString("No context to look up member '{}'", memberName));

            // enum member
            if (contextType.isEnumType())
            {
                auto enumType  = contextType.typeEnum();
                if (!enumType)
                    return reportError(err, node, TempString("Internal Compiler Error: enum type with no enum"));

                auto enumOption  = enumType->findOption(memberName);
                if (!enumOption)
                    return reportError(err, node, TempString("Option '{}' not found in enum '{}'", memberName, contextType));

                // change into specialized "enum constant"
                // NOTE: we can't store numerical value here since we don't know the actual enum size
                node->op = FunctionNodeOp::EnumConst;
                node->type = contextType; // enum type
                node->data.name = memberName; // enum option name
                node->data.enumStub = enumType;
                node->children.clear();
                return true;
            }

            // TODO: promote weak pointer to strong pointer

            // pointed object access
            auto isSharedPtr = contextType.isSharedPtr();
            auto isStruct = contextType.isStruct();
            auto isClassType = contextType.isClassType();
            if (isSharedPtr || isStruct || isClassType)
            {
                auto classType  = contextType.typeClass();
                if (!classType)
                    return reportError(err, node, TempString("Internal Compiler Error: type with no class"));

                auto classMember  = classType->findStub(memberName);
                if (!classMember)
                {
                    // hack: allow function aliases
                    auto parentOp  = parentStack.back();
                    if (parentOp->op == FunctionNodeOp::Call)
                    {
                        // only filter functions that match the argument count
                        auto argCount = parentOp->children.size() - 1;

                        // get aliased functions
                        InplaceArray<const StubFunction *, 16> aliasedFunctions;
                        HashSet<const StubFunction*> mutedFunctions;
                        classType->findAliasedFunctions(memberName, argCount, mutedFunctions, aliasedFunctions);

                        // we have some functions, create a speculative node
                        if (!aliasedFunctions.empty())
                        {
                            node->op = FunctionNodeOp::FunctionAlias;
                            node->data.aliasFunctions = aliasedFunctions;
                            return true;
                        }
                    }

                    return reportError(err, node, TempString("Unknown member '{}' in '{}'", memberName, classType->name));
                }

                // can we access it ?
                if (!m_lib.canAccess(classMember, m_function))
                    return reportError(err, node, TempString("'{}' is not accessible from current function", memberName));

                // a property, yay
                if (classMember->stubType == StubType::Property)
                {
                    auto classProperty  = static_cast<const StubProperty*>(classMember);
                    if (isSharedPtr)
                    {
                        // setup as context lookup node
                        node->op = contextType.reference ? FunctionNodeOp::ContextRef : FunctionNodeOp::Context;
                        node->data.classVar = classProperty;
                        node->type = FunctionTypeInfo(classProperty->typeDecl).makeRef();

                        // create a context property access node
                        auto propNode  = m_mem.create<FunctionNode>();
                        propNode->op = FunctionNodeOp::VarClass;
                        propNode->scope = node->scope;
                        propNode->location = node->location;
                        propNode->data.classVar = static_cast<const StubProperty*>(classProperty);
                        propNode->type = FunctionTypeInfo(classProperty->typeDecl).makeRef();
                        node->add(propNode);

                        /*if (contextType.constant || !contextType.reference)
                            node->type = node->type.makeConst();*/

                        return true;
                    }
                    else if (isStruct)
                    {
                        node->op = contextType.reference ? FunctionNodeOp::MemberOffsetRef : FunctionNodeOp::MemberOffset;
                        node->type = FunctionTypeInfo(classProperty->typeDecl);
                        node->data.classVar = classProperty;

                        if (contextType.reference)
                            node->type = node->type.makeRef();

                        if (contextType.constant || !contextType.reference)
                            node->type = node->type.makeConst();

                        return true;
                    }
                }
                else if (classMember->stubType == StubType::Function)
                {
                    auto funcStub = static_cast<const StubFunction *>(classMember);
                    if (isSharedPtr)
                    {
                        if (funcStub->flags.test(StubFlag::Static))
                        {
                            node->op = FunctionNodeOp::FunctionStatic;
                            node->data.function = funcStub;
                            node->children.clear();
                        }
                        else if (funcStub->flags.test(StubFlag::Final))
                        {
                            node->op = FunctionNodeOp::FunctionFinal;
                            node->data.function = funcStub;
                        }
                        else
                        {
                            node->op = FunctionNodeOp::FunctionVirtual;
                            node->data.function = funcStub;
                        }
                        return true;
                    }
                    else if (isClassType)
                    {
                        if (!funcStub->flags.test(StubFlag::Static))
                            return reportError(err, node, TempString("'{}' is a not a static function and cannot be accessed via class scope", memberName));

                        node->op = FunctionNodeOp::FunctionStatic;
                        node->data.function = funcStub;
                        node->children.clear();

                        return true;
                    }
                    else if (isStruct)
                    {
                        if (funcStub->flags.test(StubFlag::Static))
                        {
                            node->op = FunctionNodeOp::FunctionStatic;
                            node->data.function = funcStub;
                            node->children.clear();
                        }
                        else
                        {
                            node->op = FunctionNodeOp::FunctionFinal;
                            node->data.function = funcStub;
                        }

                        //return reportError(err, node, TempString("Cannot directly call function '{}' on a structure, use {}.{} syntax", memberName, classType->name, memberName));
                        return true;
                    }
                }
                else if (classMember->stubType == StubType::Constant)
                {
                    if (isClassType)
                        return makeIntoConstantNode(err, node, static_cast<const StubConstant*>(classMember));

                    return reportError(err, node, TempString("'{}' is a constant and should be accessed via class scope: {}.{}", memberName, classType->name, memberName));
                }

                // member with no sense
                if (classMember != nullptr)
                    return reportError(err, node, TempString("'{}' has no useful meaning here, try to use class scope: {}.{}", memberName, classType->name, memberName));
            }

            // oh well
            return reportError(err, node, TempString("No idea how to lookup member '{}' in '{}'", memberName, contextType));
        }

        bool FunctionCode::resolveIfElse(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            if (node->children.size() != 2 && node->children.size() != 3)
                return reportError(err, node, TempString("Internal Compiler Error: invalid node count for if statement"));

            auto boolType = FunctionTypeInfo(m_lib.createEngineType(node->location, "bool"_id)).makeConst();
            if (!makeIntoMatchingType(err, node->children[0], boolType, false))
                return reportError(err, node, TempString("Conditional expression should be of boolean type"));

            return true;
        }

        bool FunctionCode::resolveLoop(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            if (node->children.size() != 3)
                return reportError(err, node, TempString("Internal Compiler Error: invalid node count for loop statement"));

            auto boolType = FunctionTypeInfo(m_lib.createEngineType(node->location, "bool"_id)).makeConst();
            if (!makeIntoMatchingType(err, node->children[0], boolType, false))
                return reportError(err, node, TempString("Loop condition should be of boolean type"));

            return true;
        }

        bool FunctionCode::resolveAssign(IErrorHandler& err, FunctionNode*& node, TNodeStack& parentStack)
        {
            if (node->children.size() != 2)
                return reportError(err, node, TempString("Internal Compiler Error: not enough nodes for assignment"));

            auto & lValue = node->children[0];
            if (!lValue)
                return reportError(err, node, TempString("Internal Compiler Error: not l-value for assignment"));

            auto & rValue = node->children[1];
            if (!rValue)
                return reportError(err, node, TempString("Internal Compiler Error: not r-value for assignment"));

            if (!lValue->type)
                return reportError(err, node, TempString("Internal Compiler Error: invalid l-value type for assignment"));

            if (!rValue->type)
                return reportError(err, node, TempString("Internal Compiler Error: invalid r-value type for assignment"));

            if (!lValue->type.reference)
                return reportError(err, node, TempString("Can't assign to a non l-value (missing reference on type '{}')", lValue->type));

            if (lValue->type.constant)
                return reportError(err, node, TempString("Can't assign to a constant l-value"));

            auto rValueType = lValue->type.removeRef().makeConst();
            if (!makeIntoMatchingType(err, rValue, rValueType, false))
                return false;

            // assign has no value to return, makes life simple
            node->type = FunctionTypeInfo();
            return true;
        }

    } // script
} // base