/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "scriptJitTypeLib.h"

#include "core/object/include/rttiClassRef.h"
#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//--

JITTypeLib::JITTypeLib(LinearAllocator& mem, const IJITNativeTypeInsight& nativeTypes)
    : m_nativeTypes(nativeTypes)
    , m_hasErrors(false)
    , m_mem(mem)
{}

JITTypeLib::~JITTypeLib()
{
    m_stubMap.clear();
    m_stubTypeDeclMap.clear();
    m_engineTypeMap.clear();
    m_importedFunctionMap.clear();
}

bool JITTypeLib::resolveTypeSizesAndLayouts()
{
    // update sizes of structures
    while (updateStructSizes()) {};

    // update sizes of classes
    while (updateClassSizes()) {};

    // all ok, for now
    return true;
}

bool JITTypeLib::updateMemberLayout(JITType* type)
{
    uint32_t newSize = 0;
    uint32_t newAlign = 1;

    for (auto& prop : type->fields)
    {
        auto propAlignment  = prop.jitType->runtimeAlign;
        auto propSize  = prop.jitType->runtimeSize;

        prop.runtimeOffset = Align(newSize, propAlignment);
        newAlign = std::max(newAlign, propAlignment);
        newSize = prop.runtimeOffset + propSize;
    }

    if ((type->runtimeSize == newSize) && (type->runtimeAlign == newAlign))
        return false;

    TRACE_INFO("Scripted struct '{}' {}->{} (align {})", type->name, type->runtimeSize, newSize, newAlign);
    type->runtimeSize = newSize;
    type->runtimeAlign = newAlign;

    type->simpleCopyCompare = true;
    type->requiresConstructor = false;
    type->requiresDestructor = false;
    type->zeroInitializedConstructor = true;

    for (auto& prop : type->fields)
    {
        if (!prop.jitType->zeroInitializedConstructor)
            type->zeroInitializedConstructor = false;

        if (!prop.jitType->simpleCopyCompare)
            type->simpleCopyCompare = false;

        if (prop.jitType->requiresConstructor)
            type->requiresConstructor = true;

        if (prop.jitType->requiresDestructor)
            type->requiresDestructor = true;
    }

    TRACE_INFO("Scripted struct '{}' traits: SimpleCopy: {}, Needs Ctor: {}, Needs DTor: {}, ZeroMem: {}", type->name, type->simpleCopyCompare, type->requiresConstructor, type->requiresDestructor, type->zeroInitializedConstructor);
    return true;
}

bool JITTypeLib::updateScriptedMemberLayout(JITType* type)
{
    uint32_t newSize = 0;
    uint32_t newAlignment = 1;

    InplaceArray<const JITClassMember*, 100> members;
    collectFields(type, true, members);

    for (auto prop  : members)
    {
        auto propAlignment  = prop->jitType->runtimeAlign;
        auto propSize  = prop->jitType->runtimeSize;

        auto offset  = Align(newSize, propAlignment);
        ((JITClassMember*)prop)->runtimeOffset = offset;

        newAlignment = std::max(newAlignment, propAlignment);
        newSize = offset + propSize;
    }

    // do not update if not required
    if (newSize == type->runtimeExtraSize && newAlignment == type->runtimeExtraAlign)
        return false;

    // update size
    TRACE_INFO("Scripted class '{}' {}->{} (align {})", type->name, type->runtimeExtraSize, newSize, newAlignment);
    type->runtimeExtraSize = newSize;
    type->runtimeExtraAlign = newAlignment;

    // changed, another pass may be required
    return true;
}

bool JITTypeLib::updateStructSizes()
{
    bool somethingChanged = false;

    for (auto ptr  : m_scriptedStructs)
        somethingChanged |= updateMemberLayout(ptr);

    return somethingChanged;
}

bool JITTypeLib::updateClassSizes()
{
    bool somethingChanged = false;

    for (auto ptr  : m_scriptedClasses)
        somethingChanged |= updateScriptedMemberLayout(ptr);

    return somethingChanged;
}

