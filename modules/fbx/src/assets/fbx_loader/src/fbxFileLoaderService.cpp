/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: loader #]
***/

#include "build.h"
#include "fbxFileData.h"
#include "fbxFileLoaderService.h"

#include "base/io/include/utils.h"
#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/containers/include/inplaceArray.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resourceCookingInterface.h"

namespace fbx
{
    //---

    RTTI_BEGIN_TYPE_CLASS(FileLoadingService);
    RTTI_END_TYPE();

    FileLoadingService::FileLoadingService()
        : m_fbxManager(nullptr)
    {}

    FileLoadingService::~FileLoadingService()
    {}

    base::app::ServiceInitializationResult FileLoadingService::onInitializeService(const base::app::CommandLine& cmdLine)
    {
        m_fbxManager = FbxManager::Create();
        if (!m_fbxManager)
        {
            TRACE_ERROR("Unable to create FBX Manager! FBX import/export will not be avaiable");
            return base::app::ServiceInitializationResult::Silenced;
        }

        TRACE_SPAM("Autodesk FBX SDK version {}", m_fbxManager->GetVersion());

        FbxIOSettings* ios = FbxIOSettings::Create(m_fbxManager, IOSROOT);
        m_fbxManager->SetIOSettings(ios);

	    //FbxString lPath = FbxGetApplicationDirectory();
        //m_fbxManager->LoadPluginsDirectory(lPath.Buffer());

        int lSDKMajor=0, lSDKMinor=0,  lSDKRevision=0;
        FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);
        TRACE_SPAM("Autodesk FBX file version: {}.{}.{}", lSDKMajor, lSDKMinor, lSDKRevision);

