/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptLibrary.h"
#include "scriptTypeCasting.h"

#include "core/script/include/scriptPortableStubs.h"
#include "core/containers/include/hashSet.h"
#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//--

StubLibrary::StubLibrary(mem::LinearAllocator& mem, StringView primaryModuleName)
    : m_mem(mem)
    , m_primaryModuleName(primaryModuleName)
{
    // create the primary module interface
    m_primaryModule = m_mem.create<StubModule>();
    m_primaryModule->name = StringID(primaryModuleName);
    m_modules.pushBack(m_primaryModule);
}

StubLibrary::~StubLibrary()
{
}

void StubLibrary::findAliasedFunctions(StringID name, uint32_t expectedArgumentCount, Array<const StubFunction*>& outAliasedFunctions) const
{
    // TODO: optimize

    for (auto module  : m_modules)
        for (auto file  : module->files)
            for (auto stub  : file->stubs)
                if (auto func  = stub->asFunction())
                    if (func->aliasName == name && func->args.size() == expectedArgumentCount)
                        outAliasedFunctions.pushBack(func);
}

const Stub* StubLibrary::findRootStubInContext(StringID name, const Stub* context) const
{
    // find in current context
    if (auto stub  = findChildStubInContext(name, context))
        return stub;

    // no root stub found, try your luck in higher scope
    return context ? findRootStubInContext(name, context->owner) : nullptr;
}

const Stub* StubLibrary::findChildStubInContext(StringID name, const Stub* context) const
{
    if (context == nullptr)
    {
        for (auto module  : m_modules)
            if (module->name == name)
                return module;
    }
    else if (context->stubType == StubType::File)
    {
        auto file = static_cast<const StubFile *>(context);
        /*for (auto import  : file->stubs)
            if (module->name == name)
                return module;*/
    }
    else if (context->stubType == StubType::Module)
    {
        auto module = static_cast<const StubModule *>(context);
        return module->findStub(name);
    }
    else if (context->stubType == StubType::ModuleImport)
    {
        auto module = static_cast<const StubModuleImport *>(context);
		if (module->m_importedModuleData)
			return module->m_importedModuleData->findStub(name);
    }
    else if (context->stubType == StubType::Class)
    {
        auto klass = static_cast<const StubClass *>(context);
        return klass->findStub(name);
    }
    else if (context->stubType == StubType::Function)
    {
        auto func = static_cast<const StubFunction *>(context);
        for (auto arg : func->args)
            if (arg->name == name)
                return arg;
    }

    return nullptr;
}

const Stub* StubLibrary::findStubInContext(StringID name, const Stub* context) const
{
    // split the complex type name
    InplaceArray<StringView, 6> nameParts;
    name.view().slice(".", false, nameParts);

    // empty name
    if (nameParts.empty())
        return nullptr;

    // find the root stub for the first symbol, we ALWAYS prefer local (or direct) definitions
    const Stub* stub = findRootStubInContext(nameParts[0], context);
    if (!stub)
    {
        // try looking into the imports
        for (auto importedModule : m_primaryModule->imports)
        {
			stub = findRootStubInContext(nameParts[0], importedModule);
			if (stub)
                break;
        }

        if (!stub)
            return nullptr;
    }

    // if we have more identifiers in the name resolve them as well
    for (uint32_t i=1; i<nameParts.size() && stub; ++i)
        stub = findChildStubInContext(nameParts[i], stub);

    return stub;
}

const StubTypeDecl* StubLibrary::resolveSimpleTypeReference(StringID name, const Stub* context)
{
    auto lock = CreateLock(m_lock);

    // quick check for engine types (the most of the uses)
    {
        StubTypeDecl *ret = nullptr;
        if (m_cacheEngineTypes.find(name, ret))
            return ret;
    }

    // find stub
    auto stub  = findStubInContext(name, context);
    if (!stub)
        return nullptr;

    // skip functions
    if (stub->asFunction())
        return nullptr;

    // a type name ?
    if (auto typeName  = stub->asTypeName())
        return typeName->linkedType;

    // we can only link to a class or an enum
    if (!stub->asClass() && !stub->asEnum())
        return nullptr;

    // do we have a type ref for this stub ?
    StubTypeRef* ref = nullptr;
    if (!m_cachedTypeRefs.find(stub, ref))
    {
        ref = m_mem.create<StubTypeRef>();
        ref->location = stub->location;
        ASSERT(stub->stubType == StubType::Enum || stub->stubType == StubType::Class);
        ref->resolvedStub = stub;
        ref->name = StringID(stub->fullName());
        m_cachedTypeRefs[stub] = ref;
    }

    // create a simple type decl
    StubTypeDecl* ret = nullptr;
    if (!m_cachedTypeSimpleDecls.find(ref, ret))
    {
        ret = m_mem.create<StubTypeDecl>();
        ret->location = stub->location;
        ret->metaType = StubTypeType::Simple;
        ret->referencedType = ref;
        ASSERT(stub->stubType == StubType::Enum || stub->stubType == StubType::Class);
        ref->name = StringID(stub->fullName());
        m_cachedTypeSimpleDecls[ref] = ret;
    }

    return ret;
}

StubFile* StubLibrary::createFile(StubModule* module, const StringBuf& depotPath, const StringBuf& absolutePath)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubFile>();
    ret->owner = module;
    ret->depotPath = depotPath;
    ret->absolutePath = absolutePath;
    module->files.pushBack(ret);

    return ret;
}

StubModuleImport* StubLibrary::createModuleImport(const StubLocation& location, StubFile* file, StringID name)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubModuleImport>();
    ret->owner = file;
    ret->name = name;
    file->stubs.pushBack(ret);

    return ret;
}

StubEnumOption* StubLibrary::createEnumOption(const StubLocation& location, Stub* owner, StringID name)
{
    ASSERT(owner != nullptr);

    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubEnumOption>();
    ret->location = location;
    ret->owner = owner;
    ret->name = name;
    return ret;
}

StubEnum* StubLibrary::createEnum(const StubLocation& location, StringID name, Stub* owner)
{
    ASSERT(owner != nullptr);

    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubEnum>();
    ret->location = location;
    ret->owner = owner;
    ret->name = name;

    attachStub(owner, ret);

    return ret;
}

StubTypeName* StubLibrary::createTypeAlias(const StubLocation& location, StringID name, const StubTypeDecl* typeDecl, Stub* owner)
{
    ASSERT(typeDecl != nullptr);

    auto lock = CreateLock(m_lock);

    StubTypeName* ret = nullptr;
    ret = m_mem.create<StubTypeName>();
    ret->location = location;
    ret->owner = owner;
    ret->linkedType = typeDecl;
    ret->name = name;

    attachStub(owner, ret);
    return ret;
}

StubClass* StubLibrary::createClass(const StubLocation& location, StringID name, Stub* owner)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubClass>();
    ret->location = location;
    ret->owner = owner;
    ret->name = name;

    attachStub(owner, ret);

    return ret;
}

StubFunctionArg* StubLibrary::createFunctionArg(const StubLocation& location, StringID name, StubFunction* owner)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubFunctionArg>();
    ret->location = location;
    ret->owner = owner;
    ret->name = name;
    return ret;
}

StubFunction* StubLibrary::createFunction(const StubLocation& location, StringID name, Stub* owner)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubFunction>();
    ret->location = location;
    ret->owner = owner;
    ret->name = name;

    attachStub(owner, ret);

    return ret;
}

StubProperty* StubLibrary::createProperty(const StubLocation& location, StringID name, Stub* owner)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubProperty>();
    ret->location = location;
    ret->owner = owner;
    ret->name = name;

    attachStub(owner, ret);

    return ret;
}

//--

