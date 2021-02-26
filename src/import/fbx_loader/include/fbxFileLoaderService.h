/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: loader #]
***/

#pragma once

#include "core/resource/include/resource.h"
#include "core/app/include/localService.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//---

// service for  import/export, holds the  singletons
class IMPORT_FBX_LOADER_API FBXFileLoadingService : public app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(FBXFileLoadingService, app::ILocalService);

public:
    FBXFileLoadingService();
    virtual ~FBXFileLoadingService();

    /// get the raw FbxManager
    INLINE fbxsdk::FbxManager* manager() const { return m_fbxManager; }

    /// load the  blob from a data buffer
    fbxsdk::FbxScene* loadScene(const Buffer& data, Matrix& outAssetToEngineConversionMatrix) const;


private:
    // ILocalService
    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    fbxsdk::FbxManager* m_fbxManager;
    Mutex m_lock;
    int m_fbxFormatID;
};

//---

END_BOOMER_NAMESPACE_EX(assets)
