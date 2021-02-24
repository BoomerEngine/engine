/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml\prv #]
***/

#include "build.h"
#include "xmlDocument.h"
#include "xmlTextWriter.h"

#include "base/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE(base::xml)

TextWriter::TextWriter(IFormatStream& builder)
    : m_builder(builder)
{
    m_builder.append("<?xml version=\"1.0\" standalone=\"yes\"?>\n");
}

TextWriter::~TextWriter()
{
}

void TextWriter::writeNode(const IDocument& doc, NodeID id, uint32_t depth)
{
    // get name of the node, the invalid nodes have no name
    auto nodeName = doc.nodeName(id);
    if (nodeName.empty())
        return; 

    // entry
    m_builder.appendPadding(' ', depth * 2);
    m_builder.append("<");
    m_builder.append(nodeName.data(), nodeName.length());

    // emit attributes
    {
        auto attrId = doc.nodeFirstAttribute(id);
        while (attrId)
        {
            auto attrName = doc.attributeName(attrId);
            auto attrValue = doc.attributeValue(attrId);
            if (!attrName.empty())
            {

                m_builder.append(" ");
                m_builder.append(attrName.data(), attrName.length());
                m_builder.append("=\"");
                m_builder.append(attrValue.data(), attrValue.length());
                m_builder.append("\"");
            }

            attrId = doc.nextAttribute(attrId);
        }
    }

    // do we have to emit children or value
    auto nodeValue = doc.nodeValue(id);
    auto childId = doc.nodeFirstChild(id);
    if (!childId && nodeValue.empty())
    {
        // end the start tag
        m_builder.append("/>\n");
    }
    else
    {
        // end the start tag
        m_builder.append(">");

        // write value
        {
            auto cur  = nodeValue.data();
            auto end  = cur + nodeValue.length();
            while (cur < end)
            {
                if (*cur == '<')
                    m_builder.append("&lt;");
                else if (*cur == '>')
                    m_builder.append("&gt;");
                else if (*cur == '&')
                    m_builder.append("&amp;");
                else if (*cur == '\"')
                    m_builder.append("&quot;");
                else if (*cur == '\'')
                    m_builder.append("&pos;");
                else
                {
                    char str[2] = {*cur, 0};
                    m_builder.append(str);
                }

                cur += 1;
            }
        }

        // write children
        // NOTE: mixed content is not supported
        if (childId != 0)
        {
            m_builder.append("\n");

            while (childId)
            {
                writeNode(doc, childId, depth + 1);
                childId = doc.nodeSibling(childId);
            }

            m_builder.appendPadding(' ', depth * 2);
        }

        // write the end tag
        m_builder.append("</");
        m_builder.append(nodeName.data(), nodeName.length());
        m_builder.append(">\n");
    }
}

END_BOOMER_NAMESPACE(base::xml)