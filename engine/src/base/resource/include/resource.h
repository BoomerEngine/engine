/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#pragma once

#include "resourceMountPoint.h"
#include "resourcePath.h"

#include "base/containers/include/hashSet.h"
#include "base/object/include/object.h"

namespace base
{
    namespace res
    {
        class IResourceLoader;
        class IResourceLoaderCached;
        class ResourceLoaderStub;
       
        //-----

        // Base resource class
        class BASE_RESOURCE_API IResource : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(IResource, IObject);

        public:
            // Get the resource key (class + path)
            INLINE const ResourceKey& key() const { return m_key; }

            // Get the resource path used to load this resource
            INLINE const ResourcePath& path() const { return m_key.path(); }

            // Get the mount point for the package this resource was loaded from
            INLINE const ResourceMountPoint& mountPoint() const { return m_mountPoint; }

            // Get runtime only unique ID, allows to identify resources by simpler means than just the number
            // NOTE: the ID gets reset if the resource is reloaded
            INLINE ResourceUniqueID runtimeUniqueId() const { return m_runtimeUniqueId; }

            // Get the resource internal version, changed every time the markModified is called or resource is reloaded
            // Can be used to track global changed on the resource and for example refresh the preview, etc
            INLINE ResourceRuntimeVersion runtimeVersion() const { return m_runtimeVersion; }

            // Get resource metadata, NOTE: valid only for binary resources, can be loaded separately
            INLINE const MetadataPtr& metadata() const { return m_metadata; }

            // Query new version of this resource (if reload was applied)
            INLINE const ResourceHandle& reloadedData() const { return m_reloadedData; }

            // Get modified flag
            INLINE bool modified() const { return m_modified; }

            // Is this a resource stub ?
            INLINE bool stub() const { return m_stub; }

            //--

            IResource();
            virtual ~IResource();

            // mark resource as modified
            // NOTE: this propagates the event to the loader
            // NOTE: the isModified flag is NOT stored in the resource but on the side of the managing structure (like ManagedDepot)
            virtual void markModified() override;

            // invalidate runtime version of the resource, may force users to refresh preview
            void invalidateRuntimeVersion();

            // validate resource - reset the modified flag
            void resetModifiedFlag();

            // bind resource to a loader/key/mountpoint it was loaded from
            void bindToLoader(IResourceLoader* loader, const ResourceKey& key, const ResourceMountPoint& mountPoint, bool stub);

            // Get the raw resource loader used to load this resource
            RefPtr<IResourceLoader> loader() const;

            //---

            // new version of this resource was loaded
            virtual void applyReload(const base::res::ResourceHandle& reloaded);

            //---

            // Find resource class based on the class hash
            static SpecificClassType<IResource> FindResourceClassByHash(ResourceClassHash hash);

            // Find resource class based on resource extension
            static SpecificClassType<IResource> FindResourceClassByExtension(StringView<char> extension);

            // Find resource class based on resource path
            static SpecificClassType<IResource> FindResourceClassByPath(StringView<char> path);

            // Get resource description for given class
            static StringBuf GetResourceDescriptionForClass(ClassType resourceClass);

            // Get resource extension for given class
            static StringBuf GetResourceExtensionForClass(ClassType resourceClass);

            //---

            // Drop any editor only data for this resource
            // Called when building deployable packages
            virtual void discardEditorData();

            //---

            // Retrieve the loading (streaming) distance for this resource
            // NOTE: this is implementation specific
            // NOTE: returns false if there's no well determined streaming distance
            virtual bool calcResourceStreamingDistance(float& outDistance) const;

            //--

            // Bind new metadata
            void metadata(const MetadataPtr& data);

            //--

        private:
            RefWeakPtr<IResourceLoader> m_loader; // loader used to load this resource, NOTE: not set if never loaded
            ResourceKey m_key; // resource path passed to loader to load this resource
            ResourceMountPoint m_mountPoint; // mount point of the package we are coming from

            ResourceUniqueID m_runtimeUniqueId; // resource runtime unique ID, can be used to index maps instead of pointer
            ResourceRuntimeVersion m_runtimeVersion; // internal runtime version of the resource, can be observed and a callback can be attached

            MetadataPtr m_metadata; // source dependencies and stuff
            ResourceHandle m_reloadedData; // new version of this resource

            bool m_modified = false;
            bool m_stub = false;

            //--

            friend class IResourceLoader;
            friend class IResourceLoaderCached;
            friend class ResourceLoaderStub;
        };

        //---

    } // res
} // base