/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "engine/ui/include/uiElement.h"
#include "gpu/device/include/renderingResources.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

// computed histogram
struct ImageHistogramData : public IReferencable
{
    uint8_t channel = 0;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    uint32_t totalPixelCount = 0;
    uint32_t maxBucketValue = 0;
    Array<uint32_t> buckets;
};

//--

// histogram computation pending job
class EDITOR_IMAGE_EDITOR_API ImageHistogramPendingData : public gpu::IDownloadDataSink
{
public:
    ImageHistogramPendingData(uint32_t totalPixelCount, uint32_t numBuckets, uint8_t channel);
    ~ImageHistogramPendingData();

    // get the computed histogram data, valid only after some time
    const RefPtr<ImageHistogramData> fetchDataIfReady();

private:
    SpinLock m_lock;
    RefPtr<ImageHistogramData> m_data;

    uint32_t m_totalPixelCount = 0;
    uint32_t m_numBuckets = 0;
    uint8_t m_channel = 0;

	//--

	virtual void processRetreivedData(const void* dataPtr, uint32_t dataSize, const gpu::ResourceCopyRange& info) override final;
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
extern EDITOR_IMAGE_EDITOR_API RefPtr<ImageHistogramPendingData> ComputeHistogram(const gpu::ImageSampledView* view, const ImageComputationSettings& settings);

//--

END_BOOMER_NAMESPACE_EX(ed)