void JITTypeLib::collectFields(const JITType* type, bool extendedBuffer, Array<const JITClassMember*>& outMembers) const
{
    if (type->baseClass)
        collectFields(type->baseClass, extendedBuffer, outMembers);

    for (auto& member : type->fields)
        if (member.extendedBuffer == extendedBuffer)
            outMembers.pushBack(&member);
}

void JITTypeLib::printTypePrototypes(IFormatStream& f) const
{
    // resolve

    // forward declare
    f << "/* Type forward declarations */\n";
    for (auto type  : m_allTypes)
    {
        if (type->emitPrototype)
            f << type->jitName << ";\n";

        if (type->emitExtraPrototype)
            f << type->jitExtraName << ";\n";

    }
    f << "\n";

    // emit structs
    f << "/* Type definitions */\n";
    for (auto type  : m_allTypes)
    {
        if (type->emitPrototype)
        {
            f.appendf("/* {} */\n", type->name);

            f.appendf("__attribute((packed, align({}))) ", type->runtimeAlign);
            f.appendf("{} {\n", type->jitName);

            uint64_t lastOffset = 0;
            uint32_t paddingIndex = 0;

            InplaceArray<const JITClassMember*, 100> members;
            collectFields(type, false, members);

            for (auto member  : members)
            {
                ASSERT(member->runtimeOffset >= lastOffset);
                if (member->runtimeOffset > lastOffset)
                    f.appendf(" uint8_t __padding{}[{}];\n", paddingIndex++, member->runtimeOffset - lastOffset);

                f.appendf("  {} {};\n", member->jitType->jitName, member->name);
                lastOffset = member->runtimeOffset + member->jitType->runtimeSize;
            }

            if (lastOffset < type->runtimeSize)
                f.appendf(" uint8_t __finalPadding[{}];\n", type->runtimeSize - lastOffset);

            f.append("};\n\n");
        }
    }
    f << "\n";

    // class types
    f << "/* Class type definitions */\n";
    for (auto type  : m_allTypes)
    {
        if (type->emitExtraPrototype)
        {
            f.appendf("/* {} */\n", type->name);

            f.appendf("__attribute((packed, align({}))) ", type->runtimeExtraAlign);
            f.appendf("{} {\n", type->jitExtraName);

            uint64_t lastOffset = 0;
            uint32_t paddingIndex = 0;

            InplaceArray<const JITClassMember*, 100> members;
            collectFields(type, true, members);

            for (auto member  : members)
            {
                ASSERT(member->runtimeOffset >= lastOffset);
                if (member->runtimeOffset > lastOffset)
                    f.appendf(" uint8_t __padding{}[{}];\n", paddingIndex++, member->runtimeOffset - lastOffset);

                f.appendf("  {} {};\n", member->jitType->jitName, member->name);
                lastOffset = member->runtimeOffset + member->jitType->runtimeSize;
            }

            if (lastOffset < type->runtimeExtraSize)
                f.appendf(" uint8_t __finalPadding[{}];\n", type->runtimeExtraSize - lastOffset);

            f.append("};\n\n");
        }
    }
    f << "\n";

    // direct call function prototypes
    f << "/* Direct call functions */\n";
    for (auto func  : m_allFunctions)
    {
        if (func->jitDirectCallName)
        {
            if (func->returnType)
                f << func->returnType->jitName;
            else
                f << "void";
            f << " (*" << func->jitDirectCallName << ")(";

            for (uint32_t i=0; i<func->args.size(); ++i)
            {
                auto& arg = func->args[i];

                if (i > 0)
                    f << ", ";

                f << arg.jitType->jitName;

                if (arg.passAsReference)
                    f << "* ";
                else
                    f << " ";

                f << arg.name;
            }

            f << ");\n";
        }
    }
    f << "\n";
}

