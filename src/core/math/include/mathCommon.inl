/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

ALWAYS_INLINE uint8_t FloatTo255(float col)
{
    if (col >= 0.003921568627450980392156862745098f)
    {
        if (col <= 0.9960784313725490196078431372549f)
            return (uint8_t)std::lround(col * 255.0f);
        else
            return 255;
    }
    else
    {
        return 0;
    }
}

//--

ALWAYS_INLINE float Snap(float val, float grid)
{
    if (grid > 0.0f)
    {
        int64_t numGridUnits = (int64_t)std::round(val / grid);
        return numGridUnits * grid;
    }
    else
    {
        return val;
    }
}

ALWAYS_INLINE double Snap(double val, double grid)
{
    if (grid > 0.0)
    {
        int64_t numGridUnits = (int64_t)std::round(val / grid);
        return numGridUnits * grid;
    }
    else
    {
        return val;
    }
}

//--

END_BOOMER_NAMESPACE();
