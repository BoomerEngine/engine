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

            //--

            /// get the path to the resource being imported, this is a depot path
            virtual const ResourcePath& queryResourcePath() const = 0;

            /// get the mount point for cooked resource, this is the base directory of the package the resource is at
            virtual const ResourceMountPoint& queryResourceMountPoint() const = 0;

            /// get the path to the source data
            virtual const StringBuf& queryImportPath() const = 0;

            /// get the configuration object of given type, creates empty one if missing
            /// NOTE: the configuration objects CANNOT BE CHANGED by the importer, they are user-editable or can be set by other tools but during import they are read only
            virtual const IResourceConfiguration* queryConfigration(SpecificClassType<IResourceConfiguration> configClass) const = 0;

            /// get specific configuration object of given type, creates empty one if missing
            /// NOTE: the configuration objects CANNOT BE CHANGED by the importer, they are user-editable or can be set by other tools but during import they are read only
            template< typename T >
            INLINE const T* queryConfigration() const
            {
                return static_cast<const T*>(queryConfigration(T::GetStaticClass()));
            }

            //--

            /// load content of a file to a buffer (can also open a memory mapped file for actual disk files, but it's going to be transparent any way)
            /// NOTE: we can open ANY source file here but usually we open the one we want to import ie. the queryImportPath()
            /// Good example of importer that may open another file is the .obj importer that can read materials from .mtl
            virtual Buffer loadSourceFileContent(StringView<char> assetImportPath) const = 0;

            /// load source asset of given type
            virtual SourceAssetPtr loadSourceAsset(StringView<char> assetImportPath, SpecificClassType<ISourceAsset> sourceAssetClass) const = 0;

            /// load source asset of given type
            template< typename T >
            INLINE RefPtr<T> loadSourceAsset(StringView<char> assetImportPath) const
            {
                return rtti_cast<T>(loadSourceAsset(assetImportPath, T::GetStaticClass()));
            }

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
            virtual bool findSourceFile(StringView<char> assetImportPath, StringView<char> inputPath, StringBuf& outImportPath, uint32_t maxScanDepth = 2) const = 0;

            //--

            /// report a follow up import (other asset that we should import automatically)
            virtual void followupImport(StringView<char> assetImportPath, StringView<char> depotPath, const Array<ResourceConfigurationPtr>& config = Array<ResourceConfigurationPtr>()) = 0;

            //--

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
        };

        //--

    } // res
} // base