void JITTypeLib::printCallForwarderDeclarations(IFormatStream& f) const
{
    for (auto jitFunc  : m_allFunctions)
    {
        // if we have a direct native call than use it
        if (!jitFunc->jitDirectCallName.empty())
            continue;

        // format of the call forwarder depends on the function
        if (!jitFunc->canReturnDirectly)
            f.appendf("inline void {}(void* context, int mode, void* parentFrame, void* returnPtr", jitFunc->jitName);
        else if (jitFunc->returnType)
            f.appendf("inline {} {}(void* context, int mode, void* parentFrame", jitFunc->returnType->jitName, jitFunc->jitName);
        else
            f.appendf("inline void {}(void* context, int mode, void* parentFrame", jitFunc->jitName);

        for (auto &param : jitFunc->args)
        {
            f << ", ";
            f << param.jitType->jitName;
            if (param.passAsReference)
                f << "* ";
            else
                f << " ";
            f << param.name;
        }
        f << ");\n";
    }
    f << "\n";
}

void JITTypeLib::printCallForwarders(IFormatStream& f) const
{
    for (auto jitFunc  : m_allFunctions)
    {
        // if we have a direct native call than use it
        if (!jitFunc->jitDirectCallName.empty())
            continue;

        // format of the call forwarder depends on the function
        if (!jitFunc->canReturnDirectly)
            f.appendf("INLINE void {}(void* context, int mode, void* parentFrame, void* returnPtr", jitFunc->jitName);
        else if (jitFunc->returnType)
            f.appendf("INLINE {} {}(void* context, int mode, void* parentFrame", jitFunc->returnType->jitName, jitFunc->jitName);
        else
            f.appendf("INLINE void {}(void* context, int mode, void* parentFrame", jitFunc->jitName);

        for (auto& param : jitFunc->args)
        {
            f << ", ";
            f << param.jitType->jitName;
            if (param.passAsReference)
                f << "* ";
            else
                f << " ";
            f << param.name;
        }
        f << ") {\n";

        auto canCallDirectly = jitFunc->stub->flags.test(StubFlag::Static) || jitFunc->stub->flags.test(StubFlag::Final);
        auto localJitName = m_localyImplementedFunctionsMap.find(jitFunc->stub);
        if (canCallDirectly && localJitName && localJitName->m_isFastCall)
        {
            if (jitFunc->canReturnDirectly && jitFunc->returnType)
                f.appendf("  {} returnValue = {0};\n", jitFunc->returnType->jitName);

            if (!jitFunc->canReturnDirectly)
                f.appendf("  {}(context, parentFrame, returnPtr", localJitName->m_bodyName);
            else if (jitFunc->returnType)
                f.appendf("  {}(context, parentFrame, &returnValue", localJitName->m_bodyName);
            else
                f.appendf("  {}(context, parentFrame, 0", localJitName->m_bodyName);

            for (auto& param : jitFunc->args)
            {
                f << ", ";
                f << param.name;
            }
            f << ");\n";

            if (jitFunc->canReturnDirectly && jitFunc->returnType)
                f.append("  return returnValue;\n");
        }
        else
        {
            f << "  struct FunctionCallingParams params;\n";

            if (jitFunc->canReturnDirectly)
            {
                if (jitFunc->returnType != nullptr)
                {
                    f.appendf("  {} returnValue = {0};\n", jitFunc->returnType->jitName);
                    f << "  params._returnPtr = &returnValue;\n";
                }
                else
                {
                    f << "  params._returnPtr = 0;\n";
                }
            }
            else
            {
                f << "  params._returnPtr = returnPtr;\n";
            }

            for (uint32_t i = 0; i < jitFunc->args.size(); ++i)
            {
                f.appendf("  params._argPtr[{}] = ", i);

                auto &param = jitFunc->args[i];
                if (!param.passAsReference)
                    f << "&";
                f << param.name;
                f << ";\n";
            }

            if (canCallDirectly && localJitName)
            {
                f.appendf("  {}(context, parentFrame, &params);\n", localJitName->m_bodyName);
            }
            else
            {
                // route through general backend (the target function may be native, or special, or failed to JIT, etc)
                f.appendf("  EI->_fnCall(EI->self, context, {}, mode, parentFrame, &params);\n", jitFunc->assignedID);
            }

            if (jitFunc->canReturnDirectly && jitFunc->returnType)
                f.append("  return returnValue;\n");

        }

        f << "}\n\n";
    }
}

