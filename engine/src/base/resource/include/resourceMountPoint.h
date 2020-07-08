/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

namespace base
{
    namespace res
    {
        class ResourcePath;

        /// Helper string to describe where are the resources mounted to
        /// Similar to resource path but always has to have the "/" at the end (unless it's empty)
        class BASE_RESOURCE_API ResourceMountPoint
        {
        public:
            INLINE ResourceMountPoint() {};
            INLINE ResourceMountPoint(const ResourceMountPoint& other) = default;
            INLINE ResourceMountPoint(ResourceMountPoint&& other) = default;
            INLINE ResourceMountPoint& operator=(const ResourceMountPoint& other) = default;
            INLINE ResourceMountPoint& operator=(ResourceMountPoint&& other) = default;
            INLINE ~ResourceMountPoint() = default;
            ResourceMountPoint(StringView<char> stringPath);

            /// Convert to chars
            INLINE const char* c_str() const { return m_path.c_str(); }

            /// Get the inner string
            INLINE const StringBuf& path() const { return m_path; }

            /// Is the the root mount point ?
            INLINE bool root() const { return m_path.empty(); }

            /// Compare paths for being equal (uses the fast hash comparison)
            INLINE bool operator==(const ResourceMountPoint& other) const { return m_path == other.m_path; }

            /// Compare paths for being equal (uses the fast hash comparison)
            INLINE bool operator!=(const ResourceMountPoint& other) const { return m_path != other.m_path; }

            /// Compare path hashes (for sorted containers)
            INLINE bool operator<(const ResourceMountPoint& other) const { return m_path < other.m_path; }

            //--

            // can we service this file
            bool containsPath(StringView<char> path) const;

            //--

            // translate resource path to a path relative to this mount point
            // NOTE: will fail if the path is outside the mount point
            bool translatePathToRelative(StringView<char> path, StringBuf& outRelativePath) const;

            // translate resource path to a path relative to this mount point
            // NOTE: will fail if the path is outside the mount point
            bool translatePathToRelative(ResourcePath path, ResourcePath& outRelativePath) const;

            //--

            // expand relative path to a full path using this mount point as a base
            void expandPathFromRelative(StringView<char> relativePath, StringBuf& fullPath) const;

            // expand relative path to a full path using this mount point as a base
            void expandPathFromRelative(ResourcePath relativePath, ResourcePath& outAbsolutePath) const;

            //--

            /// Get the root mount point (usually the project root directory is mounted there)
            static const ResourceMountPoint& ROOT();

        protected:
            StringBuf m_path;
            uint64_t m_hash; // path's hash - copied out for faster access

            static const char PATH_SEPARATOR = '/';
            static const char WRONG_PATH_SEPARATOR = '\\';
        };

    } // res
} // base
