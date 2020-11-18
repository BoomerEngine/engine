/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: camera #]
***/

#include "build.h"
#include "renderingFrameCameraContext.h"

namespace rendering
{
    namespace scene
    {
		//--

		CameraContext::CameraContext(const base::StringBuf& cameraLabel)
			: m_name(cameraLabel)
		{}

		CameraContext::~CameraContext()
		{}

		//--

	} // scene
} // rendering