/*StubTypeDecl* StubLibrary::createVoidType(const StubLocation& location)
{
    auto lock = CreateLock(lock);

    auto ret  = m_mem.create<StubTypeDecl>();
    ret->location = location;
    ret->metaType = StubTypeType::Void;
    return ret;
}*/


StubTypeDecl* StubLibrary::createEngineType(const StubLocation& location, StringID name)
{
    ASSERT(name);

    auto lock = CreateLock(m_lock);

    StubTypeDecl* ret = nullptr;
    if (m_cacheEngineTypes.find(name, ret))
        return ret;

    ret = m_mem.create<StubTypeDecl>();
    ret->location = location;
    ret->metaType = StubTypeType::Engine;
    ret->name = name;

    m_cacheEngineTypes[name] = ret;
    return ret;
}

StubTypeRef* StubLibrary::createTypeRef(const StubLocation& location, StringID name, const Stub* context)
{
    ASSERT(context != nullptr);
    ASSERT(name != nullptr);

    auto lock = CreateLock(m_lock);

    auto ref  = m_mem.create<StubTypeRef>();
    ref->location = location;
    ref->owner = context;
    ref->name = name;
    m_unresolvedTypes.pushBack(ref);
    return ref;
}

StubTypeRef* StubLibrary::createTypeRef(const Stub* resolvedStub)
{
    auto lock = CreateLock(m_lock);
    return createTypeRef_NoLock(resolvedStub);
}

StubTypeRef* StubLibrary::createTypeRef_NoLock(const Stub* resolvedStub)
{
    ASSERT(resolvedStub != nullptr);

    if (resolvedStub->stubType == StubType::TypeRef)
        return (StubTypeRef*)(resolvedStub);

    StubTypeRef* ret = nullptr;
    if (m_cachedTypeRefs.find(resolvedStub, ret))
        return ret;

    auto ref  = m_mem.create<StubTypeRef>();
    ref->location = resolvedStub->location;
    ref->resolvedStub = resolvedStub;
    ref->name = StringID(resolvedStub->fullName());

    m_cachedTypeRefs[resolvedStub] = ref;
    return ref;
}

StubTypeDecl* StubLibrary::createSimpleType(const StubLocation& location, const Stub* stub)
{
    ASSERT(stub != nullptr);

    auto lock = CreateLock(m_lock);

    StubTypeDecl* ret = nullptr;
    ret = m_mem.create<StubTypeDecl>();
    ret->location = location;
    ret->metaType = StubTypeType::Simple;
    ret->referencedType = createTypeRef_NoLock(stub);

    m_unresolvedDeclarations.pushBack(ret);
    return ret;
}

StubTypeDecl* StubLibrary::createClassType(const StubLocation& location, const Stub* classType)
{
    ASSERT(classType != nullptr);

    auto lock = CreateLock(m_lock);

    StubTypeDecl* ret = nullptr;
    ret = m_mem.create<StubTypeDecl>();
    ret->location = location;
    ret->metaType = StubTypeType::ClassType;
    ret->referencedType = createTypeRef_NoLock(classType);

    m_unresolvedDeclarations.pushBack(ret);
    return ret;
}

StubTypeDecl* StubLibrary::createSharedPointerType(const StubLocation& location, const Stub* classType)
{
    ASSERT(classType);

    auto lock = CreateLock(m_lock);

    StubTypeDecl* ret = nullptr;
    ret = m_mem.create<StubTypeDecl>();
    ret->location = location;
    ret->metaType = StubTypeType::PtrType;
    ret->referencedType = createTypeRef_NoLock(classType);

    m_unresolvedDeclarations.pushBack(ret);
    return ret;
}

StubTypeDecl* StubLibrary::createWeakPointerType(const StubLocation& location, const Stub* classType)
{
    ASSERT(classType);

    auto lock = CreateLock(m_lock);

    StubTypeDecl* ret = nullptr;
    ret = m_mem.create<StubTypeDecl>();
    ret->location = location;
    ret->metaType = StubTypeType::WeakPtrType;
    ret->referencedType = createTypeRef_NoLock(classType);

    m_unresolvedDeclarations.pushBack(ret);
    return ret;
}

StubTypeDecl* StubLibrary::createDynamicArrayType(const StubLocation& location, const StubTypeDecl* innerType)
{
    ASSERT(innerType);

    auto lock = CreateLock(m_lock);

    StubTypeDecl* ret = nullptr;
    ret = m_mem.create<StubTypeDecl>();
    ret->location = location;
    ret->metaType = StubTypeType::DynamicArrayType;
    ret->innerType = innerType;
    return ret;
}

StubTypeDecl* StubLibrary::createStaticArrayType(const StubLocation& location, const StubTypeDecl* innerType, uint32_t arraySize)
{
    ASSERT(innerType);

    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubTypeDecl>();
    ret->location = location;
    ret->metaType = StubTypeType::StaticArrayType;
    ret->arraySize = arraySize;
    ret->innerType = innerType;
    return ret;
}

//--

StubConstantValue* StubLibrary::createConstValueInt(const StubLocation& location, int64_t value)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubConstantValue>();
    ret->location = location;
    ret->m_valueType = StubConstValueType::Integer;
    ret->value.i = value;
    return ret;
}

StubConstantValue* StubLibrary::createConstValueUint(const StubLocation& location, uint64_t value)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubConstantValue>();
    ret->location = location;
    ret->m_valueType = StubConstValueType::Unsigned;
    ret->value.u = value;
    return ret;
}

StubConstantValue* StubLibrary::createConstValueFloat(const StubLocation& location, double value)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubConstantValue>();
    ret->location = location;
    ret->m_valueType = StubConstValueType::Float;
    ret->value.f = value;
    return ret;
}

StubConstantValue* StubLibrary::createConstValueBool(const StubLocation& location, bool value)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubConstantValue>();
    ret->location = location;
    ret->m_valueType = StubConstValueType::Bool;
    ret->value.i = value ? 1 : 0;
    return ret;
}

StubConstantValue* StubLibrary::createConstValueString(const StubLocation& location, StringView value)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubConstantValue>();
    ret->location = location;
    ret->m_valueType = StubConstValueType::String;
    ret->text = m_mem.strcpy(value.data(), value.length());
    return ret;
}

StubConstantValue* StubLibrary::createConstValueName(const StubLocation& location, StringID value)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubConstantValue>();
    ret->location = location;
    ret->m_valueType = StubConstValueType::Name;
    ret->text = m_mem.strcpy(value.c_str());
    return ret;
}

StubConstantValue* StubLibrary::createConstValueCompound(const StubLocation& location, const StubTypeDecl* structTypeRef)
{
    ASSERT(structTypeRef != nullptr);

    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubConstantValue>();
    ret->location = location;
    ret->m_valueType = StubConstValueType::Compound;
    ret->compoundType = structTypeRef;
    return ret;
}

void StubLibrary::attachStub(Stub* owner, Stub* stub)
{
    ASSERT(owner != nullptr);

    if (owner->stubType == StubType::File)
    {
        auto module  = (StubFile*)owner;
        module->stubs.pushBack(stub);
    }
    else if (owner->stubType == StubType::Class)
    {
        auto module  = static_cast<StubClass*>(owner);
        module->stubs.pushBack(stub);
    }
    else
    {
        ASSERT(!"Invalid owner for stub attachment");
    }
}

StubConstant* StubLibrary::createConst(const StubLocation& location, StringID name, Stub* owner)
{
    auto lock = CreateLock(m_lock);

    auto ret  = m_mem.create<StubConstant>();
    ret->location = location;
    ret->name = name;
    ret->owner = owner;

    attachStub(owner, ret);

    return ret;
}

