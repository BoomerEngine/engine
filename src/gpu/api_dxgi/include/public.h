/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "gpu_api_dxgi_glue.inl"

#include <DXGI.h>

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

class DXGIHelper;

//--

extern GPU_API_DXGI_API const char* TranslateDXGIError(HRESULT hRet);

extern GPU_API_DXGI_API const char* TranslateDXError(HRESULT hRet);

extern GPU_API_DXGI_API const char* TranslateDXGIFormatName(DXGI_FORMAT format);

extern GPU_API_DXGI_API void ValidateDXGIResult(HRESULT hRet, const char* expr, const char* file, uint32_t line);

extern GPU_API_DXGI_API void ValidateDXResult(HRESULT hRet, const char* expr, const char* file, uint32_t line);

extern GPU_API_DXGI_API void ValidateDXRelease(IUnknown** pPtr);

//--

END_BOOMER_NAMESPACE_EX(gpu::api)

#ifndef BUILD_RELEASE
	#define DXGI_PROTECT(expr) { HRESULT hRet = expr; gpu::api::ValidateDXGIResult(hRet, #expr, __FILE__, __LINE__); }
	#define DX_PROTECT(expr) { HRESULT hRet = expr; gpu::api::ValidateDXResult(hRet, #expr, __FILE__, __LINE__); }
#else
	#define DXGI_PROTECT(expr) { expr; }
	#define DX_PROTECT(expr) { expr; }
#endif

#define DX_RELEASE(expr) gpu::api::ValidateDXRelease((IUnknown**) &expr)

