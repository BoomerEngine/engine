/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#include "build.h"
#include "debugGeometryAssets.h"

#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/deviceService.h"

#include "core/image/include/imageUtils.h"
#include "core/image/include/imageView.h"
#include "core/image/include/image.h"

#include "engine/atlas/include/dynamicGlyphAtlas.h"
#include "engine/atlas/include/dynamicImageAtlas.h"
#include "engine/atlas/include/dynamicImageAtlasEntry.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(DebugGeometryImage);
RTTI_END_TYPE();

DebugGeometryImage::DebugGeometryImage(Image* data)
{
    if (data)
    {
        ImagePtr tempData;
        
        if (data->channels() != 4)
        {
            tempData = ConvertChannels(data->view(), 4);
            data = tempData;
        }

        m_entry = RefNew<DynamicImageAtlasEntry>(data);
        GetService<DebugGeometryAssetService>()->imageAtlas()->attach(m_entry);
        m_id = m_entry->id();
    }
}

DebugGeometryImage::DebugGeometryImage(StringView depotPath)
{
    if (auto data = LoadImageFromDepotPath(depotPath))
    {
        if (data->channels() != 4)
            data = ConvertChannels(data->view(), 4);

        m_entry = RefNew<DynamicImageAtlasEntry>(data);
        GetService<DebugGeometryAssetService>()->imageAtlas()->attach(m_entry);
        m_id = m_entry->id();
    }
}

DebugGeometryImage::~DebugGeometryImage()
{
    if (m_entry)
    {
        if (auto service = GetService<DebugGeometryAssetService>())
            service->imageAtlas()->remove(m_entry);
        m_entry.reset();
    }
}

//--

RTTI_BEGIN_TYPE_CLASS(DebugGeometryAssetService);
RTTI_METADATA(DependsOnServiceMetadata).dependsOn<DeviceService>();
RTTI_END_TYPE();

DebugGeometryAssetService::DebugGeometryAssetService()
{}

DebugGeometryAssetService::~DebugGeometryAssetService()
{}

void DebugGeometryAssetService::flushUpdates(gpu::CommandWriter& cmd)
{
    m_glyphAtlas->update(cmd);
    m_imageAtlas->update(cmd);
}

bool DebugGeometryAssetService::onInitializeService(const CommandLine& cmdLine)
{
    m_imageAtlas = RefNew<DynamicImageAtlas>(ImageFormat::SRGBA8);
    m_glyphAtlas = RefNew<DynamicGlyphAtlas>();
    return true;
}

void DebugGeometryAssetService::onShutdownService()
{
    m_imageAtlas.reset();
    m_glyphAtlas.reset();
}

void DebugGeometryAssetService::onSyncUpdate()
{

}

//--

END_BOOMER_NAMESPACE();