/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/containers/include/stringID.h"

namespace base
{
    // clipboard data
    // NOTE: text data is always stored as UTF16
    class BASE_CONTAINERS_API ClipboardData
    {
    public:
        ClipboardData();
        ClipboardData(const StringBuf& txt);
        ClipboardData(const UTF16StringBuf& txt);
        ClipboardData(const Buffer& buffer, StringID type);
        INLINE ClipboardData(const ClipboardData& other) = default;
        INLINE ClipboardData(ClipboardData&& other) = default;
        INLINE ClipboardData& operator=(const ClipboardData& other) = default;
        INLINE ClipboardData& operator=(ClipboardData&& other) = default;

        // is the data empty ?
        INLINE bool empty() const { return !m_data; }

        // get type of the data
        INLINE StringID type() const { return m_type; }

        // get data itself
        INLINE const Buffer& data() const { return m_data; }

        // is this text data ?
        INLINE bool isText() const { return (m_type == "Text"); }

        //---

        // extract as string (will encode into UTF8)
        StringBuf stringDataUTF8() const;

        // extract as string
        UTF16StringBuf stringDataUTF16() const;

    private:
        StringID m_type;
        Buffer m_data;
    };

    /// base clipboard handler
    class BASE_CONTAINERS_API IClipboardHandler : public base::NoCopy
    {
    public:
        virtual ~IClipboardHandler();

        // do we have data in the clipboard ?
        virtual bool hasClipboardData(StringID dataType) = 0;

        // get data from clipboard of given format
        virtual bool clipboardData(StringID dataType, ClipboardData& outData) = 0;

        // set data to clipboard (multiple formats supported)
        virtual bool clipboardData(const ClipboardData* dataFormats, uint32_t numDataFormats) = 0;
    };

} // base