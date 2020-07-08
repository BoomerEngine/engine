/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\xml\loader #]
***/

#pragma once

#include "base/object/include/streamTextReader.h"
#include "base/object/include/serializationLoader.h"
#include "base/object/include/streamTextReader.h"
#include "base/xml/include/public.h"
#include "base/containers/include/hashMap.h"
#include "base/containers/include/array.h"

namespace base
{
    namespace res
    {
        namespace xml
        {
            namespace prv
            {
                class LoaderObjectRegistry;
                class LoaderReferenceResolver;

                class LoaderState;
                typedef RefPtr<LoaderState> LoaderStatePtr;

                /// instance of the loader
                class LoaderState : public IReferencable
                {
                public:
                    LoaderState();
                    virtual ~LoaderState();

                    // load objects from given XML data buffer
                    // the buffer is processed and objects are created
                    static LoaderStatePtr LoadContent(const Buffer& xmlData, const stream::LoadingContext& context);

                    /// post load all objects
                    void postLoad();

                    /// extract result
                    void extractResults(stream::LoadingResult& result) const;

                private:
                    UniquePtr<LoaderObjectRegistry> m_objectRegistry;
                    UniquePtr<LoaderReferenceResolver> m_referenceResolver;

                    ClassType m_selectiveLoadingClass;
                    res::IResourceLoader* m_resourceLoader;
                    bool m_selectiveLoadingObjectCreated;

                    // create the actual objects and map the ones with IDs to the map
                    bool createObjects(const base::xml::IDocument& doc, base::xml::NodeID id, const ObjectPtr& parentObject, const ObjectPtr& rootObject);

                    // create actual object from node definition
                    bool createSingleObject(const base::xml::IDocument& doc, base::xml::NodeID objectNodeID, const ObjectPtr& parentObject, ObjectPtr& outCreatedObject);

                    // load object content, use the provided resource loader to resolve dependencies
                    bool loadObjects(const base::xml::IDocument& doc);

                    // load particular object
                    bool loadSingleObject(const base::xml::IDocument& doc, base::xml::NodeID objectNodeID, const ObjectPtr& objectPtr);

                };

            } // prv
        } // binary
    } // res
} // base
