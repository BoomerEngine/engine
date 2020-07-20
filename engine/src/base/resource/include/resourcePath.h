/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/containers/include/hashMap.h"

namespace base
{
    class StringBuf;

    namespace res
    {

        //--

        class ResourcePathBuilder;
        class ResourceKey;

        //--

        /// Resource path container
        /// NOTE: data is persistent and allocated from internal storage (linear allocator)
        /// TODO: refcounting ? typical resource path has 100 characters so 1MB for 10k paths, it's not terrible
        struct ResourcePathData : public NoCopy
        {
            static const char PATH_SEPARATOR = '/';
            static const char WRONG_PATH_SEPARATOR = '\\';
            static const uint32_t MAX_PATH_LENGTH = 400;

            uint64_t hash; // full path hash

            StringView<char> path; // engine/textures/lena.png - ZERO TERMINATED for c_str()
            StringView<char> dirPart; // engine/textures/
            StringView<char> fileNamePart; // lena.png
            StringView<char> fileStemPart; // lena
            StringView<char> extensionPart; // png

            //---

            // allocate internal path storage, slice and dice the path
            // NOTE: this fails if path string is empty or invalid (malformated) 
            static const ResourcePathData* Build(const StringView<char> fullPathString);
        };

        //--

        /// Path to resources (e.g. engine/textures/lena.png:mux=rrrr:downscale=2)
        /// Consists of many parts that can be accessed separately
        class BASE_RESOURCE_API ResourcePath
        {
        public:
            INLINE ResourcePath() : m_data(nullptr) {};
            INLINE ResourcePath(const ResourcePath& other) : m_data(other.m_data) {};
            INLINE ResourcePath(ResourcePath&& other) : m_data(other.m_data) { other.m_data = nullptr; }
            INLINE ResourcePath(const ResourcePathData* data) : m_data(data) {};
            INLINE ~ResourcePath() {};

            INLINE ResourcePath& operator=(const ResourcePathData* data) { m_data = data; return *this; }
            INLINE ResourcePath& operator=(const ResourcePath& other) { m_data = other.m_data; return *this; }
            INLINE ResourcePath& operator=(ResourcePath&& other) { if (this != &other) { m_data = other.m_data; other.m_data = nullptr; } return *this; }

            ResourcePath(StringView<char> path);
            ResourcePath& operator=(StringView<char> path);

            //--

            /// check for validity
            INLINE operator bool() const { return valid(); }

            /// is this an empty path ?
            INLINE bool empty() const { return (m_data == nullptr); }

            /// is the a valid path ?
            INLINE bool valid() const { return (m_data != nullptr); }

            //---

            /// get the internal data container (shared between paths)
            /// NOTE: this is null for empty path
            INLINE const ResourcePathData* data() const { return m_data; }

            /// get the full string view of the path (engine/textures/lena.png:mux=rrrr)
            INLINE StringView<char> view() const { return m_data ? m_data->path : StringView<char>(); }

            /// get the resource path part (engine/textures/lena.png)
            INLINE StringView<char> path() const { return m_data ? m_data->path : StringView<char>(); }

            /// get extension part of the resource path (png)
            INLINE StringView<char> extension() const { return m_data ? m_data->extensionPart : StringView<char>(); }

            /// get the file name (lena.png)
            INLINE StringView<char> fileName() const { return m_data ? m_data->fileNamePart : StringView<char>(); }

            /// get the file stem part (lena)
            INLINE StringView<char> fileStem() const { return m_data ? m_data->fileStemPart : StringView<char>(); }

            /// get the directory (engine/textures/)
            INLINE StringView<char> directory() const { return m_data ? m_data->dirPart : StringView<char>(); }

            //--

            /// Compare paths for being equal (uses the fast hash comparison)
            INLINE bool operator==(const ResourcePath& other) const { return m_data == other.m_data; }

            /// Compare paths for being equal (uses the fast hash comparison)
            INLINE bool operator!=(const ResourcePath& other) const { return m_data != other.m_data; }

            /// Compare path hashes (for sorted containers)
            INLINE bool operator<(const ResourcePath& other) const { return (uint8_t*)m_data < (uint8_t*)other.m_data; }

