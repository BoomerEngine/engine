/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptPortableStubs.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//--

RTTI_BEGIN_TYPE_ENUM(StubType);
    RTTI_ENUM_OPTION(None);
    RTTI_ENUM_OPTION(Module);
    RTTI_ENUM_OPTION(File);
    RTTI_ENUM_OPTION(TypeName);
    RTTI_ENUM_OPTION(TypeDecl);
    RTTI_ENUM_OPTION(TypeRef);
    RTTI_ENUM_OPTION(Constant);
    RTTI_ENUM_OPTION(ConstantValue);
    RTTI_ENUM_OPTION(EnumOption);
    RTTI_ENUM_OPTION(Enum);
    RTTI_ENUM_OPTION(Property);
    RTTI_ENUM_OPTION(Function);
    RTTI_ENUM_OPTION(FunctionArg);
    RTTI_ENUM_OPTION(Class);
    RTTI_ENUM_OPTION(Opcode);
RTTI_END_TYPE();

//--

IStubWriter::~IStubWriter()
{}

IStubReader::~IStubReader()
{}

//--

void StubLocation::print(IFormatStream& f) const
{
    if (file != nullptr)
    {
        f << file->absolutePath;
        f << "(";
        f << line;
        f << ")";
    }
    else
    {
        f << "unknown location";
    }
}

void StubLocation::write(IStubWriter& f) const
{
    f.writeRef(file);
    f.writeUint32(line);
}

void StubLocation::read(IStubReader& f)
{
    file = f.readRef<StubFile>();
    line = f.readUint32();
}

//--

Stub* Stub::Create(LinearAllocator& mem, StubType type)
{
#define TEST(x) case StubType::x: return mem.create<Stub##x>();
    switch (type)
    {
        TEST(Module);
        TEST(ModuleImport);
        TEST(File);
        TEST(TypeName);
        TEST(TypeDecl);
        TEST(TypeRef);
        TEST(Constant);
        TEST(ConstantValue);
        TEST(EnumOption);
        TEST(Enum);
        TEST(Property);
        TEST(Function);
        TEST(FunctionArg);
        TEST(Class);
        TEST(Opcode);
    }
#undef TEST

    TRACE_ERROR("Trying to create unknown script stub object {}", (uint32_t)type);
    return nullptr;
}

static void AssembleName(const Stub* stub, StringBuilder& f)
{
    if (stub->owner)
        AssembleName(stub->owner, f);

    if (stub->name)
    {
        if (!f.empty())
            f << ".";
        f << stub->name;
    }
}

StringBuf Stub::fullName() const
{
    StringBuilder txt;
    AssembleName(this, txt);
    return txt.toString();
}

void Stub::write(IStubWriter& f) const
{
    f.writeRef(owner);
    location.write(f);
    f.writeUint32(flags.rawValue());
    f.writeName(name);
}

void Stub::read(IStubReader& f)
{
    owner = f.readRef();
    location.read(f);
    flags = StubFlags(f.readUint32());
    name = f.readName();
}

    void Stub::postLoad()
    {}

//--

const Stub* StubModule::findStub(StringID name) const
{
    const Stub* ret = nullptr;
    stubMap.find(name, ret);
    return ret;
}

void StubModule::extractGlobalFunctions(Array<StubFunction*>& outFunctions) const
{
    for (auto file  : files)
        file->extractGlobalFunctions(outFunctions);
}

void StubModule::extractClassFunctions(Array<StubFunction*>& outFunctions) const
{
    for (auto file  : files)
        file->extractClassFunctions(outFunctions);
}

void StubModule::extractEnums(Array<StubEnum*>& outEnums) const
{
    for (auto file  : files)
        file->extractEnums(outEnums);
}

void StubModule::extractClasses(Array<StubClass*>& outClasses) const
{
    for (auto file  : files)
        file->extractClasses(outClasses);
}

void StubModule::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    stubMap.clear();

    auto files  = std::move(this->files);
    for (auto file  : files)
    {
        const_cast<StubFile*>(file)->prune(usedStubs, numRemoved);
        if (!file->stubs.empty())
        {
            files.pushBack(file);

            for (auto stub  : file->stubs)
                stubMap[stub->name] = stub;
        }
        else
        {
            TRACE_INFO("Pruned file '{}' because it's not used", file->depotPath);
            numRemoved += 1;
        }
    }

    auto imports  = std::move(this->imports);
    for (auto import  : imports)
    {
        const_cast<StubModule*>(import)->prune(usedStubs, numRemoved);
        if (!import->files.empty())
        {
            imports.pushBack(import);
        }
        else
        {
            TRACE_INFO("Pruned import module '{}' because it's not used", import->name);
            numRemoved += 1;
        }
    }
}

