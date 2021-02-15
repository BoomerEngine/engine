/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\output #]
***/

#include "build.h"
#include "output.h"
#include "format.h"
#include "spinLock.h"

namespace base
{
    namespace logging
    {
        //---

        /// global sink table
        struct SinkTable
        {
        public:
            // add a sink
            void attach(ILogSink* sink);

            // remove a sink
            void detach(ILogSink* sink);

            // print line to all sinks
            void print(OutputLevel level, const char* file, uint32_t line, const char* module, const char* context, const char* text);

            //--

            // sink table instance
            static SinkTable& GetInstance();

        private:
            SpinLock m_lock;

            static const uint32_t MAX_SINKS = 4;

            uint8_t m_numSinks = 0;
            ILogSink* m_sinks[MAX_SINKS]; 
        };

        //---

        /// line buffer for collecting the lines
        /// NOTE: lines that are to long are simply not printed
        /// NOTE: line buffer when signaled always contains line ending (platform specific)
        class LineBuffer
        {
        public:
            LineBuffer();

            // clear content
            void clear();

            // add stuff, signals with true when we get a line
            bool append(const char*& str, const char* endStr);

            //---

            INLINE bool empty() const { return 0 == m_index; }

            INLINE const char* c_str() const { return m_line; }

            INLINE bool append(const char* str) { return append(str, str + strlen(str)); }


        private:
            static const uint32_t MAX_LINE_LENGTH = 1024;

            uint32_t m_index;
            char m_line[MAX_LINE_LENGTH + 8];
        };

        //---

        /// a format stream implementation for the logger
        class LineAssembler : public IFormatStream
        {
        public:
            LineAssembler();

            void changeContext(const char* contextName);

            void takeOwnership(OutputLevel level, const char* moduleName, const char* contextFile, uint32_t contextLine);
            void releaseOwnership();

            ILogSink* mountLocalSink(ILogSink* localSink);
            ILogSink* localSink();

            // IFormatStream
            virtual IFormatStream& append(const char* str, uint32_t len) override;

        private:
            LineBuffer m_line;

            uint32_t m_currentLine = 0;
            const char* m_curentFile = nullptr;
			const char* m_curentContext = nullptr;
            const char* m_curentModule = nullptr;
            OutputLevel m_currentLevel = OutputLevel::Info;

            ILogSink* m_localSink = nullptr;

            std::atomic<uint32_t> m_owningThread = 0;

            static char PREFIX_BUFFER[64];
        };

        //---

    } // logging
} // base
