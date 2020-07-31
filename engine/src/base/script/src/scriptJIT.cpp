/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptEnvironment.h"
#include "scriptCompiledProject.h"
#include "scriptJIT.h"

#include "base/containers/include/inplaceArray.h"
#include "base/object/include/rttiArrayType.h"
#include "base/object/include/rttiProperty.h"
#include "base/io/include/timestamp.h"
#include "base/io/include/ioSystem.h"

#if defined(PLATFORM_POSIX)
    #include <dlfcn.h>
#include <base/object/include/rttiFunctionPointer.h>

#elif defined(PLATFORM_WINDOWS)
    #include <Windows.h>
#endif

namespace base
{
    namespace script
    {
        //---

        base::ConfigProperty<StringID> cvJITCompilerClass("Script.JIT", "CompilerClass", "base::script::JITTCC"_id);
        base::ConfigProperty<bool> cvJITCompilerEmitExceptions("Script.JIT", "EmitExceptions", true);
        base::ConfigProperty<bool> cvJITCompilerEmitLines("Script.JIT", "EmitLines", false);
        base::ConfigProperty<bool> cvJITCompilerEmitSymbols("Script.JIT", "EmitSymbols", true);

        //---

        JITProject::JITProject(void* handle, const io::AbsolutePath& path)
            : m_path(path)
            , m_handle(handle)
            , m_bound(false)
        {}

        JITProject::~JITProject()
        {
            if (m_handle)
            {
#if defined(PLATFORM_POSIX)
                dlclose(data);
#elif defined(PLATFORM_WINDOWS)
                FreeLibrary((HMODULE)m_handle);
#endif
                m_handle = nullptr;
            }
        }

        // helper class used to bind stuff defined in the JIT module to the engine runtime
        class JITProjectBinder : public NoCopy
        {
        public:
            JITProjectBinder()
                : m_numErrors(0)
            {
            }

            //--

            struct ExportedFunction
            {
                const rtti::Function* m_func = nullptr;
                rtti::TFunctionJittedWrapperPtr m_ptr = nullptr;
                uint64_t m_codeHash = 0;
            };

            Array<Type> m_typeTable;
            Array<const rtti::Function*> m_functionImportTable;
            Array<ExportedFunction> m_functionExportTable;
            uint32_t m_numErrors;

            void bindInterface(jit::JITInit& init)
            {
                init.self = this;
                init._fnReportImportCounts = &ReportImportCounts;
                init._fnReportImportType = &ReportImportType;
                init._fnReportExportFunction = &ReportExportFunction;
                init._fnReportImportFunction = &ReportImportFunction;
                init._fnInitNameConst = &InitNameConst;
                init._fnInitStringConst = &InitStringConst;
                init._fnInitTypeConst = &InitTypeConst;
            }

        private:
            static void ReportImportCounts(void* self, int maxTypeId, int maxFuncId)
            {
                auto binder  = (JITProjectBinder*)self;

                binder->m_functionImportTable.clear();
                binder->m_functionImportTable.resizeWith(maxFuncId, nullptr);

                binder->m_typeTable.clear();
                binder->m_typeTable.resizeWith(maxTypeId, nullptr);
            }

            static void ReportImportType(void* self, int typeId, const char* name)
            {
                auto binder  = (JITProjectBinder*)self;

                if (typeId > binder->m_typeTable.lastValidIndex())
                {
                    TRACE_ERROR("JIT: out of bound type ID {} for type '{}'", typeId, name);
                    binder->m_numErrors += 1;
                    return;
                }

                auto typeName = StringID(name);
                auto type  = RTTI::GetInstance().findType(typeName);
                if (!type)
                {
                    TRACE_ERROR("JIT: Unrecognized type '{}' referenced as ID {}", typeName, typeId);
                    binder->m_numErrors += 1;
                    return;
                }

                binder->m_typeTable[typeId] = type;
                TRACE_INFO("JIT: Resolved {} as '{}'", typeId, typeName);
            }

