/***
* Boomer Engine v4
* Written by Tomasz "RexDex" Jonarski
*
* [#filter: http #]
***/

#include "build.h"
#include "requestServerHeaders.h"

BEGIN_BOOMER_NAMESPACE(base::http)

//---

static uint8_t DayOfTheWeek(uint32_t y, uint32_t m, uint32_t d)
{
    const uint8_t t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    y -= m < 3;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

bool DateTime::valid() const
{
    if (year <= 1900 || year >= 2077) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false; // TODO: constrain better
    if (hour > 23) return false;
    if (min > 59) return false;
    if (sec > 59) return false;
    return true;
}

void DateTime::print(IFormatStream& f) const
{
    const char* dayName[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    const char* monthName[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    if (valid())
    {
        // day of the week
        f << dayName[DayOfTheWeek(year, month, day)];
        f << ", ";

        // date
        if (day < 10) f << "0";
        f << day;

        f << monthName[month - 1];
        f << " ";

        f << year;
        f << " ";

        // time
        if (hour < 10) f << "0";
        f << hour;
        f << ":";
        if (min < 10) f << "0";
        f << min;
        f << ":";
        if (sec < 10) f << "0";
        f << sec;
    }
}

//--

bool DateTime::Parse(StringView txt, DateTime& out)
{
    return true;
}

DateTime DateTime::Now()
{
    DateTime ret;

#if 0
#ifdef PLATFORM_WINDOWS
    SYSTEMTIME stUTC;
    GetSystemTime(&stUTC);

    SYSTEMTIME stLocal;
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    ret.year = stLocal.wYear;
    ret.month = stLocal.wMonth;
    ret.day = stLocal.wDay;
    ret.hour = stLocal.wHour;
    ret.min = stLocal.wMinute;
    ret.sec = stLocal.wSecond;
#elif defined(PLATFORM_POSIX)
    time_t t;
    time(&t);

    // get string representation
    tm timeData;
    auto timeDataPtr  = gmtime_r(&t, &timeData);

    // build string
    ret.year = timeDataPtr->tm_year + 1900;
    ret.month = timeDataPtr->tm_mon + 1;
    ret.dat = timeDataPtr->tm_mday;
    ret.hour = timeDataPtr->tm_hour;
    ret.min = timeDataPtr->tm_min;
    ret.sec = timeDataPtr->tm_sec;
#endif
#endif
    return ret;
}

//---

void RequestHeader::print(IFormatStream& f) const
{
    f << method << " " << url << " " << version << "\r\n";

    if (!host.empty())
        f << "Host: " << host << "\r\n";

    if (!userAgent.empty())
        f << "User-Agent: " << userAgent << "\r\n";

    if (keepAlive)
        f << "Connection: keep-alive" << "\r\n";

    for (auto param : params.pairs())
        f << param.key << ": " << param.value << "\r\n";

    if (!contentType.empty())
        f << "Content-Type: " << contentType << "\r\n";

    f << "Content-Length: " << contentLength << "\r\n";
}

const char RequestHeader::SYMBOL_ARGS = ' ';
const char RequestHeader::SYMBOL_KEY = ':';
const char RequestHeader::SYMBOL_LF = 10;
const char RequestHeader::SYMBOL_CR = 13;

void RequestHeader::SkipWhitespaces(const char*& txt, const char* endTxt)
{
    while (txt < endTxt && (*txt <= ' ' && *txt != '\r' && *txt != '\n'))
        txt += 1;
}

bool RequestHeader::ParseText(const char*& txt, const char* endTxt, StringBuf& outText, char aditionalDelimiter /*= 0*/)
{
    StringView ret;
    if (!ParseText(txt, endTxt, ret, aditionalDelimiter))
        return false;

    outText = StringBuf(ret);
    return true;
}

bool RequestHeader::ParseText(const char*& txt, const char* endTxt, StringView& outText, char aditionalDelimiter /*= 0*/)
{
    const char* cur = txt;

    SkipWhitespaces(cur, endTxt);

    const char* textStart = nullptr;
    while (cur < endTxt && (*cur != aditionalDelimiter))
    {
        if (*cur != SYMBOL_LF && *cur != SYMBOL_CR)
        {
            if (!textStart)
                textStart = cur;
        }

        ++cur;
    }

    if (!textStart)
        return false;

    outText = StringView(textStart, cur).trim();
    txt = cur;
    return true;
}

bool RequestHeader::ParseLineEnd(const char*& txt, const char* endTxt)
{
    const char* cur = txt;

    if (cur < endTxt)
    {
        if (*cur == SYMBOL_CR)
            cur += 1;

        if (cur < endTxt && *cur == SYMBOL_LF)
        {
            txt = cur + 1;
            return true;
        }
    }

    return false;
}

bool RequestHeader::ProcessHeaderParam(StringView name, StringView value, RequestHeader& outHeader)
{
    if (0 == name.caseCmp("host"))
    {
        outHeader.host = StringBuf(value);
    }
    else if (0 == name.caseCmp("user-agent"))
    {
        outHeader.userAgent = StringBuf(value);
    }
    else if (0 == name.caseCmp("connection"))
    {
        if (INDEX_NONE != value.findStrNoCase("keep-alive"))
            outHeader.keepAlive = true;
    }
    else if (0 == name.caseCmp("content-length"))
    {
        return value.match(outHeader.contentLength) == MatchResult::OK;
    }
    else if (0 == name.caseCmp("content-type"))
    {
        outHeader.contentType = StringBuf(value);
    }
    else if (0 == name.caseCmp("upgrade"))
    {
        outHeader.websocketUpgrade = true;
    }
    else if (0 == name.caseCmp("connection"))
    {
        outHeader.websocketConnection = StringBuf(value);
    }
    else if (0 == name.caseCmp("origin"))
    {
        outHeader.websocketOrigin = StringBuf(value);
    }
    else if (0 == name.caseCmp("sec-websocket-key"))
    {
        outHeader.secWebSocketKey = StringBuf(value);
    }
    else if (0 == name.caseCmp("sec-websocket-version"))
    {
        outHeader.secWebSocketVersion = StringBuf(value);
    }
    else if (0 == name.caseCmp("sec-websocket-protocol"))
    {
        const char* stream = value.data();
        const char* streamEnd = value.data() + value.length();

        StringView txt;
        while (ParseText(stream, streamEnd, txt, ','))
            outHeader.secWebSocketProtocols.pushBack(StringBuf(txt));
    }
    else if (0 == name.caseCmp("sec-websocket-extensions"))
    {
        const char* stream = value.data();
        const char* streamEnd = value.data() + value.length();

        StringView txt;
        while (ParseText(stream, streamEnd, txt, ','))
            outHeader.secWebSocketExtensions.pushBack(StringBuf(txt));
    }
    else
    {
        outHeader.params[StringID(name)] = StringBuf(value);
    }

    return true;
}

bool RequestHeader::Parse(const void* data, uint32_t dataSize, uint32_t& payloadStartOffset, RequestHeader& outHeader)
{
    const char* cur = (const char*)data;
    const char* end = cur + dataSize;

    // Parse: <method> <uri> <version>
    // Method: GET POST HEAD etc
    // URI: url part
    // Version: HTML/1.1
    if (!ParseText(cur, end, outHeader.method, SYMBOL_ARGS))
    {
        TRACE_WARNING("HttpHeader: Failed to parse method stirng");
        return false;
    }
    if (!ParseText(cur, end, outHeader.url, SYMBOL_ARGS))
    {
        TRACE_WARNING("HttpHeader: Failed to url stirng");
        return false;
    }
    if (!ParseText(cur, end, outHeader.version, SYMBOL_LF))
    {
        TRACE_WARNING("HttpHeader: Failed to parse version stirng");
        return false;
    }

    TRACE_INFO("HttpHeader: '{}' '{}' '{}'", outHeader.method, outHeader.url, outHeader.version);

    if (!ParseLineEnd(cur, end))
    {
        TRACE_WARNING("HttpHeader: Expected line end");
        return false;
    }

    // additional data
    for (;;)
    {
        // end of header
        if (ParseLineEnd(cur, end))
        {
            TRACE_INFO("HttpHeader: End of header found");
            break;
        }

        // content name
        StringView key;
        if (!ParseText(cur, end, key, SYMBOL_KEY))
        {
            TRACE_WARNING("HttpHeader: Failed to parse argument name");
            return false;
        }

        // KEY shit
        if (cur >= end || *cur != SYMBOL_KEY)
        {
            TRACE_WARNING("HttpHeader: No ':' for key '{}", key);
            return false;
        }
        cur += 1;

        StringView value;
        if (ParseText(cur, end, value, SYMBOL_LF))
        {
            if (!ProcessHeaderParam(key, value, outHeader))
            {
                TRACE_WARNING("HttpHeader: No value for '{}", key);
                return false;
            }

            if (!ParseLineEnd(cur, end))
            {
                TRACE_WARNING("HttpHeader: No line and after '{}' value '{}'", key, value);
                return false;
            }

            TRACE_INFO("HttpHeader: Parsed '{}' = '{}'", key, value);
        }
    }

    // no errors in header
    payloadStartOffset = cur - (const char*)data;
    TRACE_INFO("HttpHeader: parsed {} bytes, the rest ({} in provided buffer) is assumed content", payloadStartOffset, dataSize - payloadStartOffset);
    return true;
}

//---

RequestHeaderParser::RequestHeaderParser()
    : m_state(State::BuildingHeader)
    , m_numHeaderBytes(0)
    , m_numContentBytes(0)
{
    memset(m_headerBytes, 0xCC, sizeof(m_headerBytes));
}

void RequestHeaderParser::reportError(const StringView txt)
{
    if (m_state != State::Error)
    {
        TRACE_ERROR("HTTP Header parsing error: {}", txt);
        m_state = State::Error;
    }
}

void RequestHeaderParser::clear()
{
    m_numHeaderBytes = 0;
    m_numContentBytes = 0;
    m_state = State::BuildingHeader;
    m_currentHeader.reset();
}

void RequestHeaderParser::push(const void* data, uint32_t size, Array<RefPtr<RequestHeader>>& outParseHeaders)
{
    auto readPtr  = (const uint8_t*)data;
    auto readEndPtr  = readPtr + size;

    bool somethingEmitted = false;
    while (m_state != State::Error && (readPtr < readEndPtr || somethingEmitted))
    {
        somethingEmitted = false;
        if (m_state == State::BuildingHeader)
        {
            if (!m_currentHeader)
                m_currentHeader = RefNew<RequestHeader>();

            auto maxCopy  = std::min<uint32_t>(MAX_HEADER_SIZE - m_numHeaderBytes, readEndPtr - readPtr);
            memcpy(m_headerBytes + m_numHeaderBytes, readPtr, maxCopy);
            m_numHeaderBytes += maxCopy;
            readPtr += maxCopy;

            // nothing to parse
            if (!m_numHeaderBytes)
                break;

            // try parse what we have
            uint32_t payloadStartOffset = 0;
            if (!RequestHeader::Parse(m_headerBytes, m_numHeaderBytes, payloadStartOffset, *m_currentHeader))
            {
                TRACE_INFO("Failed header:\n{}", *m_currentHeader);

                if (m_numHeaderBytes == MAX_HEADER_SIZE)
                {
                    reportError("Failed to parse HTTP header from received stream. Corrupted data?");
                    break;
                }
            }
            else // we got data
            {
                TRACE_INFO("Parsed header:\n{}", *m_currentHeader);

                // prepare output buffer
                if (m_currentHeader->contentLength)
                    m_currentHeader->contentBuffer = Buffer::Create(POOL_TEMP, m_currentHeader->contentLength);
                m_numContentBytes = 0;

                // copy any spare data
                auto dataLeftInHeaderBuffer = m_numHeaderBytes - payloadStartOffset;
                auto dataToCopyToPayloadFromHeaderBuffer = std::min<uint32_t>(m_currentHeader->contentLength, dataLeftInHeaderBuffer);
                if (dataToCopyToPayloadFromHeaderBuffer)
                {
                    memcpy(m_currentHeader->contentBuffer.data(), m_headerBytes + payloadStartOffset, dataToCopyToPayloadFromHeaderBuffer);
                    m_numContentBytes += dataToCopyToPayloadFromHeaderBuffer;
                }

                // shift rest of the data
                auto unprocessedHeaderDataRemainingInBufferOffset = payloadStartOffset + m_currentHeader->contentLength;
                if (m_numHeaderBytes > unprocessedHeaderDataRemainingInBufferOffset)
                {
                    m_numHeaderBytes -= unprocessedHeaderDataRemainingInBufferOffset;
                    memmove(m_headerBytes, m_headerBytes + unprocessedHeaderDataRemainingInBufferOffset, m_numHeaderBytes);
                }
                else
                {
                    m_numHeaderBytes -= unprocessedHeaderDataRemainingInBufferOffset;
                }

                // if we have all the data for current packet emit it, if not enter content parsing mode
                ASSERT(m_numContentBytes <= m_currentHeader->contentLength);
                if (m_currentHeader->contentLength == m_numContentBytes)
                {
                    outParseHeaders.pushBack(m_currentHeader);
                    m_currentHeader.reset();
                    somethingEmitted = true;
                }
                else
                {
                    // we should have NOTHING LEFT in header buffer
                    ASSERT(m_numHeaderBytes == 0);
                    m_state = State::BuildingData;
                }
            }
        }
        else if (m_state == State::BuildingData)
        {
            ASSERT(m_numHeaderBytes == 0);
            ASSERT(m_numContentBytes <= m_currentHeader->contentLength);

            auto maxCopy = std::min<uint32_t>(m_currentHeader->contentLength - m_numContentBytes, readEndPtr - readPtr);
            memcpy(m_currentHeader->contentBuffer.data() + m_numContentBytes, readPtr, maxCopy);
            m_numContentBytes += maxCopy;
            readPtr += maxCopy;

            if (m_currentHeader->contentLength == m_numContentBytes)
            {
                outParseHeaders.pushBack(m_currentHeader);
                somethingEmitted = true;
                m_currentHeader.reset();
                m_state = State::BuildingHeader;
            }
        }
    }

    ASSERT(m_state == State::Error || (readPtr == readEndPtr));
}

//--

END_BOOMER_NAMESPACE(base::http)
