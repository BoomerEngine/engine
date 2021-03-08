/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "core/containers/include/hashMap.h"

BEGIN_BOOMER_NAMESPACE()

//---

typedef uint64_t SourceAssetFingerprintType;

//---

// fingerprint of asset file
class CORE_RESOURCE_COMPILER_API SourceAssetFingerprint
{
public:
    INLINE SourceAssetFingerprint() {};
    explicit INLINE SourceAssetFingerprint(SourceAssetFingerprintType val) : m_value(val) {};
    INLINE SourceAssetFingerprint(const SourceAssetFingerprint& other) = default;
    INLINE SourceAssetFingerprint(SourceAssetFingerprint&& other) = default;
    INLINE SourceAssetFingerprint& operator=(const SourceAssetFingerprint& other) = default;
    INLINE SourceAssetFingerprint& operator=(SourceAssetFingerprint&& other) = default;
    INLINE ~SourceAssetFingerprint() {};

    //--

    // get raw value
    INLINE const SourceAssetFingerprintType& rawValue() const { return m_value; }

    // is the fingerprint empty (not computed) ?
    INLINE bool empty() const { return m_value == 0; }

    // check if the fingerprint is valid
    INLINE operator bool() const { return m_value != 0; }

    //--

    // compare fingerprints
    INLINE bool operator==(const SourceAssetFingerprint& other) const { return m_value == other.m_value; }
    INLINE bool operator!=(const SourceAssetFingerprint& other) const { return m_value != other.m_value; }
    INLINE bool operator<(const SourceAssetFingerprint& other) const { return m_value < other.m_value; }

    //--

    // print to text
    void print(IFormatStream& f) const;

    // Custom type implementation requirements - serialization
    void writeBinary(SerializationWriter& stream) const;
    void readBinary(SerializationReader& stream);

    //--

    // hashmap entry
    static uint32_t CalcHash(const SourceAssetFingerprint& entry);

private:
    SourceAssetFingerprintType m_value = 0;
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
extern CORE_RESOURCE_COMPILER_API FingerpintCalculationStatus CalculateMemoryFingerprint(const void* data, uint64_t size, IProgressTracker* progress, SourceAssetFingerprint& outFingerpint);

/// calculate file content fingerprint by reading data from async file, returns false if canceled or error occurs
/// NOTE: this implementation will interleave reading and calculations using fibers 
CAN_YIELD extern CORE_RESOURCE_COMPILER_API FingerpintCalculationStatus CalculateFileFingerprint(IAsyncFileHandle* file, bool childFiber, IProgressTracker* progress, SourceAssetFingerprint& outFingerpint);

/// calculate file content fingerprint by reading data from async file, returns false if canceled or error occurs
extern CORE_RESOURCE_COMPILER_API FingerpintCalculationStatus CalculateFileFingerprint(IReadFileHandle* file, IProgressTracker* progress, SourceAssetFingerprint& outFingerpint);

///--

END_BOOMER_NAMESPACE()
