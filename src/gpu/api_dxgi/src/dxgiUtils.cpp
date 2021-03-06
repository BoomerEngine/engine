/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: dxgi #]
***/

#include "build.h"
#include "dxgiHelper.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//---

const char* TranslateDXGIFormatName(DXGI_FORMAT format)
{
	switch (format)
	{
#define TEST(x) case x: return #x;
		TEST(DXGI_FORMAT_UNKNOWN);
		TEST(DXGI_FORMAT_R32G32B32A32_TYPELESS);
		TEST(DXGI_FORMAT_R32G32B32A32_FLOAT);
		TEST(DXGI_FORMAT_R32G32B32A32_UINT);
		TEST(DXGI_FORMAT_R32G32B32A32_SINT);
		TEST(DXGI_FORMAT_R32G32B32_TYPELESS);
		TEST(DXGI_FORMAT_R32G32B32_FLOAT);
		TEST(DXGI_FORMAT_R32G32B32_UINT);
		TEST(DXGI_FORMAT_R32G32B32_SINT);
		TEST(DXGI_FORMAT_R16G16B16A16_TYPELESS);
		TEST(DXGI_FORMAT_R16G16B16A16_FLOAT);
		TEST(DXGI_FORMAT_R16G16B16A16_UNORM);
		TEST(DXGI_FORMAT_R16G16B16A16_UINT);
		TEST(DXGI_FORMAT_R16G16B16A16_SNORM);
		TEST(DXGI_FORMAT_R16G16B16A16_SINT);
		TEST(DXGI_FORMAT_R32G32_TYPELESS);
		TEST(DXGI_FORMAT_R32G32_FLOAT);
		TEST(DXGI_FORMAT_R32G32_UINT);
		TEST(DXGI_FORMAT_R32G32_SINT);
		TEST(DXGI_FORMAT_R32G8X24_TYPELESS);
		TEST(DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
		TEST(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
		TEST(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT);
		TEST(DXGI_FORMAT_R10G10B10A2_TYPELESS);
		TEST(DXGI_FORMAT_R10G10B10A2_UNORM);
		TEST(DXGI_FORMAT_R10G10B10A2_UINT);
		TEST(DXGI_FORMAT_R11G11B10_FLOAT);
		TEST(DXGI_FORMAT_R8G8B8A8_TYPELESS);
		TEST(DXGI_FORMAT_R8G8B8A8_UNORM);
		TEST(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		TEST(DXGI_FORMAT_R8G8B8A8_UINT);
		TEST(DXGI_FORMAT_R8G8B8A8_SNORM);
		TEST(DXGI_FORMAT_R8G8B8A8_SINT);
		TEST(DXGI_FORMAT_R16G16_TYPELESS);
		TEST(DXGI_FORMAT_R16G16_FLOAT);
		TEST(DXGI_FORMAT_R16G16_UNORM);
		TEST(DXGI_FORMAT_R16G16_UINT);
		TEST(DXGI_FORMAT_R16G16_SNORM);
		TEST(DXGI_FORMAT_R16G16_SINT);
		TEST(DXGI_FORMAT_R32_TYPELESS);
		TEST(DXGI_FORMAT_D32_FLOAT);
		TEST(DXGI_FORMAT_R32_FLOAT);
		TEST(DXGI_FORMAT_R32_UINT);
		TEST(DXGI_FORMAT_R32_SINT);
		TEST(DXGI_FORMAT_R24G8_TYPELESS);
		TEST(DXGI_FORMAT_D24_UNORM_S8_UINT);
		TEST(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
		TEST(DXGI_FORMAT_X24_TYPELESS_G8_UINT);
		TEST(DXGI_FORMAT_R8G8_TYPELESS);
		TEST(DXGI_FORMAT_R8G8_UNORM);
		TEST(DXGI_FORMAT_R8G8_UINT);
		TEST(DXGI_FORMAT_R8G8_SNORM);
		TEST(DXGI_FORMAT_R8G8_SINT);
		TEST(DXGI_FORMAT_R16_TYPELESS);
		TEST(DXGI_FORMAT_R16_FLOAT);
		TEST(DXGI_FORMAT_D16_UNORM);
		TEST(DXGI_FORMAT_R16_UNORM);
		TEST(DXGI_FORMAT_R16_UINT);
		TEST(DXGI_FORMAT_R16_SNORM);
		TEST(DXGI_FORMAT_R16_SINT);
		TEST(DXGI_FORMAT_R8_TYPELESS);
		TEST(DXGI_FORMAT_R8_UNORM);
		TEST(DXGI_FORMAT_R8_UINT);
		TEST(DXGI_FORMAT_R8_SNORM);
		TEST(DXGI_FORMAT_R8_SINT);
		TEST(DXGI_FORMAT_A8_UNORM);
		TEST(DXGI_FORMAT_R1_UNORM);
		TEST(DXGI_FORMAT_R9G9B9E5_SHAREDEXP);
		TEST(DXGI_FORMAT_R8G8_B8G8_UNORM);
		TEST(DXGI_FORMAT_G8R8_G8B8_UNORM);
		TEST(DXGI_FORMAT_BC1_TYPELESS);
		TEST(DXGI_FORMAT_BC1_UNORM);
		TEST(DXGI_FORMAT_BC1_UNORM_SRGB);
		TEST(DXGI_FORMAT_BC2_TYPELESS);
		TEST(DXGI_FORMAT_BC2_UNORM);
		TEST(DXGI_FORMAT_BC2_UNORM_SRGB);
		TEST(DXGI_FORMAT_BC3_TYPELESS);
		TEST(DXGI_FORMAT_BC3_UNORM);
		TEST(DXGI_FORMAT_BC3_UNORM_SRGB);
		TEST(DXGI_FORMAT_BC4_TYPELESS);
		TEST(DXGI_FORMAT_BC4_UNORM);
		TEST(DXGI_FORMAT_BC4_SNORM);
		TEST(DXGI_FORMAT_BC5_TYPELESS);
		TEST(DXGI_FORMAT_BC5_UNORM);
		TEST(DXGI_FORMAT_BC5_SNORM);
		TEST(DXGI_FORMAT_B5G6R5_UNORM);
		TEST(DXGI_FORMAT_B5G5R5A1_UNORM);
		TEST(DXGI_FORMAT_B8G8R8A8_UNORM);
		TEST(DXGI_FORMAT_B8G8R8X8_UNORM);
		TEST(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM);
		TEST(DXGI_FORMAT_B8G8R8A8_TYPELESS);
		TEST(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
		TEST(DXGI_FORMAT_B8G8R8X8_TYPELESS);
		TEST(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB);
		TEST(DXGI_FORMAT_BC6H_TYPELESS);
		TEST(DXGI_FORMAT_BC6H_UF16);
		TEST(DXGI_FORMAT_BC6H_SF16);
		TEST(DXGI_FORMAT_BC7_TYPELESS);
		TEST(DXGI_FORMAT_BC7_UNORM);
		TEST(DXGI_FORMAT_BC7_UNORM_SRGB);
		TEST(DXGI_FORMAT_AYUV);
		TEST(DXGI_FORMAT_Y410);
		TEST(DXGI_FORMAT_Y416);
		TEST(DXGI_FORMAT_NV12);
		TEST(DXGI_FORMAT_P010);
		TEST(DXGI_FORMAT_P016);
		TEST(DXGI_FORMAT_420_OPAQUE);
		TEST(DXGI_FORMAT_YUY2);
		TEST(DXGI_FORMAT_Y210);
		TEST(DXGI_FORMAT_Y216);
		TEST(DXGI_FORMAT_NV11);
		TEST(DXGI_FORMAT_AI44);
		TEST(DXGI_FORMAT_IA44);
		TEST(DXGI_FORMAT_P8);
		TEST(DXGI_FORMAT_A8P8);
		TEST(DXGI_FORMAT_B4G4R4A4_UNORM);
		TEST(DXGI_FORMAT_P208);
		TEST(DXGI_FORMAT_V208);
		TEST(DXGI_FORMAT_V408);
	}
#undef TEST

	return "Unknown";
}

//---

const char* TranslateDXGIError(HRESULT hRet)
{
#define TEST_ERR(x) case x: return #x;
	switch (hRet)
	{
		TEST_ERR(DXGI_ERROR_ACCESS_DENIED)
		TEST_ERR(DXGI_ERROR_ACCESS_LOST)
		TEST_ERR(DXGI_ERROR_ALREADY_EXISTS)
		TEST_ERR(DXGI_ERROR_CANNOT_PROTECT_CONTENT)
		TEST_ERR(DXGI_ERROR_DEVICE_HUNG)
		TEST_ERR(DXGI_ERROR_DEVICE_REMOVED)
		TEST_ERR(DXGI_ERROR_DEVICE_RESET)
		TEST_ERR(DXGI_ERROR_DRIVER_INTERNAL_ERROR)
		TEST_ERR(DXGI_ERROR_FRAME_STATISTICS_DISJOINT)
		TEST_ERR(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE)
		TEST_ERR(DXGI_ERROR_INVALID_CALL)
		TEST_ERR(DXGI_ERROR_MORE_DATA)
		TEST_ERR(DXGI_ERROR_NAME_ALREADY_EXISTS)
		TEST_ERR(DXGI_ERROR_NONEXCLUSIVE)
		TEST_ERR(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
		TEST_ERR(DXGI_ERROR_NOT_FOUND)
		TEST_ERR(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED)
		TEST_ERR(DXGI_ERROR_REMOTE_OUTOFMEMORY)
		TEST_ERR(DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE)
		TEST_ERR(DXGI_ERROR_SDK_COMPONENT_MISSING)
		TEST_ERR(DXGI_ERROR_SESSION_DISCONNECTED)
		TEST_ERR(DXGI_ERROR_UNSUPPORTED)
		TEST_ERR(DXGI_ERROR_WAIT_TIMEOUT)
		TEST_ERR(DXGI_ERROR_WAS_STILL_DRAWING)
	}
#undef TEST_ERR

	return "Unknown";
}

void ValidateDXGIResult(HRESULT hRet, const char* expr, const char* file, uint32_t line)
{
	if (hRet == S_OK)
		return;

#ifndef BUILD_RELEASE
	TRACE_ERROR("Runtime DXGI API error: {}", TranslateDXGIError(hRet));
	TRACE_ERROR("{}({}): DXGI: Failed expression: {}", file, line, expr);

#ifdef BUILD_DEBUG
	DEBUG_CHECK_EX(hRet != S_OK, TempString("Runtime DXGI API error: {} (0x{}) at {}", TranslateDXGIError(hRet), Hex(hRet), expr));
#endif
#endif
}

void ValidateDXResult(HRESULT hRet, const char* expr, const char* file, uint32_t line)
{
	if (hRet == S_OK)
		return;

#ifndef BUILD_RELEASE
	TRACE_ERROR("Runtime DX API error: {}", TranslateDXGIError(hRet));
	TRACE_ERROR("{}({}): DX: Failed expression: {}", file, line, expr);

#ifdef BUILD_DEBUG
	DEBUG_CHECK_EX(hRet != S_OK && hRet != S_FALSE, TempString("Runtime DX API error: {} (0x{}) at {}", TranslateDXError(hRet), Hex(hRet), expr));
#endif
#endif
}

//---

const char* TranslateDXError(HRESULT hRet) {
#define TEST_ERR(x) case x: return #x;
	switch (hRet)
	{
		TEST_ERR(D3D11_ERROR_FILE_NOT_FOUND);
		TEST_ERR(D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS)
		TEST_ERR(D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS)
		TEST_ERR(D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD)
		TEST_ERR(E_FAIL)
		TEST_ERR(E_INVALIDARG)
		TEST_ERR(E_OUTOFMEMORY)
		TEST_ERR(E_NOTIMPL)
	}
#undef TEST_ERR

	return TranslateDXGIError(hRet);
}

//---

void ValidateDXRelease(IUnknown** expr)
{
	if (expr && *expr)
	{
		(*expr)->Release();
		*expr = nullptr;
	}
}

//---

END_BOOMER_NAMESPACE_EX(gpu::api)
