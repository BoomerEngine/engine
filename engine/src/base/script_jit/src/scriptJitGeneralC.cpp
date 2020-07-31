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

#include "base/script/include/scriptCompiledProject.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/absolutePath.h"
#include "base/io/include/timestamp.h"

namespace base
{
    namespace script
    {

        //--

        base::mem::PoolID POOL_JIT("Script.JIT");

        //--

        RTTI_BEGIN_TYPE_CLASS(JITGeneralC);
        RTTI_END_TYPE();

        JITGeneralC::JITGeneralC()
            : m_mem(POOL_JIT)
            , m_emitExceptions(false)
            , m_emitLines(false)
        {}

        JITGeneralC::~JITGeneralC()
        {
        }

        bool JITGeneralC::compile(const IJITNativeTypeInsight& typeInsight, const CompiledProjectPtr& project, const io::AbsolutePath& outputModulePath, const Settings& settings)
        {
            // setup settings
            m_emitExceptions = settings.emitExceptions;
            m_emitLines = settings.emitOriginalLines;

            // generate some common shit
            m_data = project->data().get();
            m_typeLib.create(m_mem, typeInsight);
            m_constCache.create(m_mem, *m_typeLib);

            // generate code for all functions
            for (auto stub  : m_data->allStubs())
                if (auto func  = stub->asFunction())
                    exportFunction(func);

            // type lib failed ?
            if (m_typeLib->hasErrors())
            {
                TRACE_ERROR("JIT: Failed to properly resolve all types used in compiled scripts, JIT failed");
                return false;
            }

            // generate code for functions
            if (!generateCode())
            {
                TRACE_ERROR("JIT: Failed to properly generate code for all functions, JIT failed");
                return false;
            }

            // resolve layouts of types
            if (!m_typeLib->resolveTypeSizesAndLayouts())
            {
                TRACE_ERROR("JIT: Failed to resolve layouts of types used in JITed code");
                return false;
            }

            // type lib failed ?
            if (m_typeLib->hasErrors())
            {
                TRACE_ERROR("JIT: Failed to properly resolve all types used in compiled scripts, JIT failed");
                return false;
            }

            // let's give it a try
            return true;
        }

        void JITGeneralC::exportFunction(const StubFunction* func)
        {
            // do not export functions that are imported (even if we have code)
            if (func->flags.test(StubFlag::Import))
                return;

            // map class
            const JITType* classType = nullptr;
            if (func->owner && func->owner->asClass())
                classType = m_typeLib->resolveType(func->owner);

            // generate entry
            auto entry  = m_mem.create<ExportedFunction>();
            entry->stub = func;
            entry->jitClass = classType;
            entry->functionName = m_mem.strcpy(func->fullName().c_str());
            entry->jitName = m_mem.strcpy(TempString("__jit_func_{}_{}", func->name, m_exportedFunctions.size()).c_str());

            // can we call this function in a fast mode ? (all value passed types must be simple)
            bool canCallInFastMode = true;
            for (auto arg  : func->args)
            {
                if (arg->flags.test(StubFlag::Ref) || arg->flags.test(StubFlag::Out))
                    continue; // we are passing a pointer any way

                auto argType  = m_typeLib->resolveType(arg->typeDecl);
                if (argType->requiresDestructor || argType->requiresConstructor || !argType->simpleCopyCompare)
                    canCallInFastMode = false;
            }

            // if we can call this function in fast mode than generate a different wrapepr for it
            if (canCallInFastMode)
            {
                entry->jitLocalName = m_mem.strcpy(TempString("__local_func_{}_{}", func->name, m_exportedFunctions.size()).c_str());

                if (func->name == "__ctor"_id)
                    classType->ctorFuncName = entry->jitLocalName;
                else if (func->name == "__dtor"_id)
                    classType->dtorFuncName = entry->jitLocalName;
            }

            // add to map
            m_exportedFunctionMap[func] = entry;
            m_exportedFunctions.pushBack(entry);
       }

        bool JITGeneralC::generateCode()
        {
            bool valid = true;

            for (auto func  : m_exportedFunctions)
            {
                // in fast mode call we have direct access to arguments
                auto fastCall  = !func->jitLocalName.empty();

                // generate code
                JITFunctionWriterC writer(m_mem, *m_typeLib, *m_constCache, func->stub, fastCall, m_emitExceptions, m_emitLines);
                if (!writer.emitOpcodes(func->code))
                {
                    TRACE_ERROR("{}({}): Failed to JIT function '{}'", func->stub->location.file->absolutePath, func->stub->location.line, func->stub->name.c_str());
                    valid = false;
                }

                // report that we have a local version of the function
                if (fastCall)
                    m_typeLib->reportLocalFunctionBody(func->stub, func->jitLocalName, true);
                else
                    m_typeLib->reportLocalFunctionBody(func->stub, func->jitName, false);
            }

            return valid;
        }

        //---

        static io::AbsolutePath GetTempFilePath()
        {
            auto filePath  = base::io::SystemPath(io::PathCategory::TempDir).addDir("jit");
            //auto randomFileName  = StringBuf(TempString("jit_{}.c", io::TimeStamp::GetNow().toSafeString()));
            auto randomFileName  = StringBuf(TempString("jit_temp.c", io::TimeStamp::GetNow().toSafeString()));
            filePath.appendFile(randomFileName.c_str());
            return filePath;
        }

