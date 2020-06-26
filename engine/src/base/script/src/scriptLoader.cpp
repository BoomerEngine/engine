/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptLoader.h"
#include "scriptPortableStubs.h"
#include "scriptPortableData.h"
#include "scriptTypeRegistry.h"
#include "scriptClass.h"
#include "scriptObject.h"

#include "base/object/include/rttiClassRefType.h"
#include "base/object/include/rttiArrayType.h"
#include "base/object/include/rttiHandleType.h"

namespace base
{
    namespace script
    {

        //--

        Loader::Loader(TypeRegistry& registry)
            : m_typeRegistry(registry)
        {}

        Loader::~Loader()
        {
            m_allSymbols.clearPtr();
        }

        void Loader::linkModule(const PortableData& data)
        {
            auto& stubs = data.allStubs();
            for (auto stub  : stubs)
            {
                auto import = stub->flags.test(StubFlag::Import) || stub->flags.test(StubFlag::ImportDependency);

                if (auto typeDecl  = stub->asTypeDecl())
                {
                    m_typeMap[typeDecl] = m_allTypeDecls.size();
                    auto& info = m_allTypeDecls.emplaceBack();
                    info.m_stub = typeDecl;
                }
                else if (auto classStub  = stub->asClass())
                {
                    createSymbol(classStub->fullName(), classStub, import);
                }
                else if (auto enumStub  = stub->asEnum())
                {
                    createSymbol(enumStub->fullName(), enumStub, import);
                }
                else if (auto funcStub  = stub->asFunction())
                {
                    createSymbol(funcStub->fullName(), funcStub, import);
                }
                else if (auto propStub  = stub->asProperty())
                {
                    createSymbol(propStub->fullName(), propStub, import);
                }
            }
        }

        Loader::Symbol* Loader::createSymbol(StringView<char> name, const Stub* stub, bool import)
        {
            ASSERT(!name.empty());

            // create symbol wraper
            Symbol* symbol = nullptr;
            if (!m_symbolNamedMap.find(name, symbol))
            {
                // create symbol
                symbol = MemNewPool(POOL_SCRIPTS, Symbol);
                symbol->m_fullName = name;
                symbol->m_stubType = stub->stubType;

                m_symbolNamedMap[name] = symbol;
                m_allSymbols.pushBack(symbol);

                // add to lists
                if (stub->stubType == StubType::Class)
                    m_allClasses.pushBack(symbol);
                else if (stub->stubType == StubType::Function)
                    m_allFunc.pushBack(symbol);
                else if (stub->stubType == StubType::Enum)
                    m_allEnums.pushBack(symbol);
                else if (stub->stubType == StubType::Property)
                    m_allProps.pushBack(symbol);
                else
                {
                    ASSERT(!"Invalid stub type");
                }
            }
            // check compatibility, in very rare cases a struct may change to enum, etc
            else if (symbol->m_stubType != stub->stubType)
            {
                TRACE_ERROR("{}: error: Symbol '{}' was previously as something else at '{}'", stub->location, name, symbol->m_exportStub->location);
                return nullptr;
            }

            // link export/import sides
            if (import)
            {
                //TRACE_INFO("{}: imported as '{}'", stub->location, name);
                symbol->m_importStubs.pushBack(stub);
                m_numImports += 1;
            }
            else if (symbol->m_exportStub != nullptr)
            {
                TRACE_ERROR("{}: error: Symbol '{}' already exported from '{}'", stub->location, name, symbol->m_exportStub->location);
                symbol->m_importStubs.pushBack(stub);
                m_numImports += 1;
            }
            else
            {
                TRACE_INFO("{}: exported as '{}'", stub->location, name);
                symbol->m_exportStub = stub;
                m_numExports += 1;
            }

            m_symbolStubMap[stub] = symbol;
            return symbol;
        }

        bool Loader::validate()
        {
            // stats
            TRACE_INFO("Script loaded has {} symbols ({} exports, {} imports)", m_symbolNamedMap.size(), m_numExports, m_numImports);
            TRACE_INFO("Found {} type declarations to resolve", m_allTypeDecls.size());
            TRACE_INFO("Found {} properties to resolve", m_allProps.size());
            TRACE_INFO("Discovered {} overall class types", m_allClasses.size());
            TRACE_INFO("Discovered {} overall enum types", m_allEnums.size());
            TRACE_INFO("Discovered {} overall functions", m_allFunc.size());

            // make sure all class functions are properly linked to class symbols
            if (!findParentSymbols())
                return false;

            // make sure that all imported/exported symbols have exactly the same definition
            if (!matchImportExports())
                return false;

            // make sure that all symbols that are nowhere to be found in the scripts (no export stub) are found in the engine
            // NOTE: this also validates the content of native classes, enums and functions with the declarations we had in the script stubs
            if (!resolveEngineImports())
                return false;

            // double check that all symbols that we are using are resolved
            if (!ensureSymbolsResolvable())
                return false;

            // we should be fine
            return true;
        }

        bool Loader::findParentSymbols()
        {
            bool valid = true;

            for (auto func  : m_allFunc)
            {
                if (auto ownerStub  = func->anyStub()->owner->asClass())
                {
                    auto className = StringID(ownerStub->fullName());
                    if (!m_symbolNamedMap.find(className, func->m_classOwner))
                    {
                        TRACE_ERROR("{}: error: unable to find symbol declaration for class '{}' in which the function was declared", func->anyStub()->location, className);
                        valid = false;
                    }
                }
            }

            for (auto prop  : m_allProps)
            {
                if (auto ownerStub  = prop->anyStub()->owner->asClass())
                {
                    auto className = StringID(ownerStub->fullName());
                    if (!m_symbolNamedMap.find(className, prop->m_classOwner))
                    {
                        TRACE_ERROR("{}: error: unable to find symbol declaration for class '{}' in which the property was declared", prop->anyStub()->location, className);
                        valid = false;
                    }
                }
            }

            return valid;
        }