void JITTypeLib::printImports(IFormatStream &f) const
{
    f.appendf("DCL_COUNTS({}, {})\n", m_allTypes.size(), m_allFunctions.size());

    for (auto jitType  : m_allTypes)
        f.appendf("DCL_TYPE({}, \"{}\")\n", jitType->assignedID, jitType->name.c_str());

    for (auto jitFunc  : m_allFunctions)
    {
        if (jitFunc->jitClass)
        {
            if (jitFunc->jitDirectCallName)
                f.appendf("DCL_CLASS_FUNC({}, \"{}\", \"{}\", &{})", jitFunc->assignedID, jitFunc->jitClass->name, jitFunc->functionName, jitFunc->jitDirectCallName);
            else
                f.appendf("DCL_CLASS_FUNC({}, \"{}\", \"{}\", 0)", jitFunc->assignedID, jitFunc->jitClass->name, jitFunc->functionName);
        }
        else
        {
            if (jitFunc->jitDirectCallName)
                f.appendf("DCL_GLOBAL_FUNC({}, \"{}\", &{})", jitFunc->assignedID, jitFunc->functionName, jitFunc->jitDirectCallName);
            else
                f.appendf("DCL_GLOBAL_FUNC({}, \"{}\", 0)", jitFunc->assignedID, jitFunc->functionName);
        }
        f << "\n";
    }
}

void JITTypeLib::reportError(const Stub* owner, StringView txt)
{
    if (owner && owner->location.file)
    {
        TRACE_ERROR("{}({}): error: {}", owner->location.file->absolutePath.c_str(), owner->location.line, txt);
    }
    else
    {
        TRACE_ERROR("JIT: error: {}", txt);
    }

    m_hasErrors = true;
}

static bool IsTrivialType(StringID engineTypeName)
{
    return (engineTypeName == "bool"_id) ||
            (engineTypeName == "uint8_t"_id) || (engineTypeName == "uint16_t"_id) || (engineTypeName == "uint32_t"_id) || (engineTypeName == "uint64_t"_id) ||
            (engineTypeName == "char"_id) || (engineTypeName == "short"_id) || (engineTypeName == "int"_id) || (engineTypeName == "int64_t"_id) ||
            (engineTypeName == "float"_id) || (engineTypeName == "double"_id);
}

