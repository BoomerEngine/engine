/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"

#include "canvas.h"
#include "service.h"
#include "renderer.h"

#include "core/containers/include/bitUtils.h"

#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/shaderService.h"

#include "engine/atlas/include/dynamicGlyphAtlas.h"
#include "engine/atlas/include/dynamicImageAtlas.h"
#include "engine/atlas/include/dynamicImageAtlasEntry.h"
#include "core/image/include/imageUtils.h"

BEGIN_BOOMER_NAMESPACE()

//---

ConfigProperty<uint32_t> cvCanvasGlyphCacheAtlasSize("Rendering.Canvas", "GlyphCacheAtlasSize", 2048);
ConfigProperty<uint32_t> cvCanvasGlyphCacheAtlasPageCount("Rendering.Canvas", "GlyphCacheAtlasPageCount", 1);

//---
		
RTTI_BEGIN_TYPE_CLASS(CanvasService);
	RTTI_METADATA(DependsOnServiceMetadata).dependsOn<gpu::DeviceService>();
	RTTI_METADATA(DependsOnServiceMetadata).dependsOn<gpu::ShaderService>();	
RTTI_END_TYPE();

//---


RTTI_BEGIN_TYPE_NATIVE_CLASS(CanvasImage);
RTTI_END_TYPE();

CanvasImage::CanvasImage(const Image* data, bool wrapU /*= false*/, bool wrapV /*= false*/)
    : m_wrapU(wrapU)
    , m_wrapV(wrapV)
{
    if (data)
    {
        ImagePtr tempData;

        if (data->channels() != 4)
        {
            tempData = ConvertChannels(data->view(), 4);
            data = tempData;
        }

        m_entry = RefNew<DynamicImageAtlasEntry>(data, wrapU, wrapV);
        GetService<CanvasService>()->imageAtlas()->attach(m_entry);

        m_id = m_entry->id();

        m_width = m_entry->width();
        m_height = m_entry->height();
    }
}

CanvasImage::CanvasImage(StringView depotPath, bool wrapU /*= false*/, bool wrapV /*= false*/)
{
    if (auto data = LoadImageFromDepotPath(depotPath))
    {
        if (data->channels() != 4)
            data = ConvertChannels(data->view(), 4);

        m_entry = RefNew<DynamicImageAtlasEntry>(data, wrapU, wrapV);
        GetService<CanvasService>()->imageAtlas()->attach(m_entry);

        m_id = m_entry->id();

        m_width = m_entry->width();
        m_height = m_entry->height();
    }
}

CanvasImage::~CanvasImage()
{
    if (m_entry)
    {
        if (auto service = GetService<CanvasService>())
            service->imageAtlas()->remove(m_entry);
        m_entry.reset();
    }
}

//---

CanvasService::CanvasService()
{}

//--

bool CanvasService::onInitializeService(const CommandLine& cmdLine)
{
    m_imageAtlas = RefNew<DynamicImageAtlas>(ImageFormat::RGBA8_UNORM); // canvas is in SRGB space
    m_glyphAtlas = RefNew<DynamicGlyphAtlas>();

	m_renderer = new CanvasRenderer();

	return true;
}

void CanvasService::onShutdownService()
{
	delete m_renderer;
	m_renderer = nullptr;

	m_glyphAtlas.reset();
	m_imageAtlas.reset();
}

void CanvasService::onSyncUpdate()
{
	// nothing
}

//--

void CanvasService::sync(gpu::CommandWriter& cmd)
{
}

void CanvasService::render(gpu::CommandWriter& cmd, const Canvas& c)
{
	cmd.opBeginBlock("CanvasRender");

	{
		cmd.opBeginBlock("SyncAtlas");
		m_glyphAtlas->update(cmd);
		m_imageAtlas->update(cmd);
	}

	CanvasRenderer::RenderInfo info;
	info.imageAtlasSRV = m_imageAtlas->imageSRV();
	info.imageEntriesSRV = m_imageAtlas->imageEntriesSRV();
    info.glyphAtlasSRV = m_glyphAtlas->imageSRV();
    info.glypEntriesSRV = m_glyphAtlas->imageEntriesSRV();

	m_renderer->render(cmd, info, c);
}

//--

END_BOOMER_NAMESPACE()
