/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: loader #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/app/include/localService.h"

namespace fbx
{
    //---

    struct DataNode;
    struct SkeletonBone;
    struct SkeletonBuilder;
    struct MaterialMapper;

    //---

    // service for  import/export, holds the  singletons
    class ASSETS_FBX_LOADER_API FileLoadingService : public base::app::ILocalService
    {
        RTTI_DECLARE_VIRTUAL_CLASS(FileLoadingService, base::app::ILocalService);

    public:
        FileLoadingService();
        virtual ~FileLoadingService();

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

} // fbx