const JITType* JITTypeLib::createFromNativeType(StringID engineTypeName)
{
    // use existing
    JITType* ret = nullptr;
    if (m_engineTypeMap.find(engineTypeName, ret))
        return ret;

    // get engine type info
    auto typeInfo = m_nativeTypes.typeInfo(engineTypeName);
    if (!typeInfo)
    {
        reportError(nullptr, TempString("Type '{}' in not reported as native for JIT compilation", engineTypeName));
        m_engineTypeMap[engineTypeName] = nullptr;
        return nullptr;
    }

    // create local type info
    ret = m_mem.create<JITType>();
    ret->native = true;
    ret->imported = false;
    ret->name = engineTypeName;
    ret->staticArraySize = typeInfo.staticArraySize;
    ret->requiresDestructor = typeInfo.requiresDestructor;
    ret->requiresConstructor = typeInfo.requiresConstructor;
    ret->simpleCopyCompare = typeInfo.simpleCopyCompare;
    ret->zeroInitializedConstructor = typeInfo.zeroInitializationConstructor;
    ret->runtimeSize = typeInfo.runtimeSize;
    ret->runtimeAlign = typeInfo.runtimeAlign;

    if (typeInfo.metaType == MetaType::Class)
        ret->metaType = JITMetaType::Class;
    else if (typeInfo.metaType == MetaType::Enum)
        ret->metaType = JITMetaType::Enum;
    else
        ret->metaType = JITMetaType::Engine;

    if (typeInfo.innerTypeName)
        ret->innerType = createFromNativeType(typeInfo.innerTypeName);

    if (typeInfo.baseClassName)
        ret->baseClass = createFromNativeType(typeInfo.baseClassName);

    for (auto& memberInfo : typeInfo.localMembers)
    {
        auto& localMemberInfo = ret->fields.emplaceBack();
        localMemberInfo.extendedBuffer = false;
        localMemberInfo.native = true;
        localMemberInfo.name = memberInfo.name;
        localMemberInfo.runtimeOffset = memberInfo.runtimeOffset;
        localMemberInfo.jitType = createFromNativeType(memberInfo.typeName);
    }

    if (IsTrivialType(engineTypeName))
    {
        ret->jitName = ret->name.view(); // use direct type name
    }
    else if (typeInfo.metaType == MetaType::Enum || typeInfo.metaType == MetaType::Bitfield)
    {
        if (typeInfo.runtimeSize == 1)
            ret->jitName = "uint8_t";
        else if (typeInfo.runtimeSize == 2)
            ret->jitName = "uint16_t";
        else if (typeInfo.runtimeSize == 4)
            ret->jitName = "uint32_t";
        else if (typeInfo.runtimeSize == 4)
            ret->jitName = "uint64_t";

        if (typeInfo.metaType == MetaType::Enum)
        {
            ret->enumOptions.reserve(typeInfo.options.size());

            for (auto& option : typeInfo.options)
                ret->enumOptions[option.name] = option.value;
        }
    }
    else if (typeInfo.metaType == MetaType::StrongHandle)
    {
        ret->jitName = "struct StrongPtrWrapper";
    }
    else if (typeInfo.metaType == MetaType::StrongHandle)
    {
        ret->jitName = "struct WeakPtrWrapper";
    }
    else if (typeInfo.metaType == MetaType::ClassRef)
    {
        ret->jitName = "struct ClassPtrWrapper";
    }
    else if (typeInfo.metaType == MetaType::Class || typeInfo.metaType == MetaType::Simple)
    {
        InplaceArray<StringView, 10> parts;
        engineTypeName.view().slice(":", false, parts);

        StringBuilder jitName;
        jitName << "struct __type";

        for (auto& part : parts)
        {
            jitName << "_";
            jitName << part;
        }

        ret->emitPrototype = true;
        ret->jitName = m_mem.strcpy(jitName.c_str());
    }
    else
    {
        reportError(nullptr, TempString("Type '{}' in not exportable into JIT directly", engineTypeName));
    }

    if (ret)
    {
        ret->assignedID = (int) m_allTypes.size();
        m_allTypes.pushBack(ret);
    }

    m_engineTypeMap[engineTypeName] = ret;
    return ret;
}

const JITType* JITTypeLib::resolveEngineType(StringID engineTypeName)
{
    return createFromNativeType(engineTypeName);
}