            static void ReportImportFunction(void* self, int funcId, const char* className, const char* funcName, jit::TNativeFunctionPtr* nativeFunctionPtr)
            {
                auto binder  = (JITProjectBinder*)self;

                if (funcId > binder->m_functionImportTable.lastValidIndex())
                {
                    TRACE_ERROR("JIT: out of bound function ID {} for function '{}'", funcId, funcName);
                    binder->m_numErrors += 1;
                    return;
                }

                if (className)
                {
                    auto classType  = RTTI::GetInstance().findClass(StringID(className));
                    if (!classType)
                    {
                        TRACE_ERROR("JIT: Unrecognized class '{}' (needed for function '{}') when looking up function ID {}'", className, funcName, funcId);
                        binder->m_numErrors += 1;
                        return;
                    }

                    auto func  = classType->findFunction(StringID(funcName));
                    if (!func)
                    {
                        TRACE_ERROR("JIT: Unrecognized function '{}' in class '{}' when looking up function ID {}", funcName, className, funcId);
                        binder->m_numErrors += 1;
                        return;
                    }

                    binder->m_functionImportTable[funcId] = func;
                    TRACE_INFO("JIT: Resolved {} as class function '{}' in '{}'", funcId, funcName, className);

                    if (nativeFunctionPtr)
                    {
                        auto& nativePtr = func->nativeFunctionPointer();
                        if (!nativePtr)
                        {
                            TRACE_ERROR("JIT: Class '{}' function '{}' does not have a native function pointer (ID {})", className, funcName, funcId);
                            binder->m_numErrors += 1;
                        }
                        else if (nativePtr.flag != 2)
                        {
                            TRACE_ERROR("JIT: Class '{}' function '{}' uses unsupported calling convention {} (ID {})", className, funcName, nativePtr.flag, funcId);
                            binder->m_numErrors += 1;
                        }
                        else
                        {
                            *nativeFunctionPtr = nativePtr.ptr.f.func;
                        }
                    }
                }
                else
                {
                    auto func  = RTTI::GetInstance().findGlobalFunction(StringID(funcName));
                    if (!func)
                    {
                        TRACE_ERROR("JIT: Unrecognized global function '{}' when looking up function ID {}", funcName, funcId);
                        binder->m_numErrors += 1;
                        return;
                    }

                    binder->m_functionImportTable[funcId] = func;
                    TRACE_INFO("JIT: Resolved {} as global function '{}'", funcId, funcName);

                    if (nativeFunctionPtr)
                    {
                        auto& nativePtr = func->nativeFunctionPointer();
                        if (!nativePtr)
                        {
                            TRACE_ERROR("JIT: Global function '{}' does not have a native function pointer (ID {})", funcName, funcId);
                            binder->m_numErrors += 1;
                        }
                        else if (nativePtr.flag != 2)
                        {
                            TRACE_ERROR("JIT: Global function '{}' uses unsupported calling convention {} (ID {})", funcName, nativePtr.flag, funcId);
                            binder->m_numErrors += 1;
                        }
                        else
                        {
                            *nativeFunctionPtr = nativePtr.ptr.f.func;
                        }
                    }
                }
            }

            static void ReportExportFunction(void* self, const char* className, const char* funcName,  uint64_t codeHash, rtti::TFunctionJittedWrapperPtr funcPtr)
            {
                auto binder  = (JITProjectBinder*)self;

                const rtti::Function* func = nullptr;
                if (className)
                {
                    auto classType  = RTTI::GetInstance().findClass(StringID(className));
                    if (!classType)
                    {
                        TRACE_ERROR("JIT: Unrecognized class '{}' (needed for function '{}') when trying to define export function", className, funcName);
                        binder->m_numErrors += 1;
                        return;
                    }

                    func = classType->findFunction(StringID(funcName));
                    if (!func)
                    {
                        TRACE_ERROR("JIT: Unrecognized function '{}' in class '{}' when trying to define export function", funcName, className);
                        binder->m_numErrors += 1;
                        return;
                    }

                    TRACE_INFO("JIT: Resolved export function '{}' in '{}'", funcName, className);
                }
                else
                {
                    func = RTTI::GetInstance().findGlobalFunction(StringID(funcName));
                    if (!func)
                    {
                        TRACE_ERROR("JIT: Unrecognized global function '{}' when trying to define export function", funcName, className);
                        binder->m_numErrors += 1;
                        return;
                    }

                    TRACE_INFO("JIT: Resolved global export function '{}'", funcName);
                }

                if (func)
                {
                    auto &entry = binder->m_functionExportTable.emplaceBack();
                    entry.m_func = func;
                    entry.m_ptr = funcPtr;
                    entry.m_codeHash = codeHash;
                }
            }

