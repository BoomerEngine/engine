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

        ///---

        /// resource path + resource class
        class BASE_RESOURCE_API ResourceKey
        {
        public:
            INLINE ResourceKey() {};
            INLINE ResourceKey(const ResourceKey& other) = default;
            INLINE ResourceKey(ResourceKey&& other);
            INLINE ResourceKey(StringView<char> path, SpecificClassType<IResource> classType);
            INLINE ResourceKey& operator=(const ResourceKey& other) = default;
            INLINE ResourceKey& operator=(ResourceKey&& other);

            INLINE bool operator==(const ResourceKey& other) const;
            INLINE bool operator!=(const ResourceKey& other) const;

            //--

            ALWAYS_INLINE const StringBuf& path() const;
            ALWAYS_INLINE SpecificClassType<IResource> cls() const;

            INLINE bool empty() const;
            INLINE bool valid() const; // only non empty paths are valid

            INLINE StringView<char> view() const;

            INLINE operator bool() const;

            INLINE static uint32_t CalcHash(const ResourceKey& key);

            // NOTE: all path parts are just string views into the path string!
            INLINE StringView<char> fileName() const; // lena.png
            INLINE StringView<char> fileStem() const; // lena
            INLINE StringView<char> extension() const; // png
            INLINE StringView<char> directories() const; // engine/textures/

            //--

            // print in editor format ie. "Texture:engine/textures/lena.png"
            void print(IFormatStream& f) const;

            //--

            // parse from a string, usually does a split near ":" to extract class name vs path
            static bool Parse(StringView<char> path, ResourceKey& outKey);

            //--

            /// build unique event key for this path (slow)
            GlobalEventKey buildEventKey() const;

            //--

            // empty key
            static const ResourceKey& EMPTY();

            //--

        private:
            StringBuf m_path;
            SpecificClassType<IResource> m_class;
        };

    } // res

    //--

    // make resource path
    INLINE res::ResourceKey MakePath(StringView<char> path, SpecificClassType<res::IResource> classType)
    {
        return res::ResourceKey(path, classType);
    }

    // make resource path
    template< typename T >
    INLINE extern res::ResourceKey MakePath(StringView<char> path)
    {
        return res::ResourceKey(path, T::GetStaticClass());
    }

    //--

    // given a path as a context and a relative path (that can contain .. and .) builds a new path
    // NOTE: if the relative path is given in absolute format (starting with "/" then the context path is not used)
    // NOTE: if context path is a file path (ie. contains the file name) then that file name is ignored
    extern BASE_RESOURCE_API bool ApplyRelativePath(StringView<char> contextPath, StringView<char> relativePath, StringBuf& outPath);

    // Scan path combinations up to given depth
    // For example, assuming context path = "/rendering/assets/meshes/box.fbx"
    // and pathParts being "D:\Work\Assets\Box\Textures\wood.jpg" (note: it does not have to be any valid depot path)
    // This function will call the testFunc with each of the following paths, whenever testFunc returns true that patch is chosen by the function and written to outPath
    //  "/rendering/assets/meshes/wood.jpg"
    //  "/rendering/assets/meshes/Textures/wood.jpg"
    //  "/rendering/assets/meshes/Box/Textures/wood.jpg"
    //  "/rendering/assets/wood.jpg"
    //  "/rendering/assets/Textures/wood.jpg"
    //  "/rendering/assets/Box/Textures/wood.jpg"
    //  "/rendering/wood.jpg"
    //  "/rendering/Textures/wood.jpg"
    //  "/rendering/Box/Textures/wood.jpg"
    //  etc, up to the maxScanDepth
    extern BASE_RESOURCE_API bool ScanRelativePaths(StringView<char> contextPath, StringView<char> pathParts, uint32_t scanDepth, StringBuf& outPath, const std::function<bool(StringView<char>)>& testFunc);

    //--

    enum class DepotPathClass : uint8_t
    {
        Invalid, // invalid path
        AbsoluteFilePath, // stats with "/" does not end with "/", cannot be empty
        AbsoluteDirectoryPath, // stats with "/" ends with "/", cannot be empty
        RelativeFilePath, // does not stat with "/" does not end with "/", cannot be empty
        RelativeDirectoryPath, // does not stat with "/" ends with "/", can be empty

        // following are only used only in checks:
        AnyFilePath, // can't end with "/"
        AnyDirectoryPath, // must end with "/"
        AnyAbsolutePath, // begins with "/"
        AnyRelativePath, // begins with "/"
        Any, // just check of invalid chars
    };

    // check if given character can be used in a file name
    extern BASE_RESOURCE_API bool IsValidPathChar(uint32_t ch);

    // given a wanted name of something return file name that is safe to use in the file system
    extern BASE_RESOURCE_API bool MakeSafeFileName(StringView<char> text, StringBuf& outFixedFileName);

    // check if given file name is a valid depot file name (just name, no extension) - follows Windows guidelines + does NOT allow dot in the file name
    extern BASE_RESOURCE_API bool ValidateFileName(StringView<char> fileName);

    // check if given file name is a valid depot file name with extension
    extern BASE_RESOURCE_API bool ValidateFileNameWithExtension(StringView<char> fileName);

    // check if given depot path is valid
    extern BASE_RESOURCE_API bool ValidateDepotPath(StringView<char> path, DepotPathClass pathClass = DepotPathClass::AbsoluteFilePath);

    // classify path based on content
    extern BASE_RESOURCE_API DepotPathClass ClassifyDepotPath(StringView<char> path);

    //--

} // base

#include "resourcePath.inl"