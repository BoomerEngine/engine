/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importFileFingerprint.h"
#include "base/object/include/streamOpcodeReader.h"
#include "base/object/include/streamOpcodeWriter.h"
#include "base/io/include/ioFileHandle.h"
#include "base/io/include/ioAsyncFileHandle.h"

namespace base
{
    namespace res
    {
        //--

        RTTI_BEGIN_CUSTOM_TYPE(ImportFileFingerprint);
            RTTI_TYPE_TRAIT().zeroInitializationValid().noDestructor().fastCopyCompare();
            RTTI_BIND_NATIVE_BINARY_SERIALIZATION(ImportFileFingerprint);
            RTTI_BIND_NATIVE_PRINT(ImportFileFingerprint);
        RTTI_END_TYPE();

        //--

        typedef CRC32 FingerPrintCalculator;

        //--

        void ImportFileFingerprint::print(IFormatStream& f) const
        {
            if (m_value != 0)
            {
                f.appendf("CRC32: {}", Hex((uint32_t)m_value));
            }
            else
            {
                f.append("INVALID");
            }
        }

        void ImportFileFingerprint::writeBinary(stream::OpcodeWriter& stream) const
        {
            stream.writeTypedData<uint8_t>(0);
            stream.writeTypedData(m_value);
        }

        void ImportFileFingerprint::readBinary(stream::OpcodeReader& stream)
        {
            uint8_t type = 0;
            stream.readTypedData(type);

            if (type == 0)
                stream.readTypedData(m_value);
        }

        uint32_t ImportFileFingerprint::CalcHash(const ImportFileFingerprint& entry)
        {
            return entry.m_value;
        }

        //--

        static const uint64_t MIN_BATCH_SIZE = 4096;

        static const ConfigProperty<uint64_t> cvFingerprintMemoryBatchSize("Assets.Fingerprint", "MemoryBatchSize", 1U << 20);
        static const ConfigProperty<uint64_t> cvFingerprintSyncFileBatchSize("Assets.Fingerprint", "SyncFileBatchSize", 64 << 10);
        static const ConfigProperty<uint64_t> cvFingerprintAsyncFileBatchSize("Assets.Fingerprint", "AsyncFileBatchSize", 1U << 20);
        static const ConfigProperty<double> cvFingerprintFiberAutoYieldInterval("Assets.Fingerprint", "FiberAutoYieldInterval", 20);

        //--

        struct AutoYielder
        {
        public:
            NativeTimePoint nextYield;
            bool shouldYeild = false;
            double yieldInterval = 0.0f;

            AutoYielder()
            {
                shouldYeild = !Fibers::GetInstance().isMainFiber();
                yieldInterval = std::clamp<double>(cvFingerprintFiberAutoYieldInterval.get() / 1000.0, 0.0001, 1.000);
                nextYield = NativeTimePoint::Now() + yieldInterval;
            }

            void conditionalYield()
            {
                if (shouldYeild)
                {
                    if (nextYield.reached())
                    {
                        Fibers::GetInstance().yield();
                        nextYield = NativeTimePoint::Now() + yieldInterval;
                    }
                }
            }
        };

        /// calculate file content fingerprint, returns false if canceled or error occurs
        FingerpintCalculationStatus CalculateMemoryFingerprint(const void* data, uint64_t size, IProgressTracker* progress, ImportFileFingerprint& outFingerpint)
        {
            ASSERT_EX(data != nullptr, "No data");

            uint64_t originalSize = size;
            auto* readPtr = (const uint8_t*)data;

            const auto batchSize = std::max<uint64_t>(MIN_BATCH_SIZE, cvFingerprintMemoryBatchSize.get());

            AutoYielder yielder;
            FingerPrintCalculator calc;
            while (size)
            {
                // update progress
                if (progress)
                {
                    if (progress->checkCancelation())
                        return FingerpintCalculationStatus::Canceled;
                    progress->reportProgress(originalSize - size, originalSize, "Calculating file fingerprint");
                }

                // this is a very log job, release CPU resources from time to time
                yielder.conditionalYield();

                // calculate CRC
                const auto readSize = std::min(batchSize, size);
                calc.append(readPtr, readSize);
                
                // advance
                size -= readSize;
                readPtr += readSize;
            }

            outFingerpint = ImportFileFingerprint(calc.crc());
            return FingerpintCalculationStatus::OK;
        }

