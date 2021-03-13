/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "editor_assets_glue.inl"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

struct ResourceInfo
{
    ResourcePtr resource; // loaded resource
    ResourceMetadataPtr metadata; // loaded resource metadata

    StringBuf metadataDepotPath; // path to metadata file, always .xmeta, always valid
    StringBuf resourceExtension; // asset extension (some editors are special)
    StringBuf resourceDepotPath; // path to asset file, may NOT be xfile

    bool customFormat = false; // is this a native file (our own serialization, or is this a custom format - csv, txt, etc)
};

class ResourceEditor;
typedef RefPtr<ResourceEditor> ResourceEditorPtr;

class IResourceEditorAspect;
typedef RefPtr<IResourceEditorAspect> ResourceEditorAspectPtr;

//--

static const uint32_t FILE_MARK_COPY = 1;
static const uint32_t FILE_MARK_CUT = 2;

class AssetBrowserWindow;
class AssetBrowserService;
class AssetBrowserTabFiles;
class AssetBrowserTreePanel;

class AssetImportStatusCheck;
typedef RefPtr<AssetImportStatusCheck> AssetImportStatusCheckPtr;
typedef RefWeakPtr<AssetImportStatusCheck> AssetImportStatusCheckWeakPtr;

//--

class IChangelist;
typedef RefPtr<IChangelist> ChangelistPtr;

class IVersionControl;
typedef UniquePtr<IVersionControl> VersionControlPtr;

struct VersionControlFileState;
struct VersionControlResult;

//--

END_BOOMER_NAMESPACE_EX(ed)


