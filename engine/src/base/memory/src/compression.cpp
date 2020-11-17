/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: utils #]
***/

#include "build.h"

#include "thirdparty/lz4/lz4.h"
#include "thirdparty/lz4/lz4hc.h"

#define ZLIB_CONST
#include "thirdparty/zlib/zlib.h"

namespace base
{
    namespace mem
    {
        //--

        // allocator for the ZLib-internals
        namespace prv
        {
            uint32_t ZLIB_HEADER_SIZE = sizeof(uint32_t) * 2;
            uint32_t ZLIB_MAGIC = 0x5a4c4942;//'ZLIB';

            static voidpf ZlibAlloc(voidpf, uInt items, uInt size)
            {
                return  mem::GlobalPool<POOL_ZLIB>::Alloc(size * items, 1);
            }

            static void ZlibFree(voidpf, voidpf address)
            {
                mem::GlobalPool<POOL_ZLIB>::Free(address);
            }
        } // prv

        //--

        void* CompressZlib(const void* uncompressedDataPtr, uint64_t uncompressedSize, uint64_t& outCompressedSize, PoolTag pool)
        {
            void* ret = nullptr;

            z_stream zstr;
            memzero(&zstr, sizeof(zstr));

            zstr.next_in = (Bytef*)uncompressedDataPtr;
            zstr.avail_in = uncompressedSize;
            zstr.zalloc = &prv::ZlibAlloc;
            zstr.zfree = &prv::ZlibFree;

// start compression
auto initRet  = deflateInit(&zstr, Z_BEST_COMPRESSION);
DEBUG_CHECK_EX(initRet == Z_OK, "Failed to initialize z-lib deflate");
if (initRet == Z_OK)
{
    // estimate the size of output memory
    auto neededSize  = deflateBound(&zstr, uncompressedSize);

    // allocate the output buffer of at least equal size as the input
    ret = mem::AllocateBlock(pool, neededSize, 16, "CompressZLib");
    DEBUG_CHECK_EX(ret, "Out of memory when allocating buffer for data compression");
    if (ret)
    {
        zstr.next_out = static_cast<Bytef*>(ret);
        zstr.avail_out = neededSize;

        // single step DEFLATE
        auto result  = deflate(&zstr, Z_FINISH);
        DEBUG_CHECK_EX(result == Z_STREAM_END, "Failed to finish z-lib compression in one step, more memory is needed than what deflateBound returned");
        if (result == Z_STREAM_END)
        {
            outCompressedSize = zstr.total_out;
        }
        else
        {
            mem::FreeBlock(ret);
            ret = nullptr;
        }
    }

    // failed to compress but it was still initialized
    deflateEnd(&zstr);
}

return ret;
        }

        bool CompressZlib(const void* uncompressedDataPtr, uint64_t uncompressedSize, void* compressedDataPtr, uint64_t compressedDataMaxSize, uint64_t& outCompressedRealSize)
        {
            z_stream zstr;
            memzero(&zstr, sizeof(zstr));

            zstr.next_in = (Bytef*)uncompressedDataPtr;
            zstr.avail_in = uncompressedSize;
            zstr.zalloc = &prv::ZlibAlloc;
            zstr.zfree = &prv::ZlibFree;

            // start compression
            auto initRet  = deflateInit(&zstr, Z_BEST_COMPRESSION);
            DEBUG_CHECK_EX(initRet == Z_OK, "Failed to initialize z-lib deflate");
            if (initRet == Z_OK)
            {
                zstr.next_out = static_cast<Bytef*>(compressedDataPtr);
                zstr.avail_out = compressedDataMaxSize;

                // single step DEFLATE
                auto result  = deflate(&zstr, Z_FINISH);
                DEBUG_CHECK_EX(result == Z_STREAM_END, "Failed to finish z-lib compression in one step, more memory is needed than what deflateBound returned");
                if (result == Z_STREAM_END)
                {
                    outCompressedRealSize = zstr.total_out;
                    deflateEnd(&zstr);
                    return true;
                }

                // failed to compress but it was still initialized
                deflateEnd(&zstr);
            }

            return false;
        }