        bool Loader::matchImportExports()
        {
            bool valid = true;

            for (auto symbol  : m_allSymbols)
            {
                // ideally compare the the exported symbol vs. the imports
                auto refStub  = symbol->m_exportStub;
                if (!refStub)
                    refStub = symbol->m_importStubs.front();

                // compare
                for (auto otherStub  : symbol->m_importStubs)
                {
                    if (otherStub != refStub)
                    {
                        if (!MatchStub(refStub, otherStub))
                        {
                            TRACE_ERROR("{}: error: Mismatched definition of '{}', see previous definition at {}", otherStub->location, symbol->m_fullName, refStub->location);

                            if (!MatchStub(refStub, otherStub))
                                valid = false;
                            break;
                        }
                    }
                }
            }

            return true;
        }

        bool Loader::checkAliasingWithEngine(Symbol* symbol)
        {
            ASSERT(symbol->m_exportStub != nullptr);

            if (symbol->m_stubType == StubType::Class)
            {
                auto engineClass  = RTTI::GetInstance().findClass(symbol->m_fullName);
                if (engineClass != nullptr && !engineClass->scripted())
                {
                    TRACE_ERROR("{}: error: Class '{}' is already defined as native engine class and cannot be redeclared in scripts this way", symbol->m_exportStub->location, symbol->m_fullName);
                    return false;
                }
            }
            else if (symbol->m_stubType == StubType::Enum)
            {
                auto engineEnum  = RTTI::GetInstance().findEnum(symbol->m_fullName);
                if (engineEnum != nullptr && !engineEnum->scripted())
                {
                    TRACE_ERROR("{}: error: Enum '{}' is already defined as native engine enum and cannot be redeclared in scripts this way", symbol->m_exportStub->location, symbol->m_fullName);
                    return false;
                }
            }
            else if (symbol->m_stubType == StubType::Function && symbol->m_classOwner)
            {
                if (auto engineClass  = RTTI::GetInstance().findClass(symbol->m_classOwner->m_fullName))
                {
                    auto funcName = symbol->m_exportStub->name;
                    auto func  = engineClass->findFunctionNoCache(funcName);
                    if (func != nullptr && !func->scripted())
                    {
                        if (MatchFunctionSignature(symbol->m_exportStub->asFunction(), func))
                        {
                            TRACE_WARNING("{}: error: Class function '{}' was moved from scripts to native code, ignoring script definition", symbol->m_exportStub->location, symbol->m_fullName);
                            symbol->m_importStubs.pushBack(symbol->m_exportStub);
                            symbol->m_exportStub = nullptr;
                        }
                        else
                        {
                            TRACE_ERROR("{}: error: Class function '{}' is already defined as native function and cannot be redeclared in scripts this way", symbol->m_exportStub->location, symbol->m_fullName);
                            return false;
                        }
                    }
                }
            }
            else if (symbol->m_stubType == StubType::Function && !symbol->m_classOwner)
            {
                auto engineFunction  = RTTI::GetInstance().findGlobalFunction(symbol->m_fullName);
                if (engineFunction != nullptr && !engineFunction->scripted())
                {
                    if (MatchFunctionSignature(symbol->m_exportStub->asFunction(), engineFunction))
                    {
                        TRACE_WARNING("{}: error: Global function '{}' was moved from scripts to native code, ignoring script definition", symbol->m_exportStub->location, symbol->m_fullName);
                        symbol->m_importStubs.pushBack(symbol->m_exportStub);
                        symbol->m_exportStub = nullptr;
                    }
                    else
                    {
                        TRACE_ERROR("{}: error: Global function '{}' is already defined as native function and is not compatible", symbol->m_exportStub->location, symbol->m_fullName);
                        return false;
                    }
                }
            }

            return true;
        }

        bool Loader::ensureSymbolsResolvable()
        {
            bool valid = true;

            HashSet<StringID> failedEngineTypes;

            for (auto& type : m_allTypeDecls)
            {
                auto typeDecl  = type.m_stub;

                switch (typeDecl->metaType)
                {
                    case StubTypeType::Engine:
                    {
                        auto engineType  = RTTI::GetInstance().findType(typeDecl->name);
                        if (!engineType)
                        {
                            if (failedEngineTypes.insert(typeDecl->name))
                            {
                                TRACE_ERROR("{}: error: Engine type '{}' not found", typeDecl->location, typeDecl->name);
                                valid = false;
                            }
                        }
                        break;
                    }

                    case StubTypeType::Simple:
                    case StubTypeType::WeakPtrType:
                    case StubTypeType::ClassType:
                    case StubTypeType::PtrType:
                    {
                        Symbol* symbol = nullptr;
                        if (typeDecl->referencedType)
                            m_symbolStubMap.find(typeDecl->referencedType->resolvedStub, symbol);

                        if (!symbol)
                        {
                            TRACE_ERROR("{}: error: Using stub that was not reported as import not export", typeDecl->location);
                            valid = false;
                        }
                        break;
                    }

                    case StubTypeType::DynamicArrayType:
                    case StubTypeType::StaticArrayType:
                    {
                        int typeIndex = INDEX_NONE;
                        m_typeMap.find(typeDecl->innerType, typeIndex);

                        if (INDEX_NONE == typeIndex)
                        {
                            TRACE_ERROR("{}: error: Using inner type was not mapped previously", typeDecl->location);
                            valid = false;
                        }
                        break;
                    }
                }
            }

            for (auto symbol  : m_allSymbols)
            {
                if (symbol->m_exportStub == nullptr)
                {
                    if (!symbol->resolved() && !symbol->anyStub()->flags.test(StubFlag::Opcode))
                    {
                        TRACE_ERROR("{}: error: Strange error, in the end we failed to resolve this symbol ({})", symbol->anyStub()->location, symbol->m_fullName);
                        valid = false;
                    }
                }
                else if (symbol->m_stubType == StubType::Class)
                {
                    // TODO: check that class can be created

                    if (!symbol->m_exportStub->flags.test(StubFlag::Struct))
                    {
                        auto nativeClassBase  = FindNativeClassBase(symbol->m_exportStub->asClass());
                        if (!nativeClassBase)
                            valid = false;
                    }
                }
                else if (symbol->m_stubType == StubType::Enum)
                {
                    // TODO: check that enum can be created
                }
                else if (symbol->m_stubType == StubType::Property)
                {
                    // TODO: check that property can be created
                }
                else if (symbol->m_stubType == StubType::Function)
                {
                    // TODO: check that function can be created
                }
            }

            return valid;
        }

