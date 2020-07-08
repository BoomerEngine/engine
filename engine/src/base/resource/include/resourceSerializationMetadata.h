/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\metadata #]
***/

#include "base/object/include/rttiMetadata.h"

namespace base
{
    //------

    // Defines loader that should be used for loading the object
    // NOTE: this is used only if the object is the root object of the serialization (e.g. a resource)
    class BASE_RESOURCE_API SerializationLoaderMetadata : public rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SerializationLoaderMetadata, rtti::IMetadata);

    public:
        SerializationLoaderMetadata();
        ~SerializationLoaderMetadata();

        template< typename T >
        INLINE SerializationLoaderMetadata& bind()
        {
            m_createLoader = []() { return CreateSharedPtr<T>(); };
            return *this;
        }

        // create instance of the loader
        RefPtr<stream::ILoader> createLoader() const;

    private:
        typedef std::function<RefPtr<stream::ILoader>()>  TCreateLoaderFunc;
        TCreateLoaderFunc m_createLoader;
    };

    //------

    // Defines saver that should be used for saving the object
    // NOTE: this is used only if the object is the root object of the serialization (e.g. a resource)
    class BASE_RESOURCE_API SerializationSaverMetadata : public rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SerializationSaverMetadata, rtti::IMetadata);

    public:
        SerializationSaverMetadata();
        ~SerializationSaverMetadata();

        template< typename T >
        INLINE SerializationSaverMetadata& bind()
        {
            m_createSaver = []() { return CreateSharedPtr<T>(); };
            return *this;
        }

        // create instance of the saver
        RefPtr<stream::ISaver> createSaver() const;

    private:
        typedef std::function<RefPtr<stream::ISaver>()>  TCreateSaverFunc;
        TCreateSaverFunc m_createSaver;
    };

    //------

} // base
