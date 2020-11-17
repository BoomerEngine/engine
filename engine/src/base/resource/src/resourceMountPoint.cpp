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

        ResourceMountPoint::ResourceMountPoint(StringView stringPath)
        {
            if (!stringPath.empty())
            {
                StringBuf str(stringPath);
                str.replaceChar('\\', '/');

                if (!str.endsWith("/"))
                    str = TempString("{}/", str);

                ASSERT_EX(ValidateDepotPath(str, DepotPathClass::AbsoluteDirectoryPath), "Mount path is not valid");

                m_path = std::move(str);
                m_hash = str.cRC64();
            }
        }

        bool ResourceMountPoint::containsPath(StringView path) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(path, DepotPathClass::AnyAbsolutePath), false);

            return path.beginsWith(m_path);
        }

        bool ResourceMountPoint::translatePathToRelative(StringView path, StringBuf& outRelativePath) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(path, DepotPathClass::AnyAbsolutePath), false);

            if (!path.beginsWith(m_path))
                return false;

            outRelativePath = StringBuf(path.subString(m_path.length()));
            ASSERT(ValidateDepotPath(outRelativePath, DepotPathClass::AnyRelativePath));

            return true;
        }

        void ResourceMountPoint::expandPathFromRelative(StringView relativePath, StringBuf& fullPath) const
        {
            if (!relativePath.empty())
            {
                DEBUG_CHECK_RETURN(ValidateDepotPath(relativePath, DepotPathClass::AnyRelativePath));

                fullPath = TempString("{}{}", m_path, relativePath);

                ASSERT(ValidateDepotPath(fullPath, DepotPathClass::AnyAbsolutePath));
            }
            else
            {
                fullPath = m_path;
            }
        }

        //--

    } // res
} // base