        bool Loader::resolveEngineImports()
        {
            bool valid = true;

            for (auto symbol  : m_allSymbols)
            {
                // this symbol is exported by scripts, make sure it's not aliased with something that engine exports already
                // this may happen if we for example had a slow function in scripts that we moved to native code for performance reasons
                if (symbol->m_exportStub != nullptr)
                {
                    valid &= checkAliasingWithEngine(symbol);
                    continue;
                }
            }

            for (auto symbol  : m_allClasses)
            {
                // skip exported symbols
                if (symbol->m_exportStub != nullptr)
                    continue;

                // missing engine import name
                auto classStub  = symbol->anyStub()->asClass();
                if (classStub->engineImportName.empty())
                {
                    TRACE_ERROR("{}: error: Class '{}' has valid engine class name specified", classStub->location, symbol->m_fullName);
                    valid = false;
                    continue;
                }

                // find engine class
                auto engineClass  = RTTI::GetInstance().findClass(classStub->engineImportName);
                if (engineClass == nullptr)
                {
                    TRACE_ERROR("{}: error: Class '{}' references missing engine class '{}'", classStub->location, symbol->m_fullName, classStub->engineImportName);
                    valid = false;
                    continue;
                }

                // resolve
                symbol->m_resolved.m_class = engineClass.ptr();
            }

            for (auto symbol  : m_allEnums)
            {
                // skip exported symbols
                if (symbol->m_exportStub != nullptr)
                    continue;

                // missing engine import name
                auto enumStub  = symbol->anyStub()->asEnum();
                if (enumStub->engineImportName.empty())
                {
                    TRACE_ERROR("{}: error: Enum '{}' has valid engine class name specified", enumStub->location, symbol->m_fullName);
                    valid = false;
                    continue;
                }

                // find engine enum
                auto engineEnum   = RTTI::GetInstance().findEnum(enumStub->engineImportName);
                if (engineEnum == nullptr)
                {
                    TRACE_ERROR("{}: error: Enum '{}' references missing engine type '{}'", enumStub->location, symbol->m_fullName, enumStub->engineImportName);
                    valid = false;
                    continue;
                }

                // check that all of the declared enum options are found in the enum
                for (auto optionStub  : enumStub->options)
                {
                    auto optionName = optionStub->name;

                    int64_t value = 0;
                    if (!engineEnum->findValue(optionName, value))
                    {
                        TRACE_ERROR("{}: error: Missing enum option '{}' in enum '{}' imported from '{}'", optionStub->location, optionName, enumStub->fullName(), enumStub->engineImportName);
                        valid = false;
                        continue;
                    }

                    if (optionStub->hasUserAssignedValue && optionStub->assignedValue != value)
                    {
                        TRACE_WARNING("{}: error: Enum option '{}' in enum '{}' (imported from '{}') has different value in engine ({}) than one predefined in scripts ({})", optionStub->location, optionName, enumStub->fullName(), enumStub->engineImportName, value, optionStub->assignedValue);
                        continue;
                    }
                }

                // resolve
                symbol->m_resolved.m_enum = engineEnum;
            }

            for (auto symbol  : m_allProps)
            {
                // skip exported symbols
                if (symbol->m_exportStub != nullptr)
                    continue;

                // class was not resolved, skip
                ASSERT(symbol->m_classOwner != nullptr);
                if (!symbol->m_classOwner->m_resolved.m_class)
                    continue;

                // find the property in the class
                auto name = symbol->anyStub()->name;
                auto engineClass  = symbol->m_classOwner->m_resolved.m_class;
                auto engineProp  = engineClass->findProperty(name);
                if (!engineProp)
                {
                    TRACE_ERROR("{}: error: Property '{}' is not defined in class '{}' (imported from '{}')", symbol->anyStub()->location, name, symbol->m_classOwner->m_fullName, engineClass->name());
                    valid = false;
                    continue;
                }

                // validate symbol type
                if (!MatchPropertyType(engineProp->type(), symbol->anyStub()->asProperty()->typeDecl))
                {
                    TRACE_ERROR("{}: error: Type of property '{}' in class '{}' (imported from '{}') is not compatible with engine type ({})", symbol->anyStub()->location, name, symbol->m_classOwner->m_fullName, engineClass->name(), engineProp->type()->name());
                    valid = false;
                    continue;
                }

                // resolve
                symbol->m_resolved.m_property = engineProp;
            }

            for (auto symbol  : m_allFunc)
            {
                // skip exported symbols
                if (symbol->m_exportStub != nullptr || symbol->m_classOwner)
                    continue;

                // if function is implemented as an opcode we don't have to have it defined
                auto funcStub  = symbol->anyStub()->asFunction();
                if (funcStub->opcodeName)
                    continue;

                // find global function
                auto engineFunc   = RTTI::GetInstance().findGlobalFunction(symbol->m_fullName);
                if (engineFunc == nullptr)
                {
                    TRACE_ERROR("{}: error: Missing global function '{}'", funcStub->location, symbol->m_fullName);
                    valid = false;
                    continue;
                }

                // validate function semantic
                if (!MatchFunctionSignature(funcStub, engineFunc))
                {
                    TRACE_ERROR("{}: error: Global function '{}' has different engine semantic than the one declared in scripts", funcStub->location, symbol->m_fullName);
                    valid = false;
                    continue;
                }

                symbol->m_resolved.m_function = engineFunc;
            }

            for (auto symbol  : m_allFunc)
            {
                // skip exported symbols
                if (symbol->m_exportStub != nullptr || !symbol->m_classOwner)
                    continue;

                // class was not resolved, skip
                if (!symbol->m_classOwner->m_resolved.m_class)
                    continue;

                // find global function
                auto funcStub  = symbol->anyStub()->asFunction();
                auto engineClass  = symbol->m_classOwner->m_resolved.m_class;
                auto engineFunc  = engineClass->findFunction(funcStub->name);
                if (engineFunc == nullptr)
                {
                    TRACE_ERROR("{}: error: Missing function '{}' from class '{}' (imported from '{}')", funcStub->location, symbol->m_fullName, funcStub->owner->fullName(), engineClass->name());
                    valid = false;
                    continue;
                }

                // validate function semantic
                if (!MatchFunctionSignature(funcStub, engineFunc))
                {
                    TRACE_ERROR("{}: error: Class function '{}' has different engine semantic than the one declared in scripts", funcStub->location, symbol->m_fullName);
                    valid = false;
                    continue;
                }

                symbol->m_resolved.m_function = engineFunc;
            }

            return valid;
        }

