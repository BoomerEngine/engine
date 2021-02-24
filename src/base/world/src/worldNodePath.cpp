/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
*
***/

#include "build.h"
#include "worldNodePath.h"
#include "worldRawLayer.h"
#include "base/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE(base::world)

//--

NodePathBuilder::NodePathBuilder(StringView layerDepotPath)
{
    if (!layerDepotPath.empty())
    {
        auto filePath = layerDepotPath.afterFirst("/layers/");
        DEBUG_CHECK_RETURN_EX(filePath, "Layer path not inside layers folder");

        static const auto layerExtension = res::IResource::GetResourceExtensionForClass(RawLayer::GetStaticClass());
        filePath = filePath.beforeLast(TempString(".{}", layerExtension));
        DEBUG_CHECK_RETURN_EX(filePath, "Layer file does not have layer extension");

        pushPath(layerDepotPath);
    }
}

NodePathBuilder::~NodePathBuilder()
{}

NodePathBuilder NodePathBuilder::parent() const
{
    DEBUG_CHECK_EX(!m_parts.empty(), "Path has no parents");

    NodePathBuilder ret(*this);
    ret.pop();
    return ret;
}

void NodePathBuilder::pop()
{
    DEBUG_CHECK_RETURN_EX(!m_parts.empty(), "Path has no parents");
    m_parts.popBack();
}

void NodePathBuilder::pushSingle(StringID id)
{
    DEBUG_CHECK_RETURN_EX(!id.empty(), "Can't push empty ID");
    DEBUG_CHECK_RETURN_EX(id != "..", "Can't push special ID like that");
    DEBUG_CHECK_RETURN_EX(id != ".", "Can't push special ID like that");

    m_parts.pushBack(id);
}

void NodePathBuilder::pushPath(StringView path)
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

void NodePathBuilder::print(IFormatStream& f) const
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

StringBuf NodePathBuilder::toString() const
{
    StringBuilder txt;
    print(txt);           
    return txt.toString();
}

NodeGlobalID NodePathBuilder::toID() const
{
    CRC64 crc;

    if (!m_parts.empty())
    {
        crc << "NODE"; // so we don't have empty shit
        for (const auto part : m_parts)
            crc << part.view();
    }

    return crc;
}

//--

END_BOOMER_NAMESPACE(base::world)