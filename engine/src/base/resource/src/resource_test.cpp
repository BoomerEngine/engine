/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"
#include "base/containers/include/public.h"
#include "base/containers/include/hashMap.h"
#include "base/system/include/scopeLock.h"
#include "base/io/include/ioFileHandleMemory.h"

#include "resource.h"
#include "resourceLoader.h"
#include "resourcePath.h"
#include "resourceTags.h"
#include "resourceFileSaver.h"

DECLARE_TEST_FILE(Resource);

using namespace base;

namespace tests
{
    class TestResource : public res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TestResource, res::IResource);

    public:
        TestResource()
        {};

        StringBuf m_data;
        res::Ref<TestResource> m_ref;
    };

    RTTI_BEGIN_TYPE_CLASS(TestResource);
        RTTI_PROPERTY(m_data);
        RTTI_PROPERTY(m_ref);
        RTTI_METADATA(res::ResourceExtensionMetadata).extension("dupa");
    RTTI_END_TYPE();

    class SimpleRawLoader : public res::IResourceLoader
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SimpleRawLoader, res::IResourceLoader);

    public:
        SimpleRawLoader()
        {}

        virtual void update() override
        {

        }

        virtual bool initialize(const app::CommandLine& cmdLine) override
        {
            return true;
        }

        void addBufferResource(StringView<char> path, const Buffer& buf, ClassType classPtr)
        {
            // create entry
            auto entry  = MemNew(FakeResource);
            entry->key = res::ResourceKey(path, classPtr.cast<res::IResource>());
            entry->data = buf;
            m_resources.set(entry->key, entry);
        }

        void addObjectResource(StringView<char> path, const res::ResourceHandle& resource)
        {
            base::io::MemoryWriterFileHandle writer;

            base::res::FileSavingContext context;
            context.rootObject.pushBack(resource);

            ASSERT_TRUE(base::res::SaveFile(&writer, context));

            const auto data = writer.extract();

            ASSERT_FALSE(data.empty());

            addBufferResource(path, data, resource->cls());
        }

        virtual bool acquireLoadedResource(const res::ResourceKey& key, res::ResourceHandle& outRet) override final
        {
            return false;
        }

        virtual CAN_YIELD res::ResourceHandle loadResource(const res::ResourceKey& key) override final
        {
/*            FakeResource* entry = nullptr;

            {
                ScopeLock<> lock(m_lock);
                if (!m_resources.find(key, entry))
                    return false;
            }


            // determine loader to use for the resource, 99% it will be the binary loader
            auto& loaderMetaData = key.cls()->findMetadataRef<SerializationLoaderMetadata>();
            auto loader = loaderMetaData.createLoader();
            ASSERT_EX(loader, "Failed to create loader for resource");

            stream::LoadingContext loadingContext;
            loadingContext.m_parent = nullptr; // resources from files are never parented
            loadingContext.m_resourceLoader = this;
            loadingContext.m_contextName = StringBuf(key.path().view());

            // find a resource loader capable of loading resource
            stream::LoadingResult loadingResults;
            stream::MemoryReader memoryReader(entry->data);
            if (!loader->loadObjects(memoryReader, loadingContext, loadingResults) || loadingResults.m_loadedRootObjects.empty())
            {
                TRACE_ERROR("Failed to deserialize cached version of '{}'", key);
                return false;
            }

            // get the root object
            return rtti_cast<res::IResource>(loadingResults.m_loadedRootObjects[0]);*/
            return nullptr;
        }

    private:
        struct FakeResource
        {
            res::ResourceKey key;
            Buffer data;
        };

        typedef HashMap< res::ResourceKey, FakeResource* > TResources;
        TResources m_resources;

        Mutex m_lock;
    };

    RTTI_BEGIN_TYPE_CLASS(SimpleRawLoader);
    RTTI_END_TYPE();

} // tests

static auto RES_A = "resa.dupa";
static auto RES_B = "resb.dupa";
static auto RES_C = "resc.dupa";

static auto RES_MISSING = "missing.dupa";
static auto RES_CROPPED = "crop.dupa";
static auto RES_INVALID = "invalid.dupa";
static auto RES_TRICKY = "tricky.dupa";

class ResourceFixture : public ::testing::Test
{
public:
    ResourceFixture()
    {}

    virtual ~ResourceFixture()
    {
    }

    virtual void SetUp() override final
    {
        m_loader = CreateUniquePtr<tests::SimpleRawLoader>();

        // simple valid resource
        {
            auto obj = CreateSharedPtr<tests::TestResource>();
            obj->m_data = "This is a resource test";
            m_loader->addObjectResource(RES_A, obj);
        }

        // resource with connection to A
        {
            auto obj = CreateSharedPtr<tests::TestResource>();
            obj->m_data = "This is another resource";
            //((res::BaseReference&)obj->m_ref).set(res::ResourceKey(RES_A, tests::TestResource::GetStaticClass()));
            m_loader->addObjectResource(RES_B, obj);
        }

        // invalid resource
        {
            auto buf = Buffer::Create(POOL_TEMP, 2000);

            auto ptr = buf.data();
            for (uint32_t i = 0; i < buf.size(); ++i)
                ptr[i] = (uint8_t)rand();

            m_loader->addBufferResource(RES_INVALID, buf, tests::TestResource::GetStaticClass());
        }

        // cropped resource
        {
            auto buf = Buffer::Create(POOL_TEMP, 20);

            auto ptr = buf.data();
            for (uint32_t i = 0; i < buf.size(); ++i)
                ptr[i] = (uint8_t)rand();

            m_loader->addBufferResource(RES_CROPPED, buf, tests::TestResource::GetStaticClass());
        }
    }

