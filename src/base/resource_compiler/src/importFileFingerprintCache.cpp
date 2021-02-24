/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importFileFingerprint.h"
#include "importFileFingerprintCache.h"
#include "base/resource/include/resourceTags.h"

BEGIN_BOOMER_NAMESPACE(base::res)

//--

RTTI_BEGIN_TYPE_CLASS(ImportFileFingerprintTimestampEntry);
    RTTI_PROPERTY(timestamp);
    RTTI_PROPERTY(fingerprint);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_CLASS(ImportFileFingerprintCacheEntry);
    RTTI_PROPERTY(absolutePathUTF8);
    RTTI_PROPERTY(timestampEntries);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(ImportFingerprintCache);
    RTTI_METADATA(ResourceExtensionMetadata).extension("v4fingerprints");
    RTTI_METADATA(ResourceDescriptionMetadata).description("Fingerprint Cache");
    RTTI_METADATA(ResourceTagColorMetadata).color(0x70, 0x70, 0x70);
    RTTI_PROPERTY(m_entries);
RTTI_END_TYPE();

ImportFingerprintCache::ImportFingerprintCache()
{}

ImportFingerprintCache::~ImportFingerprintCache()
{}

void ImportFingerprintCache::clear()
{
    m_entries.clear();
    m_entriesMap.clear();
    markModified();
}

void ImportFingerprintCache::conformPath(StringView path, StringBuf& outPath) const
{
    outPath = StringBuf(path);
    //outPath = outPath.toLower();
    //outPath.replaceChar('\\', '/');
}

bool ImportFingerprintCache::findEntry(StringView path, io::TimeStamp timestamp, ImportFileFingerprint& outFingerprint)
{
    DEBUG_CHECK_EX(path, "Invalid path");
    DEBUG_CHECK_EX(!timestamp.empty(), "Invalid timestamp");

    if (!path || timestamp.empty())
        return false;

    StringBuf conformedPath;
    conformPath(path, conformedPath);

    uint32_t index = 0;
    if (!m_entriesMap.find(conformedPath, index))
        return false;

    // TODO: sorted array

    const auto& entry = m_entries[index];
    const auto timestampValue = timestamp.value();
    for (const auto& timeEntry : entry.timestampEntries)
    {
        if (timeEntry.timestamp == timestampValue)
        {
            outFingerprint = timeEntry.fingerprint;
            return true;
        }
    }

    return false;
}

void ImportFingerprintCache::storeEntry(StringView path, io::TimeStamp timestamp, const ImportFileFingerprint& fingerprint)
{
    DEBUG_CHECK_EX(path, "Invalid path");
    DEBUG_CHECK_EX(!timestamp.empty(), "Invalid timestamp");

    if (!path || timestamp.empty())
        return;

    StringBuf conformedPath;
    conformPath(path, conformedPath);

    // find/create entry
    uint32_t index = 0;
    if (!m_entriesMap.find(conformedPath, index))
    {
        index = m_entries.size();
        auto& entry = m_entries.emplaceBack();
        entry.absolutePathUTF8 = conformedPath;
        m_entriesMap[conformedPath] = index;
    }

    // find/create time entry
    auto& entry = m_entries[index];
    const auto timestampValue = timestamp.value();
    for (auto& timeEntry : entry.timestampEntries)
    {
        if (timeEntry.timestamp == timestampValue)
        {
            TRACE_INFO("Fingerprint: Stored fingerprint for '{}' at {}: {}", path, timestamp, fingerprint);
            timeEntry.fingerprint = fingerprint;
            markModified();
            return;
        }
    }

    // add new entry
    auto& timeEntry = entry.timestampEntries.emplaceBack();
    timeEntry.fingerprint = fingerprint;
    timeEntry.timestamp = timestampValue;
    TRACE_INFO("Fingerprint: Stored NEW fingerprint for '{}' at {}: {}", path, timestamp, fingerprint);

    markModified();
}

void ImportFingerprintCache::rebuildMap()
{
    m_entriesMap.reserve(m_entries.size());

    for (uint32_t i = 0; i < m_entries.size(); ++i)
    {
        TRACE_INFO("Fingerprint: Loaded entry '{}': {}", m_entries[i].absolutePathUTF8, i);
        m_entriesMap[m_entries[i].absolutePathUTF8] = i;
    }
}

void ImportFingerprintCache::onPostLoad()
{
    TBaseClass::onPostLoad();
    rebuildMap();
}

//--

END_BOOMER_NAMESPACE(base::res)
