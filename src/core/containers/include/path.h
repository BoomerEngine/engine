/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

// given a path as a context and a relative path (that can contain .. and .) builds a new path
// NOTE: if the relative path is given in absolute format (starting with "/" then the context path is not used)
// NOTE: if context path is a file path (ie. contains the file name) then that file name is ignored
extern CORE_CONTAINERS_API bool ApplyRelativePath(StringView contextPath, StringView relativePath, StringBuf& outPath);

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
extern CORE_CONTAINERS_API bool ScanRelativePaths(StringView contextPath, StringView pathParts, uint32_t scanDepth, StringBuf& outPath, const std::function<bool(StringView)>& testFunc);

// given a base path and a second path (both absolute) build a relative path that takes you from base to second
extern CORE_CONTAINERS_API bool BuildRelativePath(StringView basePath, StringView targetPath, StringBuf& outRelativePath);

//--

// check if given character can be used in a file name
extern CORE_CONTAINERS_API bool IsValidPathChar(uint32_t ch);

// given a wanted name of something return file name that is safe to use in the file system
extern CORE_CONTAINERS_API bool MakeSafeFileName(StringView text, StringBuf& outFixedFileName);

// check if given file name is a valid depot file name (just name, no extension) - follows Windows guidelines + does NOT allow dot in the file name
extern CORE_CONTAINERS_API bool ValidateFileName(StringView fileName);

// check if given file name is a valid depot file name with extension
extern CORE_CONTAINERS_API bool ValidateFileNameWithExtension(StringView fileName);

// check if given depot file path is valid
extern CORE_CONTAINERS_API bool ValidateDepotFilePath(StringView path);

// check if given depot directory path is valid
extern CORE_CONTAINERS_API bool ValidateDepotDirPath(StringView path);

// conform a path to depot file path
extern CORE_CONTAINERS_API StringBuf ConformDepotFilePath(StringView path);

// conform a path to depot directory path
extern CORE_CONTAINERS_API StringBuf ConformDepotDirectoryPath(StringView path);

//--

END_BOOMER_NAMESPACE()