void StubLibrary::formatTypeName(const StubTypeDecl* typeDecl, StringBuilder& f) const
{
    switch (typeDecl->metaType)
    {
        case StubTypeType::Simple:
        {
            ASSERT(!typeDecl->referencedType->name.empty());
            f << typeDecl->referencedType->name;
            break;
        }

        case StubTypeType::Engine:
        {
            auto name = typeDecl->name.view().afterLastOrFull("::");
            ASSERT(!name.empty());
            f << name;
            break;
        }

        case StubTypeType::ClassType:
        {
            f << "class_";
            ASSERT(!typeDecl->referencedType->name.empty());
            f << typeDecl->referencedType->name;
            break;
        }

        case StubTypeType::PtrType:
        {
            f << "ptr_";
            ASSERT(!typeDecl->referencedType->name.empty());
            f << typeDecl->referencedType->name;
            break;
        }

        case StubTypeType::WeakPtrType:
        {
            f << "weak_";
            ASSERT(!typeDecl->referencedType->name.empty());
            f << typeDecl->referencedType->name;
            break;
        }

        case StubTypeType::DynamicArrayType:
        {
            f << "array_";
            formatTypeName(typeDecl->innerType, f);
            break;
        }

        case StubTypeType::StaticArrayType:
        {
            f << "sarray_";
            formatTypeName(typeDecl->innerType, f);
            break;
        }
    }
}

StringID StubLibrary::formatOperatorName(const StubFunction* func) const
{
    StringBuilder f;

    f << func->operatorName;

    for (auto arg  : func->args)
    {
        f << "_";

        if (arg->flags.test(StubFlag::Ref))
            f << "ref_";
        if (arg->flags.test(StubFlag::Out))
            f << "out_";

        formatTypeName(arg->typeDecl, f);
    }

    if (func->returnTypeDecl != nullptr)
    {
        f << "_";
        formatTypeName(func->returnTypeDecl, f);
    }

    return StringID(f.c_str());
}

StringID StubLibrary::formatCastName(const StubFunction* func) const
{
    StringBuilder f;

    f << "cast";

    for (auto arg  : func->args)
    {
        f << "_";
        formatTypeName(arg->typeDecl, f);
    }

    if (func->returnTypeDecl != nullptr)
    {
        f << "_";
        formatTypeName(func->returnTypeDecl, f);
    }

    return StringID(f.c_str());
}

bool StubLibrary::isUnaryOperator(StringID name) const
{
    if (name.view().beginsWith("opNotEqual"))
        return false;

    if (name.view().beginsWith("opIncrement")
        || name.view().beginsWith("opDecrement")
        || name.view().beginsWith("opPostIncrement")
        || name.view().beginsWith("opPostDecrement")
        || name.view().beginsWith("opBinaryNot") // ~
        || name.view().beginsWith("opNot") // !
        || name.view().beginsWith("opNegate")) // -
        return true;

    return false;
}

bool StubLibrary::buildFunction(IErrorHandler& err, StubFunction* func)
{
    if (func->flags.test(StubFlag::Operator))
    {
        // hack for unary/binary operartors that depend on number of arguments
        if (func->name == "opSubtract" && func->args.size() == 1)
        {
            func->name = "opNegate"_id;
        }
        else if (func->name == "opAdd" && func->args.size() == 1)
        {
            func->name = "opPlus"_id;
        }
        else if (func->name == "opIncrement" && func->args.size() == 2)
        {
            func->name = "opPostIncrement"_id;
            func->args.resize(1);
        }
        else if (func->name == "opDecrement" && func->args.size() == 2)
        {
            func->name = "opPostDecrement"_id;
            func->args.resize(1);
        }

        if (isUnaryOperator(func->name))
        {
            if (func->args.size() != 1)
            {
                reportError(err, func, TempString("Unary operator '{}' should take one argument", func->name));
                return false;
            }
            else if (func->name.view().beginsWith("opIncrement") || func->name.view().beginsWith("opDecrement")
                        || func->name.view().beginsWith("opPostIncrement") || func->name.view().beginsWith("opPostDecrement"))
            {
                if (!func->args[0]->flags.test(StubFlag::Out))
                {
                    reportError(err, func, TempString("Operator first argument should be passed by output reference (out)"));
                    return false;
                }
            }
        }
        else
        {
            if (func->args.size() != 2)
            {
                reportError(err, func,  TempString("Binary operator '{}' should take two argument", func->name));
                return false;
            }
        }

        if (func->returnTypeDecl == nullptr)
        {
            reportError(err, func, TempString("Operator should return a value"));
            return false;
        }

        func->operatorName = func->name;
        func->name = formatOperatorName(func);
    }
    else if (func->flags.test(StubFlag::Cast))
    {
        func->name = formatCastName(func);

        if (func->args.size() != 1)
        {
            reportError(err, func, TempString("Cast operator should take one argument"));
            return false;
        }

        if (func->returnTypeDecl == nullptr)
        {
            reportError(err, func, TempString("Cast operator should return a value"));
            return false;
        }
    }

    return true;
}

bool StubLibrary::buildClass(IErrorHandler& err, StubClass* classType)
{
    bool valid = true;

    for (auto ptr : classType->stubs)
    {
        if (ptr->stubType == StubType::Function)
        {
            auto func = static_cast<StubFunction *>(ptr);

            if (!func->opcodeName.empty() && !func->flags.test(StubFlag::Static))
            {
                reportError(err, ptr, TempString("Only static or globa functions may be aliased to opcodes (ie. we can't have a 'this' in opcode)"));
                valid = false;
                continue;
            }

            if (func->flags.test(StubFlag::Operator))
            {
                reportError(err, ptr,  TempString("Operator can only be defined at global scope"));
                valid = false;
                continue;
            }
            else if (func->flags.test(StubFlag::Cast))
            {
                reportError(err, ptr, TempString("Cast operator can only be defined at global scope"));
                valid = false;
                continue;
            }
        }
        else if (ptr->stubType == StubType::Class)
        {
            auto innerClassStub = static_cast<StubClass *>(ptr);
            valid &= buildClass(err, innerClassStub);
        }

        // already defined ?
        if (auto existing = classType->findStub(ptr->name))
        {
            reportError(err, ptr, BaseTempString<1024>("{} was already defined here: {}", ptr->name, existing->location));
            valid = false;
            continue;
        }

        // add to class map
        classType->stubMap[ptr->name] = ptr;
    }

    return valid;
}

bool StubLibrary::buildNamedMaps(IErrorHandler& err)
{
    bool valid = true;

    // process stubs from primary module
    for (auto file : m_primaryModule->files)
    {
        for (auto ptr : file->stubs)
        {
            ASSERT(ptr->owner->asFile()->owner == m_primaryModule);

            // warn about useless flag
            if (ptr->flags.test(StubFlag::Protected))
                reportWarning(err, ptr, TempString("Global symbol can only be declared as private or public"));

            // fixup name
            if (ptr->stubType == StubType::Function)
            {
                auto func = static_cast<StubFunction *>(ptr);
                valid &= buildFunction(err, func);
            }

            // process class
            if (ptr->stubType == StubType::Class)
            {
                auto classType = static_cast<StubClass *>(ptr);
                valid &= buildClass(err, classType);
            }

            // already defined IN THE SAME SCOPE ?
            if (auto existing = findChildStubInContext(ptr->name, ptr->owner))
            {
                reportError(err, ptr, BaseTempString<1024>("{} already defined: {}", ptr->name, existing->location));
                valid = false;
                continue;
            }

            // map stub by name
            m_primaryModule->stubMap[ptr->name] = ptr;
        }
    }

    return valid;
}

static const StubModule* FindModule(const Stub* stub)
{
    if (!stub)
        return nullptr;

    if (stub->stubType == StubType::Module)
        return static_cast<const StubModule*>(stub);

    return FindModule(stub->owner);
}

