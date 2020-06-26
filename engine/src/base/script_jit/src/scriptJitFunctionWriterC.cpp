/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "scriptJitTypeLib.h"
#include "scriptJitConstantCache.h"
#include "scriptJitGeneralC.h"
#include "scriptJitFunctionWriterC.h"

#include "base/containers/include/inplaceArray.h"

namespace base
{
    namespace script
    {
        //--

        JITOpcodeStream::JITOpcodeStream(const StubFunction* func)
            : m_func(func)
            , m_pos(0)
        {
            m_end = m_func->opcodes.size();
        }

        const StubOpcode* JITOpcodeStream::peek() const
        {
            if (m_pos < m_end)
                return m_func->opcodes[m_pos];
            return nullptr;
        }

        const StubOpcode* JITOpcodeStream::read()
        {
            if (m_pos < m_end)
                return m_func->opcodes[m_pos++];
            return nullptr;
        }

        //--

        JITFunctionWriterC::JITFunctionWriterC(mem::LinearAllocator& mem, JITTypeLib& types, JITConstCache& consts, const StubFunction* func, bool hasDirectParamAccess, bool emitExceptions, bool emitLines)
            : m_mem(mem)
            , m_types(types)
            , m_consts(consts)
            , m_func(func)
            , m_hasErrors(false)
            , m_hasDirectParamAccess(hasDirectParamAccess)
            , m_emitExceptions(emitExceptions)
            , m_emitLines(emitLines)
            , m_lastLine(0)
        {}

        JITFunctionWriterC::~JITFunctionWriterC()
        {}

        bool JITFunctionWriterC::emitOpcodes(StringBuf& outCode)
        {
            // emit statements
            JITOpcodeStream ops(m_func);
            while (ops)
            {
                auto op  = ops.read();
                processStatement("context", op, ops);
            }

            // no code in case of reported errors
            if (m_hasErrors)
                return false;

            // glue code parts
            m_prolog << m_code;
            m_prolog << "(void)0;\n";
            outCode = m_prolog.toString();
            return true;
        }

        void JITFunctionWriterC::reportError(const Stub* op, StringView<char> txt)
        {
            if (op && op->location.file)
            {
                TRACE_ERROR("{}({}): error: {}", op->location.file->absolutePath.c_str(), op->location.line, txt);
            }
            else
            {
                TRACE_ERROR("JIT: error: {}", txt);
            }

            m_hasErrors = true;
        }

        //--

        StringView<char> JITFunctionWriterC::copyString(const StringView<char> str)
        {
            auto copiedString  = m_mem.strcpy(str.data(), str.length());
            return StringView<char>(copiedString, str.length());
        }

        JITCodeChunkC JITFunctionWriterC::makeChunk(const StubOpcode* op, const JITType* type, StringView<char> str)
        {
            JITCodeChunkC ret;
            ret.jitType = type;
            ret.pointer = false;
            ret.text = copyString(str);
            ret.op = op;
            return ret;
        }

        StringView<char> JITFunctionWriterC::makeLabel(const StubOpcode* label)
        {
            StringView<char> ret;
            if (m_labelMap.find(label, ret))
                return ret;

            auto index  = m_labelMap.size();
            ret = copyString(TempString("Label{}", index));
            m_labelMap[label] = ret;
            return ret;
        }

        JITCodeChunkC JITFunctionWriterC::makePointer(const JITCodeChunkC& chunk)
        {
            if (!chunk.text || chunk.pointer)
                return chunk;

            JITCodeChunkC ret = chunk;
            ret.pointer = true;

            StringBuilder txt;
            txt << "&(";
            txt << chunk;
            txt << ")";
            ret.text = copyString(txt.view());

            return ret;
        }

        JITCodeChunkC JITFunctionWriterC::makeValue(const JITCodeChunkC& chunk)
        {
            if (!chunk.text || !chunk.pointer)
                return chunk;

            JITCodeChunkC ret = chunk;
            ret.pointer = false;

            StringBuilder txt;
            txt << "(*(";
            txt << chunk;
            txt << "))";
            ret.text = copyString(txt.view());

            return ret;
        }

        JITCodeChunkC JITFunctionWriterC::makeLocalVar(const StubOpcode* context, int localIndex, const Stub* stubType, StringID knownName)
        {
            if (!stubType)
            {
                reportError(context, TempString("Unspecified type for local var '{}' (index {})", knownName, localIndex));
                return JITCodeChunkC();
            }

            auto jitType  = m_types.resolveType(stubType);
            if (!jitType)
            {
                reportError(context, TempString("Unresolved type for local var '{}' (index {})", knownName, localIndex));
                return JITCodeChunkC();
            }

            if (!jitType->jitName)
            {
                reportError(context, TempString("Type '{}' for local var '{}' (index {}) is not JITable", jitType->name, knownName, localIndex));
                return JITCodeChunkC();
            }

            JITCodeChunkC ret;
            if (m_localsMap.find(localIndex, ret))
                return ret;

            StringView<char> name;
            if (knownName)
                name = copyString(TempString("___local_{}_{}", knownName, localIndex));
            else
                name = copyString(TempString("___local_{}", localIndex));

            // add code to prologue
            m_prolog.appendf("{} {} = {0};\n", jitType->jitName, name);

            // return wrapper
            ret.text = name;
            ret.pointer = false;
            ret.jitType = jitType;
            ret.op = context;
            m_localsMap[localIndex] = ret;
            return ret;
        }

        void JITFunctionWriterC::makeCtor(const JITType* type, bool isMemoryZeroed, StringView<char> str, bool fromPointer)
        {
            if (isMemoryZeroed && type->zeroInitializedConstructor)
                return;

            if (!type->requiresConstructor)
                return;

            /*if (type->ctorFuncName)
            {
                if (fromPointer)
                    code.appendf("{}({}, stackFrame, 0);\n", type->ctorFuncName, str);
                else
                    code.appendf("{}(&({}}), stackFrame, 0);\n", type->ctorFuncName, str);
            }
            else*/
            {
                ASSERT(type->assignedID != -1);

                if (fromPointer)
                    m_code.appendf("CTOR({}, {});\n", type->assignedID, str);
                else
                    m_code.appendf("CTOR({}, &({}));\n", type->assignedID, str);
            }
        }