        // setup import settings
        ios->SetBoolProp(IMP_FBX_MATERIAL, true);
        ios->SetBoolProp(IMP_FBX_TEXTURE, true);
        ios->SetBoolProp(IMP_FBX_LINK, true);
        ios->SetBoolProp(IMP_FBX_SHAPE, true);
        ios->SetBoolProp(IMP_FBX_GOBO, true);
        ios->SetBoolProp(IMP_FBX_ANIMATION, true);
        ios->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);

        // get format
        m_fbxFormatID = m_fbxManager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX (*.fbx)");

        return base::app::ServiceInitializationResult::Finished;
    }

	extern bool GFBXSceneClosed;

    void FileLoadingService::onShutdownService()
    {
		GFBXSceneClosed = true;

        if (m_fbxManager != nullptr)
        {
            m_fbxManager->Destroy();
            m_fbxManager = nullptr;
        }
    }

    void FileLoadingService::onSyncUpdate()
    {

    }

    LoadedFilePtr FileLoadingService::loadScene(const base::io::AbsolutePath& absPath) const
    {
        auto data = base::io::LoadFileToBuffer(absPath);
        if (!data)
        {
            TRACE_ERROR("Failed to load data from {}", absPath);
            return nullptr;
        }

        return loadScene(data);
    }

    namespace helper
    {

        class MemoryStreamClass : public FbxStream
        {
        public:
            MemoryStreamClass(int formatID, const base::Buffer& data)
                : m_data(data)
                , m_pos(0)
                , m_size(0)
                , m_dataPtr(nullptr)
                , m_formatID(formatID)
            {
                m_dataPtr = m_data.data();
                m_size = m_data.size();
            }

            ~MemoryStreamClass()
            {
                Close();
            }

            virtual EState GetState() override
            {
                return FbxStream::eOpen;
            }

            virtual bool Open( void* /*pStreamData*/ ) override
            {
                m_pos = 0;
                return true;
            }

            virtual bool Close() override
            {
                return true;
            }

            virtual bool Flush() override
            {
                return true;
            }

            virtual int Write(const void* pData, int pSize)
            {
                return 0;
            }

            virtual int Read (void* pData, int pSize) const
            {
                auto sizeLeft = m_size - m_pos;
                if (pSize > sizeLeft)
                    pSize = sizeLeft;

                memcpy(pData, m_dataPtr + m_pos, pSize);
                m_pos += pSize;
                return pSize;
            }

            virtual int GetReaderID() const
            {
                return m_formatID;
            }

            virtual int GetWriterID() const
            {
                return -1;
            }

            void Seek( const FbxInt64& pOffset, const FbxFile::ESeekPos& pSeekPos )
            {
                switch ( pSeekPos )
                {
                    case FbxFile::eBegin:
                        m_pos = (long)pOffset;
                        break;
                    case FbxFile::eCurrent:
                        m_pos += pOffset;
                        break;
                    case FbxFile::eEnd:
                        m_pos = m_size - (long)pOffset;
                        break;
                }
            }

            virtual long GetPosition() const
            {
                return m_pos;
            }

            virtual void SetPosition( long pPosition )
            {
                m_pos = pPosition;
            }

            virtual int GetError() const override
            {
                return 0;
            }

            virtual void ClearError() override
            {

            }

        private:
            base::Buffer m_data;
            const uint8_t* m_dataPtr;
            mutable uint64_t m_pos;
            uint64_t m_size;
            int m_formatID;
        };

    } // helper

    LoadedFilePtr FileLoadingService::loadScene(const base::Buffer& data) const
    {
        FbxImporter* lImporter = nullptr;
        FbxScene* lScene = nullptr;

        // create a memory stream
        helper::MemoryStreamClass stream(m_fbxFormatID, data);

        // create and initialize the importer
        {
            auto lock = base::CreateLock(m_lock);

            // Create an importer.
            auto lImporter = FbxImporter::Create(m_fbxManager, "");
            if (!lImporter)
            {
                TRACE_ERROR("Failed to create FBX importer");
                return nullptr;
            }

            // setup from stream
            if (!lImporter->Initialize(&stream))
            {
                TRACE_ERROR("Call to FbxImporter::Initialize() failed with {}", lImporter->GetStatus().GetErrorString());
                return nullptr;
            }

            // we only support FBX
            if (!lImporter->IsFBX())
            {
                TRACE_ERROR("Loaded file is not a valid FBX file");
                lImporter->Destroy();
                return nullptr;
            }

            // get file version
            int lFileMajor, lFileMinor, lFileRevision;
            lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);
            TRACE_INFO("FBX file version {}.{}.{}", lFileMajor, lFileMinor, lFileRevision);

            // create an empty scene and import content into it
            lScene = FbxScene::Create(m_fbxManager, "");
            if (!lImporter->Import(lScene))
            {
                lScene->Destroy();
                lImporter->Destroy();
                return nullptr;
            }

            //FbxAxisSystem exportAxes(FbxAxisSystem::EPreDefinedAxisSystem::eMax);
            //exportAxes.ConvertScene(lScene);

            // Scene units are meteres
            FbxSystemUnit::m.ConvertScene(lScene);

            // destroy importer
            lImporter->Destroy();
        }

        // Convert axes to engine
        FbxAxisSystem exportAxes(FbxAxisSystem::eZAxis, (FbxAxisSystem::EFrontVector) -FbxAxisSystem::eParityOdd, FbxAxisSystem::eLeftHanded);
        FbxAMatrix a, b;
        lScene->GetGlobalSettings().GetAxisSystem().GetMatrix(a);
        exportAxes.GetMatrix(b);
        auto conversionMatrix = a.Inverse() * b;

        // create blob
        auto ret = base::CreateSharedPtr<LoadedFile>(lScene);
        if (!ret->captureNodes(ToMatrix(conversionMatrix)))
        {
            TRACE_ERROR("FBX file contains broken content");
            return nullptr;
        }

        TRACE_INFO("Found {} node(s) in the FBX", ret->nodes().size());
        return ret;
    }

    LoadedFilePtr FileLoadingService::loadScene(base::res::IResourceCookerInterface& cooker) const
    {
        // load raw data
        auto sourceFilePath = cooker.queryResourcePath().path();
        auto data = cooker.loadToBuffer(sourceFilePath);
        if (!data)
        {
            TRACE_ERROR("Failed to load content of '{}'", sourceFilePath);
            return nullptr;
        }

        // deserialize FBX file
        return loadScene(data);
    }

    //---

} // fbx