static const StubClass* FindClass(const Stub* stub)
{
    if (!stub)
        return nullptr;

    if (stub->stubType == StubType::Class)
        return static_cast<const StubClass*>(stub);

    return FindClass(stub->owner);
}

static bool CheckClassProtectedPrivate(const Stub* stub, const StubClass* stubClass, const StubClass* fromClass)
{
    if (stub->flags.test(StubFlag::Private))
    {
        return fromClass == stubClass;
    }
    else if (stub->flags.test(StubFlag::Protected))
    {
        if (!fromClass)
            return false;

        return fromClass->is(stubClass);
    }

    return true;
}

bool StubLibrary::canAccess(const Stub* stub, const Stub* fromStub) const
{
    // stub: PropertyA -> ClassB -> ModuleC
    // stub: FunctionA -> ClassB -> ModuleC
    // stub: FunctionA -> ModuleB
    // stub: ConstA -> ModuleB

    // fromStub: ClassA -> ModuleB
    // fromStub: FunctionA -> ClassB -> ModuleC
    // fromStub: FunctionA -> ModuleB

    // Class is accessible only if:
    //  - it's public
    //  - it's private and the StubModule is the same

    // Enum/Const is accessible if:
    //  - it's public
    //  - it's private and the StubModule is the same
    //  - owner class is accessible

    // Function is accessible only if:
    //  - it's public
    //  - class function is protected and accessed from same class or class lower in hierarchy
    //  - class function is private and accessed from the same class
    //  - class function's class is accessible
    //  - global function is private and accessed from the same module

    // Property is accessible only if:
    //  - it's public
    //  - class property is protected and accessed from same class or class lower in hierarchy
    //  - class property is private and accessed from the same class
    //  - class property's class is accessible

    if (stub->stubType == StubType::Class)
    {
        // for private
        if (stub->flags.test(StubFlag::Private))
            return FindModule(stub) == FindModule(fromStub);
    }
    else if (stub->stubType == StubType::Enum || stub->stubType == StubType::Constant || stub->stubType == StubType::Function || stub->stubType == StubType::Property)
    {
        if (auto stubClass  = FindClass(stub))
        {
            // class must be accessible
            if (!canAccess(stubClass, fromStub))
                return false;

            // are we accessed from inside a class ?
            auto fromClass  = FindClass(fromStub);
            if (!CheckClassProtectedPrivate(stub, stubClass, fromClass))
                return false;
        }
        else
        {
            // for private module must be the same
            if (stub->flags.test(StubFlag::Private))
                return FindModule(stub) == FindModule(fromStub);
        }
    }

    return true;
}

void StubLibrary::reportWarning(IErrorHandler& err, const StubLocation& loc, StringView txt)
{
    if (loc.file)
        err.reportWarning(loc.file->absolutePath, loc.line, txt);
}

void StubLibrary::reportWarning(IErrorHandler& err, const Stub* stub, StringView txt)
{
    reportWarning(err, stub ? stub->location : StubLocation(), txt);
}

void StubLibrary::reportError(IErrorHandler& err, const StubLocation& loc, StringView txt)
{
    if (loc.file)
        err.reportError(loc.file->absolutePath, loc.line, txt);
    else
        err.reportError(StringBuf(), 0, txt);
}

void StubLibrary::reportError(IErrorHandler& err, const Stub* stub, StringView txt)
{
    reportError(err, stub ? stub->location : StubLocation(), txt);
}

bool StubLibrary::linkClass(IErrorHandler& err, StubClass* ptr)
{
    // imported class must have engine import alias
    if (ptr->flags.test(StubFlag::Import))
    {
        if (ptr->engineImportName.empty())
        {
            reportError(err, ptr, TempString("Imported type '{}' must have engine type alias specified", ptr->name));
            return false;
        }
    }

    // fixup the imported class
    if (ptr->flags.test(StubFlag::Class) && !ptr->flags.test(StubFlag::Import) && ptr->baseClassName.empty())
        ptr->baseClassName = "Core.ScriptedObject"_id;

    // find base class
    if (!ptr->baseClassName.empty())
    {
        if (ptr->flags.test(StubFlag::Struct))
        {
            reportError(err, ptr, TempString("Structure '{}' is not allowed to have a base class", ptr->name));
            return false;
        }

        auto stub = findStubInContext(ptr->baseClassName, ptr);
        if (!stub)
        {
            reportError(err, ptr, TempString("Base class '{}' not found", ptr->baseClassName));
            return false;
        }
        else if (stub->stubType != StubType::Class)
        {
            reportError(err, ptr, TempString("Base type '{}' is not a class", ptr->baseClassName));
            return false;
        }
        else
        {
            auto baseClass  = static_cast<const StubClass *>(stub);

            if (!canAccess(baseClass, ptr))
            {
                reportError(err, ptr,TempString("Base class '{}' is not accessible form class {}", ptr->baseClassName, ptr->name));
                return false;
            }
            else if (baseClass->is(ptr))
            {
                reportError(err, ptr, TempString("Using class '{}' as base creates recursive class hierarchy on {}", ptr->baseClassName, ptr->name));
                return false;
            }
            else
            {
				//reportWarning(err, ptr, TempString("Base class set to '{}'", stub->fullName()));
                ptr->baseClass = static_cast<const StubClass *>(stub);
                const_cast<StubClass*>(ptr->baseClass)->derivedClasses.pushBack(ptr);
            }
        }
    }
    else if (ptr->flags.test(StubFlag::Class))
    {
        if (ptr->flags.test(StubFlag::Import) && ptr->name != "Object"_id)
        {
            reportError(err, ptr, TempString("Imported class '{}' must have a base class specified", ptr->name));
            return false;
        }
    }

    // find parent class
    if (!ptr->parentClassName.empty())
    {
        if (ptr->flags.test(StubFlag::Struct))
        {
            reportError(err, ptr, TempString("Structure '{}' is not allowed to have a parent class", ptr->name));
            return false;
        }

        auto stub = findStubInContext(ptr->parentClassName, ptr);
        if (!stub)
        {
            reportError(err, ptr, TempString("Parent class '{}' not found", ptr->parentClassName));
            return false;
        }
        else if (stub->stubType != StubType::Class)
        {
            reportError(err, ptr, TempString("Parent type '{}' is not a class", ptr->parentClassName));
            return false;
        }
        else
        {
            auto parentClass  = static_cast<const StubClass *>(stub);
            if (!canAccess(parentClass, ptr))
            {
                reportError(err, ptr, TempString("Parent class '{}' is not accessible form here", ptr->parentClassName));
                return false;
            }
            else
            {
                ptr->parentClass = static_cast<const StubClass *>(stub);
                const_cast<StubClass*>(ptr->parentClass)->childClasses.pushBack(ptr);
            }
        }
    }

    // class seems fine
    TRACE_SPAM("Validated class '{}'", ptr->fullName());
    return true;
}