            //---

            /// Empty resource path representing no resource
            static const ResourcePath& EMPTY();

            //--

            // get a string representation of path
            void print(base::IFormatStream& f) const;

            //--

            /// Get path hash (for HashMap), returns hash of the full path string
            INLINE static uint32_t CalcHash(const ResourcePath& path) { return path.m_data ? path.m_data->hash : 0; }

            //--

        protected:
            const ResourcePathData* m_data;
        };

        ///---

        /// resource path + resource class
        class BASE_RESOURCE_API ResourceKey
        {
        public:
            INLINE ResourceKey()
                : m_class(nullptr)
            {}

            INLINE ResourceKey(const ResourceKey& other)
                : m_class(other.m_class)
                , m_path(other.m_path)
            {}

            INLINE ResourceKey(ResourceKey&& other)
                : m_class(other.m_class)
                , m_path(std::move(other.m_path))
            {
                other.m_class = nullptr;
            }

            INLINE ResourceKey(const ResourcePath& path, SpecificClassType<IResource> classType)
                : m_class(classType)
                , m_path(path)
            {
                ASSERT_EX(classType != nullptr && !path.empty(), "Invalid initialization");
            }

            INLINE ResourceKey& operator=(const ResourceKey& other)
            {
                m_class = other.m_class;
                m_path = other.m_path;
                return *this;
            }

            INLINE ResourceKey& operator=(ResourceKey&& other)
            {
                if (this != &other)
                {
                    m_class = other.m_class;
                    m_path = std::move(other.m_path);
                    other.m_class = nullptr;
                }
                return *this;
            }

            // key requires valid class and path
            INLINE bool empty() const { return m_class == nullptr || m_path.empty(); }

            // key requires valid class and path
            INLINE bool valid() const { return m_class != nullptr && m_path.valid(); }

            // fast check for validity
            INLINE operator bool() const { return valid(); }

            // get the path part
            INLINE const ResourcePath& path() const { return m_path; }

            // get the loadable class
            INLINE SpecificClassType<IResource> cls() const { return m_class; }

            // test for equality - both class and path must match
            INLINE bool operator==(const ResourceKey& other) const { return (m_path == other.m_path) && (m_class == other.m_class); }

            // test for inequality
            INLINE bool operator!=(const ResourceKey& other) const { return !operator==(other); }

            //--

            // print in editor format ie. "Texture$engine/textures/lena.png:mux=rrr"
            void print(IFormatStream& f) const;

            /// print in a safe file-system safe path (engine/textures/lena(mux=rrr).Texture.png)
            void printForFileName(IFormatStream& f) const;

            // parse from a string, usually does a split near "$" to extract class name vs path
            static bool Parse(StringView<char> path, ResourceKey& outKey);

            //--

            // empty key
            static const ResourceKey& EMPTY();

            //--

            /// Get path hash (for HashMap), returns hash of the full path string
            INLINE static uint32_t CalcHash(const ResourceKey& key) { return ResourcePath::CalcHash(key.m_path); }

            //--

            /// build unique event key (slow)
            GlobalEventKey buildEventKey() const;

            //--

        private:
            ResourcePath m_path;
            SpecificClassType<IResource> m_class;
        };

        //--

        /// helper class to set parameters on a resource path
        class BASE_RESOURCE_API ResourcePathBuilder : public base::NoCopy
        {
        public:
            ResourcePathBuilder();
            ResourcePathBuilder(ResourcePath path);

            //---

            StringBuf directory; // "engine/textures/"
            StringBuf file; // lena
            StringBuf extension; // .png

            //---

            // get full file path
            StringBuf filePath() const;

            // set file path (it's split in parts)
            void filePath(StringView<char> txt);

            //---

            /// print back to a single text
            void print(IFormatStream& f) const;

            //---

            /// get a resource path
            ResourcePath toPath() const;
        };

        //--

        extern BASE_RESOURCE_API bool ApplyRelativePath(StringView<char> contextPath, StringView<char> relativePath, StringBuf& outPath);

        //--

    } // res
} // base