        static io::AbsolutePath GetPrologCodeFile()
        {
            return base::io::SystemPath(io::PathCategory::ExecutableDir).addFile("jit.h");
        }

        void JITGeneralC::printFunctionSignature(IFormatStream& f, const StubFunction* func)  const
        {
            if (func->returnTypeDecl != nullptr)
            {
                if (auto stub  = m_typeLib->resolveType(func->returnTypeDecl))
                {
                    f << stub->name;
                    f << " ";
                }
            }

            if (func->owner)
            {
                if (auto funcClass  = func->owner->asClass())
                {
                    if (auto stub  = m_typeLib->resolveType(funcClass))
                    {
                        f << stub->name;
                        f << "::";
                    }
                }
            }

            f << func->name;
            f << "(";

            bool firstArg = true;
            for (auto arg  : func->args)
            {
                if (!firstArg) f << ", ";
                firstArg = false;

                if (arg->flags.test(StubFlag::Ref))
                    f << "ref ";
                if (arg->flags.test(StubFlag::Out))
                    f << "out";

                if (auto stub  = m_typeLib->resolveType(arg->typeDecl))
                    f << stub->name;

                f << " ";
                f << arg->name;
            }

            f << ")";
        }

        void JITGeneralC::printFunctionBodies(base::IFormatStream &f) const
        {
            for (auto info  : m_exportedFunctions)
            {
                // skip functions that failed JITing
                if (info->code.empty())
                    continue;

                // in fast mode call we have direct access to arguments
                if (info->jitLocalName.empty())
                {
                    // we have general wrapepr only
                    f.appendf("void {}(void* context, void* stackFrame, struct FunctionCallingParams* params) {\n", info->jitName);
                    f << info->code;
                    f.append("}\n\n");
                }
                else
                {
                    // generate a direct call wrapper
                    f.appendf("void {}(void* context, void* stackFrame, void* resultPtr", info->jitLocalName);
                    for (uint32_t i=0; i<info->stub->args.size(); ++i)
                    {
                        f << ", ";

                        auto arg  = info->stub->args[i];

                        auto argType  = m_typeLib->resolveType(arg->typeDecl);
                        auto asPointer  = arg->flags.test(StubFlag::Ref) || arg->flags.test(StubFlag::Out);
                        if (asPointer)
                            f.appendf("{}* {}", argType->jitName, arg->name);
                        else
                            f.appendf("{} {}", argType->jitName, arg->name);
                    }
                    f << ") {\n";
                    f << info->code;
                    f.append("}\n\n");

                    // generate a general wrapper that forwards the call
                    f.appendf("void {}(void* context, void* stackFrame, struct FunctionCallingParams* params) {\n", info->jitName);
                    f.appendf("  {}(context, stackFrame, params ? params->_returnPtr : 0", info->jitLocalName);
                    for (uint32_t i=0; i<info->stub->args.size(); ++i)
                    {
                        auto arg  = info->stub->args[i];

                        f << ", ";

                        auto argType  = m_typeLib->resolveType(arg->typeDecl);
                        auto asPointer  = arg->flags.test(StubFlag::Ref) || arg->flags.test(StubFlag::Out);
                        if (!asPointer)
                            f << " *";

                        f.appendf("(({}*)params->_argPtr[{}])", argType->jitName, i);
                    }

                    f.append(");\n");
                    f.append("}\n\n");
                }
            }
        }

        void JITGeneralC::printFunctionExports(IFormatStream& f) const
        {
            for (auto info  : m_exportedFunctions)
            {
                if (!info->code.empty())
                {
                    if (info->jitClass)
                        f.appendf("DCL_CLASS_JIT(\"{}\", \"{}\", 0x{}ULL, {})", info->jitClass->name, info->functionName, info->codeHash, info->jitName);
                    else
                        f.appendf("DCL_GLOBAL_JIT(\"{}\", 0x{}ULL, {})", info->functionName, info->codeHash, info->jitName);

                    f << "  /* ";
                    printFunctionSignature(f, info->stub);
                    f << "  */\n";
                }
            }
        }

        io::AbsolutePath JITGeneralC::writeTempSourceFile()  const
        {
            StringBuilder f;

            // load generic prolog
            StringBuf prolog;
            io::LoadFileToString(GetPrologCodeFile(), prolog);
            f << prolog;

            // declare all struct types
            m_typeLib->printTypePrototypes(f);

            // dump constant declarations
            m_constCache->printConstVars(f);

            // print call forwarders for imported functions
            m_typeLib->printCallForwarderDeclarations(f);

            // print function code
            printFunctionBodies(f);

            // print call actual forwarders, we may choose to call local functions here
            m_typeLib->printCallForwarders(f);

            // emit initialization function
            f << "void _bindModuleToEngine(struct EngineToJIT* ei, struct JITInit* init) {\n";
            f << "EI = ei;\n";
            m_typeLib->printImports(f);
            m_constCache->printConstInit(f);
            printFunctionExports(f);
            f << "}\n";

            // save the file
            auto tempFilePath  = GetTempFilePath();
            if (!io::SaveFileFromString(tempFilePath, f.toString()))
                return io::AbsolutePath();
            return tempFilePath;
        }

        //--

    } // script
} // base
