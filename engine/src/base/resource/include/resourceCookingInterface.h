/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\cooking  #]
***/

#pragma once

#include "base/resource/include/resource.h"

namespace base
{
    namespace res
    {

        //---

        /// helper class used during cooking of raw resources
        /// main purpose is to monitor the opened files and captures the metadata about them (CRC & timestamp)
        /// all loading from external files during the cooking of a resource shall be done through this interface
        /// NOTE: the only allowed paths are the "resolvable paths" that are relative with respect to the file being cooked
        class BASE_RESOURCE_API IResourceCookerInterface : public IProgressTracker
        {
        public:
            virtual ~IResourceCookerInterface();

            //---

            /// get the path to the resource being processed, this is a depot path
            virtual const ResourcePath& queryResourcePath() const = 0;

            /// get the context path of the resource being cooked (usually it's an absolute file path)
            virtual const StringBuf& queryResourceContextName() const = 0;

            /// get the mount point for cooked resource, this is the base directory of the package the resource is at
            virtual const ResourceMountPoint& queryResourceMountPoint() const = 0;

            /// discover paths in directory, makes a complex dependency on the
            virtual bool discoverResolvedPaths(StringView<char> relativePath, bool recurse, StringView<char> extension, Array<StringBuf>& outFileSystemPaths) = 0;

            /// resolve a resolvable path to a relative depot path
            /// very useful for resources that have relative entries
            virtual bool queryResolvedPath(StringView<char> relativePath, StringView<char> contextFileSystemPath, bool isLocal, StringBuf& outFileSystemPath) = 0;

            /// query a source absolute file path path so it can be nicely printed in case of errors
            virtual bool queryContextName(StringView<char> fileSystemPath, StringBuf& contextName) = 0;
                        
            /// query information about file
            /// NOTE: file will be assumed to be a dependency when you call this
            /// NOTE: getting file CRC is very costly
            virtual bool queryFileInfo(StringView<char> fileSystemPath, uint64_t* outCRC, uint64_t* outSize, io::TimeStamp* outTimeStamp, bool makeDependency=true) = 0;

            /// create a content reader
            virtual io::FileHandlePtr createReader(StringView<char> fileSystemPath) = 0;

            /// load file into a buffer
            virtual Buffer loadToBuffer(StringView<char> fileSystemPath) = 0;

            /// load file into a string
            virtual bool loadToString(StringView<char> fileSystemPath, StringBuf& outString) = 0;

            /// Is this a final cooker ?
            virtual bool finalCooker() const = 0;
       
            //--

            /// load manifest file matching current file we are cooking, extension is determined from manifest type
            /// if the manifest can't be loaded a default one is created
            virtual ResourceHandle loadManifestFile(StringView<char> outputPartName, ClassType expectedManifestClass) = 0;

            template< typename T >
            CAN_YIELD INLINE RefPtr<T> loadManifestFile(StringView<char> outputPartName = StringBuf::EMPTY())
            {
                return rtti_cast<T>(loadManifestFile(outputPartName, T::GetStaticClass()));
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
            /// Once a file is found it's automatically marked as dependency
            /// NOTE: this function only marks the found file as the dependency so the dependency system will properly recognize changes if files found via this function are being moved
            virtual bool findFile(StringView<char> contextPath, StringView<char> inputPath, StringBuf& outFileSystemPath, uint32_t maxScanDepth = 2) = 0;

            //--

            // load a normal fully cooked resource for further processing, ie. load Mesh to process it into RenderMesh
            // this function will copy the dependencies of the original resource file
            virtual ResourceHandle loadDependencyResource(const ResourceKey& key) = 0;

            template< typename T >
            CAN_YIELD INLINE RefPtr<T> loadDependencyResource(const ResourcePath& resourcePath)
            {
                return rtti_cast<T>(loadDependencyResource(ResourceKey(resourcePath, T::GetStaticClass())));
            }
        };

        //---

        // parsing error reporter
        class BASE_RESOURCE_API CookerErrorReporter : public parser::IErrorReporter
        {
        public:
            CookerErrorReporter(IResourceCookerInterface& cooker);

            INLINE uint32_t errors() const { return m_numErrors; }
            INLINE uint32_t warnings() const { return m_numWarnings; }

            void translateContextPath(const parser::Location& loc, parser::Location& outAbsoluteLocation);

            virtual void reportError(const parser::Location& loc, StringView<char> message) override;
            virtual void reportWarning(const parser::Location& loc, StringView<char> message) override;

        private:
            IResourceCookerInterface& m_cooker;
            uint32_t m_numErrors = 0;
            uint32_t m_numWarnings = 0;
        };

        //---

        // include handler that loads appropriate dependencies
        class BASE_RESOURCE_API CookerIncludeHandler : public parser::IIncludeHandler
        {
        public:
            CookerIncludeHandler(IResourceCookerInterface& cooker);

            void addIncludePath(StringView<char> includePath);

            bool checkFileExists(StringView<char> path) const;
            bool resolveIncludeFile(bool global, StringView<char> path, StringView<char> referencePath, StringBuf& outPath) const;

            virtual bool loadInclude(bool global, StringView<char> path, StringView<char> referencePath, Buffer& outContent, StringBuf& outPath) override;

        private:
            IResourceCookerInterface& m_cooker;

            Array<StringBuf> m_includePaths;
            Array<Buffer> m_retainedBuffers;
        };

        //---

    } // res
} // base