        //--

        bool Loader::MatchTypeRef(const StubTypeRef* a, const StubTypeRef* b)
        {
            if (!a || !b || !a->resolvedStub || !b->resolvedStub)
                return false;

            if (a->resolvedStub->stubType != b->resolvedStub->stubType)
                return false;

            return a->resolvedStub->fullName() == b->resolvedStub->fullName();
        }

        bool Loader::MatchTypeDecl(const StubTypeDecl* a, const StubTypeDecl* b)
        {
            if (!a || !b)
                return a == nullptr && b == nullptr;

            if (a->metaType != b->metaType)
                return false;

            switch (a->metaType)
            {
                case StubTypeType::ClassType:
                case StubTypeType::PtrType:
                case StubTypeType::WeakPtrType:
                case StubTypeType::Simple:
                    return MatchTypeRef(a->referencedType, b->referencedType);

                case StubTypeType::Engine:
                    return a->name == b->name;

                case StubTypeType::DynamicArrayType:
                    return MatchTypeDecl(a->innerType, b->innerType);

                case StubTypeType::StaticArrayType:
                    if (a->arraySize != b->arraySize)
                        return false;
                    return MatchTypeDecl(a->innerType, b->innerType);
            }

            return true;
        }

        bool Loader::MatchFunctionArg(const StubFunctionArg* a, const StubFunctionArg* b)
        {
            if (!a || !b)
                return false;

            if (a->flags.test(StubFlag::Ref) != b->flags.test(StubFlag::Ref))
                return false;

            if (a->flags.test(StubFlag::Out) != b->flags.test(StubFlag::Out))
                return false;

            if (a->flags.test(StubFlag::Explicit) != b->flags.test(StubFlag::Explicit))
                return false;

            return MatchTypeDecl(a->typeDecl, b->typeDecl);
        }

        bool Loader::MatchFunctionDecl(const StubFunction* a, const StubFunction* b)
        {
            if (!a || !b)
                return false;

            if (!MatchTypeDecl(a->returnTypeDecl, b->returnTypeDecl))
                return false;

            if (a->args.size() != b->args.size())
                return false;

            if (a->flags.test(StubFlag::Static) != b->flags.test(StubFlag::Static))
                return false;
            if (a->flags.test(StubFlag::Operator) != b->flags.test(StubFlag::Operator))
                return false;
            if (a->flags.test(StubFlag::Cast) != b->flags.test(StubFlag::Cast))
                return false;
            if (a->flags.test(StubFlag::Final) != b->flags.test(StubFlag::Final))
                return false;
            if (a->opcodeName != b->opcodeName)
                return false;

            for (uint32_t i=0; i<a->args.size(); ++i)
                if (!MatchFunctionArg(a->args[i], b->args[i]))
                    return false;

            return true;
        }

        bool Loader::MatchEnumDecl(const StubEnum* a, const StubEnum* b)
        {
            if (!a || !b)
                return false;

            if (a->engineImportName != b->engineImportName)
                return false;

            // TODO
            return true;
        }

        bool Loader::MatchPropertyDecl(const StubProperty* a, const StubProperty* b)
        {
            if (!a || !b)
                return false;

            if (!MatchTypeDecl(a->typeDecl, b->typeDecl))
                return false;

            // TODO: match default value ? it's encoded in the code any way

            if (a->flags.test(StubFlag::Import) != b->flags.test(StubFlag::Import))
                return false;
            if (a->flags.test(StubFlag::Const) != b->flags.test(StubFlag::Const))
                return false;

            return true;
        }

        bool Loader::MatchClassRef(const StubClass* a, const StubClass* b)
        {
            if (!a || !b)
                return (a == nullptr) && (b == nullptr);

            if (a->fullName() != b->fullName())
                return false;

            return true;
        }

        bool Loader::MatchClassStub(const Stub* a, const Stub* b)
        {
            ASSERT(a != nullptr);

            if (!b)
                return false;

            if (a->stubType != b->stubType)
                return false;

            if (a->stubType == StubType::Function)
            {
                if (!MatchFunctionDecl(a->asFunction(), b->asFunction()))
                    return false;
            }
            else if (a->stubType == StubType::Property)
            {
                if (!MatchPropertyDecl(a->asProperty(), b->asProperty()))
                    return false;
            }

            return true;
        }

        bool Loader::MatchStub(const Stub* a, const Stub* b)
        {
            if (!a || !b)
                return false;

            if (a->stubType != b->stubType)
                return false;

            if (a->stubType == StubType::Function)
            {
                if (!MatchFunctionDecl(a->asFunction(), b->asFunction()))
                    return false;
            }
            else if (a->stubType == StubType::Class)
            {
                if (!MatchClassDecl(a->asClass(), b->asClass()))
                    return false;
            }
            else if (a->stubType == StubType::Enum)
            {
                if (!MatchEnumDecl(a->asEnum(), b->asEnum()))
                    return false;
            }

            return true;
        }

