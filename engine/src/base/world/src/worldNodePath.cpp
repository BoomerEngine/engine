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
#include "base/containers/include/stringBuilder.h"

namespace base
{
    namespace world
    {

        NodePath NodePath::operator[](const StringView name) const
        {
            return operator[](StringID(name));
        }

        NodePath NodePath::operator[](const NodePathPart& childName) const
        {
            ASSERT(!childName.empty());

            CRC64 pathCRC(m_hash);
            pathCRC << "/";
            pathCRC << childName;

            auto parentNode = new RootPathElement(m_parent, m_name, m_hash);
            return NodePath(parentNode, childName, pathCRC.crc());
        }

        uint32_t NodePath::CalcHash(const NodePath& path)
        {
            return (uint32_t)path.m_hash;
        }

        void NodePath::RootPathElement::print(IFormatStream& str) const
        {
            if (m_parent)
            {
                m_parent->print(str);
                str << "/";
            }

            str << m_name;
        }

        void NodePath::print(IFormatStream& str) const
        {
            if (m_parent)
            {
                m_parent->print(str);
                str << "/";
            }

            str << m_name;
        }

        bool NodePath::Parse(const StringBuf& str, NodePath& outPath)
        {
            if (str.empty())
            {
                outPath = NodePath();
                return true;
            }

            // eat the first char
            auto start  = str.c_str();
            auto path  = start;
            if (*start == '/')
            {
                start += 1;
                path += 1;
            }

            // split into directories, create as we go into the path
            auto curPath = NodePath();
            while (*path)
            {
                ASSERT(*path != '\\');

                // split after each path breaker
                if (*path == '/')
                {
                    // extract directory name
                    if (path > start)
                        curPath = curPath[StringView(start, path)];

                    // advance
                    start = path + 1;
                }

                ++path;
            }

            // last part
            if (path > start)
                curPath = curPath[StringView(start, path)];

            // return final path element
            TRACE_INFO("'{}' '{}'", curPath, str);
            outPath = curPath;
            return true;
        }

    } // world
} // base