void StubModule::write(IStubWriter& f) const
{
    Stub::write(f);
    f.writeRefList(files);
    f.writeRefList(imports);
}

void StubModule::read(IStubReader& f)
{
    Stub::read(f);
    f.readRefList(files);
    f.readRefList(imports);
}

void StubModule::postLoad()
{
    Stub::postLoad();

    stubMap.clear();

    for (auto file  : files)
        for (auto stub  : file->stubs)
            if (stub)
                stubMap[stub->name] = stub;
}

//--

void StubModuleImport::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    if (m_importedModuleData)
    {
        const_cast<StubModule *>(m_importedModuleData)->prune(usedStubs, numRemoved);

        if (m_importedModuleData->files.empty())
            m_importedModuleData = nullptr;
    }
}

void StubModuleImport::write(IStubWriter& f) const
{
    Stub::write(f);
    f.writeRef(m_importedModuleData);
}

void StubModuleImport::read(IStubReader& f)
{
    Stub::read(f);
    f.readRef(m_importedModuleData);
}

//--

void StubFile::extractGlobalFunctions(Array<StubFunction*>& outFunctions) const
{
    for (auto stub  : stubs)
        if (stub->stubType == StubType::Function)
            outFunctions.pushBack(static_cast<StubFunction*>(stub));
}

void StubFile::extractClassFunctions(Array<StubFunction*>& outFunctions) const
{
    for (auto stub  : stubs)
    {
        if (stub->stubType == StubType::Class)
        {
            auto classStub  = static_cast<StubClass *>(stub);
            classStub->extractFunctions(outFunctions);
        }
    }
}

void StubFile::extractEnums(Array<StubEnum*>& outEnums) const
{
    for (auto stub  : stubs)
    {
        if (stub->stubType == StubType::Class)
        {
            auto classStub  = static_cast<StubClass *>(stub);
            classStub->extractEnums(outEnums);
        }
        else if (stub->stubType == StubType::Enum)
        {
            auto enumStub  = static_cast<StubEnum *>(stub);
            outEnums.pushBack(enumStub);
        }
    }
}

void StubFile::extractClasses(Array<StubClass*>& outClasses) const
{
    for (auto stub  : stubs)
    {
        if (stub->stubType == StubType::Class)
        {
            auto classStub  = static_cast<StubClass *>(stub);
            outClasses.pushBack(classStub);

            classStub->extractClasses(outClasses);
        }
    }
}

void StubFile::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    auto oldList  = std::move(stubs);
    for (auto ptr  : oldList)
    {
        if (usedStubs.contains(ptr))
            stubs.pushBack(ptr);
        else
            numRemoved += 1;
    }

    for (auto ptr  : stubs)
        ptr->prune(usedStubs, numRemoved);
}


void StubFile::write(IStubWriter& f) const
{
    Stub::write(f);

    f.writeRefList(stubs);
    f.writeString(depotPath);
    f.writeString(absolutePath);
}

void StubFile::read(IStubReader& f)
{
    Stub::read(f);

    f.readRefList(stubs);
    depotPath = StringBuf(f.readString());
    absolutePath = StringBuf(f.readString());
}

//--

void StubTypeName::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    if (linkedType)
    {
        ASSERT(usedStubs.contains(linkedType));
    }
}

void StubTypeName::write(IStubWriter& f) const
{
    Stub::write(f);
    f.writeRef(linkedType);
}

void StubTypeName::read(IStubReader& f)
{
    Stub::read(f);
    f.readRef(linkedType);
}

//--

void StubEnumOption::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{

}

void StubEnumOption::write(IStubWriter& f) const
{
    Stub::write(f);

    f.writeBool(hasUserAssignedValue);
    if (hasUserAssignedValue)
        f.writeInt64(assignedValue);
}

void StubEnumOption::read(IStubReader& f)
{
    Stub::read(f);

    hasUserAssignedValue = f.readBool();
    if (hasUserAssignedValue)
        assignedValue = f.readInt64();
    else
        assignedValue = 0;
}


//--

const StubEnumOption* StubEnum::findOption(StringID name) const
{
    const StubEnumOption* ret = nullptr;
    optionsMap.find(name, ret);
    return ret;
}