bool StubLibrary::linkEnum(IErrorHandler& err, StubEnum* enumStub)
{
    if (enumStub->flags.test(StubFlag::Import))
    {
        if (enumStub->engineImportName.empty())
        {
            reportError(err, enumStub, BaseTempString<1024>("Imported enum '{}' should have an engine type alias specified", enumStub->name));
            return false;
        }
    }

    HashMap<int64_t, const StubEnumOption*> valueMap;

    int64_t nextValue = 0;
    for (auto option  : enumStub->options)
    {
        if (auto existing  = enumStub->findOption(option->name))
        {
            reportError(err, enumStub, BaseTempString<1024>("Enum option '{}' was already defined here: {}", option->name, existing->location));
            return false;
        }

        if (enumStub->flags.test(StubFlag::Import))
        {
            if (option->hasUserAssignedValue)
            {
                reportError(err, enumStub, BaseTempString<1024>("Enum option '{}' can't have predefined value because enum '{}' is imported", option->name, enumStub->name));
                return false;
            }
        }

        enumStub->optionsMap[option->name] = option;

        int64_t usedValue;
        if (option->hasUserAssignedValue)
            usedValue = option->assignedValue;
        else
            usedValue = nextValue;

        const StubEnumOption* existingValue = nullptr;
        valueMap.find(usedValue, existingValue);
        if (existingValue)
            reportWarning(err, option, BaseTempString<1024>("Enum value {} was already used here: {}", usedValue, existingValue->location));

        option->assignedValue = usedValue;
        valueMap[usedValue] = option;
        nextValue = usedValue + 1;
    }

    return true;
}

bool StubLibrary::linkClasses(IErrorHandler& err)
{
    bool valid = true;

    Array<StubClass *> allClasses;
    allClasses.reserve(4096);
    m_primaryModule->extractClasses(allClasses);

	for (auto importModule  : m_primaryModule->imports)
		importModule->extractClasses(allClasses);			

    for (auto ptr : allClasses)
        valid &= linkClass(err, ptr);

    return true;
}

bool StubLibrary::linkEnums(IErrorHandler& err)
{
    bool valid = true;

    Array<StubEnum *> allEnums;
    allEnums.reserve(4096);
    m_primaryModule->extractEnums(allEnums);

	/*for (auto importModule  : m_primaryModule->imports)
		importModule->extractEnums(allEnums);*/

    for (auto ptr : allEnums)
        valid &= linkEnum(err, ptr);

    return true;
}

bool StubLibrary::validate(IErrorHandler& err)
{
    // link classes to base and parent classes
    // NOTE: this does not require types to be resolved
    if (!linkClasses(err))
        return false;

    // check enums
    if (!linkEnums(err))
        return false;

    // generate the default functions (mostly ctor/dtor) for classes
    if (!createAutomaticClassFunctions(err))
        return false;

    // resolve references to types
    if (!resolveTypeRefs(err))
        return false;

    // validate type declarations
    if (!resolveTypeDecls(err))
        return false;

    // resolve parent functions, requires linked classes and known types
    if (!linkFunctions(err))
        return false;

    // make sure properties make sense
    if (!checkProperties(err))
        return false;

    // find casting functions
    // NOTE: checks global functions in ALL modules (most of the operators are in Core)
    if (!buildCastMatrix(err))
        return false;

    // type lib seems valid
    return true;
}

bool StubLibrary::resolveTypeRefs(IErrorHandler& err)
{
    bool valid = true;

    for (auto ref  : m_unresolvedTypes)
    {
        auto targetStub = findStubInContext(ref->name, ref->owner);
        if (!targetStub)
        {
            reportError(err, ref, TempString("Unrecognized symbol '{}'", ref->fullName()));

            targetStub = findStubInContext(ref->name, ref->owner);
            valid = false;
        }

        //TRACE_INFO("{}: resolved as {} '{}'", ref->location, targetStub->stubType, targetStub->fullName())
        ref->resolvedStub = targetStub;
    }

    return valid;
}

bool StubLibrary::resolveTypeDecls(IErrorHandler& err)
{
    bool valid = true;

    // pull in the type names
    for (auto decl  : m_unresolvedDeclarations)
    {
        // NOTE: a while loop so we can pull in through multiple type defs
        while (auto typeName  = decl->referencedType ? decl->referencedType->resolvedStub->asTypeName() : nullptr)
        {
            ASSERT(typeName->linkedType != nullptr)

            //TRACE_INFO("{}: resolved from '{}' defined {}", decl->location, typeName->linkedType->fullName(), typeName->location)

            decl->metaType = typeName->linkedType->metaType;
            decl->referencedType = typeName->linkedType->referencedType;
            decl->innerType = typeName->linkedType->innerType;
            decl->arraySize = typeName->linkedType->arraySize;
            decl->name = typeName->linkedType->name;
        }
    }

    // validate that the referenced types actually make sense
    for (auto decl  : m_unresolvedDeclarations)
    {
        if (decl->metaType == StubTypeType::Engine)
        {
            ASSERT(decl->referencedType == nullptr);
            ASSERT(decl->innerType == nullptr);
            ASSERT(!decl->name.empty());
        }
        else if (decl->metaType == StubTypeType::Simple)
        {
            ASSERT(decl->referencedType != nullptr);
            ASSERT(decl->referencedType->resolvedStub != nullptr);
            ASSERT( decl->name.empty());

            if (auto classType  = decl->referencedType->resolvedStub->asClass())
            {
                if (!classType->flags.test(StubFlag::Struct))
                {
                    reportError(err, decl, TempString("Type '{}' should is a class and can't be used directly, try ptr<> or weak<>", decl->referencedType->resolvedStub->fullName()));
                    valid = false;
                }
            }
            else if (auto enumType  = decl->referencedType->resolvedStub->asEnum())
            {
                // TODO: checks ?
            }
            else
            {
                reportError(err, decl, TempString("Type '{}' should be a struct or enum", decl->referencedType->resolvedStub->fullName()));
                valid = false;
            }
        }
        else if (decl->metaType == StubTypeType::ClassType || decl->metaType == StubTypeType::PtrType || decl->metaType == StubTypeType::WeakPtrType)
        {
            ASSERT(decl->referencedType != nullptr);
            ASSERT(decl->referencedType->resolvedStub != nullptr);
            ASSERT( decl->name.empty());

            auto classType  = decl->referencedType->resolvedStub->asClass();
            if (!classType)
            {
                reportError(err, decl, TempString("Type '{}' should be a class", decl->referencedType->resolvedStub->fullName()));
                valid = false;
            }
            else if (classType->flags.test(StubFlag::Struct))
            {
                reportError(err, decl, TempString("Type '{}' is not a class but a struct and cannot be used in pointers", decl->referencedType->resolvedStub->fullName()));
                valid = false;
            }
        }
        else if (decl->metaType == StubTypeType::StaticArrayType || decl->metaType == StubTypeType::DynamicArrayType)
        {
            ASSERT(decl->innerType != nullptr);
            ASSERT(decl->name.empty());
        }
        else
        {
            ASSERT(!"Invalid type type");
        }

        //TRACE_INFO("{}: final type name '{}'", decl->location, decl->fullName());
    }

    return valid;
}

static bool IsTypeUsingStructDirectly(const StubTypeDecl* typeDecl, const StubClass* structStub)
{
    if (!typeDecl)
        return false;

    if (typeDecl->referencedType && typeDecl->referencedType->resolvedStub == structStub)
        return true;

    switch (typeDecl->metaType)
    {
        case StubTypeType::Simple:
        {
            if (auto classStub  = typeDecl->referencedType->resolvedStub->asClass())
            {
                for (auto classInnerStub  : classStub->stubs)
                {
                    if (auto classProp  = classInnerStub->asProperty())
                    {
                        if (IsTypeUsingStructDirectly(classProp->typeDecl, structStub))
                            return true;
                    }
                }
            }
        }

        case StubTypeType::StaticArrayType:
        return IsTypeUsingStructDirectly(typeDecl->innerType, structStub);
    }

    return false;
}

