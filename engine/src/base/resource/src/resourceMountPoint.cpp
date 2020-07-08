/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "resourcePath.h"
#include "resourceMountPoint.h"

namespace base
{
    namespace res
    {
        static ResourceMountPoint TheRootMountPoint;

        //--

        const ResourceMountPoint& ResourceMountPoint::ROOT()
        {
            return TheRootMountPoint;
        }

        ResourceMountPoint::ResourceMountPoint(StringView<char> stringPath)
        {
            if (!stringPath.empty())
            {
                StringBuf str(stringPath);
                str.replaceChar('\\', '/');

                if (!str.endsWith("/"))
                    str = TempString("{}/", str);

                m_path = std::move(str);
                m_hash = str.cRC64();
            }
        }

        bool ResourceMountPoint::containsPath(StringView<char> path) const
        {
            if (m_path.empty())
                return true;

            return path.beginsWith(m_path);
        }

        bool ResourceMountPoint::translatePathToRelative(ResourcePath path, ResourcePath& outRelativePath) const
        {
            if (path.empty())
                return false;

            if (root())
            {
                outRelativePath = path;
                return true;
            }

            if (!path.path().beginsWith(m_path))
                return false;

            ResourcePathBuilder builder(path);
            builder.directory = builder.directory.subString(m_path.length());

            outRelativePath = builder.toPath();
            return true;
        }

        bool ResourceMountPoint::translatePathToRelative(StringView<char> path, StringBuf& outRelativePath) const
        {
            if (path.empty())
                return false;

            if (root())
            {
                outRelativePath = StringBuf(path);
                return true;
            }

            if (!path.beginsWith(m_path))
                return false;

            outRelativePath = StringBuf(path.subString(m_path.length()));
            return true;
        }

        void ResourceMountPoint::expandPathFromRelative(StringView<char> relativePath, StringBuf& fullPath) const
        {
            if (!relativePath.empty())
            {
                if (root())
                {
                    fullPath = StringBuf(relativePath);
                }
                else
                {
                    fullPath = TempString("{}{}", m_path, relativePath);
                }
            }
            else
            {
                fullPath = StringBuf::EMPTY();
            }
        }

        void ResourceMountPoint::expandPathFromRelative(ResourcePath relativePath, ResourcePath& outAbsolutePath) const
        {
            if (!relativePath.empty())
            {
                if (root())
                {
                    outAbsolutePath = relativePath;
                }
                else
                {
                    ResourcePathBuilder builder(relativePath);
                    expandPathFromRelative(builder.directory, builder.directory);
                    outAbsolutePath = builder.toPath();
                }
            }
            else
            {
                outAbsolutePath = ResourcePath();
            }
        }

        //--

    } // res
} // base