            static void InitStringConst(void* self, void* str, const char* data)
            {
                *((StringBuf*)str) = StringBuf(data);
            }

            static void InitTypeConst(void* self, void* str, const char* data)
            {
                *((Type*)str) = RTTI::GetInstance().findType(StringID(data));
            }

            static void InitNameConst(void* self, void* str, const char* data)
            {
                *((StringID*)str) = StringID(data);
            }
        };

        void* JITProject::findFunction(const char* name) const
        {
#if defined(PLATFORM_POSIX)
            return dlsym(data, name);
#elif defined(PLATFORM_WINDOWS)
            return GetProcAddress((HMODULE)m_handle, name);
#else
            return nullptr;
#endif
        }

        bool JITProject::bind()
        {
            // already bound
            if (m_bound)
                return true;

            // find the binding function
            auto bindFunction = (jit::TInitFunc)findFunction("__bindModuleToEngine");
            if (!bindFunction)
            {
                bindFunction = (jit::TInitFunc)findFunction("_bindModuleToEngine");
                if (!bindFunction)
                {
                    bindFunction = (jit::TInitFunc) findFunction("bindModuleToEngine");
                    if (!bindFunction)
                    {
                        TRACE_ERROR("JIT module '{}' does not contain binding function, make sure it was compiled properly", absolutePath());
                        return false;
                    }
                }
            }

            // fill in engine interface
            m_interface.self = this;
            m_interface._fnLog = &InterfaceLog;
            m_interface._fnThrowException = &InterfaceThrowException;
            m_interface._fnTypeCtor = &InterfaceTypeCtor;
            m_interface._fnTypeDtor = &InterfaceTypeDtor;
            m_interface._fnTypeCompare = &InterfaceTypeCompare;
            m_interface._fnTypeCopy = &InterfaceTypeCopy;
            m_interface._fnCall = &InterfaceCall;
            m_interface._fnNew = &InterfaceNew;
            m_interface._fnCall = &InterfaceCall;
            m_interface._fnNew = &InterfaceNew;
            m_interface._fnWeakToBool = &InterfaceWeakToBool;
            m_interface._fnWeakToStrong = &InterfaceWeakToStrong;
            m_interface._fnStrongToWeak = &InterfaceStrongToWeak;
            m_interface._fnStrongFromPtr = &InterfaceStrongFromPtr;
            m_interface._fnEnumToName = &InterfaceEnumToName;
            m_interface._fnNameToEnum = &InterfaceNameToEnum;
            m_interface._fnDynamicStrongCast = &InterfaceDynamicStrongCast;
            m_interface._fnDynamicWeakCast = &InterfaceDynamicWeakCast;
            m_interface._fnMetaCast = &InterfaceMetaCast;
            m_interface._fnClassToName = &InterfaceClassToName;
            m_interface._fnClassToString = &InterfaceClassToString;

            // fill in the binding interface
            jit::JITInit init;
            JITProjectBinder binder;
            binder.bindInterface(init);

            // call the bind function
            (*bindFunction)(&m_interface, &init);

            // failed ?
            if (binder.m_numErrors != 0)
            {
                TRACE_ERROR("JIT module '{}' failed to bind with '{}' errors", absolutePath(), binder.m_numErrors);
                return false;
            }

            // transfer data
            TRACE_INFO("JIT module '{}' bound, {} imported types, {} imported functions, {} JIT functions", absolutePath(), binder.m_typeTable.size(), binder.m_functionImportTable.size(), binder.m_functionExportTable.size());
            m_functionTable = std::move(binder.m_functionImportTable);
            m_typeTable = std::move(binder.m_typeTable);

            // bind JIT code to function
            uint32_t numFailedFunctions = 0;
            for (auto& info : binder.m_functionExportTable)
                if (!const_cast<rtti::Function*>(info.m_func)->bindJITFunction(info.m_codeHash, info.m_ptr))
                    numFailedFunctions += 1;

            // report binding problems with functions
            if (numFailedFunctions > 0)
            {
                TRACE_INFO("JIT module '{}' has {} function(s) that failed to bind, performance drops may be possible", absolutePath(), numFailedFunctions);
            }

            // we are now bound
            m_bound = true;
            return true;
        }