bool StubLibrary::checkClassProperties(IErrorHandler& err, StubClass* classStub)
{
    for (auto localClassStub  : classStub->stubs)
    {
        if (auto localProp = localClassStub->asProperty())
        {
            if (localProp->flags.test(StubFlag::Import))
            {
                if (!classStub->flags.test(StubFlag::Import))
                {
                    reportError(err, localProp, TempString("Property '{}' can't be imported because class '{}' is not an imported class", localProp->name, classStub->name));
                    return false;
                }
            }
            else
            {
                if (classStub->flags.test(StubFlag::Import))
                {
                    if (classStub->flags.test(StubFlag::Struct))
                    {
                        reportError(err, localProp, TempString("Unable to extend imported structure '{}' with additional scripted property '{}', this is not allowed. Did you forget the 'import' before var?", classStub->name, localProp->name));
                    }
                    else
                    {

                    }
                    return false;
                }
            }

            if (classStub->baseClass != nullptr)
            {
                if (auto baseProperty = classStub->baseClass->findStub(localProp->name))
                {
                    if (baseProperty->stubType == StubType::Property)
                    {
                        reportError(err, localProp, TempString("Property '{}' hides one defined in base class '{}' here: {}", localProp->name, baseProperty->owner->name, baseProperty->location));
                        return false;
                    }
                }
            }

            if (localProp->typeDecl && localProp->typeDecl->metaType == StubTypeType::Simple && localProp->typeDecl->referencedType->resolvedStub)
            {
                if (auto localPropClass = localProp->typeDecl->referencedType->resolvedStub->asClass())
                {
                    if (!localPropClass->flags.test(StubFlag::Struct))
                    {
                        reportError(err, localProp, TempString("Property '{}' uses class '{}' directly instead of using a pointer. Did you mean to use ptr<{}> ?", localProp->name, localPropClass->name, localPropClass->name));
                        return false;
                    }
                }
            }

            if (classStub->flags.test(StubFlag::Struct))
            {
                if (IsTypeUsingStructDirectly(localProp->typeDecl, classStub))
                {
                    reportError(err, localProp, TempString("Property '{}' uses type that creates a cycle on '{}'", localProp->name, classStub->name));
                    return false;
                }
            }
        }
    }

    return true;
}

bool StubLibrary::checkProperties(IErrorHandler& err)
{
    bool valid = true;

    Array<StubClass*> allClasses;
    allClasses.reserve(4096);
    m_primaryModule->extractClasses(allClasses);

    for (auto classPtr  : allClasses)
        valid &= checkClassProperties(err, classPtr);

    return valid;
}

bool StubLibrary::matchTypeSignature(const StubTypeDecl* a, const StubTypeDecl* b) const
{
    if (!a)
        return b == nullptr;
    if (!b)
        return a == nullptr;

    if (a->metaType != b->metaType)
        return false;

    switch (a->metaType)
    {
        case StubTypeType::Simple:
        case StubTypeType::Engine:
        case StubTypeType::ClassType:
        case StubTypeType::PtrType:
        case StubTypeType::WeakPtrType:
            return a->name == b->name;

        case StubTypeType::DynamicArrayType:
            return matchTypeSignature(a->innerType, b->innerType);

        case StubTypeType::StaticArrayType:
            if (a->arraySize != b->arraySize)
                return false;
            return matchTypeSignature(a->innerType, b->innerType);
    }

    return false;
}

bool StubLibrary::matchTypeSignature(const StubFunctionArg* a, const StubFunctionArg* b) const
{
    if (a->flags.test(StubFlag::Explicit) != b->flags.test(StubFlag::Explicit))
        return false;
    if (a->flags.test(StubFlag::Ref) != b->flags.test(StubFlag::Ref))
        return false;
    if (a->flags.test(StubFlag::Out) != b->flags.test(StubFlag::Out))
        return false;

    return matchTypeSignature(a->typeDecl, b->typeDecl);
}

bool StubLibrary::matchFunctionSignature(const StubFunction* a, const StubFunction* b) const
{
    if (a->args.size() != b->args.size())
        return false;

    for (uint32_t i=0; i<a->args.size(); ++i)
    {
        if (!matchTypeSignature(a->args[i], b->args[i]))
            return false;
    }

    return matchTypeSignature(a->returnTypeDecl, b->returnTypeDecl);
}

bool StubLibrary::linkFunction(IErrorHandler& err, StubFunction* ptr)
{
    // detect duplicated arguments, what a shame do do it here
    HashSet<StringID> argMap;
    for (auto arg  : ptr->args)
    {
        if (argMap.contains(arg->name))
        {
            reportError(err, arg, TempString("Function argument '{}' is already defined", arg->name));
            return false;
        }
        argMap.insert(arg->name);
    }

    // if we are a function inside a class find our base function
    const StubClass* funcClass = ptr->owner ? ptr->owner->asClass() : nullptr;
    if (funcClass)
    {
        // if we have base class link to our base function (for super call)
 		if (auto baseClass  = funcClass->baseClass)
		{
			while (baseClass)
			{
				if (auto stub  = baseClass->findStubLocal(ptr->name))
				{
					ptr->baseFunction = stub->asFunction();
					break;
				}

				baseClass = baseClass->baseClass;
			}
		}              
    }

    // some checks are required for signal functions
    if (ptr->flags.test(StubFlag::Signal))
    {
        if (funcClass == nullptr || funcClass->flags.test(StubFlag::Struct))
        {
            reportError(err, ptr, TempString("Signal '{}' should be defined inside a class", ptr->name));
            return false;
        }

        if (ptr->returnTypeDecl != nullptr)
        {
            if (!ptr->returnTypeDecl->isType<bool>())
            {
                reportError(err, ptr, TempString("Signal '{}' should not return any value or return bool", ptr->name));
                return false;
            }
        }

        if (!ptr->name.view().beginsWith("On"))
        {
            reportError(err, ptr, TempString("Signal '{}' name should start with \"On\" by convention", ptr->name));
            return false;
        }
    }

    if (ptr->flags.test(StubFlag::Import))
    {
        if (funcClass && !funcClass->flags.test(StubFlag::Import))
        {
            reportError(err, ptr, TempString("Function '{}' can't be imported because class '{}' is not an imported class", ptr->name, funcClass->name));
            return false;
        }
    }

    /*if (ptr->flags.test(StubFlag::Static))
    {
        if (funcClass == nullptr)
        {
            reportWarning(err, ptr, TempString("Static flag on function '{}' is meaningless outside a class", ptr->name));
        }
    }*/

    if (ptr->flags.test(StubFlag::Override))
    {
        if (ptr->baseFunction == nullptr)
        {
			if (funcClass != nullptr)
			{
				StringBuilder baseClasses;
				if (auto baseClass  = funcClass->baseClass)
				{
					while (baseClass)
					{
						if (!baseClasses.empty())
							baseClasses << ", ";
						baseClasses << baseClass->fullName();

						baseClass = baseClass->baseClass;
					}
				}

				reportError(err, ptr, TempString("Function '{}' is not a valid override since there's no matching function in any of the base classes: {}", ptr->name, baseClasses.c_str()));
			}
			else
			{
				reportError(err, ptr, TempString("Override function '{}' should be declared inside a class", ptr->name));
			}

            return false;
        }
    }
    else
    {
        if (ptr->baseFunction != nullptr)
        {
            reportError(err, ptr,  TempString("Function '{}' is not declared as override", ptr->name));
            return false;
        }
    }

    if (ptr->baseFunction)
    {
        if (ptr->baseFunction->flags.test(StubFlag::Final))
        {
            reportError(err, ptr, TempString("Function '{}' was declared as final in base class", ptr->name));
            return false;
        }

        if (!matchFunctionSignature(ptr, ptr->baseFunction))
        {
            reportError(err, ptr, TempString("Function {} does not match signature of the base function", ptr->name));
            return false;
        }
    }

    return true;
}