    virtual void TearDown() override final
    {
        m_loader.reset();
    }

    INLINE res::IResourceLoader& loader() const
    {
        return *m_loader;
    }

    
private:
    UniquePtr<tests::SimpleRawLoader> m_loader;
};

/*TEST_F(ResourceFixture, ResourceLocatedInDepot)
{
    res::ResourceMountPoint mountPoint;
    EXPECT_TRUE(rawLoader().supportsLoading(RES_A, tests::TestResource::GetStaticClass(), mountPoint)) << "This resource should be found";
    EXPECT_FALSE(rawLoader().supportsLoading(RES_A, res::IResource::GetStaticClass(), mountPoint)) << "This resource should be not be found because of wrong class";
    EXPECT_FALSE(rawLoader().supportsLoading(RES_MISSING, tests::TestResource::GetStaticClass(), mountPoint)) << "This resource should not be found";
}*/

/*TEST_F(ResourceFixture, TokenForMissingResourceNotCreated)
{
auto token = loader().issueLoadingRequest(RES_MISSING);
ASSERT_TRUE(token != nullptr) << "Token should be returned";
ASSERT_TRUE(token->hasFinished()) << "Failed resources should be marked right away";
ASSERT_TRUE(token->hasFailed()) << "Token should be invalid";
}*/

TEST_F(ResourceFixture, ResourceLoads)
{
    auto resource = loader().loadResource<tests::TestResource>(RES_A);
    ASSERT_TRUE(!!resource) << "No resource loaded";

    ASSERT_TRUE(resource->m_data == "This is a resource test");
}

TEST_F(ResourceFixture, InvalidResource)
{
    auto resource = loader().loadResource<tests::TestResource>(RES_INVALID);
    ASSERT_FALSE(resource);
}

TEST_F(ResourceFixture, CroppedResource)
{
    auto resource = loader().loadResource<tests::TestResource>(RES_INVALID);
    ASSERT_FALSE(resource);
}

TEST_F(ResourceFixture, ResourceLoadsWithImports)
{
    auto resource = loader().loadResource<tests::TestResource>(RES_B);
    ASSERT_TRUE(!!resource);

    ASSERT_TRUE(resource->m_data == "This is another resource");

    ASSERT_TRUE(!resource->m_ref.empty()) << "Pointer to other object is missing";

    //auto other = resource->stub.peak();
    //ASSERT_TRUE(!!other) << "Other object is missing";
    //ASSERT_TRUE(other->data == "This is a resource test");
    ASSERT_STREQ(RES_A, resource->m_ref.key().view().data());
}

//----

class ResourceFixtureXML : public ::testing::Test
{
public:
    ResourceFixtureXML()
        : m_loader(nullptr)
    {
    }

    virtual ~ResourceFixtureXML()
    {
    }

    virtual void SetUp() override final
    {
        m_loader = base::CreateUniquePtr<tests::SimpleRawLoader>();

        // simple valid resource
        {
            auto obj = CreateSharedPtr<tests::TestResource>();
            obj->m_data = "This is a resource test";
            m_loader->addObjectResource(RES_A, obj);
        }

        // resource with connection to A
        {
            auto obj = CreateSharedPtr<tests::TestResource>();
            obj->m_data = "This is another resource";
            //((res::BaseReference&)obj->m_ref).set(res::ResourceKey(RES_A, tests::TestResource::GetStaticClass()));
            m_loader->addObjectResource(RES_B, obj);
        }
    }

    virtual void TearDown() override final
    {
        m_loader.reset();
    }

    INLINE res::IResourceLoader& loader() const
    {
        return *m_loader;
    }    

private:
    UniquePtr<tests::SimpleRawLoader> m_loader;
};

TEST_F(ResourceFixtureXML, ResourceLoads)
{
    auto resource = loader().loadResource<tests::TestResource>(RES_A);
    ASSERT_TRUE(!!resource) << "There should be a token";

    ASSERT_TRUE(resource->m_data == "This is a resource test");
}

TEST_F(ResourceFixtureXML, ResourceLoadsWithImports)
{
    auto resource = loader().loadResource<tests::TestResource>(RES_B);
    ASSERT_TRUE(!!resource) << "No resource loaded";

    ASSERT_TRUE(resource->m_data == "This is another resource");

    ASSERT_TRUE(!resource->m_ref.empty()) << "Pointer to other object is missing";

    //.auto other = resource->stub.peak();
    //ASSERT_TRUE(!!other) << "Other object is missing";
    //ASSERT_TRUE(other->data == "This is a resource test");
    ASSERT_STREQ(RES_A, resource->m_ref.key().view().data());
}