        JITProjectPtr JITProject::Load(const io::AbsolutePath& path)
        {
#if defined(PLATFORM_POSIX)
            auto utf8Path = path.ansi_str();
            void* handle = dlopen(utf8Path.c_str(), RTLD_LOCAL | RTLD_NOW);
            if (!handle)
            {
                TRACE_ERROR("Failed to load dynamic library from '{}': {}", path.c_str(), dlerror());
                return nullptr;
            }

            return CreateSharedPtr<JITProject>(handle, path);
#elif defined(PLATFORM_WINDOWS)
            HANDLE hLib = LoadLibraryW(path.c_str());
            if (NULL == hLib)
            {
                TRACE_ERROR("Failed to load dynamic library from '{}'", path.c_str());
                return nullptr;
            }

            return CreateSharedPtr<JITProject>((void*)hLib, path);
#else
            TRACE_ERROR("Platform does not support loading dynamic library '{}'", path.c_str());
            return nullptr;
#endif
        }

        static StringBuf GetCoreScriptModuleName(StringView<char> path)
        {
            InplaceArray<StringView<char>, 10> parts;
            path.beforeLast(".").slice("/", false, parts);

            StringBuilder builder;
            for (auto& part : parts)
            {
                if (!builder.empty())
                    builder << "_";
                builder << part;
            }

            return builder.toString();
        }

        static io::AbsolutePath GetJITModulePath(StringView<char> path)
        {
            auto coreProjectModuleName = GetCoreScriptModuleName(path.beforeLast("."));

            auto nonce = io::TimeStamp::GetNow().toSafeString();
            auto compiledModuleFileName = StringBuf(TempString("{}_{}.jit", coreProjectModuleName, nonce));

            auto filePath = base::io::SystemPath(io::PathCategory::TempDir).addDir("jit");
            filePath.appendFile(compiledModuleFileName.uni_str());

            return filePath;
        }

        JITProjectPtr JITProject::Compile(const CompiledProjectPtr& project)
        {
            ScopeTimer timer;

            // find JIt class
            auto jitClass  = RTTI::GetInstance().findClass(cvJITCompilerClass.get());
            if (!jitClass)
            {
                TRACE_ERROR("JIT: Compiler class '{}' not found, unable to compile project '{}'", cvJITCompilerClass.get(), project->path());
                return nullptr;
            }

            // generate path where we will store the compiled module
            auto jitPath = GetJITModulePath(project->path());
            TRACE_INFO("JIT: Scripts '{}' will be JITed into '{}'", project->path(), jitPath);

            // setup JIT compiler settings
            IJITCompiler::Settings settings;
            settings.emitExceptions = cvJITCompilerEmitExceptions.get();
            settings.emitSymbols = cvJITCompilerEmitSymbols.get();
            settings.emitOriginalLines = cvJITCompilerEmitLines.get();

            // run compiler
            auto compiler = jitClass->create<IJITCompiler>();
            if (!compiler->compile(IJITNativeTypeInsight::GetCurrentTypes(), project, jitPath, settings))
            {
                TRACE_ERROR("JIT: Failed to JIT '{}'", project->path());
                return nullptr;
            }

            // try to load JITed module
            TRACE_INFO("Scripts '{}' were JITed in {}", project->path(), TimeInterval(timer.timeElapsed()));
            return Load(jitPath);
        }

        //---

        void JITProject::InterfaceLog(void* self, const char* txt)
        {
            fprintf(stderr, "ScriptJIT: %s\n", txt);
        }

        void JITProject::InterfaceThrowException(void* self, const rtti::IFunctionStackFrame* frame, const char* file, int line, const char* txt)
        {
            fprintf(stderr, "ScriptJIT: %s(%d): error: %s\n", file, line, txt);
        }

        void JITProject::InterfaceTypeCtor(void* self, int typeId, void* data)
        {
            auto project  = (JITProject*) self;
            auto type  = project->m_typeTable[typeId];
            type->construct(data);
        }