void StubEnum::write(IStubWriter& f) const
{
    Stub::write(f);

    if (flags.test(StubFlag::Import))
        f.writeName(engineImportName);

    f.writeRefList(options);
}

void StubEnum::read(IStubReader& f)
{
    Stub::read(f);

    if (flags.test(StubFlag::Import))
        engineImportName = f.readName();

    f.readRefList(options);
}

void StubEnum::postLoad()
{
    Stub::postLoad();

        optionsMap.clear();
        for (auto option  : options)
            optionsMap[option->name] = option;
}

void StubEnum::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    numRemoved += options.size();
    options.clear();
    optionsMap.clear();
}

//--

void StubTypeRef::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    if (resolvedStub)
    {
        ASSERT(usedStubs.contains(resolvedStub));
    }
}

void StubTypeRef::write(IStubWriter& f) const
{
    Stub::write(f);

    //ASSERT(resolvedStub != nullptr)
    f.writeRef(resolvedStub);
}

void StubTypeRef::read(IStubReader& f)
{
    Stub::read(f);

    f.readRef(resolvedStub);
    //ASSERT(resolvedStub != nullptr)
}

StringBuf StubTypeRef::fullName() const
{
    if (resolvedStub)
        return resolvedStub->fullName();
    else
        return StringBuf(name.view());
}

const StubClass* StubTypeRef::classType() const
{
    if (resolvedStub)
        return resolvedStub->asClass();
    return nullptr;
}

const StubEnum* StubTypeRef::enumType() const
{
    if (resolvedStub)
        return resolvedStub->asEnum();
    return nullptr;
}

bool StubTypeRef::isEnumType() const
{
    return resolvedStub && resolvedStub->stubType == StubType::Enum;
}

bool StubTypeRef::isClassType() const
{
    return resolvedStub && resolvedStub->stubType == StubType::Class;
}

bool StubTypeRef::Match(const StubTypeRef* a, const StubTypeRef* b)
{
    if (a->resolvedStub && b->resolvedStub)
        return a->resolvedStub == b->resolvedStub;

    if (a->owner == b->owner)
        return a->name == b->name;

    return false;
}

//--

StringBuf StubTypeDecl::fullName() const
{
    StringBuilder txt;
    printableName(txt);
    return txt.toString();
}

void StubTypeDecl::printableName(IFormatStream& txt) const
{
    switch (metaType)
    {
        case StubTypeType::Simple:
            txt << referencedType->fullName();
            break;

        case StubTypeType::Engine:
            txt << name;
            break;

        case StubTypeType::ClassType:
            txt << "class<";
            txt << referencedType->fullName();
            txt << ">";
            break;

        case StubTypeType::PtrType:
            txt << "ptr<";
            txt << referencedType->fullName();
            txt << ">";
            break;

        case StubTypeType::WeakPtrType:
            txt << "weak<";
            txt << referencedType->fullName();
            txt << ">";
            break;

        case StubTypeType::DynamicArrayType:
            txt << "array<";
            innerType->printableName(txt);
            txt << ">";
            break;

        case StubTypeType::StaticArrayType:
            innerType->printableName(txt);
            txt.appendf("[{}]", arraySize);
            break;

        default:
            txt << "unknown";
    }
}

void StubTypeDecl::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    if (referencedType)
    {
        ASSERT(usedStubs.contains(referencedType));
    }

    if (innerType)
    {
        ASSERT(usedStubs.contains(innerType));
    }
}

void StubTypeDecl::write(IStubWriter& f) const
{
    Stub::write(f);

    f.writeUint8((uint8_t)metaType);

    if (metaType == StubTypeType::Simple || metaType == StubTypeType::ClassType || metaType == StubTypeType::PtrType || metaType == StubTypeType::WeakPtrType)
    {
        f.writeRef(referencedType);
    }
    else if (metaType == StubTypeType::DynamicArrayType)
    {
        f.writeRef(innerType);
    }
    else if (metaType == StubTypeType::StaticArrayType)
    {
        f.writeRef(innerType);
        f.writeUint32(arraySize);
    }
}

void StubTypeDecl::read(IStubReader& f)
{
    Stub::read(f);

    metaType = (StubTypeType) f.readUint8();

    if (metaType == StubTypeType::Simple || metaType == StubTypeType::ClassType || metaType == StubTypeType::PtrType || metaType == StubTypeType::WeakPtrType)
    {
        f.readRef(referencedType);
    }
    else if (metaType == StubTypeType::DynamicArrayType)
    {
        innerType = f.readRef<StubTypeDecl>();
    }
    else if (metaType == StubTypeType::StaticArrayType)
    {
        innerType = f.readRef<StubTypeDecl>();
        arraySize = f.readUint32();
    }
}

