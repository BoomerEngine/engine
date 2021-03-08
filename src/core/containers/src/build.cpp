/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# dependency: base_system, base_memory, base_test #]
***/

#include "build.h"

DECLARE_MODULE(PROJECT_NAME)
{
}

BEGIN_BOOMER_NAMESPACE()

class Base64DecodingTable
{
public:
    Base64DecodingTable()
    {
        memset(decoding, 0, sizeof(decoding));

        const char* ptr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        uint8_t value = 0;
        while (*ptr)
            decoding[*ptr++] = value++ + 1;
    }

    INLINE bool isBase64(char ch) const
    {
        return decoding[ch] != 0;
    }

    INLINE uint8_t value(char ch) const
    {
        return decoding[ch] - 1;
    }

private:
    uint8_t decoding[256];
};

static Base64DecodingTable GBase64Table;

class HexDecodingTable
{
public:
    HexDecodingTable()
    {
        memset(decoding, 0, sizeof(decoding));
        decoding['A'] = 10;
        decoding['B'] = 11;
        decoding['C'] = 12;
        decoding['D'] = 13;
        decoding['E'] = 14;
        decoding['F'] = 15;
        decoding['a'] = 10;
        decoding['b'] = 11;
        decoding['c'] = 12;
        decoding['d'] = 13;
        decoding['e'] = 14;
        decoding['f'] = 15;
        decoding['0'] = 0;
        decoding['1'] = 1;
        decoding['2'] = 2;
        decoding['3'] = 3;
        decoding['4'] = 4;
        decoding['5'] = 5;
        decoding['6'] = 6;
        decoding['7'] = 7;
        decoding['8'] = 8;
        decoding['9'] = 9;
    }
       
    INLINE uint8_t value(char ch) const
    {
        return decoding[ch];
    }

    INLINE uint8_t value2(const char* ch) const
    {
        return (decoding[ch[0]] << 4) | decoding[ch[1]];
    }

private:
    uint8_t decoding[256];
};

static HexDecodingTable GHexTable;

void* DecodeBase64(const char* startTxt, const char* endTxt, uint32_t& outDataSize, const PoolTag& poolID /*= POOL_MEM_BUFFER*/)
{
    if (!startTxt || endTxt == startTxt)
        return nullptr;

    auto length = endTxt - startTxt;
    auto size = (length * 6) / 8;
    auto ret = AllocateBlock(poolID, size, 1, "DecodeBase64");
    auto writePtr  = (uint8_t*)ret;

    uint32_t i = 0;
    uint8_t chars[4];
    while (startTxt < endTxt)
    {
        auto ch = *startTxt++;

        if (ch == '=')
            break;

        if (ch <= ' ') // allow and filter white spaces from BASE64 content
            continue;

        if (!GBase64Table.isBase64(ch))
            break;

        chars[i++] = GBase64Table.value(ch);
        if (i == 4)
        {
            *writePtr++ = (chars[0] << 2) + ((chars[1] & 0x30) >> 4);
            *writePtr++ = ((chars[1] & 0xf) << 4) + ((chars[2] & 0x3c) >> 2);
            *writePtr++ = ((chars[2] & 0x3) << 6) + chars[3];
            i = 0;
        }
    }

    if (i)
    {
        uint8_t data[2];
        data[0] = (chars[0] << 2) + ((chars[1] & 0x30) >> 4);
        data[1] = ((chars[1] & 0xf) << 4) + ((chars[2] & 0x3c) >> 2);

        for (uint32_t j = 0; (j < i - 1); j++)
            *writePtr++ += data[j];
    }

    outDataSize = size;
    return ret;
}
    
//--

char* DecodeCString(const char* startTxt, const char* endTxt, uint32_t& outDataSize, const PoolTag& poolID /*= POOL_MEM_BUFFER*/)
{
    if (!startTxt)
        return nullptr;

    if (endTxt == startTxt)
    {
        outDataSize = 0;
        return GlobalPool<POOL_STRINGS, char>::Alloc(1);
    }

    uint32_t length = 0;
    {
        auto readPtr  = startTxt;
        while (readPtr < endTxt)
        {
            if (*readPtr++ == '\\')
            {
                if (readPtr == endTxt) return nullptr;
                readPtr += 1;
                length += 1;

                if (*readPtr == 'x')
                {
                    readPtr += 2;
                }
            }
            else
            {
                length += 1;
            }
        }

        if (readPtr > endTxt)
            return nullptr;
    }

    auto ret = (char*) AllocateBlock(POOL_STRINGS, length, 1, "DecodeCString");
    auto writePtr  = ret;
    {
        auto readPtr  = startTxt;
        while (readPtr < endTxt)
        {
            auto ch = *readPtr++;
            if (ch == '\\')
            {
                ch = *readPtr++;

                if (ch == 'n')
                    *writePtr++ = 10;
                else if (ch == 'r')
                    *writePtr++ = 13;
                else if (ch == 't')
                    *writePtr++ = 9;
                else if (ch == 'b')
                    *writePtr++ = 8;
                else if (ch == '0')
                    *writePtr++ = 0;
                else if (ch == '\"')
                    *writePtr++ = '\"';
                else if (ch == '\'')
                    *writePtr++ = '\'';
                else if (ch == 'x')
                {
                    *writePtr++ = GHexTable.value2(readPtr);
                    readPtr += 2;
                }
            }
            else
            {
                *writePtr++ = ch;
            }
        }
    }

    outDataSize = length;
    return ret;
}

//--

IProgressTracker::~IProgressTracker()
{}

//--

class DevNullProgressTracker : public IProgressTracker
{
public:
    DevNullProgressTracker() {};
    virtual bool checkCancelation() const override final { return false; }
    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override final { };
};

//--

static DevNullProgressTracker theNullProgressTracker;

IProgressTracker& IProgressTracker::DevNull()
{
    return theNullProgressTracker;
}

//--

END_BOOMER_NAMESPACE()