        void JITProject::InterfaceTypeDtor(void* self, int typeId, void* data)
        {
            auto project  = (JITProject*) self;
            auto type  = project->m_typeTable[typeId];
            type->destruct(data);
        }

        void JITProject::InterfaceTypeCopy(void* self, int typeId, void* dest, void* src)
        {
            auto project  = (JITProject*) self;
            auto type  = project->m_typeTable[typeId];
            type->copy(dest, src);
        }

        int JITProject::InterfaceTypeCompare(void* self, int typeId, void* a, void* b)
        {
            auto project  = (JITProject*) self;
            auto type  = project->m_typeTable[typeId];
            return type->compare(a, b);
        }

        void JITProject::InterfaceCall(void* self,  void* context, int funcId, int mode, const rtti::IFunctionStackFrame* parentFrame, rtti::FunctionCallingParams* params)
        {
            auto project  = (JITProject*) self;
            auto func  = project->m_functionTable[funcId];
            func->run(parentFrame, context, *params);
        }

        void JITProject::InterfaceNew(void* self, const rtti::IFunctionStackFrame* parentFrame, const ClassType* classPtr, ObjectPtr* strongPtr)
        {
            if (!classPtr || classPtr->empty())
            {
                parentFrame->throwException("Trying to new object from NULL class");
                return;
            }

            if (!classPtr->is(IObject::GetStaticClass()))
            {
                parentFrame->throwException(TempString("Trying to new object from class '{}' that is not an object class", classPtr->name()));
                return;
            }

            *strongPtr = classPtr->create<IObject>();
        }

        bool JITProject::InterfaceWeakToBool(void* self, ObjectWeakPtr* weakPtr)
        {
            return weakPtr && !weakPtr->empty() && !weakPtr->expired();
        }

        void JITProject::InterfaceWeakToStrong(void* self, ObjectWeakPtr* weakPtr, ObjectPtr* strongPtr)
        {
            *strongPtr = weakPtr->lock();
        }

        void JITProject::InterfaceStrongToWeak(void* self, ObjectPtr* strongPtr, ObjectWeakPtr* weakPtr)
        {
            *weakPtr = *strongPtr;
        }

        void JITProject::InterfaceStrongFromPtr(void* self, void* ptr, ObjectPtr* strongPtr)
        {
            *strongPtr = ptr ? ObjectPtr(AddRef((IObject*)ptr)) : ObjectPtr();
        }

        StringID JITProject::InterfaceEnumToName(void* self, int typeId, int64_t enumValue)
        {
            auto project  = (JITProject*) self;
            if (typeId < 0 || typeId >= project->m_typeTable.lastValidIndex())
                return StringID::EMPTY();

            auto type  = project->m_typeTable[typeId];
            if (!type || type->metaType() != rtti::MetaType::Enum)
                return StringID::EMPTY();

            auto enumType  = static_cast<const rtti::EnumType*>(type.ptr());

            StringID ret;
            enumType->findName(enumValue, ret);
            return ret;
        }

        int64_t JITProject::InterfaceNameToEnum(void* self, const rtti::IFunctionStackFrame* parentFrame, int typeId, StringID enumName)
        {
            auto project  = (JITProject*) self;
            if (typeId < 0 || typeId >= project->m_typeTable.lastValidIndex())
                return StringID::EMPTY();

            auto type  = project->m_typeTable[typeId];
            if (!type || type->metaType() != rtti::MetaType::Enum)
                return StringID::EMPTY();

            auto enumType  = static_cast<const rtti::EnumType*>(type.ptr());

            int64_t value = 0;
            if (!enumType->findValue(enumName, value))
                parentFrame->throwException(TempString("Enum '{}' has no option named '{}'", type->name(), enumName));

            return value;
        }

        void JITProject::InterfaceDynamicStrongCast(void* self, const ClassType* classPtr, ObjectPtr* inStrongPtr,  ObjectPtr* outStrongPtr)
        {
            auto targetClass = classPtr ? *classPtr : nullptr;
            auto ptr  = inStrongPtr->get();
            if (targetClass && ptr && ptr->cls()->is(targetClass))
                *outStrongPtr = *inStrongPtr;
            else
                outStrongPtr->reset();
        }

