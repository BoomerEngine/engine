//===============================================================================
// Copyright (c) 2007-2016  Advanced Micro Devices, Inc. All rights reserved.
// Copyright (c) 2004-2006 ATI Technologies Inc.
//===============================================================================
// [# filter:bc7 #]
// [# pch:disabled #]
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef _BC7_UTILS_H_
#define _BC7_UTILS_H_

#include "BC7_Definitions.h"

extern void    WriteBit(unsigned char   *base,
                    int  offset,
                    unsigned char   bitVal);

extern void GetRamp(unsigned int endpoint[][MAX_DIMENSION_BIG],
                double ramp[MAX_DIMENSION_BIG][(1<<MAX_INDEX_BITS)],
                unsigned int clusters[2],
                unsigned int componentBits[MAX_DIMENSION_BIG]);

extern void DecodeEndPoints(unsigned int endpoint[][MAX_DIMENSION_BIG],
                unsigned int componentBits[MAX_DIMENSION_BIG],
                float ep[][MAX_DIMENSION_BIG]);

extern const double  rampLerpWeights[5][1<<MAX_INDEX_BITS];

#endif

