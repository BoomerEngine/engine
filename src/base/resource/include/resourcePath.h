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
        ///---

        /// conformed resource path
        class BASE_RESOURCE_API ResourcePath
        {
        public:
            INLINE ResourcePath() {};
            INLINE ResourcePath(const ResourcePath& other) = default;
            INLINE ResourcePath(ResourcePath&& other);
            INLINE ResourcePath& operator=(const ResourcePath& other) = default;
            INLINE ResourcePath& operator=(ResourcePath&& other);

            ResourcePath(StringView path); // conforms the string to 

            INLINE bool operator==(const ResourcePath& other) const;
            INLINE bool operator!=(const ResourcePath& other) const;

            //--

            ALWAYS_INLINE const StringBuf& str() const;
            ALWAYS_INLINE uint64_t hash() const; // 64-bit unique hash (hopefully unique..)

            INLINE bool empty() const;
            INLINE bool valid() const; // only non empty paths are valid

            INLINE StringView view() const;
                        
            INLINE operator bool() const;

            INLINE static uint32_t CalcHash(const ResourcePath& key);

            //--

            StringView fileName() const; // lena.png
            StringView fileStem() const; // lena
            StringView extension() const; // png
            StringView basePath() const; // /engine/textures/

            //--

            // print in editor format ie. "Texture:engine/textures/lena.png"
            void print(IFormatStream& f) const;

            //--

            // parse from a string, usually does a split near ":" to extract class name vs path
            static bool Parse(StringView path, ResourcePath& outPath);

            //--

            // empty key
            static const ResourcePath& EMPTY();

            //--

        private:
            StringBuf m_string;
            uint64_t m_hash = 0;
        };

    } // res

    //--

    // given a path as a context and a relative path (that can contain .. and .) builds a new path
    // NOTE: if the relative path is given in absolute format (starting with "/" then the context path is not used)
    // NOTE: if context path is a file path (ie. contains the file name) then that file name is ignored
    extern BASE_RESOURCE_API bool ApplyRelativePath(StringView contextPath, StringView relativePath, StringBuf& outPath);

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
    extern BASE_RESOURCE_API bool ScanRelativePaths(StringView contextPath, StringView pathParts, uint32_t scanDepth, StringBuf& outPath, const std::function<bool(StringView)>& testFunc);

    // given a base path and a second path (both absolute) build a relative path that takes you from base to second
    extern BASE_RESOURCE_API bool BuildRelativePath(StringView basePath, StringView targetPath, StringBuf& outRelativePath);

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
    extern BASE_RESOURCE_API bool MakeSafeFileName(StringView text, StringBuf& outFixedFileName);

    // check if given file name is a valid depot file name (just name, no extension) - follows Windows guidelines + does NOT allow dot in the file name
    extern BASE_RESOURCE_API bool ValidateFileName(StringView fileName);

    // check if given file name is a valid depot file name with extension
    extern BASE_RESOURCE_API bool ValidateFileNameWithExtension(StringView fileName);

    // check if given depot path is valid
    extern BASE_RESOURCE_API bool ValidateDepotPath(StringView path, DepotPathClass pathClass = DepotPathClass::AbsoluteFilePath);

    // classify path based on content
    extern BASE_RESOURCE_API DepotPathClass ClassifyDepotPath(StringView path);

    //--

} // base

#include "resourcePath.inl"