bool StubTypeDecl::isSimpleType() const
{
    return metaType == StubTypeType::Simple || metaType == StubTypeType::Engine;
}

bool StubTypeDecl::isClassType() const
{
    return metaType == StubTypeType::ClassType;
}

bool StubTypeDecl::isSharedPointerType() const
{
    return metaType == StubTypeType::PtrType;
}

bool StubTypeDecl::isWeakPointerType() const
{
    return metaType == StubTypeType::WeakPtrType;
}

bool StubTypeDecl::isPointerType() const
{
    return metaType == StubTypeType::WeakPtrType || metaType == StubTypeType::PtrType;
}

bool StubTypeDecl::Match(const StubTypeDecl* a, const StubTypeDecl* b)
{
    if (!a || !b)
        return false;

    if (a->metaType != b->metaType)
        return false;

    if (a->metaType == StubTypeType::Engine)
        return a->name == b->name;

    if (a->metaType == StubTypeType::DynamicArrayType)
        return Match(a->innerType, b->innerType);

    if (a->metaType == StubTypeType::StaticArrayType)
        return Match(a->innerType, b->innerType) && (a->arraySize == b->arraySize);

    ASSERT(a->referencedType != nullptr);
    ASSERT(b->referencedType != nullptr);

    return StubTypeRef::Match(a->referencedType, b->referencedType);
}

const StubClass* StubTypeDecl::classType() const
{
    if (referencedType)
        return referencedType->classType();
    return nullptr;
}

const StubEnum* StubTypeDecl::enumType() const
{
    if (referencedType)
        return referencedType->enumType();
    return nullptr;
}

bool StubTypeDecl::isEnumType() const
{
    if (referencedType)
        return referencedType->isEnumType();
    return false;
}

//--

void StubConstantValue::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    if (compoundType)
    {
        ASSERT(usedStubs.contains(compoundType));
    }

    for (auto subValue  : compoundVals)
    {
        ASSERT(usedStubs.contains(subValue));
    }
}

void StubConstantValue::write(IStubWriter& f) const
{
    Stub::write(f);

    f.writeEnum(m_valueType);

    switch (m_valueType)
    {
        case StubConstValueType::Integer:
        {
            f.writeInt64(value.i);
            break;
        }

        case StubConstValueType::Bool:
        {
            f.writeBool(value.i != 0);
            break;
        }

        case StubConstValueType::Unsigned:
        {
            f.writeUint64(value.u);
            break;
        }

        case StubConstValueType::Float:
        {
            f.writeDouble(value.f);
            break;
        }

        case StubConstValueType::String:
        {
            f.writeString(text);
            break;
        }

        case StubConstValueType::Name:
        {
            f.writeName(StringID(text));
            break;
        }

        case StubConstValueType::Compound:
        {
            f.writeRef(compoundType);
            f.writeRefList(compoundVals);
            break;
        }
    }
}

void StubConstantValue::read(IStubReader& f)
{
    Stub::read(f);

    f.readEnum(m_valueType);

    switch (m_valueType)
    {
        case StubConstValueType::Integer:
        {
            value.i = f.readInt64();
            break;
        }

        case StubConstValueType::Bool:
        {
            value.i = f.readBool() ? 1 : 0;
            break;
        }

        case StubConstValueType::Unsigned:
        {
            value.u = f.readUint64();
            break;
        }

        case StubConstValueType::Float:
        {
            value.f = f.readDouble();
            break;
        }

        case StubConstValueType::String:
        {
            text = f.readString();
            break;
        }

        case StubConstValueType::Name:
        {
            text = f.readName().view();
            break;
        }

        case StubConstValueType::Compound:
        {
            f.readRef(compoundType);
            f.readRefList(compoundVals);
            break;
        }
    }
}

//--

void StubConstant::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    ASSERT(usedStubs.contains(value));
    ASSERT(usedStubs.contains(typeDecl));
}

void StubConstant::write(IStubWriter& f) const
{
    Stub::write(f);

    f.writeRef(typeDecl);
    f.writeRef(value);
}

void StubConstant::read(IStubReader& f)
{
    Stub::read(f);

    typeDecl = f.readRef<StubTypeDecl>();
    value = f.readRef<StubConstantValue>();
}

//--

