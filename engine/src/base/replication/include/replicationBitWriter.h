/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

namespace base
{
    namespace replication
    {

        /// writer of bits
        /// NOTE: this class uses reserve() + uncheckedWrite pattern, be careful!
        /// NOTE: we are strongly aiming at having JITed versions of replication encoders/decoders so this class' performance is NOT that critical
        class BASE_REPLICATION_API BitWriter : public NoCopy
        {
        public:
            typedef uint32_t WORD;

            BitWriter();
            BitWriter(void* externalMemory, uint32_t bitCapacity, bool releaseMemory = false);
            ~BitWriter();

            //--

            // get written data
            INLINE const WORD* data() const { return m_blockStart; }

            // get size of written data, in byte
            INLINE uint32_t byteSize() const { return (m_bitPos + 7)/8; }

            /// get number of written bits
            INLINE uint32_t bitSize() const { return m_bitPos; }

            //--

            /// clear data, does not release the memory
            void clear();

            /// reserve space for at least N bits
            /// NOTE: failure to reserve may stomp memory
            /// NOTE: try NOT to call this function very often
            void reserve(uint32_t numBits);

            /// align to byte boundary
            void align();

            /// write single bit
            void writeBit(bool val);

            /// write multiple bits
            void writeBits(WORD value, uint32_t numBits);

            /// write string of data (requires being aligned to byte boundary)
            void writeBlock(const void* data, uint32_t dataSize);

            /// write adaptive number, usually an array count/object ID
            void writeAdaptiveNumber(WORD count);

        private:
            static const uint32_t MAX_INTERNAL_WORDS = 32;
            static const uint32_t WORD_SIZE = sizeof(WORD) * 8;

            WORD* m_blockStart = nullptr;
            WORD* m_blockEnd = nullptr;
            WORD* m_blockPos = nullptr;

            uint32_t m_bitPos = 0;
            uint32_t m_bitCapacity = 0;
            uint32_t m_bitIndex = 0;
            bool m_releaseMemory = false;

            WORD m_internalBuffer[MAX_INTERNAL_WORDS];

            void advance();
            void grow(uint32_t requiredWords);
        };

    } // replication
} // base
