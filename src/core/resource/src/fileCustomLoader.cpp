/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/
#include "build.h"
#include "resource.h"

#include "fileCustomLoader.h"

#include "core/io/include/asyncFileHandle.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICustomFileLoader);
RTTI_END_TYPE();

ICustomFileLoader::~ICustomFileLoader()
{}

//--

RTTI_BEGIN_TYPE_CLASS(CustomFileLoaderSupportedExtensionMetadata);
RTTI_END_TYPE();

CustomFileLoaderSupportedExtensionMetadata::CustomFileLoaderSupportedExtensionMetadata()
{}

//--

namespace prv
{
    class CustomLoaderLookup : public ISingleton
    {
        DECLARE_SINGLETON(CustomLoaderLookup);

    public:
        CustomLoaderLookup()
        {
            InplaceArray<SpecificClassType<ICustomFileLoader>, 64> loaderClasses;
            RTTI::GetInstance().enumClasses(loaderClasses);

            for (const auto& cls : loaderClasses)
            {
                if (const auto metadata = cls->findMetadata<CustomFileLoaderSupportedExtensionMetadata>())
                {
                    for (const auto& ext : metadata->extensions())
                    {
                        auto& entry = m_entries.emplaceBack();
                        entry.extension = StringBuf(ext);
                        entry.loader = cls;

                        TRACE_INFO("Found custom loader '{}' for extensio '{}'", cls, ext);
                    }
                }
            }

            TRACE_INFO("Found {} custom loaders", m_entries.size());
        }

        struct Entry
        {
            StringBuf extension;
            SpecificClassType<ICustomFileLoader> loader;
        };

        Array<Entry> m_entries;

        virtual void deinit() override
        {
            m_entries.clear();
        }
    };

} // prv

void EnumerateCustomFileExtensions(Array<StringView>& outExtensions)
{
    const auto& entries = prv::CustomLoaderLookup::GetInstance().m_entries;
    outExtensions.reserve(entries.size());

    for (const auto& entry : entries)
        outExtensions.pushBack(entry.extension);
}

SpecificClassType<ICustomFileLoader> FindCustomLoaderForFileExtension(StringView ext)
{
    DEBUG_CHECK_RETURN_EX_V(ext, "Invalid extension", nullptr);

    const auto& entries = prv::CustomLoaderLookup::GetInstance().m_entries;

    for (const auto& entry : entries)
        if (entry.extension.compareWithNoCase(ext) == 0)
            return entry.loader;

    return nullptr;
}

//--

END_BOOMER_NAMESPACE()