void StubProperty::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    ASSERT(usedStubs.contains(typeDecl));
    const_cast<StubTypeDecl*>(typeDecl)->prune(usedStubs, numRemoved);
}

void StubProperty::write(IStubWriter& f) const
{
    Stub::write(f);

    f.writeRef(typeDecl);
    f.writeRef(defaultValue);
}

void StubProperty::read(IStubReader& f)
{
    Stub::read(f);

    typeDecl = f.readRef<StubTypeDecl>();
    defaultValue = f.readRef<StubConstantValue>();
}

//--

void StubFunctionArg::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    ASSERT(usedStubs.contains(typeDecl));
    const_cast<StubTypeDecl*>(typeDecl)->prune(usedStubs, numRemoved);
}

void StubFunctionArg::write(IStubWriter& f) const
{
    Stub::write(f);

    f.writeInt16(index);
    f.writeRef(typeDecl);
    f.writeRef(defaultValue);
}

void StubFunctionArg::read(IStubReader& f)
{
    Stub::read(f);

    index = f.readInt16();
    typeDecl = f.readRef<StubTypeDecl>();
    defaultValue = f.readRef<StubConstantValue>();
}

//--

const StubFunctionArg* StubFunction::findFunctionArg(StringID name) const
{
    for (auto arg  : args)
        if (arg->name == name)
            return arg;

    return nullptr;
}

void StubFunction::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    if (returnTypeDecl)
    {
        ASSERT(usedStubs.contains(returnTypeDecl));
        const_cast<StubTypeDecl*>(returnTypeDecl)->prune(usedStubs, numRemoved);
    }

    for (auto arg  : args)
    {
        ASSERT(usedStubs.contains(arg));
        const_cast<StubFunctionArg*>(arg)->prune(usedStubs, numRemoved);
    }

    numRemoved += opcodes.size();
    opcodes.clear();

    baseFunction = nullptr;
    parentFunction = nullptr;
}

void StubFunction::write(IStubWriter& f) const
{
    Stub::write(f);

    if (flags.test(StubFlag::Operator))
        f.writeName(operatorName);

    if (flags.test(StubFlag::Cast))
        f.writeInt8(castCost);

    if (flags.test(StubFlag::Opcode))
        f.writeName(opcodeName);

    if (flags.test(StubFlag::Function))
        f.writeName(aliasName);

    f.writeRef(returnTypeDecl);
    f.writeRef(baseFunction);
    f.writeRef(parentFunction);
    f.writeUint64(codeHash);

    f.writeRefList(args);

    if (!flags.test(StubFlag::Opcode))
        f.writeRefList(opcodes);
}

void StubFunction::read(IStubReader& f)
{
    Stub::read(f);

    if (flags.test(StubFlag::Operator))
        operatorName = f.readName();

    if (flags.test(StubFlag::Cast))
        castCost = f.readInt8();

    if (flags.test(StubFlag::Opcode))
        opcodeName = f.readName();

    if (flags.test(StubFlag::Function))
        aliasName = f.readName();

    f.readRef(returnTypeDecl);
    f.readRef(baseFunction);
    f.readRef(parentFunction);
    codeHash = f.readUint64();

    f.readRefList(args);

    if (!flags.test(StubFlag::Opcode))
        f.readRefList(opcodes);
}

//--

const Stub* StubClass::findStubLocal(StringID name) const
{
    const Stub* ret = nullptr;
    stubMap.find(name, ret);
    return ret;
}

void StubClass::extractFunctions(Array<StubFunction*>& outFunctions) const
{
    for (auto stub  : stubs)
    {
        if (stub->stubType == StubType::Function)
        {
            auto functionStub  = static_cast<StubFunction*>(stub);
            outFunctions.pushBack(functionStub);
        }
        else if (stub->stubType == StubType::Class)
        {
            auto classStub  = static_cast<StubClass*>(stub);
            classStub->extractFunctions(outFunctions);
        }
    }
}

void StubClass::extractClasses(Array<StubClass*>& outClasses) const
{
    for (auto stub  : stubs)
    {
        if (stub->stubType == StubType::Class)
        {
            auto classStub  = static_cast<StubClass*>(stub);
            outClasses.pushBack(classStub);

            classStub->extractClasses(outClasses);
        }
    }
}