        bool Loader::MatchClassDecl(const StubClass* a, const StubClass* b)
        {
            if (!a || !b)
                return false;

            if (a->flags.test(StubFlag::Native) != b->flags.test(StubFlag::Native))
                return false;

            if (a->engineImportName != b->engineImportName)
                return false;
            if (a->baseClassName != b->baseClassName)
                return false;
            if (a->parentClassName != b->parentClassName)
                return false;

            for (auto stubA  : a->stubs)
            {
                auto stubB  = b->findStub(stubA->name);
                if (stubB && !MatchClassStub(stubA, stubB))
                    return false;
            }

            for (auto stubB  : b->stubs)
            {
                auto stubA  = a->findStub(stubB->name);
                if (stubA && !MatchClassStub(stubB, stubA))
                    return false;
            }

            return true;
        }

        //--

        bool Loader::IsStructClass(ClassType classType)
        {
            return classType->baseClass() == nullptr;
        }

        bool Loader::MatchPropertyType(Type refType, const Stub* resolvedStub)
        {
            if (!resolvedStub)
                return false;

            if (auto resolvedClass  = resolvedStub->asClass())
            {
                if (refType->metaType() != rtti::MetaType::Class)
                    return false;

                if (refType->name() == resolvedStub->fullName())
                    return true;

                if (refType->name() == resolvedClass->engineImportName)
					return true;
            }
            else if (auto resolvedEnum  = resolvedStub->asEnum())
            {
                if (refType->metaType() != rtti::MetaType::Enum)
                    return false;

                return (refType->name() == resolvedEnum->engineImportName);
            }

            return false;
        }

        bool Loader::MatchPropertyType(Type refType, const StubTypeRef* decl)
        {
            if (!decl)
                return false;

            return MatchPropertyType(refType, decl->resolvedStub);
        }

        bool Loader::MatchPropertyType(Type refType, const StubTypeDecl* decl)
        {
            if (!decl)
                return false;

            if (decl->metaType == StubTypeType::Simple)
            {
                return MatchPropertyType(refType, decl->referencedType);
            }
            else if (decl->metaType == StubTypeType::Engine)
            {
                return refType->name() == decl->name;
            }
            else if (decl->metaType == StubTypeType::ClassType)
            {
                if (refType->metaType() != rtti::MetaType::ClassRef)
                    return false;

                auto classRefType  = static_cast<const rtti::ClassRefType*>(refType.ptr());
                return MatchPropertyType(classRefType->baseClass(), decl->referencedType);
            }
            else if (decl->metaType == StubTypeType::PtrType)
            {
                if (refType->metaType() != rtti::MetaType::StrongHandle)
                    return false;

                auto ptrType  = static_cast<const rtti::IHandleType*>(refType.ptr());
                return MatchPropertyType(ptrType->pointedClass(), decl->referencedType);
            }
            else if (decl->metaType == StubTypeType::WeakPtrType)
            {
                if (refType->metaType() != rtti::MetaType::WeakHandle)
                    return false;

                auto ptrType  = static_cast<const rtti::IHandleType*>(refType.ptr());
                return MatchPropertyType(ptrType->pointedClass(), decl->referencedType);
            }
            else if (decl->metaType == StubTypeType::DynamicArrayType)
            {
                if (refType->metaType() != rtti::MetaType::Array)
                    return false;

                auto ptrType  = static_cast<const rtti::IArrayType*>(refType.ptr());
                if (ptrType->arrayMetaType() != rtti::ArrayMetaType::Dynamic)
                    return false;

                return MatchPropertyType(ptrType->innerType(), decl->innerType);
            }
            else if (decl->metaType == StubTypeType::StaticArrayType)
            {
                if (refType->metaType() != rtti::MetaType::Array)
                    return false;

                auto ptrType  = static_cast<const rtti::IArrayType*>(refType.ptr());
                if (ptrType->arrayMetaType() != rtti::ArrayMetaType::Native)
                    return false;

                if (ptrType->arrayCapacity(nullptr) != decl->arraySize)
                    return false;

                return MatchPropertyType(ptrType->innerType(), decl->innerType);
            }

            return false;
        }

        bool Loader::MatchFunctionSignature(const StubFunction* stubFunc, const rtti::Function* engineFunc)
        {
            // check return type
            if (stubFunc->returnTypeDecl == nullptr && engineFunc->returnType().m_type != nullptr)
            {
                TRACE_ERROR("{}: error: Imported Function '{}' should return a value of type '{}'", stubFunc->location, stubFunc->name, engineFunc->returnType().m_type->name());
                return false;
            }
            else if (stubFunc->returnTypeDecl != nullptr && engineFunc->returnType().m_type == nullptr)
            {
                TRACE_ERROR("{}: error: Imported Function '{}' should not return any value", stubFunc->location, stubFunc->name);
                return false;
            }
            else if (stubFunc->returnTypeDecl != nullptr)
            {
                if (!MatchPropertyType(engineFunc->returnType().m_type, stubFunc->returnTypeDecl))
                {
                    TRACE_ERROR("{}: error: Imported Function '{}' used different return type '{}' than declared type '{}'", stubFunc->location, stubFunc->name, engineFunc->returnType().m_type->name(), stubFunc->returnTypeDecl->fullName());
                    return false;
                }
            }

            // check argument count
            if (stubFunc->args.size() != engineFunc->numParams())
            {
                TRACE_ERROR("{}: error: Imported Function '{}' has actually different number of arguments ({}) than declared ({})", stubFunc->location, stubFunc->name, engineFunc->numParams(), stubFunc->args.size());
                return false;
            }

            // check arguments
            for (uint32_t i=0; i<engineFunc->numParams(); ++i)
            {
                auto& engineParam = engineFunc->params()[i];
                auto stubParam  = stubFunc->args[i];

                if (!MatchPropertyType(engineParam.m_type, stubParam->typeDecl))
                {
                    TRACE_ERROR("{}: error: Parameter '{}' in imported function '{}' has different type ({}) than declared '{}'", stubFunc->location, stubParam->name, stubFunc->name, engineParam.m_type->name(), stubParam->typeDecl->fullName());
                    return false;
                }

                if (stubParam->flags.test(StubFlag::Out))
                {
                    if (!engineParam.m_flags.test(rtti::FunctionParamFlag::Ref))
                    {
                        TRACE_ERROR("{}: error: Output parameter '{}' in imported function '{}' is not actually declared as output in the engine", stubFunc->location, stubParam->name, stubFunc->name);
                        return false;
                    }

                    if (engineParam.m_flags.test(rtti::FunctionParamFlag::Const))
                    {
                        TRACE_ERROR("{}: error: Output parameter '{}' in imported function '{}' is declared as contant in the engine", stubFunc->location, stubParam->name, stubFunc->name);
                        return false;
                    }
                }
                else if (stubParam->flags.test(StubFlag::Ref))
                {
                    if (!engineParam.m_flags.test(rtti::FunctionParamFlag::Ref))
                    {
                        TRACE_WARNING("{}: error: Parameter '{}' in engine function '{}' is not passed by reference in C++ and will be copied on call", stubFunc->location, stubParam->name, stubFunc->name);
                    }
                }
                else if (!stubParam->flags.test(StubFlag::Ref) && !stubParam->flags.test(StubFlag::Out))
                {
                    if (engineParam.m_flags.test(rtti::FunctionParamFlag::Ref))
                    {
                        TRACE_WARNING("{}: error: Parameter '{}' in imported function '{}' should be passed by value in C++ but is passed by reference", stubFunc->location, stubParam->name, stubFunc->name);
                    }
                }
            }

            return  true;
        }