bool StubLibrary::linkFunctions(IErrorHandler& err)
{
    bool valid = true;

    // get all functions in this module
    Array<StubFunction*> allFunctions; // no inplace, we will have thousands of functions
    allFunctions.reserve(4096);
    m_primaryModule->extractGlobalFunctions(allFunctions);
    m_primaryModule->extractClassFunctions(allFunctions);

	// get other functions as well, we need to validate shit
	for (auto importModule  : m_primaryModule->imports)
	{
		importModule->extractGlobalFunctions(allFunctions);
		importModule->extractClassFunctions(allFunctions);
	}

    // process each function
    for (auto func  : allFunctions)
        valid &= linkFunction(err, func);

    return valid;
}

bool StubLibrary::createAutomaticClassFunctions(IErrorHandler& err)
{
    // get all classes in the primary module
    Array<StubClass*> allClasses;
    allClasses.reserve(4096);
    m_primaryModule->extractClasses(allClasses);

    // add constructor/destructor functions to classes
    for (auto classStub  : allClasses)
    {
        // the non-imported classes will have a ctor/dtor generated
        if (!classStub->flags.test(StubFlag::Import))
        {
            {
                auto ctor = createFunction(classStub->location, "__ctor"_id, classStub);
                ctor->flags |= StubFlag::Constructor;
            }

            {
                auto dtor = createFunction(classStub->location, "__dtor"_id, classStub);
                dtor->flags |= StubFlag::Destructor;
            }

            // TODO: copy function ?
            // TODO: compare operator ?
        }
    }

    return true;
}

bool StubLibrary::buildCastMatrix(IErrorHandler& err)
{
    // get global functions from all modules (even the imported ones)
    Array<StubFunction*> allFunctions;
    for (auto module  : m_modules)
        module->extractGlobalFunctions(allFunctions);

    // create and initialize type matrix
    m_typeCastMatrix.create();
    if (!m_typeCastMatrix->initialize(err, allFunctions))
        return false;
    return true;
}

//--

Stub* StubLibrary::createImportStub(const Stub* sourceStub, HashMap<const Stub*, Stub*>& cache)
{
    if (!sourceStub)
        return nullptr;

    Stub* ret = nullptr;
    if (cache.find(sourceStub, ret))
        return ret;

    // some stubs are NOT copied
    switch (sourceStub->stubType)
    {
        case StubType::Module:
        case StubType::Opcode:
        case StubType::ModuleImport:
            return nullptr;
    }

    ret = Stub::Create(m_mem, sourceStub->stubType);
    cache[sourceStub] = ret;

    ret->owner = createImportStub(sourceStub->owner, cache);
    ret->name = sourceStub->name;
    ret->flags = sourceStub->flags;
    ret->location.file = sourceStub->location.file ? createImportStub(sourceStub->location.file, cache)->asFile() : nullptr;
    ret->location.line = sourceStub->location.line;

    // regardless of the original state now we are an import :)
    ret->flags |= StubFlag::ImportDependency;

    return ret;
}

void StubLibrary::importFileData(const StubFile* source, StubFile* dest, HashMap<const Stub*, Stub*>& cache)
{
    dest->depotPath = source->depotPath;
    dest->absolutePath = source->absolutePath;

    for (auto sourceStub  : source->stubs)
    {
        if (auto destStub  = importStub(sourceStub, cache))
            dest->stubs.pushBack(destStub);
    }
}

void StubLibrary::importEnumData(const StubEnum* source, StubEnum* dest, HashMap<const Stub*, Stub*>& cache)
{
    dest->engineImportName = source->engineImportName;

    for (auto sourceOption  : source->options)
    {
        if (auto destStub = importStub(sourceOption, cache)->asEnumOption())
        {
            dest->options.pushBack(destStub);
            dest->optionsMap[destStub->name] = destStub;
        }
    }
}

void StubLibrary::importEnumOptionData(const StubEnumOption* source, StubEnumOption* dest, HashMap<const Stub*, Stub*>& cache)
{
    dest->assignedValue = source->assignedValue;
    dest->hasUserAssignedValue = source->hasUserAssignedValue;
}

void StubLibrary::importClassData(const StubClass* source, StubClass* dest, HashMap<const Stub*, Stub*>& cache)
{
	TRACE_INFO("Importing declaration of class '{}'", source->name);

    dest->baseClassName = source->baseClassName;
    dest->parentClassName = source->parentClassName;
    dest->engineImportName = source->engineImportName;

    for (auto sourceStub  : source->stubs)
    {
        if (auto destStub  = importStub(sourceStub, cache))
        {
            dest->stubs.pushBack(destStub);
            dest->stubMap[destStub->name] = destStub;
        }
    }
}

void StubLibrary::importPropertyData(const StubProperty* source, StubProperty* dest, HashMap<const Stub*, Stub*>& cache)
{
    dest->typeDecl = importStub(source->typeDecl, cache)->asTypeDecl();

    if (source->defaultValue)
        dest->defaultValue = importStub(source->defaultValue, cache)->asConstantValue();
}

void StubLibrary::importFunctionData(const StubFunction* source, StubFunction* dest, HashMap<const Stub*, Stub*>& cache)
{
    dest->operatorName = source->operatorName;
    dest->opcodeName = source->opcodeName;
    dest->aliasName = source->aliasName;
    dest->castCost = source->castCost;

    if (source->returnTypeDecl)
        dest->returnTypeDecl = importStub(source->returnTypeDecl, cache)->asTypeDecl();

    for (auto sourceStub  : source->args)
    {
        auto destStub  = importStub(sourceStub, cache)->asFunctionArg();
        dest->args.pushBack(destStub);
    }
}

void StubLibrary::importFunctionArgData(const StubFunctionArg* source, StubFunctionArg* dest, HashMap<const Stub*, Stub*>& cache)
{
    dest->typeDecl = importStub(source->typeDecl, cache)->asTypeDecl();
    if (source->defaultValue)
        dest->defaultValue = importStub(source->defaultValue, cache)->asConstantValue();
    dest->index = source->index;
}

void StubLibrary::importTypeNameData(const StubTypeName* source, StubTypeName* dest, HashMap<const Stub*, Stub*>& cache)
{
    dest->linkedType = importStub(source->linkedType, cache)->asTypeDecl();
}

void StubLibrary::importTypeRefData(const StubTypeRef* source, StubTypeRef* dest, HashMap<const Stub*, Stub*>& cache)
{
    // TODO: copy the resolved stub  to save on resolve time ?
    m_unresolvedTypes.pushBack(dest);
}

void StubLibrary::importTypeDeclData(const StubTypeDecl* source, StubTypeDecl* dest, HashMap<const Stub*, Stub*>& cache)
{
    dest->arraySize = source->arraySize;
    dest->metaType = source->metaType;

    if (source->innerType)
        dest->innerType = importStub(source->innerType, cache)->asTypeDecl();

    if (source->referencedType)
        dest->referencedType = importStub(source->referencedType, cache)->asTypeRef();

    m_unresolvedDeclarations.pushBack(dest);
}

void StubLibrary::importConstantData(const StubConstant* source, StubConstant* dest, HashMap<const Stub*, Stub*>& cache)
{
    dest->value = importStub(source->value, cache)->asConstantValue();
    dest->typeDecl = importStub(source->typeDecl, cache)->asTypeDecl();
}

void StubLibrary::importConstantValueData(const StubConstantValue* source, StubConstantValue* dest, HashMap<const Stub*, Stub*>& cache)
{
    dest->m_valueType = source->m_valueType;
    dest->value.u = source->value.u;

    if (source->text)
        dest->text = m_mem.strcpy(source->text.data());

    if (source->compoundType)
    {
        dest->compoundType = importStub(source->compoundType, cache)->asTypeDecl();

        for (auto sourceCompoundValue  : source->compoundVals)
            dest->compoundVals.pushBack(importStub(sourceCompoundValue, cache)->asConstantValue());
    }
}