const JITType* JITTypeLib::createScriptedEnum(const StubEnum* stub)
{
    JITType* ret = m_mem.create<JITType>();
    ret->native = false;
    ret->imported = false;
    ret->name = stub->name;
    ret->requiresDestructor = false;
    ret->requiresConstructor = false;
    ret->simpleCopyCompare = true;
    ret->zeroInitializedConstructor = true;
    ret->stub = stub;
    ret->enumOptions.reserve(stub->options.size());
    ret->runtimeSize = 1;
    ret->runtimeAlign = 1;

    // determine min/max value of enum values
    auto minValue = std::numeric_limits<int64_t>::max();
    auto maxValue = std::numeric_limits<int64_t>::min();
    {
        int64_t nextValue = 0;
        for (auto option  : stub->options)
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
    auto enumTypeSigned = (minValue < 0);
    if (enumTypeSigned)
    {
        if (minValue < std::numeric_limits<char>::min()) ret->runtimeSize = 2;
        if (maxValue > std::numeric_limits<char>::max()) ret->runtimeSize = 2;
        if (minValue < std::numeric_limits<short>::min()) ret->runtimeSize = 4;
        if (maxValue > std::numeric_limits<short>::max()) ret->runtimeSize = 4;
        if (minValue < std::numeric_limits<int>::min()) ret->runtimeSize = 8;
        if (maxValue > std::numeric_limits<int>::max()) ret->runtimeSize = 8;
    }
    else
    {
        if (maxValue > (int64_t)std::numeric_limits<uint8_t>::max()) ret->runtimeSize = 2;
        if (maxValue > (int64_t)std::numeric_limits<uint16_t>::max()) ret->runtimeSize = 4;
        if (maxValue > (int64_t)std::numeric_limits<uint32_t>::max()) ret->runtimeSize = 8;
    }

    // don't use special types for enums
    if (enumTypeSigned)
    {
        switch (ret->runtimeSize)
        {
            case 1: ret->jitName = "char"; break;
            case 2: ret->jitName = "short"; break;
            case 4: ret->jitName = "int"; break;
            case 8: ret->jitName = "int64_t"; break;
        }
    }
    else
    {
        switch (ret->runtimeSize)
        {
            case 1: ret->jitName = "uint8_t"; break;
            case 2: ret->jitName = "uint16_t"; break;
            case 4: ret->jitName = "uint32_t"; break;
            case 8: ret->jitName = "uint64_t"; break;
        }
    }

    int64_t nextValue = 0;
    for (auto option : stub->options)
    {
        int64_t usedValue;
        if (option->hasUserAssignedValue)
            usedValue = option->assignedValue;
        else
            usedValue = nextValue;

        ret->enumOptions[option->name] = usedValue;
        nextValue = usedValue + 1;
    }

    // assign index for mapping
    ret->assignedID = (int)m_allTypes.size();
    m_allTypes.pushBack(ret);
    return ret;
}

const JITType* JITTypeLib::createScriptedClass(const StubClass* stub)
{
    // find the native class
    auto nativeClass  =  stub->baseClass;
    while (nativeClass != nullptr)
    {
        if (nativeClass->engineImportName)
            break;
        nativeClass = nativeClass->baseClass;
    }

    // no native class, WTF
    ASSERT( nativeClass != nullptr);

    JITType* ret = m_mem.create<JITType>();
    ret->native = false;
    ret->imported = false;
    ret->name = stub->name;
    ret->stub = stub;

    // HACK: the type resolving can get recursive, make sure we are registered
    m_stubMap[stub] = ret;

    // copy stuff from base class
    // NOTE: we update the scripted size later
    ret->baseClass = resolveType(stub->baseClass);
    ret->requiresDestructor = true;
    ret->requiresConstructor = true;
    ret->simpleCopyCompare = false;
    ret->zeroInitializedConstructor = false;
    ret->runtimeSize = ret->baseClass->runtimeSize;
    ret->runtimeAlign = ret->baseClass->runtimeAlign;

    // use the JIT from first native class
    ret->jitName = ret->baseClass->jitName;

    // but generate custom one for the scripted class
    ret->emitExtraPrototype = true;
    ret->jitExtraName = m_mem.strcpy(TempString("struct __script_class_{}", stub->name).c_str());

    // generate members
    for (auto filedStub  : stub->stubs)
    {
        if (auto prop  = filedStub->asProperty())
        {
            auto& propInfo = ret->fields.emplaceBack();
            propInfo.runtimeOffset = 0; // NOT RESOLVED YET
            propInfo.name = prop->name;
            propInfo.native = false;
            propInfo.jitType = resolveType(prop->typeDecl);
            propInfo.extendedBuffer = true; // scripted class property
        }
    }

    // assign index for mapping
    ret->assignedID = (int)m_allTypes.size();
    m_allTypes.pushBack(ret);

    // keep around for size resolving
    m_scriptedClasses.pushBack(ret);
    return ret;
}

const JITType* JITTypeLib::createScriptedStruct(const script::StubClass *stub)
{
    JITType* ret = m_mem.create<JITType>();
    ret->native = false;
    ret->imported = false;
    ret->name = stub->name;
    ret->stub = stub;
    ret->emitPrototype = true; // yes, do emit the type
    ret->jitName = m_mem.strcpy(TempString("struct __script_struct_{}", stub->name).c_str());

    // HACK: the type resolving can get recursive, make sure we are registered
    m_stubMap[stub] = ret;

    // NOTE: not known yet, resolved later
    ret->requiresDestructor = true;
    ret->requiresConstructor = true;
    ret->simpleCopyCompare = false;
    ret->zeroInitializedConstructor = false;
    ret->runtimeSize = 0;
    ret->runtimeAlign = 1;

    // generate members
    for (auto filedStub  : stub->stubs)
    {
        if (auto prop  = filedStub->asProperty())
        {
            auto& propInfo = ret->fields.emplaceBack();
            propInfo.runtimeOffset = 0; // NOT RESOLVED YET
            propInfo.name = prop->name;
            propInfo.native = false;
            propInfo.jitType = resolveType(prop->typeDecl);
            propInfo.extendedBuffer = false;
        }
    }

    // assign index for mapping
    ret->assignedID = (int)m_allTypes.size();
    m_allTypes.pushBack(ret);

    // keep around for size resolving
    m_scriptedStructs.pushBack(ret);
    return ret;
}

const JITType* JITTypeLib::resolveType(const Stub* typeStub)
{
    // use existing
    JITType* ret = nullptr;
    if (m_stubMap.find(typeStub, ret))
        return ret;

    // enum ?
    if (auto stubEnum  = typeStub->asEnum())
    {
        if (stubEnum->engineImportName)
            return createFromNativeType(stubEnum->engineImportName);
        else
            return createScriptedEnum(stubEnum);
    }
    // class ?
    else if (auto stubClass  = typeStub->asClass())
    {
        if (stubClass->engineImportName)
            return createFromNativeType(stubClass->engineImportName);
        else if (stubClass->flags.test(StubFlag::Struct))
            return createScriptedStruct(stubClass);
        else
            return createScriptedClass(stubClass);
    }
    // type ref
    else if (auto stubTypeRef  = typeStub->asTypeDecl())
    {
        return resolveType(stubTypeRef);
    }

    // invalid stub
    reportError(typeStub, TempString("Unable to resolve JIT type for '{}'", typeStub->name));
    return nullptr;
}

const JITType* JITTypeLib::resolveType(const StubTypeDecl* typeStub)
{
    // resolve
    if (typeStub->metaType == StubTypeType::Engine)
        return createFromNativeType(typeStub->name);
    if (typeStub->metaType == StubTypeType::Simple)
        return resolveType(typeStub->referencedType->resolvedStub);

    // use existing
    JITType* ret = nullptr;
    if (m_stubTypeDeclMap.find(typeStub, ret))
        return ret;

    // build
    if (typeStub->metaType == StubTypeType::ClassType)
    {
        ret = m_mem.create<JITType>();
        ret->native = false;
        ret->imported = false;
        ret->stub = typeStub;
        ret->metaType = JITMetaType::ClassPtr;
        ret->runtimeSize = sizeof(ClassType);
        ret->runtimeAlign = __alignof(ClassType);
        ret->innerType = resolveType(typeStub->referencedType->resolvedStub); // class
        ret->name = FormatClassRefTypeName(ret->innerType->name);
        ret->jitName = "struct ClassPtrWrapper";
        ret->requiresConstructor = true;
        ret->requiresDestructor = false;
        ret->simpleCopyCompare = true;
        ret->zeroInitializedConstructor = true;
    }
    else if (typeStub->metaType == StubTypeType::PtrType)
    {
        ret = m_mem.create<JITType>();
        ret->native = false;
        ret->imported = false;
        ret->stub = typeStub;
        ret->metaType = JITMetaType::StrongPtr;
        ret->runtimeSize = sizeof(ObjectPtr);
        ret->runtimeAlign = __alignof(ObjectPtr);
        ret->innerType = resolveType(typeStub->referencedType->resolvedStub); // class
        ret->name = FormatStrongHandleTypeName(ret->innerType->name);
        ret->jitName = "struct StrongPtrWrapper";
        ret->requiresConstructor = true;
        ret->requiresDestructor = true;
        ret->simpleCopyCompare = false;
        ret->zeroInitializedConstructor = true;
    }
    else if (typeStub->metaType == StubTypeType::WeakPtrType)
    {
        ret = m_mem.create<JITType>();
        ret->native = false;
        ret->imported = false;
        ret->stub = typeStub;
        ret->metaType = JITMetaType::WeakPtr;
        ret->runtimeSize = sizeof(ObjectWeakPtr);
        ret->runtimeAlign = __alignof(ObjectWeakPtr);
        ret->innerType = resolveType(typeStub->referencedType->resolvedStub); // class
        ret->name = FormatWeakHandleTypeName(ret->innerType->name);
        ret->jitName = "struct WeakPtrWrapper";
        ret->requiresConstructor = true;
        ret->requiresDestructor = true;
        ret->simpleCopyCompare = false;
        ret->zeroInitializedConstructor = true;
    }
    else if (typeStub->metaType == StubTypeType::DynamicArrayType)
    {
        // TODO
    }
    else if (typeStub->metaType == StubTypeType::StaticArrayType)
    {
        // TODO
    }

    // invalid stub
    if (!ret)
        reportError(typeStub, TempString("Unable to resolve JIT type for '{}'", typeStub->name));

    // add index
    ASSERT(ret->assignedID == -1);
    ret->assignedID = (int) m_allTypes.size();
    m_allTypes.pushBack(ret);
    m_stubTypeDeclMap[typeStub] = ret;

    return ret;
}

void JITTypeLib::reportLocalFunctionBody(const StubFunction* func,  StringView localBodyName, bool fastCall)
{
    LocalFuncInfo info;
    info.m_bodyName = localBodyName;
    info.m_isFastCall = fastCall;
    m_localyImplementedFunctionsMap[func] = info;
}

const JITImportedFunction* JITTypeLib::resolveFunction(const StubFunction* func)
{
    JITImportedFunction* ret = 0;
    if (m_importedFunctionMap.find(func, ret))
        return ret;

    ret = m_mem.create<JITImportedFunction>();
    ret->functionName = m_mem.strcpy(func->fullName().c_str());
    ret->assignedID = (int)m_allFunctions.size();
    ret->stub = func;
    ret->jitName = m_mem.strcpy(TempString("__call_{}_{}", func->name, ret->assignedID).c_str());

    if (func->owner && func->owner->asClass())
        ret->jitClass = resolveType(func->owner);

    ret->canReturnDirectly = true;
    if (func->returnTypeDecl)
    {
        ret->returnType = resolveType(func->returnTypeDecl);

        // we are strict about the return policy
        if (ret->returnType->requiresConstructor || ret->returnType->requiresDestructor || !ret->returnType->simpleCopyCompare)
            ret->canReturnDirectly = false;
    }

    bool canCallNativeDirectly = ret->canReturnDirectly;
    for (auto arg  : func->args)
    {
        auto &entryArg = ret->args.emplaceBack();
        entryArg.stub = arg;
        entryArg.name = arg->name;
        entryArg.jitType = resolveType(arg->typeDecl);
        entryArg.passAsReference = arg->flags.test(StubFlag::Out) || arg->flags.test(StubFlag::Ref);

        // if we need to pass type by value make sure it's a POD type
        if (!entryArg.passAsReference)
        {
            if (entryArg.jitType->requiresConstructor || entryArg.jitType->requiresDestructor || !entryArg.jitType->simpleCopyCompare)
            {
                canCallNativeDirectly = false;
            }
        }
    }

    if (canCallNativeDirectly && func->flags.test(StubFlag::Native))
        ret->jitDirectCallName = m_mem.strcpy(TempString("__native_call_{}_{}", func->name, ret->assignedID).c_str());

    m_allFunctions.pushBack(ret);
    m_importedFunctionMap[func] = ret;
    return ret;
}

//--

END_BOOMER_NAMESPACE_EX(script)