        ClassType Loader::FindNativeClassBase(const StubClass* scriptedClass)
        {
            if (scriptedClass->flags.test(StubFlag::Struct))
                return nullptr;

            auto checkClass  = scriptedClass;
            while (checkClass)
            {
                if (!checkClass->engineImportName.empty())
                {
                    auto engineClass = RTTI::GetInstance().findClass(checkClass->engineImportName);
                    if (!engineClass)
                    {
                        TRACE_ERROR("{}: error: Scripted class's '{}' base class '{}' has missing native class implementation '{}'", scriptedClass->fullName(), checkClass->fullName(), checkClass->engineImportName);
                        return nullptr;
                    }
                    else if (!engineClass->is(ScriptedObject::GetStaticClass()))
                    {
                        TRACE_ERROR("{}: error: Scripted class's '{}' base class '{}' using native class implementation '{}' does not derive from ScriptObject", scriptedClass->fullName(), checkClass->fullName(), checkClass->engineImportName);
                        return nullptr;
                    }
                    else if (engineClass->isAbstract())
                    {
                        TRACE_ERROR("{}: error: Scripted class's '{}' base class '{}' using native class implementation '{}' is abstract and has no native constructor", scriptedClass->fullName(), checkClass->fullName(), checkClass->engineImportName);
                        return nullptr;
                    }

                    return engineClass;
                }

                checkClass = checkClass->baseClass;
            }

            TRACE_ERROR("{}: error: Scripted class's '{}' has no valid native class in it's class chain", scriptedClass->fullName());
            return nullptr;
        }

        //--

        Type Loader::createType(const StubTypeRef* typeRef)
        {
            ASSERT(typeRef);
            ASSERT(typeRef->resolvedStub);

            if (auto classType  = typeRef->resolvedStub->asClass())
            {
                auto typeName = classType->engineImportName ? classType->engineImportName : StringID(classType->fullName());
                auto rttiType = RTTI::GetInstance().findType(typeName);
                ASSERT(rttiType);
                return rttiType;
            }
            else if (auto enumType  = typeRef->resolvedStub->asEnum())
            {
                auto typeName = enumType->engineImportName ? enumType->engineImportName : StringID(enumType->fullName());
                auto rttiType = RTTI::GetInstance().findType(typeName);
                ASSERT(rttiType);
                return rttiType;
            }
            else
            {
                ASSERT(!"Invalid stub type");
                return nullptr;
            }
        }

        Type Loader::createType(const StubTypeDecl* typeDecl)
        {
            Type rttiType = nullptr;

            switch (typeDecl->metaType)
            {
                case StubTypeType::Engine:
                {
                    rttiType = RTTI::GetInstance().findType(typeDecl->name);
                    break;
                }

                case StubTypeType::Simple:
                {
                    rttiType = createType(typeDecl->referencedType);
                    break;
                }

                case StubTypeType::ClassType:
                {
                    auto innerType  = createType(typeDecl->referencedType);
                    auto typeName = rtti::FormatClassRefTypeName(innerType->name());
                    auto rttiType = RTTI::GetInstance().findType(typeName);
                    ASSERT(rttiType);
                    return rttiType;
                }

                case StubTypeType::PtrType:
                {
                    auto innerType  = createType(typeDecl->referencedType);
                    auto typeName = rtti::FormatStrongHandleTypeName(innerType->name());
                    rttiType = RTTI::GetInstance().findType(typeName);
                    break;
                }

                case StubTypeType::WeakPtrType:
                {
                    auto innerType  = createType(typeDecl->referencedType);
                    auto typeName = rtti::FormatWeakHandleTypeName(innerType->name());
                    rttiType = RTTI::GetInstance().findType(typeName);
                    break;
                }

                case StubTypeType::DynamicArrayType:
                {
                    auto innerType  = createType(typeDecl->innerType);
                    auto typeName = rtti::FormatDynamicArrayTypeName(innerType->name());
                    rttiType = RTTI::GetInstance().findType(typeName);
                    break;
                }

                case StubTypeType::StaticArrayType:
                {
                    auto innerType  = createType(typeDecl->innerType);
                    auto typeName = rtti::FormatNativeArrayTypeName(innerType->name(), typeDecl->arraySize);
                    rttiType = RTTI::GetInstance().findType(typeName);
                    break;
                }
            }

            return rttiType;
        }

