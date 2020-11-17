/***
* Boomer Engine v4
* Written by Tomasz "RexDex" Jonarski
*
* [#filter: http #]
***/

#pragma once

namespace base
{
    namespace http
    {
        //----

        /// HTTP Date/Time format (http://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html)
        struct DateTime
        {
        public:
            uint16_t year=0;
            uint8_t month = 0, day = 0;
            uint8_t hour = 0, min = 0, sec = 0;

            //--

            bool valid() const;

            void print(IFormatStream& f) const;

            //--

            static bool Parse(StringView txt, DateTime& out);
            static DateTime Now();

            //--
        };

        //----

        /// HTTP Request (http://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html)
        struct RequestHeader : public IReferencable
        {
            //--

            StringBuf method;
            StringBuf version;
            StringBuf url;
            StringBuf host;

            HashMap<StringBuf, StringBuf> cookies;

            Array<StringBuf> accept;

            StringBuf userAgent;
            bool keepAlive = false;

            StringBuf contentType;
            uint32_t contentLength = 0;
            Buffer contentBuffer;

            //--

            bool websocketUpgrade = false;
            StringBuf websocketOrigin;
            StringBuf websocketConnection;
            StringBuf secWebSocketKey;
            StringBuf secWebSocketVersion;
            Array<StringBuf> secWebSocketProtocols;
            Array<StringBuf> secWebSocketExtensions;

            //--

            HashMap<StringID, StringBuf>  params; // all header params

            //--

            void print(IFormatStream& f) const; // reassemble

            //--

            static bool Parse(const void* data, uint32_t dataSize, uint32_t& payloadStartOffset, RequestHeader& outHeader);

        private:
            static const char SYMBOL_ARGS;
            static const char SYMBOL_KEY;
            static const char SYMBOL_CR;
            static const char SYMBOL_LF;

            // parsing helpers
            static void SkipWhitespaces(const char*& txt, const char* endTxt);
            static bool ParseText(const char*& txt, const char* endTxt, StringView& outText, char aditionalDelimiter = 0);
            static bool ParseText(const char*& txt, const char* endTxt, StringBuf& outText, char aditionalDelimiter = 0);
            static bool ParseLineEnd(const char*& txt, const char* endTxt);
            static bool ProcessHeaderParam(StringView name, StringView value, RequestHeader& outHeader);
        };

        //----

        /// HTTP Request building context
        class RequestHeaderParser : public NoCopy
        {
        public:
            RequestHeaderParser();

            INLINE bool valid() const { return m_state != State::Error; }

            void clear();
            void push(const void* data, uint32_t size, Array<RefPtr<RequestHeader>>& outParseHeaders);

        private:
            static const uint32_t MAX_HEADER_SIZE = 1024;

            enum class State
            {
                BuildingHeader,
                BuildingData,
                Error,
            };

            State m_state;

            uint8_t m_headerBytes[MAX_HEADER_SIZE];
            uint32_t m_numHeaderBytes;
            uint32_t m_numContentBytes;

            RefPtr<RequestHeader> m_currentHeader;

            void reportError(const StringView txt);
        };

        //---

    } // http
} // base


