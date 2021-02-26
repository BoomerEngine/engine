/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "scriptJitTypeLib.h"
#include "scriptJitConstantCache.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//--

JITConstCache::JITConstCache(mem::LinearAllocator& mem, JITTypeLib& typeLib)
    : m_mem(mem)
    , m_typeLib(typeLib)
{}

JITConstCache::~JITConstCache()
{}

void JITConstCache::printConstVars(IFormatStream& f) const
{
    for (auto& info : m_stringConstants)
    {
        f << m_stringType->jitName;
        f << " ";
        f << info.m_varName;
        f << ";\n";
    }

    for (auto& info : m_nameConstants)
    {
        f << m_nameType->jitName;
        f << " ";
        f << info.m_varName;
        f << ";\n";
    }

    for (auto& info : m_typeConstants)
    {
        f << m_typeType->jitName;
        f << " ";
        f << info.m_varName;
        f << ";\n";
    }

    f << "\n";
}

void JITConstCache::printConstInit(IFormatStream& f) const
{
    for (auto& info : m_stringConstants)
    {
        f << "DCL_CONST_STR(&";
        f << info.m_varName;
        f << ", \"";
        f << info.m_string;
        f << "\");\n";
    }

    for (auto& info : m_nameConstants)
    {
        f << "DCL_CONST_NAME(&";
        f << info.m_varName;
        f << ", \"";
        f << info.m_string;
        f << "\");\n";
    }

    for (auto& info : m_typeConstants)
    {
        f << "DCL_CONST_TYPE(&";
        f << info.m_varName;
        f << ", \"";
        f << info.m_string;
        f << "\");\n";
    }

    f << "\n";
}

StringView JITConstCache::mapStringConst(const char* stringConst)
{
    auto hash  = StringView(stringConst).calcCRC64();

    uint32_t index = 0;
    if (m_stringConstantMap.find(hash, index))
        return m_stringConstants[index].m_varName;

    if (!m_stringType)
        m_stringType = m_typeLib.resolveEngineType("StringBuf"_id);

    index = m_stringConstants.size();
    auto& entry = m_stringConstants.emplaceBack();
    entry.m_varName = m_mem.strcpy(TempString("__str_{}", index).c_str());
    entry.m_string = m_mem.strcpy(stringConst);

    m_stringConstantMap[hash] = index;
    return entry.m_varName;
}

StringView JITConstCache::mapNameConst(const char* nameConst)
{
    auto hash  = StringView(nameConst).calcCRC64();

    if (!m_nameType)
        m_nameType = m_typeLib.resolveEngineType("StringID"_id);

    uint32_t index = 0;
    if (m_nameConstantMap.find(hash, index))
        return m_nameConstants[index].m_varName;

    index = m_nameConstants.size();
    auto& entry = m_nameConstants.emplaceBack();
    entry.m_varName = m_mem.strcpy(TempString("__name_{}", index).c_str());
    entry.m_string = m_mem.strcpy(nameConst);

    m_nameConstantMap[hash] = index;
    return entry.m_varName;
}

StringView JITConstCache::mapTypeConst(const char* nameConst)
{
    auto hash  = StringView(nameConst).calcCRC64();

    if (!m_nameType)
        m_typeType = m_typeLib.resolveEngineType(reflection::GetTypeName<SpecificClassType<IObject>>());

    uint32_t index = 0;
    if (m_typeConstantMap.find(hash, index))
        return m_typeConstants[index].m_varName;

    index = m_typeConstants.size();
    auto& entry = m_typeConstants.emplaceBack();
    entry.m_varName = m_mem.strcpy(TempString("__classType_{}", index).c_str());
    entry.m_string = m_mem.strcpy(nameConst);

    m_typeConstantMap[hash] = index;
    return entry.m_varName;
}

//--

END_BOOMER_NAMESPACE_EX(script)
