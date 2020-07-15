/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#include "build.h"
#include "resource.h"
#include "resourceLoader.h"
#include "resourceClassLookup.h"
#include "resourceCookingInterface.h"
#include "resourceMetadata.h"
#include "resourceTags.h"

namespace base
{
    namespace res
    {

        //---

        static std::atomic<ResourceUniqueID> GResourceUniqueID(1);

        //---

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResource);
            RTTI_METADATA(ResourceDataVersionMetadata).version(0);
            RTTI_PROPERTY(m_metadata);
        RTTI_END_TYPE();

        IResource::IResource()
            : m_runtimeUniqueId(++GResourceUniqueID)
            , m_runtimeVersion(0)
            , m_loader(nullptr)
        {}

        IResource::~IResource()
        {
            if (auto loader = m_loader.lock())
            {
                TRACE_INFO("Destroying resource 0x{} '{}'", Hex(this), key());
                loader->notifyResourceUnloaded(m_key);
            }
        }

        void IResource::applyReload(const base::res::ResourceHandle& reloaded)
        {
            DEBUG_CHECK_EX(!m_reloadedData, TempString("Resource '{}' is already reloaded with new data", key()));
            DEBUG_CHECK_EX(key() == reloaded->key(), TempString("Resource '{}' is being reloaded with data of mismatched key '{}'", key(), reloaded->key()));
            m_reloadedData = reloaded;
        }

        void IResource::metadata(const MetadataPtr& data)
        {
            DEBUG_CHECK_EX(!data->parent(), "Metadata is already parented");
            m_metadata = data;
            m_metadata->parent(this);
            m_modified = false;
        }

        void IResource::markModified()
        {
            // this resource may be contained inside other file, we still need to propagate
            TBaseClass::markModified();

            // invalidate runtime version of the resource, this may cause refresh of the preview panels or other cached data
            invalidateRuntimeVersion();

            // mark as modified
            if (!m_modified && !key().empty())
            {
                TRACE_INFO("Resource '{}' marked as modified");
            }
            m_modified = true;

            // notify any object based users of this resource that it has been modified
            postEvent("ResourceModified"_id);
        }


        void IResource::invalidateRuntimeVersion()
        {
            m_runtimeVersion += 1;
        }

        void IResource::resetModifiedFlag()
        {
            if (m_modified && !key().empty())
            {
                TRACE_INFO("Resource '{}' no longer modified", key());
            }

            m_modified = false;
        }

        //--

        SpecificClassType<IResource> IResource::FindResourceClassByHash(ResourceClassHash hash)
        {
            return prv::ResourceClassLookup::GetInstance().resolveResourceClassHash(hash);
        }

        SpecificClassType<IResource> IResource::FindResourceClassByExtension(StringView<char> extension)
        {
            return prv::ResourceClassLookup::GetInstance().resolveResourceExtension(extension);
        }

        SpecificClassType<IResource> IResource::FindResourceClassByPath(StringView<char> path)
        {
            if (path.empty())
                return nullptr;

            auto ext = path.afterFirst(".");
            if (ext.empty())
                return nullptr;

            return FindResourceClassByExtension(ext);
        }

        StringBuf IResource::GetResourceDescriptionForClass(ClassType resourceClass)
        {
            if (!resourceClass)
                return StringBuf("None");

            auto descMetaData  = resourceClass->findMetadata<ResourceDescriptionMetadata>();
            if (descMetaData && descMetaData->description() && *descMetaData->description())
                return StringBuf(descMetaData->description());

            if (resourceClass->shortName())
                return StringBuf(resourceClass->shortName().view());

            return StringBuf(resourceClass->name().view().afterLast("::"));
        }

        StringBuf IResource::GetResourceExtensionForClass(ClassType resourceClass)
        {
            if (!resourceClass)
                return StringBuf::EMPTY();

            auto extMetaData = resourceClass->findMetadata<ResourceExtensionMetadata>();
            if (extMetaData)
                return StringBuf(extMetaData->extension());

            return StringBuf::EMPTY();
        }

        void IResource::discardEditorData()
        {
        }

        bool IResource::calcResourceStreamingDistance(float& outDistance) const
        {
            return false;
        }

        RefPtr<IResourceLoader> IResource::loader() const
        {
            return m_loader.lock();
        }

        void IResource::bindToLoader(IResourceLoader* loader, const ResourceKey& key, const ResourceMountPoint& mountPoint, bool stub)
        {
            ASSERT_EX(m_loader == nullptr, "Resource already has a loader");

            // create the loading fence, it will allow loading of other resources to wait for this resource to load
            m_stub = stub;
            m_loader = loader;
            m_mountPoint = mountPoint;
            m_key = key;
        }

        //--

    } // res
} // base