        void JITFunctionWriterC::makeDtor(const JITType* type, StringView<char> str, bool fromPointer)
        {
            if (!type->requiresDestructor)
                return;

            /*if (type->dtorFuncName)
            {
                if (fromPointer)
                    code.appendf("{}({}, stackFrame, 0);\n", type->dtorFuncName, str);
                else
                    code.appendf("{}(&({}), stackFrame, 0);\n", type->dtorFuncName, str);
            }
            else*/
            {
                ASSERT(type->assignedID != -1);

                if (fromPointer)
                    m_code.appendf("DTOR({}, ({}));\n", type->assignedID, str);
                else
                    m_code.appendf("DTOR({}, &({}));\n", type->assignedID, str);
            }
        }

        void JITFunctionWriterC::makeCtor(const JITCodeChunkC& var, bool isMemoryZeroed)
        {
            makeCtor(var.jitType, isMemoryZeroed, var.text, var.pointer);
        }

        void JITFunctionWriterC::makeDtor(const JITCodeChunkC& var)
        {
            makeDtor(var.jitType, var.text, var.pointer);
        }

        void JITFunctionWriterC::makeCopy(const JITCodeChunkC& to, const JITCodeChunkC& from)
        {
            //ASSERT(to.type == from.type);
            makeCopy(to.jitType, to.text, to.pointer, from.text, from.pointer);
        }

        void JITFunctionWriterC::makeCopy(const JITType* type, StringView<char> to, bool targetPointer, StringView<char> from, bool fromPointer)
        {
            if (type->simpleCopyCompare)
            {
                if (targetPointer)
                    m_code.appendf("*(({}*) {}) = ", type->jitName, to);
                else
                    m_code.appendf("{} = ", to);

                if (fromPointer)
                    m_code.appendf("*(({}*) {});\n", type->jitName, from);
                else
                    m_code.appendf("{};\n", from);
            }
            else
            {
                ASSERT(type->assignedID != -1);
                if (targetPointer)
                    m_code.appendf("COPY({}, {}, ", type->assignedID, to);
                else
                    m_code.appendf("COPY({}, &({}), ", type->assignedID, to);

                if (fromPointer)
                    m_code.appendf("{});\n", from);
                else
                    m_code.appendf("&({}));\n", from);
            }
        }

        StringView<char> JITFunctionWriterC::makeStatementExitLabel()
        {
            m_exitLabelNeeded = true;
            return m_mem.strcpy(TempString("ExitStatement{}", m_exitLabelCounter).c_str());
        }

        JITCodeChunkC JITFunctionWriterC::makeTempVar(const StubOpcode* context, const JITType* jitType)
        {
            // invalid type
            if (!jitType)
            {
                reportError(context, TempString("Unknown type for temp variable"));
                return JITCodeChunkC();
            }

            // type must be jitable
            if (!jitType->jitName)
            {
                reportError(context, TempString("Type '{}' for temp variable is not JITable", jitType->name));
                return JITCodeChunkC();
            }

            // generate name
            auto name  = copyString(TempString("___temp{}", m_tempVarCounter++));

            // add code to prologue
            m_prolog.appendf("{} {} = {0};\n", jitType->jitName, name);

            // call constructor
            makeCtor(jitType, true, name, false);

            // add to list of temps
            auto& newTemp = m_newTemps.emplaceBack();
            newTemp.varName = name;
            newTemp.jitType = jitType;

            // return wrapper
            JITCodeChunkC ret;
            ret.text = name;
            ret.pointer = false;
            ret.jitType = jitType;
            ret.op = context;
            return ret;
        }

        bool JITFunctionWriterC::finishStatement(const StubOpcode* op)
        {
            // cleanup temps created just for this statement
            for (auto& tempVar : m_newTemps)
                if (tempVar.jitType->requiresDestructor)
                    makeDtor(tempVar.jitType, tempVar.varName, false);
            m_newTemps.reset();

            // make the statement exit label
            if (m_exitLabelNeeded)
            {
                m_code.appendf("ExitStatement{}:\n", m_exitLabelCounter);
                m_exitLabelCounter += 1;
                m_exitLabelNeeded = false;
            }

            return true;
        }