        void JITProject::InterfaceDynamicWeakCast(void* self, const ClassType* classPtr, ObjectWeakPtr* inWeakPtr, ObjectWeakPtr* outWeakPtr)
        {
            auto targetClass  = classPtr ? *classPtr : nullptr;
            auto ptr = inWeakPtr->lock();
            if (targetClass && ptr && ptr->cls()->is(targetClass))
                *outWeakPtr = *inWeakPtr;
            else
                inWeakPtr->reset();
        }

        void JITProject::InterfaceMetaCast(void* self, const ClassType* classPtr, ClassType* inClassPtr, ClassType* outClassPtr)
        {
            if (classPtr && *classPtr && *inClassPtr && inClassPtr->is(*classPtr))
                *outClassPtr = *inClassPtr;
            else
                *outClassPtr = ClassType();
        }

        StringID JITProject::InterfaceClassToName(void* self, const ClassType* classPtr)
        {
            return (classPtr || !classPtr->empty()) ? classPtr->name() : StringID::EMPTY();
        }

        void JITProject::InterfaceClassToString(void* self, const ClassType* classPtr, StringBuf* outString)
        {
            if (classPtr && !classPtr->empty())
                *outString = StringBuf(classPtr->name().view());
            else
                *outString = StringBuf::EMPTY();
        }

        //---

        namespace helper
        {
            class CurrentNativeTypeInsight : public IJITNativeTypeInsight
            {
            public:
                virtual TypeInfo typeInfo(StringID typeName) const override final
                {
                    TypeInfo ret;

                    auto type  = RTTI::GetInstance().findType(typeName);
                    if (type)
                    {
                        if (type->traits().scripted)
                        {
                            TRACE_WARNING("Trying to get native type traits for scripted type '{}'", typeName);
                        }
                        else
                        {
                            ret.metaType = type->traits().metaType;
                            ret.runtimeSize = type->traits().size;
                            ret.runtimeAlign = type->traits().alignment;
                            ret.simpleCopyCompare = type->traits().simpleCopyCompare;
                            ret.requiresConstructor = type->traits().requiresConstructor;
                            ret.requiresDestructor = type->traits().requiresDestructor;
                            ret.zeroInitializationConstructor = type->traits().initializedFromZeroMem;

                            if (type->metaType() == rtti::MetaType::Class)
                            {
                                auto classType = type.toClass();
                                ret.baseClassName = classType->baseClass() ? classType->baseClass()->name() : StringID();
                                ret.localMembers.reserve(classType->localProperties().size());

                                for (auto memberPtr  : classType->localProperties())
                                {
                                    auto& memberInfo = ret.localMembers.emplaceBack();
                                    memberInfo.name = memberPtr->name();
                                    memberInfo.typeName = memberPtr->type()->name();
                                    memberInfo.runtimeOffset = memberPtr->offset();
                                }
                            }
                            else if (type->metaType() == rtti::MetaType::Array)
                            {
                                auto arrayType = static_cast<const rtti::IArrayType*>(type.ptr());
                                ret.innerTypeName = arrayType->innerType()->name();

                                if (arrayType->arrayMetaType() == rtti::ArrayMetaType::Native)
                                    ret.staticArraySize = arrayType->arrayCapacity(nullptr);
                            }
                            else if (type->metaType() == rtti::MetaType::Enum)
                            {
                                auto enumType  = static_cast<const rtti::EnumType*>(type.ptr());

                                auto numOptions = enumType->options().size();
                                ret.options.reserve(numOptions);

                                for (uint32_t i=0; i<numOptions; ++i)
                                {
                                    auto& info = ret.options.emplaceBack();
                                    info.name = enumType->options()[i];
                                    info.value = enumType->values()[i];
                                }
                            }
                        }
                    }
                    else
                    {
                        TRACE_WARNING("Trying to get native type traits for unknown type '{}'", typeName);
                    }

                    return ret;
                }
            };
        }

        IJITNativeTypeInsight::~IJITNativeTypeInsight()
        {}

        const IJITNativeTypeInsight& IJITNativeTypeInsight::GetCurrentTypes()
        {
            static helper::CurrentNativeTypeInsight nativeTypes;
            return nativeTypes;
        }

        //---

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IJITCompiler);
        RTTI_END_TYPE();

        IJITCompiler::~IJITCompiler()
        {}

        //---


    } // script
} // base
