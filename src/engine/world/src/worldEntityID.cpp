/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
*
***/

#include "build.h"
#include "worldEntityID.h"

#include "core/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE()

//--

EntityStaticIDBuilder::EntityStaticIDBuilder(StringView layerDepotPath)
{
    if (!layerDepotPath.empty())
    {
        auto filePath = layerDepotPath.afterFirst("/layers/");
        DEBUG_CHECK_RETURN_EX(filePath, "Layer path not inside layers folder");

        filePath = filePath.beforeLast(".");
        DEBUG_CHECK_RETURN_EX(filePath, "Layer file does not have layer extension");

        pushPath(filePath);
    }
}

EntityStaticIDBuilder::~EntityStaticIDBuilder()
{}

EntityStaticIDBuilder EntityStaticIDBuilder::parent() const
{
    DEBUG_CHECK_EX(!m_parts.empty(), "Path has no parents");

    EntityStaticIDBuilder ret(*this);
    ret.pop();
    return ret;
}

void EntityStaticIDBuilder::pop()
{
    DEBUG_CHECK_RETURN_EX(!m_parts.empty(), "Path has no parents");
    m_parts.popBack();
}

void EntityStaticIDBuilder::pushSingle(StringID id)
{
    DEBUG_CHECK_RETURN_EX(!id.empty(), "Can't push empty ID");
    DEBUG_CHECK_RETURN_EX(id != "..", "Can't push special ID like that");
    DEBUG_CHECK_RETURN_EX(id != ".", "Can't push special ID like that");

    m_parts.pushBack(id);
}

void EntityStaticIDBuilder::pushPath(StringView path)
{
    // perfectly legal to have nothing
    if (path.empty())
        return;

    // if path starts with "/" it's a global path, reset current stack
    if (path.beginsWith("/"))
    {
        m_parts.reset();
        path = path.subString(1);
    }

    // split into parts
    InplaceArray<StringView, 16> parts;
    path.slice("/", false, parts);

    // add parts
    for (auto part : parts)
    {
        // special "this" part, do nothing
        if (part == ".")
            continue;

        // go to parent directory
        if (part == "..")
        {
            if (!m_parts.empty()) // TODO: hard to decide what to do now
                m_parts.popBack();
            continue;
        }

        // just add
        m_parts.pushBack(StringID(part));
    }
}

void EntityStaticIDBuilder::print(IFormatStream& f) const
{
    if (m_parts.empty())
    {
        f << "/";
    }
    else
    {
        for (const auto part : m_parts)
        {
            f << "/";
            f << part;
        }
    }
}

StringBuf EntityStaticIDBuilder::toString() const
{
    StringBuilder txt;
    print(txt);
    return txt.toString();
}

EntityStaticID EntityStaticIDBuilder::CompileFromPath(StringView path)
{
    InplaceArray<StringView, 16> parts;
    path.slice("/", false, parts);

    CRC64 crc;

    if (!parts.empty())
    {
        for (const auto part : parts)
            crc << "/" << part;
    }

    return crc;
}

EntityStaticID EntityStaticIDBuilder::toID() const
{
    CRC64 crc;

    if (!m_parts.empty())
    {
        for (const auto part : m_parts)
            crc << "/" << part.view();
    }

    return crc;
}

//--

END_BOOMER_NAMESPACE()
