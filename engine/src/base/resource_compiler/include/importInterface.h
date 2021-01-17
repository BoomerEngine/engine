/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

namespace base
{
    namespace res
    {

        //--

        /// asset import interface
        class BASE_RESOURCE_COMPILER_API IResourceImporterInterface : public IProgressTracker
        {
        public:
            virtual ~IResourceImporterInterface();

            //--

            /// Get the existing resource data (only valid if resource is already loaded)
            /// This data CAN be used to speed up importing (ie. reuse data that would not change) AND/OR transfer information (ie. material bindings)
            /// NOTE: CAN be null and SHOULD NOT BE MODIFIED!!! 
            /// NOTE: it's cleaner if importer reads all reimport settings from the resource configuration objects instead of directly hoarding them in resource
            virtual const IResource* existingData() const = 0;

            /// Get the existing resource data of specific class (only valid if resource is already loaded)
            template< typename T >
            INLINE const T* existingData() const
            {
                return rtti_cast<const T*>(existingData());
            }

            //--

            /// get the path to the resource being imported, this is a depot path
            virtual const StringBuf& queryResourcePath() const = 0;

            /// get the path to the source data
            virtual const StringBuf& queryImportPath() const = 0;

            /// get the configuration object for the importer
            virtual const ResourceConfiguration* queryConfigrationTypeless() const = 0;

            /// get specific configuration object of given type
            template< typename T >
            INLINE const T* queryConfigration() const
            {
                return rtti_cast<T>(queryConfigrationTypeless());
            }

            //--

            /// load content of a file to a buffer (can also open a memory mapped file for actual disk files, but it's going to be transparent any way)
            /// NOTE: we can open ANY source file here but usually we open the one we want to import ie. the queryImportPath()
            /// Good example of importer that may open another file is the .obj importer that can read materials from .mtl
            virtual Buffer loadSourceFileContent(StringView assetImportPath) const = 0;

            /// load source asset of given type
            virtual SourceAssetPtr loadSourceAsset(StringView assetImportPath) const = 0;

            //--

            /// helper: find a dependency file in this folder or parent folder but looking at different sub folders
            /// For example assume you are importing "rendering/assets/meshes/box.fbx"
            /// Your texture path is "D:\Work\Assets\Box\Textures\wood.jpg"
            /// This function if check for existence of following files and pick the first that exists:
            ///  "rendering/assets/meshes/wood.jpg"
            ///  "rendering/assets/meshes/Textures/wood.jpg"
            ///  "rendering/assets/meshes/Box/Textures/wood.jpg"
            ///  "rendering/assets/wood.jpg"
            ///  "rendering/assets/Textures/wood.jpg"
            ///  "rendering/assets/Box/Textures/wood.jpg"
            ///  "rendering/wood.jpg"
            ///  "rendering/Textures/wood.jpg"
            ///  "rendering/Box/Textures/wood.jpg"
            ///  etc, up to the maxScanDepth
            virtual bool findSourceFile(StringView assetImportPath, StringView inputPath, StringBuf& outImportPath, uint32_t maxScanDepth = 2) const = 0;

            // same as findSourceFile but finds files in depot
            virtual bool findDepotFile(StringView depotReferencePath, StringView depotSearchPath, StringView searchFileName, StringBuf& outDepotPath, uint32_t maxScanDepth = 2) const = 0;

            // check if depot file exists
            virtual bool checkDepotFile(StringView depotPath) const = 0;

            //--

            /// report a follow up import (other asset that we should import automatically)
            virtual void followupImport(StringView assetImportPath, StringView depotPath, const ResourceConfiguration* config = nullptr) = 0;

            //--

        };

        //--

        /// list the PRIMARY configuration classes that are for importing asset via given importer class
        class BASE_RESOURCE_COMPILER_API ResourceImporterConfigurationClassMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceImporterConfigurationClassMetadata, rtti::IMetadata);

        public:
            ResourceImporterConfigurationClassMetadata();

            template< typename T >
            INLINE ResourceImporterConfigurationClassMetadata& configurationClass()
            {
                static_assert(std::is_base_of< res::ResourceConfiguration, T >::value, "Only resource configuration classes can be specified here");
                m_class = T::GetStaticClass();
                return *this;
            }

            INLINE const SpecificClassType<ResourceConfiguration>& configurationClass() const
            {
                return m_class;
            }

        private:
            SpecificClassType<ResourceConfiguration> m_class;
        };

        //--

        /// asset importer, class accessible strictly only in the dev projects
        class BASE_RESOURCE_COMPILER_API IResourceImporter : public IReferencable
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IResourceImporter);

        public:
            virtual ~IResourceImporter();

            /// import asset, progress can be reported directly via the importer interface
            /// NOTE: this function should return imported asset WITHOUT metadata (As this is set by the importer itself
            virtual ResourcePtr importResource(IResourceImporterInterface& importer) const = 0;

            ///--

            /// list all resource classes that are importable (at least one importer exists)
            static void ListImportableResourceClasses(Array<SpecificClassType<IResource>>& outResourceClasses);

            /// find all resource classes that given file extension can be imported into, returns false if list is empty
            static bool ListImportableResourceClassesForExtension(StringView fileExtension, Array<SpecificClassType<IResource>>& outResourceClasses);

            /// find all configuration classes for importing a given file extension to given class, returns false is resource is not importable
            static bool ListImportConfigurationForExtension(StringView fileExtension, SpecificClassType<IResource> targetClass, SpecificClassType<ResourceConfiguration>& outConfigurationClass);

            /// list all importable file extensions for given resource class
            static bool ListImportableExtensionsForClass(SpecificClassType<IResource> resourceClasses, Array<StringView>& outExtensions);

            ///--
        };

        //--

    } // res
} // base