/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/ui/include/uiElement.h"
#include "rendering/device/include/renderingResources.h"

namespace ed
{

    //--

    // computed histogram
    struct ImageHistogramData : public base::IReferencable
    {
        uint8_t channel = 0;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        uint32_t totalPixelCount = 0;
        uint32_t maxBucketValue = 0;
        base::Array<uint32_t> buckets;
    };

    //--

    // histogram computation pending job
    class ASSETS_IMAGE_EDITOR_API ImageHistogramPendingData : public rendering::IDownloadDataSink
    {
    public:
        ImageHistogramPendingData(uint32_t totalPixelCount, uint32_t numBuckets, uint8_t channel);
        ~ImageHistogramPendingData();

        // get the computed histogram data, valid only after some time
        const base::RefPtr<ImageHistogramData> fetchDataIfReady();

    private:
        base::SpinLock m_lock;
        base::RefPtr<ImageHistogramData> m_data;

        uint32_t m_totalPixelCount = 0;
        uint32_t m_numBuckets = 0;
        uint8_t m_channel = 0;

		//--

		virtual void processRetreivedData(const void* dataPtr, uint32_t dataSize, const rendering::ResourceCopyRange& info) override final;
    };

    //--

    struct ImageComputationSettings
    {
        float alphaThreshold = 0.0f;
        int mipIndex = 0;
        int sliceIndex = 0;
        uint8_t channel = 0;        
    };

    /// compute histogram from rendering image, uses GPU compute shaders
    extern ASSETS_IMAGE_EDITOR_API base::RefPtr<ImageHistogramPendingData> ComputeHistogram(const rendering::ImageSampledView* view, const ImageComputationSettings& settings);

    //--

} // ed