        bool JITFunctionWriterC::processStatement(StringView<char> contextStr, const StubOpcode* op, JITOpcodeStream& stream)
        {
            // statement header
            /*if (op->location.file && m_lastLine != op->location.line)
            {
                code.appendf("#line {} \"{}\"\n", op->location.line, op->location.file->absolutePath);
                m_lastLine = op->location.line;
            }*/

            // process opcodes
            switch (op->op)
            {
                case Opcode::Breakpoint:
                {
                    processStatement(contextStr, stream.read(), stream);
                    return finishStatement(op);
                }

                case Opcode::Label:
                {
                    auto labelText  = makeLabel(op);
                    m_code.appendf("{}:\n", labelText);
                    return finishStatement(op);
                }

                case Opcode::AssignUint1:
                case Opcode::AssignUint2:
                case Opcode::AssignUint4:
                case Opcode::AssignUint8:
                case Opcode::AssignInt1:
                case Opcode::AssignInt2:
                case Opcode::AssignInt4:
                case Opcode::AssignInt8:
                case Opcode::AssignDouble:
                case Opcode::AssignFloat:
                {
                    auto ptr  = processOpcode(contextStr, stream.read(), stream);
                    auto val  = processOpcode(contextStr, stream.read(), stream);
                    m_code << makeValue(ptr);
                    m_code << " = ";
                    m_code << makeValue(val);
                    m_code << ";\n";
                    return finishStatement(op);
                }

                case Opcode::AssignAny:
                {
                    auto ptr  = processOpcode(contextStr, stream.read(), stream);
                    auto val  = processOpcode(contextStr, stream.read(), stream);
                    makeCopy(ptr, val);
                    return finishStatement(op);
                }

                case Opcode::Exit:
                {
                    finishStatement(op);
                    m_code.appendf("return;\n");
                    return true;
                }

                case Opcode::Jump:
                {
                    auto labelText  = makeLabel(op->target);
                    finishStatement(op);
                    m_code.appendf("goto {};\n", labelText);
                    return true;
                }

                case Opcode::LocalCtor:
                {
                    auto var  = makeLocalVar(op, op->value.i, op->stub, op->value.name);
                    makeCtor(var, true);
                    return true;
                }

                case Opcode::LocalDtor:
                {
                    auto var = makeLocalVar(op, op->value.i, op->stub, op->value.name);
                    makeDtor(var);
                    return true;
                }

                case Opcode::JumpIfFalse:
                {
                    auto cond = processOpcode(contextStr, stream.read(), stream);
                    auto labelText = makeLabel(op->target);

                    bool hasTempsWithDestruction = false;
                    for (auto& temp : m_newTemps)
                        if (temp.jitType->requiresDestructor)
                            hasTempsWithDestruction = true;

                    if (!hasTempsWithDestruction)
                    {
                        m_code.appendf("if ( !{} ) goto {};\n", cond, labelText);
                    }
                    else
                    {
                        auto ret = makeTempVar(op, m_types.resolveEngineType("bool"_id));
                        m_code.appendf("{} = {};\n", ret, cond);
                        finishStatement(op);
                        m_code.appendf("if (!{}) goto {};\n", ret, labelText);
                    }
                    return true;
                }

                case Opcode::ReturnAny:
                case Opcode::ReturnLoad1:
                case Opcode::ReturnLoad2:
                case Opcode::ReturnLoad4:
                case Opcode::ReturnLoad8:
                case Opcode::ReturnDirect:
                {
                    auto retPtr  = "params->_returnPtr";
                    if (m_hasDirectParamAccess)
                        retPtr = "resultPtr";

                    auto data = processOpcode(contextStr, stream.read(), stream);
                    auto retType  = m_types.resolveType(op->stub);
                    makeCopy(retType, retPtr, true, data.text, data.pointer);
                    return finishStatement(op);
                }

                case Opcode::ContextDtor:
                case Opcode::ContextCtor:
                {
                    auto prop  = op->stub->asProperty();
                    auto propType  = m_types.resolveType(prop->typeDecl);

                    auto owner  = prop->owner->asClass();
                    auto ownerType  = m_types.resolveType(owner);

                    StringBuilder var;
                    if (owner->flags.test(StubFlag::Struct) || owner->flags.test(StubFlag::Native))
                        var.appendf("( ({}*){} )->{}", ownerType->jitName, contextStr, prop->name);
                    else
                        var.appendf("( ({}*)ExternalPtr({}) )->{}", ownerType->jitExtraName, contextStr, prop->name);

                    if (op->op == Opcode::ContextCtor)
                        makeCtor(propType, true, var.view(), false);
                    else
                        makeDtor(propType, var.view(), false);

                    return finishStatement(op);
                }

                case Opcode::Switch:
                case Opcode::SwitchLabel:
                case Opcode::SwitchDefault:
                case Opcode::Conditional:
                case Opcode::Constructor:
                case Opcode::FinalFunc:
                case Opcode::VirtualFunc:
                case Opcode::StaticFunc:
                case Opcode::InternalFunc:
                    break;
            }

            // process as expression
            auto expr = processOpcode(contextStr, op, stream);
            if (!expr.text.empty())
            {
                m_code << expr.text;
                m_code << ";\n";
            }
            return finishStatement(op);
        }

        JITCodeChunkC JITFunctionWriterC::makeBinaryOp(const StubOpcode* context, const JITType* retType, const char* txt, const JITCodeChunkC& a, const JITCodeChunkC& b)
        {
            StringBuilder code;
            code.appendf(txt, a, b);

            JITCodeChunkC ret;
            ret.jitType = retType;
            ret.op = context;
            ret.text = copyString(code.view());
            return ret;
        }

        JITCodeChunkC JITFunctionWriterC::makeBinaryOp(const StubOpcode* context, const char* txt, const JITCodeChunkC& a, const JITCodeChunkC& b)
        {
            StringBuilder code;
            code.appendf(txt, a, b);

            JITCodeChunkC ret;
            ret.jitType = a.jitType;
            ret.op = context;
            ret.text = copyString(code.view());
            return ret;
        }

        JITCodeChunkC JITFunctionWriterC::makeBinaryOp(const StubOpcode* context, StringID engineType, const char* txt, const JITCodeChunkC& a, const JITCodeChunkC& b)
        {
            StringBuilder code;
            code.appendf(txt, a, b);

            JITCodeChunkC ret;
            ret.jitType = m_types.resolveEngineType(engineType);
            ret.op = context;
            ret.text = copyString(code.view());
            return ret;
        }

        JITCodeChunkC JITFunctionWriterC::makeUnaryOp(const StubOpcode* context, const JITType* retType, const char* txt, const JITCodeChunkC& a)
        {
            StringBuilder code;
            code.appendf(txt, a);

            JITCodeChunkC ret;
            ret.jitType = retType;
            ret.op = context;
            ret.text = copyString(code.view());
            return ret;
        }

        JITCodeChunkC JITFunctionWriterC::makeUnaryOp(const StubOpcode* context, const char* txt, const JITCodeChunkC& a)
        {
            StringBuilder code;
            code.appendf(txt, a);

            JITCodeChunkC ret;
            ret.jitType = a.jitType;
            ret.op = context;
            ret.text = copyString(code.view());
            return ret;
        }

        JITCodeChunkC JITFunctionWriterC::makeUnaryOp(const StubOpcode* context, StringID engineType, const char* txt, const JITCodeChunkC& a)
        {
            StringBuilder code;
            code.appendf(txt, a);

            JITCodeChunkC ret;
            ret.jitType = m_types.resolveEngineType(engineType);
            ret.op = context;
            ret.text = copyString(code.view());
            return ret;
        }

