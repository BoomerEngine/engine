/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "examples_model_viewer_glue.inl"

#include "BoomerEngine.h"

/*#include "base/containers/include/array.h"
#include "base/containers/include/inplaceArray.h"
#include "base/input/include/inputStructures.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingImageView.h"
#include "rendering/driver/include/renderingBufferView.h"
#include "rendering/runtime/include/renderingFrameCollector.h"
#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"
#include "base/system/include/spinLock.h"
#include "base/system/include/scopeLock.h"
#include "base/system/include/atomic.h"*/

using namespace base;
using namespace base::mesh;
using namespace base::image;

using base::Vector4;
using base::Vector3;
using base::Vector2;
using base::Angles;
using base::Matrix;
using base::Rect;
using base::Point;
using base::SpinLock;
using base::ScopeLock;

using rendering::IDriver;
using rendering::BufferView;

using rendering::ImageView;
using rendering::ImageView;
using rendering::ImageCreationInfo;
using rendering::SourceData;

using rendering::runtime::IDeviceObject;
using rendering::runtime::FrameCollectorNode;

using base::Array;
using base::InplaceArray;
using base::StringBuf;

using base::input::BaseEvent;
using base::input::KeyCode;
using base::image::Image;
using base::image::ImageRef;
using base::image::ImagePtr;
using base::image::ImageView;


