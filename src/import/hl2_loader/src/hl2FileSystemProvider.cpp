/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"
#include "hl2FileSystem.h"
#include "hl2FileSystemIndex.h"
#include "hl2FileSystemProvider.h"
#include "base/io/include/ioSystem.h"

namespace hl2
{
    RTTI_BEGIN_TYPE_CLASS(FileSystemProviderVPK);
    RTTI_END_TYPE();

    FileSystemProviderVPK::FileSystemProviderVPK()
    {}

    static base::StringBuf FindSteamGameDirectory()
    {
        char steamDirectoryTxt[512];
        memset(steamDirectoryTxt, 0, sizeof(steamDirectoryTxt));
        uint32_t steamDirectoryLength = sizeof(steamDirectoryTxt);

        if (!base::GetRegistryKey("SOFTWARE\\Wow6432Node\\Valve\\Steam", "InstallPath", steamDirectoryTxt, steamDirectoryLength))
        {
            TRACE_ERROR("Steam not installed");
            return base::StringBuf();
        }

        TRACE_INFO("Found Steam installation at '{}'", steamDirectoryTxt);

        base::StringBuf libFoldersPath = base::TempString("{}/steamapps/libraryfolders.vdf", steamDirectoryTxt);
        auto libFolderData = base::io::LoadFileToBuffer(libFoldersPath);
        auto libFolder = base::StringView(libFolderData);
        if (libFolder.empty())
        {
            TRACE_ERROR("Missing steamapps\\libraryfolders.vdf in Steam installation");
            return base::StringBuf();
        }

        auto gameFolderTxt = libFolder.afterFirst("\"1\"").afterFirst("\"").beforeFirst("\"");
        if (gameFolderTxt.empty())
        {
            TRACE_ERROR("Game library folder not found in 'steamapps\\libraryfolders.vdf'");
            return base::StringBuf();
        }

        base::StringBuf gameFolderPath = base::TempString("{}/steamapps/common/", gameFolderTxt);
        TRACE_INFO("Found Steam game library at '{}'", gameFolderPath);
        return gameFolderPath;
    }

    static base::StringBuf FindHL2InstallDirectory()
    {
        // user specified directory
        if (auto installDirStr = base::GetEnv("HL2_INSTALL_DIR"))
        {
            if (*installDirStr)
            {
                TRACE_INFO("Found setup for HL2_INSTALL_DIR = '{}'", installDirStr);
                return base::StringBuf(installDirStr);
            }
        }

        // find steam game dir
        if (auto steamGameDir = FindSteamGameDirectory())
        {
            base::StringBuf testFolder = base::TempString("{}Half-Life 2/", steamGameDir);
            if (base::io::FileExists(base::TempString("{}hl2.exe")))
                return testFolder;
        }

        return base::StringBuf();
    }

    base::UniquePtr<base::depot::IFileSystem> FileSystemProviderVPK::createFileSystem(base::depot::DepotStructure* owner) const
    {
        // find game installation
        auto installPath = FindHL2InstallDirectory();
        if (installPath.empty())
        {
            TRACE_ERROR("No 'Half Life 2' type game installed");
            return nullptr;
        }

        // build path to packages
        base::StringBuf packagePath = base::TempString("{}hl2/", installPath);
        TRACE_INFO("Found installed 'Half Life 2' type game, looking for packages at '{}'", packagePath);

        // load/build the index
        auto index = FileSystemIndex::Load(packagePath);
        if (!index)
        {
            TRACE_ERROR("No pak files found at '{}', verify that HL2_INSTALL_DIR is valid", packagePath);
            return nullptr;
        }

        // create wrapper
        //return base::CreateUniquePtr<PackedFileSystem>(packagePath, std::move(index));
        return nullptr;
    }

} // hl2