        JITCodeChunkC JITFunctionWriterC::processOpcode(StringView<char> contextStr, const StubOpcode* op, JITOpcodeStream& stream)
        {
            if (!op)
            {
                reportError(m_func, "Unexpected end of opcodes");
                return JITCodeChunkC();
            }

            switch (op->op)
            {
                case Opcode::Null:
                {
                    auto t = m_types.resolveEngineType("strong<base::IObject>"_id);
                    return makeChunk(op, t, "StrongNull()");
                }

                case Opcode::IntOne:
                {
                    auto t  = m_types.resolveEngineType("int"_id);
                    return makeChunk(op, t, "1");
                }

                case Opcode::IntZero:
                {
                    auto t  = m_types.resolveEngineType("int"_id);
                    return makeChunk(op, t, "0");
                }

                case Opcode::FloatConst:
                {
                    auto t  = m_types.resolveEngineType("float"_id);
                    return makeChunk(op, t, TempString("{}", op->value.f));
                }

                case Opcode::DoubleConst:
                {
                    auto t  = m_types.resolveEngineType("double"_id);
                    return makeChunk(op, t, TempString("{}", op->value.d));
                }

                case Opcode::IntConst1:
                {
                    auto t  = m_types.resolveEngineType("char"_id);
                    return makeChunk(op, t, TempString("{}", op->value.i));
                }

                case Opcode::IntConst2:
                {
                    auto t  = m_types.resolveEngineType("short"_id);
                    return makeChunk(op, t, TempString("{}", op->value.i));
                }

                case Opcode::IntConst4:
                {
                    auto t  = m_types.resolveEngineType("int"_id);
                    return makeChunk(op, t, TempString("{}", op->value.i));
                }

                case Opcode::IntConst8:
                {
                    auto t  = m_types.resolveEngineType("int64_t"_id);
                    return makeChunk(op, t, TempString("{}", op->value.i));
                }

                case Opcode::UintConst1:
                {
                    auto t  = m_types.resolveEngineType("uint8_t"_id);
                    return makeChunk(op, t, TempString("{}", op->value.u));
                }

                case Opcode::UintConst2:
                {
                    auto t  = m_types.resolveEngineType("uint16_t"_id);
                    return makeChunk(op, t, TempString("{}", op->value.u));
                }

                case Opcode::UintConst4:
                {
                    auto t  = m_types.resolveEngineType("uint32_t"_id);
                    return makeChunk(op, t, TempString("{}", op->value.u));
                }

                case Opcode::UintConst8:
                {
                    auto t  = m_types.resolveEngineType("uint64_t"_id);
                    return makeChunk(op, t, TempString("{}", op->value.u));
                }

                case Opcode::StringConst:
                {
                    auto t  = m_types.resolveEngineType("StringBuf"_id);
                    auto constName = m_consts.mapStringConst(op->value.text.c_str());
                    return makeChunk(op, t, constName);
                }

                case Opcode::NameConst:
                {
                    auto t  = m_types.resolveEngineType("StringID"_id);
                    auto constName = m_consts.mapNameConst(op->value.text.c_str());
                    return makeChunk(op, t, constName);
                };

                //case Opcode::EnumConst:

                case Opcode::ClassConst:
                {
                    auto t  = m_types.resolveEngineType(reflection::GetTypeName<SpecificClassType<IObject>>());
                    auto engineType  = m_types.resolveType(op->stub);
                    auto constType = m_consts.mapTypeConst(engineType->name.c_str());
                    return makeChunk(op, t, constType);
                }

                case Opcode::BoolTrue:
                {
                    auto t  = m_types.resolveEngineType("bool"_id);
                    return makeChunk(op, t, "1");
                }

                case Opcode::BoolFalse:
                {
                    auto t = m_types.resolveEngineType("bool"_id);
                    return makeChunk(op, t, "0");
                }

                case Opcode::LocalVar:
                {
                    auto name = op->value.name;
                    auto index = op->value.i;
                    auto type = op->stub;
                    return makeLocalVar(op, index, type, name);
                }

                case Opcode::ParamVar:
                {
                    auto index = op->value.i;
                    auto arg = m_func->args[index];
                    auto type = m_types.resolveType(arg->typeDecl);

                    if (m_hasDirectParamAccess)
                    {
                        JITCodeChunkC ret;
                        ret.op = op;
                        ret.pointer = arg->flags.test(StubFlag::Ref) || arg->flags.test(StubFlag::Out);
                        ret.text = arg->name.view();
                        ret.jitType = type;
                        return ret;
                    }
                    else
                    {
                        JITCodeChunkC ret;
                        ret.op = op;
                        ret.pointer = true;
                        ret.text = copyString(TempString("(({}*)params->_argPtr[{}])", type->jitName, index));
                        ret.jitType = type;
                        return ret;
                    }
                }

                case Opcode::StaticFunc:
                case Opcode::VirtualFunc:
                case Opcode::FinalFunc:
                {
                    auto funcStub  = op->stub->asFunction();
                    if (!funcStub)
                    {
                        reportError(op, "No function to call");
                        return JITCodeChunkC();
                    }

                    auto func  = m_types.resolveFunction(funcStub);
                    if (!func || func->jitName.empty())
                    {
                        reportError(op, "No function import to call");
                        return JITCodeChunkC();
                    }

                    // determine call mode
                    int callMode = 0;

                    // usually the return is placed in a temporary var and function call becomes a statement
                    // but there are some optimizations for simple cases
                    StringBuilder ret;
                    JITCodeChunkC returnValue;
                    bool printComma = true;
                    if (func->canReturnDirectly && func->jitDirectCallName)
                    {
                        // NOTE: we don't pass any of the internal stuff (stack, context, etc)
                        ret.appendf("(*{})(", func->jitDirectCallName);
                        printComma = false;
                    }
                    else if (func->canReturnDirectly)
                    {
                        // NOTE: we don't emit the resultPtr here
                        ret.appendf("{}({}, {}, stackFrame", func->jitName, contextStr, callMode);
                    }
                    else if (func->returnType != nullptr)
                    {
                        // we can't return directly but we DO return something
                        returnValue = makeTempVar(op, func->returnType);
                        ret.appendf("{}({}, {}, stackFrame, &{}", func->jitName, contextStr, callMode, returnValue);
                    }
                    else
                    {
                        // we don't return any value, function is a pure statement
                        ret.appendf("{}({}, {}, stackFrame, 0", func->jitName, contextStr, callMode);
                    }

                    // emit arguments
                    for (auto& arg : func->args)
                    {
                        if (printComma) ret.append(", ");
                        printComma = true;

                        auto value = processOpcode(contextStr, stream.read(), stream);
                        if (arg.passAsReference)
                            ret << makePointer(value);
                        else
                            ret << makeValue(value);
                    }

                    // end call
                    ret.appendf(")");

                    // if we have to use a temp value to store the result than return the temp, otherwise return the function call as the expression
                    if (func->canReturnDirectly)
                    {
                        returnValue.text = copyString(ret.view());
                        returnValue.jitType = func->returnType;
                        returnValue.op = op;
                        returnValue.pointer = false; // TODO: functions returning refs ?
                    }
                    else
                    {
                        // write as a statement
                        m_code << ret << ";\n";
                    }

                    return returnValue;
                }

                case Opcode::LoadAny:
                case Opcode::LoadInt1:
                case Opcode::LoadInt2:
                case Opcode::LoadInt4:
                case Opcode::LoadInt8:
                case Opcode::LoadUint1:
                case Opcode::LoadUint2:
                case Opcode::LoadUint4:
                case Opcode::LoadUint8:
                case Opcode::LoadFloat:
                case Opcode::LoadDouble:
                case Opcode::LoadStrongPtr:
                case Opcode::LoadWeakPtr:
                {
                    auto ptr = processOpcode(contextStr, stream.read(), stream);
                    return makeValue(ptr);
                }

                case Opcode::AddInt8:
                case Opcode::AddInt16:
                case Opcode::AddInt32:
                case Opcode::AddInt64:
                case Opcode::AddDouble:
                case Opcode::AddFloat:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} + {})", a, b);
                }