void StubClass::extractEnums(Array<StubEnum*>& outEnums) const
{
    for (auto stub  : stubs)
    {
        if (stub->stubType == StubType::Class)
        {
            auto classStub  = static_cast<StubClass*>(stub);
            classStub->extractEnums(outEnums);
        }
        else if (stub->stubType == StubType::Enum)
        {
            auto enumStub  = static_cast<StubEnum*>(stub);
            outEnums.pushBack(enumStub);
        }
    }
}

void StubClass::findAliasedFunctions(StringID name, uint32_t expectedArgumentCount, HashSet<const StubFunction*>& mutedFunctions, Array<const StubFunction*>& outAliasedFunctions) const
{
    for (auto stub  : stubs)
    {
        if (const auto* func  = stub->asFunction())
        {
            if (func->aliasName == name && func->args.size() == expectedArgumentCount)
            {
                if (mutedFunctions.contains(func))
                    continue;

                outAliasedFunctions.pushBack(func);

                while (func)
                {
                    mutedFunctions.insert(func);
                    func = func->baseFunction;
                }
            }
        }
    }

    if (baseClass)
        baseClass->findAliasedFunctions(name, expectedArgumentCount, mutedFunctions, outAliasedFunctions);
}

bool StubClass::is(const StubClass* baseClass) const
{
    if (baseClass == this)
        return true;

    if (baseClass)
        return baseClass->is(baseClass);

    return (baseClass == nullptr);
}

const Stub* StubClass::findStub(StringID name) const
{
    const Stub* ret = nullptr;
    if (stubMap.find(name, ret))
        return ret;

    if (baseClass != nullptr)
        return baseClass->findStub(name);

    return nullptr;
}

void StubClass::write(IStubWriter& f) const
{
    Stub::write(f);

    f.writeName(baseClassName);
    f.writeName(parentClassName);
    f.writeName(engineImportName);

    f.writeRef(baseClass);
    f.writeRefList(derivedClasses);
    f.writeRef(parentClass);
    f.writeRefList(childClasses);

    f.writeRefList(stubs);
}

void StubClass::read(IStubReader& f)
{
    Stub::read(f);

    baseClassName = f.readName();
    parentClassName = f.readName();
    engineImportName = f.readName();

    f.readRef(baseClass);
    f.readRefList(derivedClasses);
    f.readRef(parentClass);
    f.readRefList(childClasses);

    f.readRefList(stubs);
}

void StubClass::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    baseClass = nullptr;
    derivedClasses.reset();
    parentClass = nullptr;
    childClasses.reset();

    auto oldStubs = std::move(stubs);
    stubMap.clear();
    for (auto stub  : oldStubs)
    {
        if (usedStubs.contains(stub))
        {
            stubs.pushBack(stub);
            stubMap[stub->name] = stub;

            stub->prune(usedStubs, numRemoved);
        }
        else
        {
            numRemoved += 1;
        }
    }
}

void StubClass::postLoad()
{
    Stub::postLoad();

    stubMap.clear();
    for (auto stub  : stubs)
        stubMap[stub->name] = stub;
}

//--

StubOpcodeValue::StubOpcodeValue()
{
    u = 0;
}

//--

void StubOpcode::print(IFormatStream& f) const
{
    f.append(GetEnumValueName(op));

    switch (op)
    {
        case Opcode::IntConst1:
        case Opcode::IntConst2:
        case Opcode::IntConst4:
        case Opcode::IntConst8:
        {
            f.appendf(", signed value = {}", value.i);
            break;
        }

        case Opcode::UintConst1:
        case Opcode::UintConst2:
        case Opcode::UintConst4:
        case Opcode::UintConst8:
        {
            f.appendf(", unsigned value = {}", value.i);
            break;
        }

        case Opcode::FloatConst:
        {
            f.appendf(", float value = {}", value.f);
            break;
        }

        case Opcode::DoubleConst:
        {
            f.appendf(", double value = {}", value.d);
            break;
        }

        case Opcode::NameConst:
        case Opcode::StringConst:
        {
            f.appendf(", text = '{}'", value.text);
            break;
        }

        case Opcode::EnumConst:
        {
            f.appendf(", enum = {}, option = '{}'", stub->name, value.text);
            break;
        }

        case Opcode::TestEqual:
        case Opcode::TestNotEqual:
        case Opcode::DynamicCast:
        case Opcode::MetaCast:
        case Opcode::New:
        case Opcode::ContextFromRef:
        case Opcode::ContextFromPtr:
        case Opcode::ContextFromPtrRef:
        case Opcode::ContextFromValue:
        case Opcode::LoadAny:
        case Opcode::AssignAny:
        case Opcode::ReturnAny:
        {
            if (stub)
                f.appendf(", type = '{}'", stub->name);
            break;
        }

        case Opcode::StructMemberRef:
        case Opcode::StructMember:
        {
            if (stub)
                f.appendf(", name = '{}'", stub->name);
            break;
        }

        case Opcode::LocalCtor:
        case Opcode::LocalDtor:
        case Opcode::LocalVar:
        {
            f.appendf(", name = '{}'", value.name);
            if (stub)
                f.appendf(", type = '{}'", stub->name);
            break;
        }

        case Opcode::ContextCtor:
        case Opcode::ContextDtor:
        case Opcode::ContextExternalCtor:
        case Opcode::ContextExternalDtor:
        case Opcode::ContextVar:
        {
            f.appendf(", name = '{}'", value.name);
            break;
        }

        case Opcode::ParamVar:
        {
            f.appendf(", name = '{}', offset = '{}'", value.name, value.u);

            if (stub)
                f.appendf(", type = '{}'", stub->name);

            break;
        }

        case Opcode::FinalFunc:
        case Opcode::StaticFunc:
        case Opcode::VirtualFunc:
        {
            f.appendf(", func = '{}', class", stub->name);
            break;
        }

        case Opcode::Label:
        {
            f.appendf(", id = '{}'", (uint64_t)this);
            break;
        }
    }

    if (target != nullptr)
        f.appendf(", target= '{}'", (uint64_t)target);
}

