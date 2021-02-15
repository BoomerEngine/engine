/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_api_dxgi_glue.inl"

#include <DXGI.h>

namespace rendering
{
	namespace api
	{

		//--

		class DXGIHelper;

		//--

		extern RENDERING_API_DXGI_API const char* TranslateDXGIError(HRESULT hRet);

		extern RENDERING_API_DXGI_API const char* TranslateDXError(HRESULT hRet);

		extern RENDERING_API_DXGI_API const char* TranslateDXGIFormatName(DXGI_FORMAT format);

		extern RENDERING_API_DXGI_API void ValidateDXGIResult(HRESULT hRet, const char* expr, const char* file, uint32_t line);

		extern RENDERING_API_DXGI_API void ValidateDXResult(HRESULT hRet, const char* expr, const char* file, uint32_t line);

		extern RENDERING_API_DXGI_API void ValidateDXRelease(IUnknown** pPtr);

		//--

	} // api
} // rendering

#ifndef BUILD_RELEASE
	#define DXGI_PROTECT(expr) { HRESULT hRet = expr; rendering::api::ValidateDXGIResult(hRet, #expr, __FILE__, __LINE__); }
	#define DX_PROTECT(expr) { HRESULT hRet = expr; rendering::api::ValidateDXResult(hRet, #expr, __FILE__, __LINE__); }
#else
	#define DXGI_PROTECT(expr) { expr; }
	#define DX_PROTECT(expr) { expr; }
#endif

#define DX_RELEASE(expr) rendering::api::ValidateDXRelease((IUnknown**) &expr)