        void Loader::createExports()
		{
            // enums
            uint32_t createdEnums = 0;
            for (auto symbol : m_allEnums)
            {
                if (!symbol->m_exportStub)
                {
                    ASSERT(symbol->m_resolved.m_enum != nullptr);
                    continue;
                }

                // determine min/max value of enum values
                auto minValue = std::numeric_limits<int64_t>::max();
                auto maxValue = std::numeric_limits<int64_t>::min();
                {
                    int64_t nextValue = 0;
                    for (auto option : symbol->m_exportStub->asEnum()->options)
                    {
                        int64_t usedValue;
                        if (option->hasUserAssignedValue)
                            usedValue = option->assignedValue;
                        else
                            usedValue = nextValue;

                        minValue = std::min(minValue, usedValue);
                        maxValue = std::max(minValue, usedValue);
                        nextValue = usedValue + 1;
                    }
                }

                // determine size of the enum type
                uint8_t enumTypeSize = 1;
                auto enumTypeSigned = (minValue < 0);
                if (enumTypeSigned)
                {
                    if (minValue < std::numeric_limits<char>::min()) enumTypeSize = 2;
                    if (maxValue > std::numeric_limits<char>::max()) enumTypeSize = 2;
                    if (minValue < std::numeric_limits<short>::min()) enumTypeSize = 4;
                    if (maxValue > std::numeric_limits<short>::max()) enumTypeSize = 4;
                    if (minValue < std::numeric_limits<int>::min()) enumTypeSize = 8;
                    if (maxValue > std::numeric_limits<int>::max()) enumTypeSize = 8;
                }
                else
                {
                    if (maxValue > (int64_t) std::numeric_limits<uint8_t>::max()) enumTypeSize = 2;
                    if (maxValue > (int64_t) std::numeric_limits<uint16_t>::max()) enumTypeSize = 4;
                    if (maxValue > (int64_t) std::numeric_limits<uint32_t>::max()) enumTypeSize = 8;
                }

                // create the function
                auto rttiEnum = m_typeRegistry.createEnum(symbol->m_fullName, enumTypeSize);
                ASSERT(rttiEnum)
                ASSERT(symbol->m_resolved.m_enum == nullptr);
                symbol->m_resolved.m_enum = rttiEnum;
                createdEnums += 1;

                // add enum options
                {
                    int64_t nextValue = 0;
                    for (auto option : symbol->m_exportStub->asEnum()->options)
                    {
                        int64_t usedValue;
                        if (option->hasUserAssignedValue)
                            usedValue = option->assignedValue;
                        else
                            usedValue = nextValue;

                        rttiEnum->add(option->name, usedValue);
                        nextValue = usedValue + 1;
                    }
                }
            }
            TRACE_INFO("Created {} enum(s)", createdEnums);

            // class types
            Array<ScriptedStruct *> exportedStructs;
            Array<ScriptedClass *> exportedClasses;
            for (auto symbol : m_allClasses)
            {
                if (!symbol->m_exportStub)
                {
                    ASSERT(symbol->m_resolved.m_class != nullptr);
                    continue;
                }

                auto classStub = symbol->m_exportStub->asClass();
                ASSERT(classStub != nullptr);

                // should not be resolved yet
                ASSERT(symbol->m_resolved.m_class == nullptr);

                // create class
                if (classStub->flags.test(StubFlag::Class))
                {
                    auto nativeClass = FindNativeClassBase(classStub);
                    auto rttiClass = m_typeRegistry.createClass(symbol->m_fullName, nativeClass);

                    if (classStub->flags.test(StubFlag::Abstract))
                        rttiClass->markAsAbstract();

                    symbol->m_resolved.m_class = rttiClass;
                    exportedClasses.pushBack(rttiClass);
                }
                    // create struct
                else if (classStub->flags.test(StubFlag::Struct))
                {
                    auto rttiStruct = m_typeRegistry.createStruct(symbol->m_fullName);
                    symbol->m_resolved.m_class = rttiStruct;
                    exportedStructs.pushBack(rttiStruct);
                }
            }
            TRACE_INFO("Created {} classes(s)", exportedClasses.size());
            TRACE_INFO("Created {} structures(s)", exportedStructs.size());

            // link base classes
            for (auto symbol : m_allClasses)
            {
                ASSERT(symbol->m_resolved.m_class != nullptr);

                if (!symbol->m_exportStub)
                    continue;

                auto classStub = symbol->m_exportStub->asClass();
                ASSERT(classStub != nullptr);

                if (classStub->baseClass != nullptr)
                {
                    // find symbol
                    Symbol *baseClassSymbol = nullptr;
                    m_symbolStubMap.find(classStub->baseClass, baseClassSymbol);
                    ASSERT(baseClassSymbol != nullptr);
                    ASSERT(baseClassSymbol->m_stubType == StubType::Class);
                    ASSERT(baseClassSymbol->m_resolved.m_class != nullptr);

                    auto rttiBaseClass = baseClassSymbol->m_resolved.m_class;
                    const_cast<rtti::IClassType *>(symbol->m_resolved.m_class)->baseClass(rttiBaseClass);
                }
            }

            // resolve all types
            for (auto &dcl : m_allTypeDecls)
                dcl.m_resolvedType = createType(dcl.m_stub);

            // create properties
            uint32_t numCreatedProps = 0;
            for (auto symbol : m_allProps)
            {
                if (!symbol->m_exportStub)
                {
                    ASSERT(symbol->m_resolved.m_property != nullptr);
                    continue;
                }

                auto propStub = symbol->m_exportStub->asProperty();

                rtti::PropertySetup propSetup;
                propSetup.m_type = resolveType(propStub->typeDecl);
                propSetup.m_name = propStub->name;
                propSetup.m_flags = rtti::PropertyFlagBit::Scripted;

                if (propStub->flags.test(StubFlag::Editable))
                    propSetup.m_flags |= rtti::PropertyFlagBit::Editable;
                if (propStub->flags.test(StubFlag::Inlined))
                    propSetup.m_flags |= rtti::PropertyFlagBit::Inlined;
                if (propStub->flags.test(StubFlag::Const))
                    propSetup.m_flags |= rtti::PropertyFlagBit::ReadOnly;

                // TODO: import detailed settings from script

                // add to owner
                auto rttiClass = resolveClass(propStub->owner->asClass());
                if (propStub->owner->flags.test(StubFlag::Struct))
                {
                    auto scriptedProp = MemNewPool(POOL_SCRIPTS, ScriptedProperty, rttiClass, propSetup);
                    const_cast<rtti::IClassType *>(rttiClass.ptr())->addProperty(scriptedProp);
                    symbol->m_resolved.m_property = scriptedProp;
                }
                else
                {
                    propSetup.m_flags |= rtti::PropertyFlagBit::ExternalBuffer;

                    auto scriptedProp = MemNewPool(POOL_SCRIPTS, ScriptedClassProperty, rttiClass, propSetup);
                    const_cast<rtti::IClassType *>(rttiClass.ptr())->addProperty(scriptedProp);
                    symbol->m_resolved.m_property = scriptedProp;
                }

                numCreatedProps += 1;
            }
            TRACE_INFO("Created {} properties", numCreatedProps);

            // resolve structure sizes
            bool sizeChanged = false;
            do
            {
                sizeChanged = false;
                for (auto stub : exportedStructs)
                    sizeChanged |= stub->updateDataSize();
            } while (sizeChanged);

            // resolve class sizes
            do
            {
                sizeChanged = false;
                for (auto stub : exportedClasses)
                    sizeChanged |= stub->updateScriptedDataSize();
            } while (sizeChanged);

            // create functions
            uint32_t numCreatedGlobalFunc = 0;
            uint32_t numCreatedClassFunc = 0;
            for (auto symbol : m_allFunc)
            {
                if (!symbol->m_exportStub)
                {
                    ASSERT(symbol->m_resolved.m_function != nullptr || symbol->anyStub()->flags.test(StubFlag::Opcode));
                    continue;
                }

                auto funcStub = symbol->m_exportStub->asFunction();

                ClassType rttiClass = nullptr;
                if (symbol->m_classOwner)
                {
                    rttiClass = symbol->m_classOwner->m_resolved.m_class;
                    numCreatedClassFunc += 1;
                }
                else
                {
                    numCreatedGlobalFunc += 1;
                }

                // create the function
                auto rttiFunc = m_typeRegistry.createFunction(rttiClass ? funcStub->name : symbol->m_fullName, rttiClass);
                ASSERT(rttiFunc)
                symbol->m_resolved.m_function = rttiFunc;

            }
            TRACE_INFO("Created {} global functions", numCreatedGlobalFunc);
            TRACE_INFO("Created {} class functions", numCreatedClassFunc);

            // create code
            for (auto symbol : m_allFunc)
            {
                ASSERT(symbol->m_resolved.m_function != nullptr || symbol->anyStub()->flags.test(StubFlag::Opcode));

                if (!symbol->m_exportStub)
                    continue;

                // create the function code block
                auto funcStub = symbol->m_exportStub->asFunction();
                auto codeBlock = FunctionCodeBlock::Create(funcStub, funcStub->owner->asClass(), *this);
                if (!codeBlock)
                {
                    TRACE_ERROR("{}: error: Unable to generate machine VM code for function '{}'", funcStub->location, funcStub->fullName());
                    continue;
                }

                // get the return param
                rtti::FunctionParamType retParam;
                if (funcStub->returnTypeDecl != nullptr)
                    retParam.m_type = resolveType(funcStub->returnTypeDecl);

                // extarct argument types
                Array<rtti::FunctionParamType> argTypes;
                for (auto arg  : funcStub->args)
                {
                    auto& argType = argTypes.emplaceBack();
                    argType.m_type = resolveType(arg->typeDecl);

                    if (arg->flags.test(StubFlag::Ref))
                        argType.m_flags |= rtti::FunctionParamFlag::ConstRef;
                    else if (arg->flags.test(StubFlag::Out))
                        argType.m_flags |= rtti::FunctionParamFlag::Ref;
                }

                // initialize the function
                auto isStatic = (funcStub->owner->asClass() == nullptr) || funcStub->flags.test(StubFlag::Static);
                const_cast<rtti::Function*>(symbol->m_resolved.m_function)->setupScripted(retParam, argTypes, codeBlock, false, isStatic);
            }

            // bind class functions (ctor/dtor)
            for (auto stub  : exportedClasses)
                stub->bindFunctions();

            // bind structure functions (ctor/dtor)
            for (auto stub  : exportedStructs)
                stub->bindFunctions();
        }