        void* CompressLZ4(const void* uncompressedDataPtr, uint64_t uncompressedSize, uint64_t& outCompressedSize, PoolTag pool)
        {
            // estimate required size
            auto neededSize  = LZ4_compressBound(uncompressedSize);

            // Allocate a buffer for writing compressed data to
            void* ret = mem::AllocateBlock(pool, neededSize, 16, "CompressLZ4");
            DEBUG_CHECK_EX(ret, "Out of memory when allocating buffer for data compression");
            if (ret)
            {
                // compress the data
                auto compressedSize  = LZ4_compress_default((const char*)uncompressedDataPtr, (char*)ret, (int)uncompressedSize, neededSize);
                DEBUG_CHECK_EX(compressedSize != 0, "Internal error in LZ4 compression");
                if (compressedSize != 0)
                {
                    outCompressedSize = compressedSize;
                }
                else
                {
                    mem::FreeBlock(ret);
                    ret = nullptr;
                }
            }

            return ret;
        }

        bool CompressLZ4(const void* uncompressedDataPtr, uint64_t uncompressedSize, void* compressedDataPtr, uint64_t compressedDataMaxSize, uint64_t& outCompressedRealSize)
        {
            // compress the data
            auto compressedSize  = LZ4_compress_default((const char*)uncompressedDataPtr, (char*)compressedDataPtr, (int)uncompressedSize, compressedDataMaxSize);
            if (compressedSize != 0)
            {
                outCompressedRealSize = compressedSize;
                return true;
            }

            return false;
        }

        void* CompressLZ4HC(const void* uncompressedDataPtr, uint64_t uncompressedSize, uint64_t& outCompressedSize, PoolTag pool)
        {
            // estimate required size
            auto neededSize  = LZ4_compressBound(uncompressedSize);

            // Allocate a buffer for writing compressed data to
            void* ret = mem::AllocateBlock(pool, neededSize, 16, "CompressLZ4HC");
            DEBUG_CHECK_EX(ret, "Out of memory when allocating buffer for data compression");
            if (ret)
            {
                void* lzState = mem::GlobalPool<POOL_LZ4>::Alloc(LZ4_sizeofStateHC(), 16);

                // compress the data
                auto compressedSize = LZ4_compress_HC_extStateHC(lzState, (const char*)uncompressedDataPtr, (char*)ret, (int)uncompressedSize, neededSize, LZ4HC_CLEVEL_OPT_MIN);
                mem::GlobalPool<POOL_LZ4>::Free(lzState);

                DEBUG_CHECK_EX(compressedSize != 0, "Internal error in LZ4 compression");
                if (compressedSize != 0)
                {
                    outCompressedSize = compressedSize;
                }
                else
                {
                    mem::FreeBlock(ret);
                    ret = nullptr;
                }
            }

            return ret;
        }

        bool CompressLZ4HC(const void* uncompressedDataPtr, uint64_t uncompressedSize, void* compressedDataPtr, uint64_t compressedDataMaxSize, uint64_t& outCompressedRealSize)
        {
            void* lzState = mem::GlobalPool<POOL_LZ4>::Alloc(LZ4_sizeofStateHC(), 16);

            // compress the data
            auto compressedSize  = LZ4_compress_HC_extStateHC(lzState, (const char*)uncompressedDataPtr, (char*)compressedDataPtr, (int)uncompressedSize, compressedDataMaxSize, LZ4HC_CLEVEL_OPT_MIN);
            mem::GlobalPool<POOL_LZ4>::Free(lzState);

            if (compressedSize != 0)
            {
                outCompressedRealSize = compressedSize;
                return true;
            }

            return false;
        }

        //--

        void* Compress(CompressionType ct, const void* uncompressedDataPtr, uint64_t uncompressedSize, uint64_t& outCompressedSize, PoolTag pool /*= PoolTag()*/)
        {
            DEBUG_CHECK_EX(uncompressedDataPtr != nullptr, "Invalid parameter");
            DEBUG_CHECK_EX(uncompressedSize != 0, "Invalid parameter");

            if (ct == CompressionType::Zlib)
                return CompressZlib(uncompressedDataPtr, uncompressedSize, outCompressedSize, pool);
            else if (ct == CompressionType::LZ4)
                return CompressLZ4(uncompressedDataPtr, uncompressedSize, outCompressedSize, pool);
            else if (ct == CompressionType::LZ4HC)
                return CompressLZ4HC(uncompressedDataPtr, uncompressedSize, outCompressedSize, pool);
            else
                return nullptr;
        }