Stub* StubLibrary::importStub(const Stub* sourceStub, HashMap<const Stub*, Stub*>& cache)
{
    // create the cloned stub
    auto ret  = createImportStub(sourceStub, cache);
    if (!ret)
        return nullptr;

    // copy data
#define HANDLE(__type) case StubType::__type: import##__type##Data(sourceStub->as##__type(), ret->as##__type(), cache); break;
    switch (sourceStub->stubType)
    {
        HANDLE(File);
        HANDLE(Class);
        HANDLE(TypeName);
        HANDLE(TypeDecl);
        HANDLE(TypeRef);
        HANDLE(Constant);
        HANDLE(ConstantValue);
        HANDLE(Enum);
        HANDLE(EnumOption);
        HANDLE(Property);
        HANDLE(Function);
        HANDLE(FunctionArg);
    }
#undef HANDLE

    return ret;
}

const StubModule* StubLibrary::importModule(const StubModule* sourceModule, StringID importName)
{
    ScopeTimer timer;

    // primary dependency
    if (importName == m_primaryModule->name)
        return m_primaryModule;

    // skip if already loaded
    for (auto module  : m_primaryModule->imports)
            if (module->name == importName)
                return module;

    // create import container
    auto importedModule  = m_mem.create<StubModule>();
    importedModule->owner = nullptr; // nothing owns the modules, by defintiion, they are top-level stubs
	importedModule->flags |= StubFlag::Import;
    importedModule->name = importName;
    m_primaryModule->imports.pushBack(importedModule);
    m_modules.pushBack(importedModule);

    // copy files and stubs
    HashMap<const Stub*, Stub*> importedStubCache;
    importedStubCache[sourceModule] = importedModule;
    for (auto sourceFile  : sourceModule->files)
    {
        if (auto importedFile  = importStub(sourceFile, importedStubCache))
            importedModule->files.pushBack(importedFile->asFile());
    }

    // map stubs
    for (auto sourceStub  : sourceModule->stubMap.values())
    {
        Stub* destStub = nullptr;
        importedStubCache.find(sourceStub, destStub);

        if (destStub)
            importedModule->stubMap[destStub->name] = destStub;
    }

    // gather some stats
    uint32_t numFunctions = 0;
    uint32_t numClasses = 0;
    uint32_t numConstants = 0;
    uint32_t numEnums = 0;
    for (auto stub  : importedStubCache.values())
    {
        switch (stub->stubType)
        {
            case StubType::Class: numClasses += 1; break;
            case StubType::Function: numFunctions += 1; break;
            case StubType::Constant: numConstants += 1; break;
            case StubType::Enum: numEnums += 1; break;
        }
    }

    TRACE_INFO("Imported {} cls(es), {} function(s), {} constant(s) and {} enum(s) from '{}' in {}", numClasses, numFunctions, numConstants, numEnums, importName, TimeInterval(timer.timeElapsed()));
    return importedModule;
}

//--

struct UsedStubs
{
public:
    UsedStubs()
    {
        m_usedStubs.reserve(10000);
        m_visitedStubs.reserve(10000);
    }

    HashSet<const Stub*> m_usedStubs;
    HashSet<const Stub*> m_visitedStubs;

    void visitGenericStub(const Stub* stub)
    {
        if (!stub || !m_visitedStubs.insert(stub))
            return;

        switch (stub->stubType)
        {
            case StubType::Class: visitStub(stub->asClass()); break;
            case StubType::Function: visitStub(stub->asFunction()); break;
            case StubType::FunctionArg: visitStub(stub->asFunctionArg()); break;
            case StubType::Property: visitStub(stub->asProperty()); break;
            case StubType::Enum: visitStub(stub->asEnum()); break;
            case StubType::TypeDecl: visitStub(stub->asTypeDecl()); break;
            case StubType::TypeRef: visitStub(stub->asTypeRef()); break;
            case StubType::ConstantValue: visitStub(stub->asConstantValue()); break;
        }
    }

    void visitStub(const StubClass* stub)
    {
        m_usedStubs.insert(stub);
        //for (auto classStub  : stub->stubs)
            //  visitStub(classStub);
    }

    void visitStub(const StubFunction* stub)
    {
        m_usedStubs.insert(stub);
        visitGenericStub(stub->returnTypeDecl);
        for (auto ptr  : stub->args)
            visitGenericStub(ptr);
    }

    void visitStub(const StubProperty* stub)
    {
        m_usedStubs.insert(stub);
        visitGenericStub(stub->typeDecl);
        //visitStub(stub->defaultValue);
    }

    void visitStub(const StubFunctionArg* stub)
    {
        m_usedStubs.insert(stub);
        visitGenericStub(stub->typeDecl);
        //visitStub(stub->defaultValue);
    }

    void visitStub(const StubConstantValue* stub)
    {
        //m_usedStubs.insert(stub);

    }

    void visitStub(const StubEnum* stub)
    {
        m_usedStubs.insert(stub);
    }

    void visitStub(const StubTypeDecl* stub)
    {
        m_usedStubs.insert(stub);
        visitGenericStub(stub->referencedType);
        visitGenericStub(stub->innerType);
    }

    void visitStub(const StubTypeRef* stub)
    {
        m_usedStubs.insert(stub);
        visitGenericStub(stub->resolvedStub);
    }

    void exploreOpcode(const StubOpcode* op)
    {
        exploreStub(op->stub);

        switch (op->op)
        {
            case Opcode::Constructor:
            {
                auto classStub  = op->stub->asTypeDecl()->classType();
                for (auto prop  : classStub->stubs)
                    exploreStub(prop);
                break;
            }
        }
    }

    void exploreStub(const Stub* stub)
    {
        if (!stub)
            return;

        TRACE_INFO("Exploring {}: {}", stub->stubType, stub->fullName());

        if (auto classStub  = stub->asClass())
        {
            visitGenericStub(classStub);
            for (auto ptr  : classStub->stubs)
                exploreStub(ptr);
        }
        else if (auto funcStub  = stub->asFunction())
        {
            visitGenericStub(funcStub);
            for (auto ptr  : funcStub->opcodes)
                exploreOpcode(ptr);
        }
        else if (auto moduleStub  = stub->asModule())
        {
            for (auto ptr  : moduleStub->stubMap.values())
                exploreStub(ptr);
        }
        else
        {
            visitGenericStub(stub);
        }
    }
};

void StubLibrary::pruneUnusedImports()
{
    ScopeTimer timer;

    // nothing imported
    if (m_primaryModule->imports.empty())
        return;

    // visit stubs used by the code in the primary module
    UsedStubs usedStubs;
    usedStubs.exploreStub(m_primaryModule);
    TRACE_INFO("Found {} used stubs ({} visited)", usedStubs.m_usedStubs.size(), usedStubs.m_visitedStubs.size());
    for (auto ptr  : usedStubs.m_usedStubs)
        TRACE_INFO("Used {}: {}", ptr->stubType, ptr->fullName());

    // remove stuff that we don't use for imports
    uint32_t numRemovedStubs = 0;
    auto imports = std::move(m_primaryModule->imports);
    for (auto importedModule  : imports)
    {
        const_cast<StubModule*>(importedModule)->prune(usedStubs.m_usedStubs, numRemovedStubs);
        if (importedModule->stubMap.empty() || importedModule->files.empty())
        {
            TRACE_INFO("Nothing was left imported from module '{}', dropping dependency", importedModule->name);
        }
        else
        {
            m_primaryModule->imports.pushBack(importedModule);
        }
    }

    // stats
    TRACE_INFO("Pruned {} unused stubs from imported modules in {}", numRemovedStubs, timer);
}

//--

END_BOOMER_NAMESPACE_EX(script)
