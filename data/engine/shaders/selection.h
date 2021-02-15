/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
***/

//--

struct EncodedSelectable
{
    uint id;
    uint subId;
    uint pixel; // y<<16 | x
    float depth;
};

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
}

//--

shader SelectionGatherPS
{
    void EmitSelection(uint selectableId, uint subSelectionId, float depth)
    {
        if (selectableId > 0)
        {
            uvec2 pos = uvec2(gl_FragCoord.x, gl_FragCoord.y);
            if (pos.x >= SelectionAreaMinX && pos.y >= SelectionAreaMinY && pos.x < SelectionAreaMaxX && pos.y < SelectionAreaMaxY)
            {
                uint index = (pos.x - SelectionAreaMinX);
                index += (pos.y - SelectionAreaMinY) * (SelectionAreaMaxX - SelectionAreaMinX);
                
                CollectedSelectables[index].id = selectableId;
                CollectedSelectables[index].subId = subSelectionId;
                CollectedSelectables[index].pixel = pos.x + (pos.y * 65536);
                CollectedSelectables[index].depth = depth;
            }
        }
    }
}

//--