        FingerpintCalculationStatus CalculateFileFingerprint(io::IReadFileHandle* file, IProgressTracker* progress, ImportFileFingerprint& outFingerpint)
        {
            ASSERT_EX(file != nullptr, "No file");

            const auto size = file->size();
            const auto startPos = file->pos();

            if (!file->pos(0))
                return FingerpintCalculationStatus::ErrorInvalidRead;
            
            const auto batchSize = std::max<uint64_t>(MIN_BATCH_SIZE, cvFingerprintSyncFileBatchSize.get());

            auto batchBuffer = Buffer::CreateInSystemMemory(POOL_IO, batchSize);
            if (!batchBuffer)
                return FingerpintCalculationStatus::ErrorOutOfMemory;

            FingerPrintCalculator calc;
            uint64_t pos = 0;
            AutoYielder yielder;
            while (pos < size)
            {
                // update progress
                if (progress)
                {
                    if (progress->checkCancelation())
                        return FingerpintCalculationStatus::Canceled;
                    progress->reportProgress(pos, size, "Calculating file fingerprint");
                }

                // this is a very log job, release CPU resources from time to time
                yielder.conditionalYield();

                // load file content
                const auto readSize = std::min(batchSize, pos - size);
                const auto actualReadSize = file->readSync(batchBuffer.data(), readSize);
                if (actualReadSize != readSize)
                    return FingerpintCalculationStatus::ErrorInvalidRead;

                // calculate CRC
                calc.append(batchBuffer.data(), actualReadSize);

                // advance
                pos += actualReadSize;
            }

            ASSERT(pos == size);

            outFingerpint = ImportFileFingerprint(calc.crc());
            return FingerpintCalculationStatus::OK;
        }

        CAN_YIELD extern BASE_RESOURCE_COMPILER_API FingerpintCalculationStatus CalculateFileFingerprint(io::IAsyncFileHandle* file, bool childFiber, IProgressTracker* progress, ImportFileFingerprint& outFingerpint)
        {
            ASSERT_EX(file != nullptr, "No file");

            struct State
            {
                uint64_t pos = 0;
                uint64_t size = 0;
                uint64_t batchSize = 0;
                Buffer readBuffer;
                Buffer calcBuffer;
                FingerPrintCalculator calc;
                fibers::WaitCounter doneCounter;
                FingerpintCalculationStatus status;
            } state;

            state.pos = 0;
            state.size = file->size();
            state.status = FingerpintCalculationStatus::OK; // changed only if we fail for some reason

            state.batchSize = std::max<uint64_t>(MIN_BATCH_SIZE, cvFingerprintAsyncFileBatchSize.get());
            state.readBuffer = Buffer::CreateInSystemMemory(POOL_IO, state.batchSize);
            state.calcBuffer = Buffer::CreateInSystemMemory(POOL_IO, state.batchSize);

            if (!state.readBuffer || !state.calcBuffer)
                return FingerpintCalculationStatus::ErrorOutOfMemory;

            state.doneCounter = Fibers::GetInstance().createCounter("FileFingerprint", 1);

            // run fiber with processing
            RunFiber("FileFingerprint").child(childFiber) << [file, &state, progress](FIBER_FUNC)
            {
                fibers::WaitCounter processingDone;

                while (state.pos < state.size)
                {
                    // check for cancellation
                    if (progress)
                    {
                        if (progress->checkCancelation())
                        {
                            state.status = FingerpintCalculationStatus::Canceled;
                            break;
                        }

                        progress->reportProgress(state.pos, state.size, "Calculating file fingerprint");
                    }

                    // read file content, this will yield until async op completes
                    const auto readSize = std::min(state.batchSize, state.pos - state.size);
                    const auto actualReadSize = file->readAsync(state.pos, readSize, state.readBuffer.data());
                    if (actualReadSize != readSize)
                    {
                        state.status = FingerpintCalculationStatus::ErrorInvalidRead;
                        break;
                    }

                    // wait for previous processing to finish
                    Fibers::GetInstance().waitForCounterAndRelease(processingDone);

                    // swap the buffers
                    std::swap(state.calcBuffer, state.readBuffer);

                    // process the data on another fiber
                    processingDone = Fibers::GetInstance().createCounter("FileFingerprintCalc", 1);
                    RunChildFiber("FileFingerprintCalc") << [&state, actualReadSize, processingDone](FIBER_FUNC)
                    {
                        state.calc.append(state.calcBuffer.data(), actualReadSize);
                        Fibers::GetInstance().signalCounter(processingDone);
                    };
                }

                // wait for previous processing to finish
                Fibers::GetInstance().waitForCounterAndRelease(processingDone);

                // while computation is done
                Fibers::GetInstance().signalCounter(state.doneCounter);
            };

            // wait for the fiber to finish
            Fibers::GetInstance().waitForCounterAndRelease(state.doneCounter);
                
            if (state.status == FingerpintCalculationStatus::OK)
                outFingerpint = ImportFileFingerprint(state.calc.crc());

            return state.status;
        }

        //--

    } // res
} // base