        bool Compress(CompressionType ct, const void* uncompressedDataPtr, uint64_t uncompressedSize, void* compressedDataPtr, uint64_t compressedDataMaxSize, uint64_t& outCompressedRealSize)
        {
            DEBUG_CHECK_EX(uncompressedDataPtr != nullptr, "Invalid parameter");
            DEBUG_CHECK_EX(compressedDataPtr != nullptr, "Invalid parameter");
            DEBUG_CHECK_EX(uncompressedSize != 0, "Invalid parameter");

            if (ct == CompressionType::Zlib)
                return CompressZlib(uncompressedDataPtr, uncompressedSize, compressedDataPtr, compressedDataMaxSize, outCompressedRealSize);
            else if (ct == CompressionType::LZ4)
                return CompressLZ4(uncompressedDataPtr, uncompressedSize, compressedDataPtr, compressedDataMaxSize, outCompressedRealSize);
            else if (ct == CompressionType::LZ4HC)
                return CompressLZ4HC(uncompressedDataPtr, uncompressedSize, compressedDataPtr, compressedDataMaxSize, outCompressedRealSize);
            else
                return false;
        }

        Buffer Compress(CompressionType ct, const void* uncompressedDataPtr, uint64_t uncompressedSize, PoolTag pool /*= PoolTag()*/)
        {
            uint64_t compressedSize = 0;
            void* compressedData = Compress(ct, uncompressedDataPtr, uncompressedSize, compressedSize, pool);
            if (!compressedData)
                return Buffer();

            return Buffer::CreateExternal(pool, compressedSize, compressedData);
        }

        Buffer Compress(CompressionType ct, const Buffer& uncompressedData, PoolTag pool /*= PoolTag()*/)
        {
            if (!uncompressedData)
                return Buffer();

            return Compress(ct, uncompressedData.data(), uncompressedData.size(), pool);
        }

        //--

        bool DecompressZlib(const void* compressedDataPtr, uint64_t compressedSize, void* decompressedDataPtr, uint64_t decompressedSize)
        {
            z_stream zstr;
            memzero(&zstr, sizeof(zstr));

            zstr.next_in = (Bytef*)compressedDataPtr;
            zstr.avail_in = compressedSize;
            zstr.zalloc = &prv::ZlibAlloc;
            zstr.zfree = &prv::ZlibFree;

            // initialize
            auto initRet  = inflateInit(&zstr);
            DEBUG_CHECK_EX(initRet == Z_OK, "Failed to initialize z-lib inflate");
            if (initRet != Z_OK)
                return false;

            zstr.next_out = (Bytef*)decompressedDataPtr;
            zstr.avail_out = decompressedSize;

            auto result  = inflate(&zstr, Z_NO_FLUSH);
            inflateEnd(&zstr);
            return (result == Z_STREAM_END);
        }

        bool DecompressLZ4(const void* compressedDataPtr, uint64_t compressedSize, void* decompressedDataPtr, uint64_t decompressedSize)
        {
            auto compressedBytesProduced  = LZ4_decompress_safe((const char*)compressedDataPtr, (char*)decompressedDataPtr, compressedSize, decompressedSize);
            return (compressedBytesProduced == decompressedSize);
        }

        bool Decompress(CompressionType ct, const void* compressedDataPtr, uint64_t compressedSize, void* decompressedDataPtr, uint64_t decompressedSize)
        {
            DEBUG_CHECK_EX(compressedDataPtr != nullptr, "Invalid parameter");
            DEBUG_CHECK_EX(decompressedDataPtr != nullptr, "Invalid parameter");
            DEBUG_CHECK_EX(compressedSize != 0, "Invalid parameter");
            DEBUG_CHECK_EX(decompressedSize != 0, "Invalid parameter");

            if (ct == CompressionType::Zlib)
                return DecompressZlib(compressedDataPtr, compressedSize, decompressedDataPtr, decompressedSize);
            else if (ct == CompressionType::LZ4 || ct == CompressionType::LZ4HC)
                return DecompressLZ4(compressedDataPtr, compressedSize, decompressedDataPtr, decompressedSize);
            else
                return false;
        }

        Buffer Decompress(CompressionType ct, const void* compressedDataPtr, uint64_t compressedSize, uint64_t decompressedSize, PoolTag pool /*= PoolTag()*/)
        {
            if (0 != decompressedSize)
            {
                if (Buffer ret = Buffer::Create(pool, decompressedSize))
                {
                    if (Decompress(ct, compressedDataPtr, compressedSize, ret.data(), ret.size()))
                        return ret;
                }
            }

            return Buffer();
        }

        //--

    } // mem
} // base