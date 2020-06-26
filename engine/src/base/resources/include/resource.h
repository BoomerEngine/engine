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

#include "base/containers/include/mutableArray.h"
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

        /// version of the resource class internal serialization, if this does not match the resource we recook the resource
        class BASE_RESOURCES_API ResourceDataVersionMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceDataVersionMetadata, rtti::IMetadata);

        public:
            ResourceDataVersionMetadata();

            INLINE uint32_t version() const
            {
                return m_version;
            }

            INLINE void version(uint32_t version)
            {
                m_version = version;
            }

        private:
            uint32_t m_version;
        };

        //-----

        // Base resource class
        class BASE_RESOURCES_API IResource : public IObject
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

        //------

        // Resource extension (class metadata)
        class BASE_RESOURCES_API ResourceExtensionMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceExtensionMetadata, rtti::IMetadata);

        public:
            ResourceExtensionMetadata();

            INLINE ResourceExtensionMetadata& extension(const char* ext)
            {
                m_ext = ext;
                return *this;
            }

            INLINE const char* extension() const { return m_ext; }

        private:
            const char* m_ext;
        };

        //------

        // Resource can only be loaded as a baked resource
        class BASE_RESOURCES_API ResourceBakedOnlyMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceBakedOnlyMetadata, rtti::IMetadata);

        public:
            ResourceBakedOnlyMetadata();
        };

        //------

        // Resource extension postfix for manifests
        class BASE_RESOURCES_API ResourceManifestExtensionMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceManifestExtensionMetadata, rtti::IMetadata);

        public:
            ResourceManifestExtensionMetadata();

            INLINE ResourceManifestExtensionMetadata& extension(const char* ext)
            {
                m_ext = ext;
                return *this;
            }

            INLINE const char* extension() const { return m_ext; }

        private:
            const char* m_ext;
        };

        //------

        // Resource description (class metadata)
        class BASE_RESOURCES_API ResourceDescriptionMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceDescriptionMetadata, rtti::IMetadata);

        public:
            ResourceDescriptionMetadata();

            INLINE ResourceDescriptionMetadata& description(const char* desc)
            {
                m_desc = desc;
                return *this;
            }

            INLINE const char* description() const { return m_desc; }

        private:
            const char* m_desc;
        };

        //------

        // Resource editor color
        class BASE_RESOURCES_API ResourceTagColorMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceTagColorMetadata, rtti::IMetadata);

        public:
            ResourceTagColorMetadata();

            INLINE ResourceTagColorMetadata& color(Color color)
            {
                m_color = color;
                return *this;
            }

            INLINE ResourceTagColorMetadata& color(uint8_t r, uint8_t g, uint8_t b)
            {
                m_color = Color(r,g,b);
                return *this;
            }

            INLINE Color color() const { return m_color; }

        private:
            Color m_color;
        };

        //------

        // Text based resource, when possible saved in text formats
        // NOTE: cooked resource is still binary
        class BASE_RESOURCES_API ITextResource : public IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ITextResource, IResource);

        public:
            ITextResource();
            virtual ~ITextResource();
        };

        //------

        // Raw text data - can be used to load raw (unprocessed) content of a depot file
        // NOTE: any file extension is always cookable into this format, 
        // this format allows "classic" cooker-less processing of data when all the loading code is engine-side
        class BASE_RESOURCES_API RawTextData : public IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RawTextData, IResource);

        public:
            RawTextData();
            RawTextData(const StringBuf& data);
            virtual ~RawTextData();

            INLINE const StringBuf& data() const { return m_data; }
            INLINE uint64_t crc() const { return m_crc; }

        private:
            StringBuf m_data;
            uint64_t m_crc;
        };

       //------

       // Raw binary data - can be used to load raw (unprocessed) content of a depot file
       // NOTE: any file extension is always cookable into this format
       // this format allows "classic" cooker-less processing of data when all the loading code is engine-side
        class BASE_RESOURCES_API RawBinaryData : public IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RawBinaryData, IResource);

        public:
            RawBinaryData();
            RawBinaryData(const Buffer& data);
            virtual ~RawBinaryData();

            INLINE const Buffer& data() const { return m_data; }
            INLINE uint64_t crc() const { return m_crc; }

        private:
            Buffer m_data;
            uint64_t m_crc;
        };

        //------

        /// metadata for raw resource to specify the target (imported) class
        class BASE_RESOURCES_API ResourceCookedClassMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceCookedClassMetadata, rtti::IMetadata);

        public:
            ResourceCookedClassMetadata();

            template< typename T >
            INLINE ResourceCookedClassMetadata& addClass()
            {
                static_assert(std::is_base_of< res::IResource, T >::value, "Only resource classes can be imported");
                m_classes.pushBackUnique(T::GetStaticClass());
                return *this;
            }

            INLINE const Array<SpecificClassType<IResource>>& classList() const
            {
                return m_classes;
            }

        private:
            Array<SpecificClassType<IResource>> m_classes;
        };

        //---

        /// metadata for input format supported for cooking (obj, jpg, etc)
        class BASE_RESOURCES_API ResourceSourceFormatMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceSourceFormatMetadata, rtti::IMetadata);

        public:
            ResourceSourceFormatMetadata();

            INLINE ResourceSourceFormatMetadata& addSourceExtension(const char* ext)
            {
                if (ext && *ext)
                    m_extensions.pushBackUnique(ext);
                return *this;
            }

            INLINE ResourceSourceFormatMetadata& addSourceExtensions(const char* extList)
            {
                InplaceArray<StringView<char>, 50> list;
                StringView<char>(extList).slice(";,", false, list);

                for (const auto& ext : list)
                    m_extensions.pushBackUnique(ext);
                return *this;
            }

            INLINE ResourceSourceFormatMetadata& addSourceClass(ClassType sourceClass)
            {
                if (sourceClass)
                    m_classes.pushBackUnique(sourceClass);
                return *this;
            }

            template< typename T >
            INLINE ResourceSourceFormatMetadata& addSourceClass()
            {
                static_assert(std::is_base_of< res::IResource, T >::value, "Only resource classes can be recooked into other resources");
                addSourceClass(T::GetStaticClass());
                return *this;
            }

            INLINE const Array<StringView<char>>& extensions() const
            {
                return m_extensions;
            }

            INLINE const Array<ClassType>& classes() const
            {
                return m_classes;
            }

        private:
            Array<StringView<char>> m_extensions;
            Array<ClassType> m_classes;
        };

        //---

        /// version of the cooker class, if this does not match the resource we recook the resource
        class BASE_RESOURCES_API ResourceCookerVersionMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceCookerVersionMetadata, rtti::IMetadata);

        public:
            ResourceCookerVersionMetadata();

            INLINE uint32_t version() const
            {
                return m_version;
            }

            INLINE void version(uint32_t version)
            {
                m_version = version;
            }

        private:
            uint32_t m_version;
        };

        //---

        /// indicate that cooker can only be used off-line (baked) and not ad-hoc
        class BASE_RESOURCES_API ResourceCookerBakingOnlyMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceCookerBakingOnlyMetadata, rtti::IMetadata);

        public:
            ResourceCookerBakingOnlyMetadata();
        };

        //---

        /// a manifest for cooked file
        /// resources like that are almost always used to bake external data into engine-eatable format
        class BASE_RESOURCES_API IResourceManifest : public res::ITextResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(IResourceManifest, res::ITextResource);

        public:
            IResourceManifest();
            virtual ~IResourceManifest();
        };

        //---

        /// a magical class that can load a resource from raw source data
        /// NOTE: file loaders are for resources that are simple enough to load directly, without lengthy baking
        class BASE_RESOURCES_API IResourceCooker : public base::IReferencable
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IResourceCooker);

        public:
            IResourceCooker();
            virtual ~IResourceCooker();

            /// report manifest classes used during cooking
            virtual void reportManifestClasses(base::Array<base::SpecificClassType<base::res::IResourceManifest>>& outManifestClasses) const;

            /// use the data at your disposal that you can query via the cooker interface
            virtual ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const = 0;
        };

        //---

    } // res
} // base