void StubOpcode::prune(const TUsedStubs& usedStubs, uint32_t& numRemoved)
{
    if (stub != nullptr)
        ASSERT(usedStubs.contains(stub));

    if (target != nullptr)
        ASSERT(usedStubs.contains(target));
}

void StubOpcode::write(IStubWriter& f) const
{
    Stub::write(f);

    f.writeEnum(op);

    switch (op)
    {
        case Opcode::IntConst1:
            f.writeInt8(value.i);
            break;

        case Opcode::UintConst1:
            f.writeUint8(value.u);
            break;

        case Opcode::IntConst2:
            f.writeInt16(value.i);
            break;

        case Opcode::UintConst2:
            f.writeUint16(value.u);
            break;

        case Opcode::IntConst4:
            f.writeInt32(value.i);
            break;

        case Opcode::UintConst4:
            f.writeUint32(value.u);
            break;

        case Opcode::IntConst8:
            f.writeInt64(value.i);
            break;

        case Opcode::UintConst8:
            f.writeUint64(value.u);
            break;

        case Opcode::FloatConst:
            f.writeFloat(value.f);
            break;

        case Opcode::DoubleConst:
            f.writeDouble(value.d);
            break;

        case Opcode::StringConst:
            f.writeString(value.text);
            break;

        case Opcode::NameConst:
            f.writeName(value.name);
            break;

        case Opcode::TestEqual:
        case Opcode::TestNotEqual:
        case Opcode::DynamicCast:
        case Opcode::MetaCast:
        case Opcode::New:
            f.writeRef(stub);
            break;

        case Opcode::AssignAny:
            f.writeRef(stub); // hmm
            break;

        case Opcode::Jump:
        case Opcode::JumpIfFalse:
            f.writeRef(target);
            break;

        case Opcode::LoadAny:
        case Opcode::ReturnAny:
        case Opcode::ReturnLoad1:
        case Opcode::ReturnLoad2:
        case Opcode::ReturnLoad4:
        case Opcode::ReturnLoad8:
        case Opcode::ReturnDirect:
            f.writeRef(stub);
            break;

        case Opcode::ParamVar:
            f.writeInt8(value.i);
            break;

        case Opcode::LogicOr:
        case Opcode::LogicAnd:
            f.writeRef(target);
            break;

        case Opcode::LocalVar:
        case Opcode::LocalCtor:
        case Opcode::LocalDtor:
            f.writeRef(stub);
            f.writeName(value.name);
            f.writeInt16(value.i);
            break;

        case Opcode::StructMemberRef:
        case Opcode::StructMember:
            f.writeRef(stub);

        case Opcode::ContextVar:
            f.writeRef(stub);
            f.writeName(value.name);
            break;

        case Opcode::ContextCtor:
        case Opcode::ContextDtor:
            f.writeRef(stub);
            break;

        case Opcode::ContextFromRef:
        case Opcode::ContextFromPtr:
        case Opcode::ContextFromPtrRef:
            f.writeRef(stub);
            f.writeRef(target);
            break;

        case Opcode::ContextFromValue:
            f.writeRef(stub);
            break;

        case Opcode::FinalFunc:
        case Opcode::StaticFunc:
        case Opcode::VirtualFunc:
            f.writeRef(stub);
            f.writeUint32(value.u);
            break;

        case Opcode::Constructor:
            f.writeRef(stub);
            f.writeUint8(value.u);
            break;

        case Opcode::EnumToInt32:
        case Opcode::EnumToInt64:
        case Opcode::EnumToName:
        case Opcode::EnumToString:
        case Opcode::Int32ToEnum:
        case Opcode::Int64ToEnum:
        case Opcode::NameToEnum:
        case Opcode::ClassConst:
            f.writeRef(stub);
            break;

        case Opcode::EnumConst:
            f.writeRef(stub);
            f.writeName(value.name);
            break;
    }
}

