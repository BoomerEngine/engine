/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/containers/include/hashMap.h"

namespace base
{
    namespace res
    {
        //---

        typedef uint64_t ImportFileFingerprintType;

        //---

        // fingerprint of asset file
        class BASE_RESOURCE_COMPILER_API ImportFileFingerprint
        {
        public:
            INLINE ImportFileFingerprint() {};
            explicit INLINE ImportFileFingerprint(ImportFileFingerprintType val) : m_value(val) {};
            INLINE ImportFileFingerprint(const ImportFileFingerprint& other) = default;
            INLINE ImportFileFingerprint(ImportFileFingerprint&& other) = default;
            INLINE ImportFileFingerprint& operator=(const ImportFileFingerprint& other) = default;
            INLINE ImportFileFingerprint& operator=(ImportFileFingerprint&& other) = default;
            INLINE ~ImportFileFingerprint() {};

            //--

            // get raw value
            INLINE const ImportFileFingerprintType& rawValue() const { return m_value; }

            // is the fingerprint empty (not computed) ?
            INLINE bool empty() const { return m_value == 0; }

            // check if the fingerprint is valid
            INLINE operator bool() const { return m_value != 0; }

            //--

            // compare fingerprints
            INLINE bool operator==(const ImportFileFingerprint& other) const { return m_value == other.m_value; }
            INLINE bool operator!=(const ImportFileFingerprint& other) const { return m_value != other.m_value; }
            INLINE bool operator<(const ImportFileFingerprint& other) const { return m_value < other.m_value; }

            //--

            // print to text
            void print(IFormatStream& f) const;

            // Custom type implementation requirements - serialization
            void writeBinary(stream::OpcodeWriter& stream) const;
            void readBinary(stream::OpcodeReader& stream);

            //--

            // hashmap entry
            static uint32_t CalcHash(const ImportFileFingerprint& entry);

        private:
            ImportFileFingerprintType m_value = 0;
        };

        ///--

        enum class FingerpintCalculationStatus : uint8_t
        {
            OK,
            Canceled,
            ErrorNoFile,
            ErrorInvalidRead,
            ErrorOutOfMemory,
        };

        /// calculate file content fingerprint, returns false if canceled or error occurs
        extern BASE_RESOURCE_COMPILER_API FingerpintCalculationStatus CalculateMemoryFingerprint(const void* data, uint64_t size, IProgressTracker* progress, ImportFileFingerprint& outFingerpint);

        /// calculate file content fingerprint by reading data from async file, returns false if canceled or error occurs
        /// NOTE: this implementation will interleave reading and calculations using fibers 
        CAN_YIELD extern BASE_RESOURCE_COMPILER_API FingerpintCalculationStatus CalculateFileFingerprint(io::IAsyncFileHandle* file, bool childFiber, IProgressTracker* progress, ImportFileFingerprint& outFingerpint);

        /// calculate file content fingerprint by reading data from async file, returns false if canceled or error occurs
        extern BASE_RESOURCE_COMPILER_API FingerpintCalculationStatus CalculateFileFingerprint(io::IReadFileHandle* file, IProgressTracker* progress, ImportFileFingerprint& outFingerpint);

        ///--

    } // res
} // base