                case Opcode::SubInt8:
                case Opcode::SubInt16:
                case Opcode::SubInt32:
                case Opcode::SubInt64:
                case Opcode::SubDouble:
                case Opcode::SubFloat:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} - {})", a, b);
                }

                case Opcode::MulSigned8:
                case Opcode::MulSigned16:
                case Opcode::MulSigned32:
                case Opcode::MulSigned64:
                case Opcode::MulUnsigned8:
                case Opcode::MulUnsigned16:
                case Opcode::MulUnsigned32:
                case Opcode::MulUnsigned64:
                case Opcode::MulDouble:
                case Opcode::MulFloat:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} * {})", a, b);
                }

                case Opcode::DivSigned8:
                case Opcode::DivSigned16:
                case Opcode::DivSigned32:
                case Opcode::DivSigned64:
                case Opcode::DivUnsigned8:
                case Opcode::DivUnsigned16:
                case Opcode::DivUnsigned32:
                case Opcode::DivUnsigned64:
                case Opcode::DivDouble:
                case Opcode::DivFloat:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} / {})", a, b);
                }

                //case Opcode::ModFloat:
                //case Opcode::ModDouble:
                //case Opcode::Passthrough:
                //case Opcode::Breakpoint:

                case Opcode::ContextVar:
                {
                    auto prop  = op->stub->asProperty();
                    auto owner  = prop->owner->asClass();

                    JITCodeChunkC ret;
                    ret.op = op;
                    ret.pointer = false;
                    ret.jitType = m_types.resolveType(prop->typeDecl);

                    StringBuilder txt;

                    auto ownerType  = m_types.resolveType(owner);

                    if (owner->flags.test(StubFlag::Struct) || owner->flags.test(StubFlag::Native))
                        txt.appendf("((({}*){})", ownerType->jitName, contextStr);
                    else
                        txt.appendf("((({}*)ExternalPtr({}))", ownerType->jitExtraName, contextStr);

                    txt.appendf("->{})", prop->name);

                    ret.text = m_mem.strcpy(txt.c_str());
                    return ret;
                }

                case Opcode::Switch:
                case Opcode::SwitchLabel:
                case Opcode::SwitchDefault:
                case Opcode::Conditional:
                case Opcode::InternalFunc:
                    break;

                case Opcode::Constructor:
                {
                    auto t  = m_types.resolveType(op->stub);

                    auto ret = makeTempVar(op, t);

                    for (uint32_t i=0; i<op->value.u; ++i)
                    {
                        auto val = makeValue(processOpcode(contextStr, stream.read(), stream));
                        m_code.appendf("{}.{} = {};\n", ret, t->fields[i].name, val);
                    }

                    return ret;
                }

                case Opcode::StructMemberRef:
                case Opcode::StructMember:
                {
                    auto prop  = op->stub->asProperty();
                    auto propType  = m_types.resolveType(prop->typeDecl);

                    auto a = processOpcode(contextStr, stream.read(), stream);

                    StringBuilder txt;
                    if (a.pointer)
                        txt.appendf("(&(({})->{}))", a, prop->name);
                    else
                        txt.appendf("(({}).{})", a, prop->name);

                    JITCodeChunkC ret;
                    ret.pointer = a.pointer;
                    ret.op = op;
                    ret.jitType = propType;
                    ret.text = copyString(txt.view());
                    return ret;
                }

                case Opcode::ContextFromValue:
                case Opcode::ContextFromRef:
                {
                    auto context = makePointer(processOpcode(contextStr, stream.read(), stream));
                    return processOpcode(TempString("({})", context), stream.read(), stream);
                }

                case Opcode::ContextFromPtr:
                case Opcode::ContextFromPtrRef:
                {
                    auto context = processOpcode(contextStr, stream.read(), stream);
                    auto exitLabel = makeStatementExitLabel();

                    if (m_emitExceptions)
                    {
                        if (context.pointer)
                            m_code.appendf("if (0 == ({}->dataPtr)) { ERROR(\"{}\", {}, \"Accessing NULL pointer\"); goto {}; }\n", context, op->location.file->depotPath, op->location.line, exitLabel);
                        else
                            m_code.appendf("if (0 == ({}.dataPtr)) { ERROR(\"{}\", {}, \"Accessing NULL pointer\"); goto {}; }\n", context, op->location.file->depotPath, op->location.line, exitLabel);
                    }
                    else
                    {
                        if (context.pointer)
                            m_code.appendf("if (0 == ({}->dataPtr)) goto {};\n", context, exitLabel);
                        else
                            m_code.appendf("if (0 == ({}.dataPtr)) goto {};\n", context, exitLabel);
                    }

                    auto innerResult = processOpcode(context.pointer ? TempString("({}->dataPtr)", context) : TempString("({}.dataPtr)", context), stream.read(), stream);

                    auto label  = stream.read();
                    ASSERT(label->op == Opcode::Label);

                    if (innerResult.text)
                        return innerResult;
                    else
                        return JITCodeChunkC();
                }

                case Opcode::ThisStruct:
                {
                    JITCodeChunkC ret;
                    ret.pointer = true;
                    ret.op = op;
                    ret.jitType = m_types.resolveType(m_func->owner->asClass());
                    ret.text = "context";
                    return ret;
                }

                case Opcode::ThisObject:
                {
                    auto type = m_types.resolveEngineType(reflection::GetTypeName<ObjectPtr>());
                    auto temp = makeTempVar(op, type);
                    m_code.appendf("EI->_fnStrongFromPtr(EI->self, {}, &{});\n", contextStr, temp);
                    return temp;
                }

                case Opcode::New:
                {
                    auto cls = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto type = m_types.resolveEngineType(reflection::GetTypeName<ObjectPtr>());
                    auto ret = makeTempVar(op, type);
                    m_code.appendf("EI->_fnNew(EI->self, stackFrame, &{}, &{});\n", cls, ret);
                    return ret;
                }

                case Opcode::Int32ToEnum:
                case Opcode::Int64ToEnum:
                case Opcode::EnumToInt64:
                case Opcode::EnumToInt32:
                {
                    return makeValue(processOpcode(contextStr, stream.read(), stream));
                }

                case Opcode::EnumToName:
                {
                    auto type = m_types.resolveType(op->stub);
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));

                    StringBuilder txt;
                    txt.appendf("EI->_fnEnumToName(EI->self, {}, {})", contextStr, type->assignedID, a);

                    return makeChunk(op, type, txt.view());
                }

                case Opcode::NameToEnum:
                {
                    auto type = m_types.resolveType(op->stub);
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));

                    StringBuilder txt;
                    txt.appendf("EI->_fnNameToEnum(EI->self, stackFrame, {}, {})", contextStr, type->assignedID, a);

                    return makeChunk(op, type, txt.view());
                }

                case Opcode::DynamicCast:
                {
                    break;
                }

                case Opcode::DynamicWeakCast:
                {
                    break;
                }

                case Opcode::MetaCast:
                {
                    break;
                }

                case Opcode::WeakToStrong:
                {
                    auto type = m_types.resolveEngineType(reflection::GetTypeName<ObjectPtr>());
                    auto ret = makeTempVar(op, type);

                    auto a = makePointer(processOpcode(contextStr, stream.read(), stream));
                    m_code.appendf("EI->_fnWeakToStrong(EI->self, {}, &{});\n", a, ret);
                    return ret;
                }

                case Opcode::WeakToBool:
                {
                    auto a = makePointer(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "bool"_id, "EI->_fnWeakToBool(EI->self, {})", a);
                }

                case Opcode::StrongToWeak:
                {
                    auto type = m_types.resolveEngineType(reflection::GetTypeName<ObjectWeakPtr>());
                    auto ret = makeTempVar(op, type);

                    auto a = makePointer(processOpcode(contextStr, stream.read(), stream));
                    m_code.appendf("EI->_fnStrongToWeak(EI->self, {}, &{});\n", a, ret);
                    return ret;
                }

                case Opcode::NameToBool:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "bool"_id, "(0 != {}._nameId)", a);
                }

                case Opcode::ClassToBool:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "bool"_id, "(0 != {}.classPtr)", a);
                }

                case Opcode::ClassToName:
                {
                    auto a = makePointer(processOpcode(contextStr, stream.read(), stream));

                    StringBuilder txt;
                    m_code.appendf("EI->_fnClassToName(EI->self, {})", contextStr, a);

                    return makeChunk(op, m_types.resolveEngineType("StringID"_id), txt.view());
                }

                case Opcode::ClassToString:
                {
                    auto a = makePointer(processOpcode(contextStr, stream.read(), stream));
                    auto ret = makeTempVar(op, m_types.resolveEngineType("StringBuf"_id));
                    m_code.appendf("EI->_fnClassToString(EI->self, {}, &{});\n", a, ret);
                    return ret;
                }

                case Opcode::CastToVariant:
                case Opcode::CastFromVariant:
                case Opcode::VariantIsValid:
                case Opcode::VariantIsPointer:
                case Opcode::VariantIsArray:
                case Opcode::VariantGetType:
                case Opcode::VariantToString:
                    break;

                case Opcode::StrongToBool:
                {
                    auto a = processOpcode(contextStr, stream.read(), stream);
                    StringBuilder txt;
                    if (a.pointer)
                        return makeUnaryOp(op, "bool"_id, "(0 != {}->dataPtr)", a);
                    else
                        return makeUnaryOp(op, "bool"_id, "(0 != {}.dataPtr)", a);
                }

                case Opcode::LogicAnd: // special
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto ret = a;
                    if (!a.text.beginsWith("___temp"))
                    {
                        ret = makeTempVar(op, m_types.resolveEngineType("bool"_id));
                        m_code.appendf("{} = {};\n", ret, a);
                    }

                    m_code.appendf("if ({}) {\n", ret);
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    m_code.appendf("{} = {};\n", ret, b);
                    m_code.appendf("}\n", ret, b);

                    auto label  = stream.read();
                    ASSERT(label->op == Opcode::Label);

                    return ret;
                }

                case Opcode::LogicOr: // special
                {
                    auto ret = makeTempVar(op, m_types.resolveEngineType("bool"_id));
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    m_code.appendf("{} = {};\n", ret, a);
                    m_code.appendf("if (!{}) {\n", ret);
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    m_code.appendf("{} = {};\n", ret, b);
                    m_code.appendf("}\n", ret, b);

                    auto label  = stream.read();
                    ASSERT(label->op == Opcode::Label);

                    return ret;
                }

                case Opcode::LogicNot:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "bool"_id, "(!{})", a);
                }

                case Opcode::LogicXor:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "({} ^ {})", a, b);
                }

                case Opcode::TestEqual:
                {
                    auto a = makePointer(processOpcode(contextStr, stream.read(), stream));
                    auto b = makePointer(processOpcode(contextStr, stream.read(), stream));

                    StringBuilder txt;
                    txt.appendf("COMPARE({}, ({}), ({}))", a.jitType->assignedID, a, b);

                    JITCodeChunkC ret;
                    ret.jitType = m_types.resolveEngineType("bool"_id);
                    ret.text = copyString(txt.view());
                    ret.op = op;
                    return ret;
                }

                case Opcode::TestNotEqual:
                {
                    auto a = makePointer(processOpcode(contextStr, stream.read(), stream));
                    auto b = makePointer(processOpcode(contextStr, stream.read(), stream));

                    StringBuilder txt;
                    txt.appendf("(!COMPARE({}, ({}), ({})))", a.jitType->assignedID, a, b);

                    JITCodeChunkC ret;
                    ret.jitType = m_types.resolveEngineType("bool"_id);
                    ret.text = copyString(txt.view());
                    ret.op = op;
                    return ret;
                }

                case Opcode::TestEqual1:
                case Opcode::TestEqual2:
                case Opcode::TestEqual4:
                case Opcode::TestEqual8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) == ({}))", a, b);
                }

                case Opcode::TestNotEqual1:
                case Opcode::TestNotEqual2:
                case Opcode::TestNotEqual4:
                case Opcode::TestNotEqual8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) != ({}))", a, b);
                }

                case Opcode::TestSignedLess1:
                case Opcode::TestSignedLess2:
                case Opcode::TestSignedLess4:
                case Opcode::TestSignedLess8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) < ({}))", a, b);
                }

                case Opcode::TestSignedLessEqual1:
                case Opcode::TestSignedLessEqual2:
                case Opcode::TestSignedLessEqual4:
                case Opcode::TestSignedLessEqual8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) <= ({}))", a, b);
                }

                case Opcode::TestSignedGreater1:
                case Opcode::TestSignedGreater2:
                case Opcode::TestSignedGreater4:
                case Opcode::TestSignedGreater8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) > ({}))", a, b);
                }

                case Opcode::TestSignedGreaterEqual1:
                case Opcode::TestSignedGreaterEqual2:
                case Opcode::TestSignedGreaterEqual4:
                case Opcode::TestSignedGreaterEqual8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) >= ({}))", a, b);
                }

                case Opcode::TestUnsignedLess1:
                case Opcode::TestUnsignedLess2:
                case Opcode::TestUnsignedLess4:
                case Opcode::TestUnsignedLess8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) < ({}))", a, b);
                }

                case Opcode::TestUnsignedLessEqual1:
                case Opcode::TestUnsignedLessEqual2:
                case Opcode::TestUnsignedLessEqual4:
                case Opcode::TestUnsignedLessEqual8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) <= ({}))", a, b);
                }

                case Opcode::TestUnsignedGreater1:
                case Opcode::TestUnsignedGreater2:
                case Opcode::TestUnsignedGreater4:
                case Opcode::TestUnsignedGreater8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) > ({}))", a, b);
                }

                case Opcode::TestUnsignedGreaterEqual1:
                case Opcode::TestUnsignedGreaterEqual2:
                case Opcode::TestUnsignedGreaterEqual4:
                case Opcode::TestUnsignedGreaterEqual8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) >= ({}))", a, b);
                }

                case Opcode::TestFloatEqual4:
                case Opcode::TestFloatEqual8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) == ({}))", a, b);
                }

                case Opcode::TestFloatNotEqual4:
                case Opcode::TestFloatNotEqual8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) != ({}))", a, b);
                }

                case Opcode::TestFloatLess4:
                case Opcode::TestFloatLess8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) < ({}))", a, b);
                }

                case Opcode::TestFloatLessEqual4:
                case Opcode::TestFloatLessEqual8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) <= ({}))", a, b);
                }

                case Opcode::TestFloatGreater4:
                case Opcode::TestFloatGreater8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) > ({}))", a, b);
                }

                case Opcode::TestFloatGreaterEqual4:
                case Opcode::TestFloatGreaterEqual8:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "bool"_id, "(({}) >= ({}))", a, b);
                }

                case Opcode::NumberToBool8:
                case Opcode::NumberToBool16:
                case Opcode::NumberToBool32:
                case Opcode::NumberToBool64:
                case Opcode::FloatToBool:
                case Opcode::DoubleToBool:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "bool"_id, "(({}) != 0)", a);
                }

                case Opcode::MinSigned8:
                case Opcode::MinSigned16:
                case Opcode::MinSigned32:
                case Opcode::MinSigned64:
                case Opcode::MinUnsigned8:
                case Opcode::MinUnsigned16:
                case Opcode::MinUnsigned32:
                case Opcode::MinUnsigned64:
                case Opcode::MinFloat:
                case Opcode::MinDouble:
                    break;

                case Opcode::MaxSigned8:
                case Opcode::MaxSigned16:
                case Opcode::MaxSigned32:
                case Opcode::MaxSigned64:
                case Opcode::MaxUnsigned8:
                case Opcode::MaxUnsigned16:
                case Opcode::MaxUnsigned32:
                case Opcode::MaxUnsigned64:
                case Opcode::MaxFloat:
                case Opcode::MaxDouble:
                    break;

                case Opcode::ClampSigned8:
                case Opcode::ClampSigned16:
                case Opcode::ClampSigned32:
                case Opcode::ClampSigned64:
                case Opcode::ClampUnsigned8:
                case Opcode::ClampUnsigned16:
                case Opcode::ClampUnsigned32:
                case Opcode::ClampUnsigned64:
                case Opcode::ClampFloat:
                case Opcode::ClampDouble:
                    break;

                case Opcode::Abs8:
                case Opcode::Abs16:
                case Opcode::Abs32:
                case Opcode::Abs64:
                case Opcode::AbsFloat:
                case Opcode::AbsDouble:
                    break;

                case Opcode::Sign8:
                case Opcode::Sign16:
                case Opcode::Sign32:
                case Opcode::Sign64:
                case Opcode::SignFloat:
                case Opcode::SignDouble:
                    break;

                case Opcode::ModSigned8:
                case Opcode::ModSigned16:
                case Opcode::ModSigned32:
                case Opcode::ModSigned64:
                case Opcode::ModUnsigned8:
                case Opcode::ModUnsigned16:
                case Opcode::ModUnsigned32:
                case Opcode::ModUnsigned64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} % {})", a, b);
                }

                case Opcode::BitAnd8:
                case Opcode::BitAnd16:
                case Opcode::BitAnd32:
                case Opcode::BitAnd64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} & {})", a, b);
                }

                case Opcode::BitOr8:
                case Opcode::BitOr16:
                case Opcode::BitOr32:
                case Opcode::BitOr64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} | {})", a, b);
                }

                case Opcode::BitXor8:
                case Opcode::BitXor16:
                case Opcode::BitXor32:
                case Opcode::BitXor64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} ^ {})", a, b);
                }

                case Opcode::BitNot8:
                case Opcode::BitNot16:
                case Opcode::BitNot32:
                case Opcode::BitNot64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "(~ {})", a);
                }

                case Opcode::BitShl8:
                case Opcode::BitShl16:
                case Opcode::BitShl32:
                case Opcode::BitShl64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} << {})", a, b);
                }

                case Opcode::BitShr8:
                case Opcode::BitShr16:
                case Opcode::BitShr32:
                case Opcode::BitShr64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} >> {})", a, b);
                }

                case Opcode::BitSar8:
                case Opcode::BitSar16:
                case Opcode::BitSar32:
                case Opcode::BitSar64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} >> {})", a, b);
                }

                case Opcode::BitAndAssign8:
                case Opcode::BitAndAssign16:
                case Opcode::BitAndAssign32:
                case Opcode::BitAndAssign64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} &= {})", a, b);
                }

                case Opcode::BitOrAssign8:
                case Opcode::BitOrAssign16:
                case Opcode::BitOrAssign32:
                case Opcode::BitOrAssign64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} |= {})", a, b);
                }

                case Opcode::BitXorAssign8:
                case Opcode::BitXorAssign16:
                case Opcode::BitXorAssign32:
                case Opcode::BitXorAssign64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} ^= {})", a, b);
                }

                case Opcode::BitShlAssign8:
                case Opcode::BitShlAssign16:
                case Opcode::BitShlAssign32:
                case Opcode::BitShlAssign64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} <<= {})", a, b);
                }

                case Opcode::BitShrAssign8:
                case Opcode::BitShrAssign16:
                case Opcode::BitShrAssign32:
                case Opcode::BitShrAssign64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} >>= {})", a, b);
                }

                case Opcode::BitSarAssign8:
                case Opcode::BitSarAssign16:
                case Opcode::BitSarAssign32:
                case Opcode::BitSarAssign64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} >>= {})", a, b);
                }

                case Opcode::PreIncrement8:
                case Opcode::PreIncrement16:
                case Opcode::PreIncrement32:
                case Opcode::PreIncrement64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "(++({}))", a);
                }

                case Opcode::PreDecrement8:
                case Opcode::PreDecrement16:
                case Opcode::PreDecrement32:
                case Opcode::PreDecrement64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "(--({}))", a);
                }

                case Opcode::PostIncrement8:
                case Opcode::PostIncrement16:
                case Opcode::PostIncrement32:
                case Opcode::PostIncrement64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "(({})++)", a);
                }

                case Opcode::PostDecrement8:
                case Opcode::PostDecrement16:
                case Opcode::PostDecrement32:
                case Opcode::PostDecrement64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "(({})++)", a);
                }

                case Opcode::ExpandUnsigned8To16:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "uint16_t"_id, "(uint16_t)({})", a);
                }

                case Opcode::ExpandSigned8To16:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "short"_id, "(short)({})", a);
                }

                case Opcode::ExpandSigned8To32:
                case Opcode::ExpandSigned16To32:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "int"_id, "(int)({})", a);
                }

                case Opcode::ExpandUnsigned8To32:
                case Opcode::ExpandUnsigned16To32:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "uint32_t"_id, "(uint32_t)({})", a);
                }

                case Opcode::ExpandSigned8To64:
                case Opcode::ExpandSigned16To64:
                case Opcode::ExpandSigned32To64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "int64_t"_id, "(int64_t)({})", a);
                }

                case Opcode::ExpandUnsigned8To64:
                case Opcode::ExpandUnsigned16To64:
                case Opcode::ExpandUnsigned32To64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "uint64_t"_id, "(uint64_t)({})", a);
                }

                case Opcode::Contract64To8:
                case Opcode::Contract32To8:
                case Opcode::Contract16To8:
                {
                    auto a = processOpcode(contextStr, stream.read(), stream);
                    return makeUnaryOp(op, "uint8_t"_id, "(uint8_t)({})", a);
                }

                case Opcode::Contract64To16:
                case Opcode::Contract32To16:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "uint16_t"_id, "(uint16_t)({})", a);
                }

                case Opcode::Contract64To32:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "uint32_t"_id, "(uint32_t)({})", a);
                }

                case Opcode::FloatToInt8:
                case Opcode::FloatToInt16:
                case Opcode::FloatToInt32:
                case Opcode::FloatToInt64:
                case Opcode::FloatToUint8:
                case Opcode::FloatToUint16:
                case Opcode::FloatToUint32:
                case Opcode::FloatToUint64:
                case Opcode::FloatToDouble:
                case Opcode::FloatFromInt8:
                case Opcode::FloatFromInt16:
                case Opcode::FloatFromInt32:
                case Opcode::FloatFromInt64:
                case Opcode::FloatFromUint8:
                case Opcode::FloatFromUint16:
                case Opcode::FloatFromUint32:
                case Opcode::FloatFromUint64:
                case Opcode::FloatFromDouble:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "float"_id, "(float)({})", a);
                }

                case Opcode::DoubleToInt8:
                case Opcode::DoubleToInt16:
                case Opcode::DoubleToInt32:
                case Opcode::DoubleToInt64:
                case Opcode::DoubleToUint8:
                case Opcode::DoubleToUint16:
                case Opcode::DoubleToUint32:
                case Opcode::DoubleToUint64:
                case Opcode::DoubleFromInt8:
                case Opcode::DoubleFromInt16:
                case Opcode::DoubleFromInt32:
                case Opcode::DoubleFromInt64:
                case Opcode::DoubleFromUint8:
                case Opcode::DoubleFromUint16:
                case Opcode::DoubleFromUint32:
                case Opcode::DoubleFromUint64:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "double"_id, "(double)({})", a);
                }

                case Opcode::NegSigned8:
                case Opcode::NegSigned16:
                case Opcode::NegSigned32:
                case Opcode::NegSigned64:
                case Opcode::NegFloat:
                case Opcode::NegDouble:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeUnaryOp(op, "(- {})", a);
                }

                case Opcode::AddAssignInt8:
                case Opcode::AddAssignInt16:
                case Opcode::AddAssignInt32:
                case Opcode::AddAssignInt64:
                case Opcode::AddAssignDouble:
                case Opcode::AddAssignFloat:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} += {})", a, b);
                }

                case Opcode::SubAssignInt8:
                case Opcode::SubAssignInt16:
                case Opcode::SubAssignInt32:
                case Opcode::SubAssignInt64:
                case Opcode::SubAssignDouble:
                case Opcode::SubAssignFloat:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} = {})", a, b);
                }

                case Opcode::MulAssignSignedInt8:
                case Opcode::MulAssignSignedInt16:
                case Opcode::MulAssignSignedInt32:
                case Opcode::MulAssignSignedInt64:
                case Opcode::MulAssignUnsignedInt8:
                case Opcode::MulAssignUnsignedInt16:
                case Opcode::MulAssignUnsignedInt32:
                case Opcode::MulAssignUnsignedInt64:
                case Opcode::MulAssignDouble:
                case Opcode::MulAssignFloat:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} *= {})", a, b);
                }

                case Opcode::DivAssignSignedInt8:
                case Opcode::DivAssignSignedInt16:
                case Opcode::DivAssignSignedInt32:
                case Opcode::DivAssignSignedInt64:
                case Opcode::DivAssignUnsignedInt8:
                case Opcode::DivAssignUnsignedInt16:
                case Opcode::DivAssignUnsignedInt32:
                case Opcode::DivAssignUnsignedInt64:
                case Opcode::DivAssignDouble:
                case Opcode::DivAssignFloat:
                {
                    auto a = makeValue(processOpcode(contextStr, stream.read(), stream));
                    auto b = makeValue(processOpcode(contextStr, stream.read(), stream));
                    return makeBinaryOp(op, "({} /= {})", a, b);
                }

                break;
            }

            // invalid opcode
            reportError(op, TempString("Opcode '{}' is not supported by JIT", op->op));
            return JITCodeChunkC();
        }

    } // script
} // base