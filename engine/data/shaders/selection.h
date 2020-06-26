/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
***/

#include "math.h"

//--

descriptor SelectionBufferCollectionParams
{
    ConstantBuffer
    {
        uint SelectionAreaMinX;
        uint SelectionAreaMinY;
        uint SelectionAreaMaxX;
        uint SelectionAreaMaxY;
        uint MaxSelectables;
    }

    attribute(layout=EncodedSelectable, uav) Buffer CollectedSelectables;
    attribute(format=r32i, uav) Buffer CollectedSelectablesCount; // just one uint for the count :(
}

//--

shader SelectionGatherPS
{
    void emitSelection(uint selectableId, uint subSelectionId, float depth)
    {
        if (selectableId > 0)
        {
            uvec2 pos = uvec2(gl_FragCoord.x, gl_FragCoord.y);
            if (pos.x >= SelectionAreaMinX && pos.y >= SelectionAreaMinY && pos.x < SelectionAreaMaxX && pos.y < SelectionAreaMaxY)
            {
                uint index = atomicIncrement(CollectedSelectablesCount[0]);
                if (index < MaxSelectables)
                {
                    CollectedSelectables[index].id = selectableId;
                    CollectedSelectables[index].pixel = pos.x + (pos.y * 65536);
                    CollectedSelectables[index].depth = depth;
                }
            }
        }
    }
}

//--