/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: loader #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/app/include/localService.h"

BEGIN_BOOMER_NAMESPACE(asset)

//---

struct FBXDataNode;
struct FBXSkeletonBone;
struct FBXSkeletonBuilder;
struct FBXMaterialMapper;

//---

// service for  import/export, holds the  singletons
class IMPORT_FBX_LOADER_API FBXFileLoadingService : public base::app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(FBXFileLoadingService, base::app::ILocalService);

public:
    FBXFileLoadingService();
    virtual ~FBXFileLoadingService();

    /// get the raw FbxManager
    INLINE fbxsdk::FbxManager* manager() const { return m_fbxManager; }

    /// load the  blob from a data buffer
    fbxsdk::FbxScene* loadScene(const base::Buffer& data, base::Matrix& outAssetToEngineConversionMatrix) const;


private:
    // ILocalService
    virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    fbxsdk::FbxManager* m_fbxManager;
    base::Mutex m_lock;
    int m_fbxFormatID;
};

//---

END_BOOMER_NAMESPACE(assets)