void StubOpcode::read(IStubReader& f)
{
    Stub::read(f);

    f.readEnum(op);
    //TRACE_INFO("Loaded op '{}'", op);

    switch (op)
    {
        case Opcode::IntConst1:
            value.i = f.readInt8();
            break;

        case Opcode::UintConst1:
            value.u = f.readUint8();
            break;

        case Opcode::IntConst2:
            value.i = f.readInt16();
            break;

        case Opcode::UintConst2:
            value.u = f.readUint16();
            break;

        case Opcode::IntConst4:
            value.i = f.readInt32();
            break;

        case Opcode::UintConst4:
            value.u = f.readUint32();
            break;

        case Opcode::IntConst8:
            value.i = f.readInt64();
            break;

        case Opcode::UintConst8:
            value.u = f.readUint64();
            break;

        case Opcode::FloatConst:
            value.f = f.readFloat();
            break;

        case Opcode::DoubleConst:
            value.d = f.readDouble();
            break;

        case Opcode::StringConst:
            value.text = f.readString();
            break;

        case Opcode::NameConst:
            value.name = f.readName();
            break;

        case Opcode::TestEqual:
        case Opcode::TestNotEqual:
        case Opcode::DynamicCast:
        case Opcode::MetaCast:
        case Opcode::New:
            f.readRef(stub);
            break;

        case Opcode::AssignAny:
            f.readRef(stub); // hmm
            break;

        case Opcode::LogicOr:
        case Opcode::LogicAnd:
            f.readRef(target);
            break;

        case Opcode::Jump:
        case Opcode::JumpIfFalse:
            f.readRef(target);
            break;

        case Opcode::LoadAny:
        case Opcode::ReturnAny:
        case Opcode::ReturnDirect:
        case Opcode::ReturnLoad1:
        case Opcode::ReturnLoad2:
        case Opcode::ReturnLoad4:
        case Opcode::ReturnLoad8:
            f.readRef(stub);
            break;

        case Opcode::ParamVar:
            value.i = f.readInt8();
            break;

        case Opcode::LocalCtor:
        case Opcode::LocalDtor:
        case Opcode::LocalVar:
            f.readRef(stub);
            value.name = f.readName();
            value.i = f.readInt16();
            break;

        case Opcode::StructMemberRef:
        case Opcode::StructMember:
            f.readRef(stub);

        case Opcode::ContextVar:
            f.readRef(stub);
            value.name = f.readName();
            break;

        case Opcode::ContextCtor:
        case Opcode::ContextDtor:
            f.readRef(stub);
            break;

        case Opcode::ContextFromRef:
        case Opcode::ContextFromPtr:
        case Opcode::ContextFromPtrRef:
            f.readRef(stub);
            f.readRef(target);
            break;

        case Opcode::ContextFromValue:
            f.readRef(stub);
            break;

        case Opcode::FinalFunc:
        case Opcode::StaticFunc:
        case Opcode::VirtualFunc:
            f.readRef(stub);
            value.u = f.readUint32();
            break;

        case Opcode::EnumToInt32:
        case Opcode::EnumToInt64:
        case Opcode::EnumToName:
        case Opcode::Int32ToEnum:
        case Opcode::Int64ToEnum:
        case Opcode::NameToEnum:
        case Opcode::EnumToString:
        case Opcode::ClassConst:
            f.readRef(stub);
            break;

        case Opcode::EnumConst:
            f.readRef(stub);
            value.name = f.readName();
            break;

        case Opcode::Constructor:
            f.readRef(stub);
            value.u = f.readUint8();
            break;
    }
}

//--

END_BOOMER_NAMESPACE_EX(script)