        //--

        Type Loader::resolveType(const StubTypeDecl* stub)
        {
            int index = INDEX_NONE;
            m_typeMap.find(stub, index);
            ASSERT(index != INDEX_NONE);

            auto& info = m_allTypeDecls[index];
            ASSERT(info.m_resolvedType != nullptr);

            return info.m_resolvedType;
        }

        ClassType Loader::resolveClass(const StubClass* stub)
        {
            Symbol* symbol = nullptr;
            m_symbolStubMap.find(stub, symbol);
            ASSERT(symbol != nullptr);

            ASSERT(symbol->m_resolved.m_class != nullptr);
            return symbol->m_resolved.m_class;
        }

        const rtti::EnumType* Loader::resolveEnum(const StubEnum* stub)
        {
            Symbol* symbol = nullptr;
            m_symbolStubMap.find(stub, symbol);
            ASSERT(symbol != nullptr);

            ASSERT(symbol->m_resolved.m_enum != nullptr);
            return symbol->m_resolved.m_enum;
        }

        const rtti::Property* Loader::resolveProperty(const StubProperty* prop)
        {
            Symbol* symbol = nullptr;
            m_symbolStubMap.find(prop, symbol);
            ASSERT(symbol != nullptr);

            ASSERT(symbol->m_resolved.m_property != nullptr);
            return symbol->m_resolved.m_property;
        }

        const rtti::Function* Loader::resolveFunction(const StubFunction* func)
        {
            Symbol* symbol = nullptr;
            m_symbolStubMap.find(func, symbol);
            ASSERT(symbol != nullptr);

            ASSERT(symbol->m_resolved.m_function != nullptr);
            return symbol->m_resolved.m_function;
        }

        //--

    } // script
} // base