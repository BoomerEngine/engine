/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: loader #]
***/

#include "build.h"
#include "fbxFileLoaderService.h"

#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/containers/include/inplaceArray.h"
#include "base/resource/include/resource.h"

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

    //static FbxAxisSystem EngineAxisSystem(FbxAxisSystem::eZAxis, (FbxAxisSystem::EFrontVector)-FbxAxisSystem::eParityEven, FbxAxisSystem::eLeftHanded);
    static FbxAxisSystem EngineAxisSystem(FbxAxisSystem::eZAxis, (FbxAxisSystem::EFrontVector)FbxAxisSystem::eParityEven, FbxAxisSystem::eLeftHanded);

    static void ResetPivot(FbxNode* node)
    {
        EFbxRotationOrder order;
        node->GetRotationOrder(FbxNode::eSourcePivot, order);

        if (order == eSphericXYZ)
            order = eEulerXYZ;

        node->SetRotationOrder(FbxNode::eDestinationPivot, order);
        node->SetPivotState(FbxNode::eDestinationPivot, FbxNode::ePivotActive);
        node->SetPivotState(FbxNode::eSourcePivot, FbxNode::ePivotActive);

        node->ResetPivotSet(FbxNode::eDestinationPivot);

        const auto count = node->GetChildCount();
        for (uint32_t i = 0; i < count; ++i)
            ResetPivot(node->GetChild(i));

        node->ResetPivotSetAndConvertAnimation(1.0f, false, false);
    }

    fbxsdk::FbxScene* FileLoadingService::loadScene(const base::Buffer& data, base::Matrix& outAssetToEngineConversionMatrix) const
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

            // fixup scale
            float sceneScaleFactor = (float)lScene->GetGlobalSettings().GetSystemUnit().GetScaleFactor();
            TRACE_INFO("FBX file scale: {}", sceneScaleFactor);

            // Convert axis system to engine 
            //FbxAxisSystem sceneAxisSystem = lScene->GetGlobalSettings().GetAxisSystem();
            //EngineAxisSystem.ConvertScene(lScene);

            // convert to meters
            FbxSystemUnit::m.ConvertScene(lScene);
            FbxSystemUnit::ConversionOptions options;
            options.mConvertRrsNodes = false;
            FbxSystemUnit(sceneScaleFactor).ConvertScene(lScene, options);

            // destroy importer
            lImporter->Destroy();
        }

        /*// Convert axes to engine
        auto conversionMatrix = base::Matrix::IDENTITY();
        {
            // get conversion matrix between our current space and the intended space
            FbxAMatrix a;
            lScene->GetGlobalSettings().GetAxisSystem().GetMatrix(a);

            static FbxAMatrix b;
            b.SetRow(0, FbxVector4(0, 1, 0, 0));
            b.SetRow(1, FbxVector4(0, 0, 1, 0));
            b.SetRow(2, FbxVector4(-1, 0, 0, 0));
            b.SetRow(3, FbxVector4(0, 0, 0, 1));
            //EngineAxisSystem.GetMatrix(b);

            // TODO: would be best to use internal FBX stuff for this but it seams to be broken ;(
            // in ideal world this will be an Identity matrix
            conversionMatrix = ToMatrix(b * a.Inverse());

            // FBX uses cm for it's scale, we use meters, make sure this gets converted
            const auto nativeScaleFactor = base::Vector3(0.01f, 0.01f, 0.01f);
            conversionMatrix.scaleColumns(nativeScaleFactor);
        }

        // remember conversion matrix
        outAssetToEngineConversionMatrix = conversionMatrix;*/

        // reset pivot
        //lScene->GetRootNode()->ResetPivotSet(FbxNode::eDestinationPivot);
        ResetPivot(lScene->GetRootNode());
        //lScene->GetRootNode()->ConvertPivotAnimationRecursive(NULL, FbxNode::eDestinationPivot, 1.0f, false);

        // scene is ready
        return lScene;
    }

    //---

} // fbx
