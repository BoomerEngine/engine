/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "public.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

struct MeshPreviewPanelSettings
{
    int forceLod = -1;
    int forceChunk = -1;

    bool showBounds = false;
    bool showSkeleton = false;
    bool showBoneNames = false;
    bool showBoneAxes = false;
    bool showSnaps = false;
    bool showCollision = false;

    bool isolateMaterials = false;
    bool highlightMaterials = false;
    HashSet<StringID> selectedMaterials;
};

END_BOOMER_NAMESPACE_EX(ed)