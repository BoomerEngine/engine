/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#pragma once

#include "base/containers/include/array.h"

namespace base
{
    namespace res
    {
        class IResourceLoader;
        class ResourceKey;
    }

    namespace jobs
    {
        class SequentialBuilder;
    }

    namespace stream
    {
        /// Function called when we are restoring an editable object, gives us a chance to reapply ID to it
        typedef std::function<void(IObject* editableObject, uint32_t editableID)> TResotoreIDsFunc;

        /// Dependency loading context
        class BASE_OBJECT_API LoadingContext
        {
        public:
            // Parent root to attach all loaded objects to
            ObjectPtr m_parent;

            // Load imported objects, true by default
            // NOTE: disabling this can improve loading performance and is the only way to load synchronously without blocking
            // NOTE: any later access to resource references will cause synchronous loading
            bool m_loadImports;

            // custom resource loader to load the imports from
            // required to preload resources, if not specified it's left empty
            res::IResourceLoader* m_resourceLoader;

            // load only first object of this class
            // used for selective metadata loading from a file
            ClassType m_selectiveLoadingClass;

            // mutate root object to this class
            ClassType m_rootObjectMutatedClass;

            // base path for translating back relative paths to absolute paths
            // eg. loading relative path "textures/lena.png" when loading with base path "engine/" will restore path to "engine/textures/lena.png"
            StringBuf m_baseReferncePath;

            // Context name for reporting any loading errors, usually a full absolute path of the file being loaded 
            StringBuf m_contextName;

            // TODO: buffer restoration context

            LoadingContext();
        };

        /// Result of loading operation
        class BASE_OBJECT_API LoadingResult
        {
        public:
            typedef Array< ObjectPtr > TLoadedRoots;

            // Root objects loaded from file (not a ISerializable ones)
            TLoadedRoots m_loadedRootObjects;

            LoadingResult();
            ~LoadingResult();
        };

        /// Loading dependency
        struct BASE_OBJECT_API LoadingDependency
        {
            StringBuf resourceDepotPath;
            ClassType resourceClass;
            bool async = false;
        };

        class ILoader;
        typedef RefPtr<ILoader> LoaderPtr;

        /// Abstract loader for serialized data
        class BASE_OBJECT_API ILoader : public IReferencable
        {
        public:
            ILoader();
            virtual ~ILoader();

            // Load object in a from given stream
            // NOTE: the underlying deserialization may cause additional resource to be loaded and this fiber may yield
            virtual CAN_YIELD bool loadObjects(stream::IBinaryReader& file, const LoadingContext& context, LoadingResult& result) = 0;

            // Load minimal amount of data (usually just the file tables) and extract list of other files we considered required for valid use of this file (ie. textures for mesh, etc)
            virtual bool extractLoadingDependencies(stream::IBinaryReader& file, bool includeAsync, Array<LoadingDependency>& outDependencies) = 0;
        };

    } // stream
} // base