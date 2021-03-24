/***a
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: format #]
***/

#include "build.h"
#include "fileMtl.h"

#include "core/io/include/io.h"
#include "core/io/include/fileHandle.h"
#include "core/containers/include/stringParser.h"
#include "core/containers/include/hashSet.h"
#include "core/containers/include/stringBuilder.h"
#include "core/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

static bool ParseColor(StringView contextName, StringParser& str, Color& outColor)
{
    Vector4 ret;
    if (!str.parseFloat(ret.x))
    {
        TRACE_WARNING("{}({}): warning: Failed to parse red component of color", contextName, str.line());
        return false;
    }

    if (!str.parseFloat(ret.y))
    {
        TRACE_WARNING("{}({}): warning: Failed to parse green component of color", contextName, str.line());
        return false;
    }

    if (!str.parseFloat(ret.z))
    {
        TRACE_WARNING("{}({}): warning: Failed to parse blue component of color", contextName, str.line());
        return false;
    }

    outColor = Color::FromVectorSRGBExact(ret);
    return true;
}

static bool ParseMaterialMap(StringView contextName, StringParser& str, MaterialMap& outMap)
{
    StringView txt;
    if (!str.parseString(txt))
        return false;

    outMap.m_path = StringBuf(txt);
    return true;
}

static bool ParseMaterial(StringView contextName, StringParser& str, Material& outMat)
{
    while (str.parseWhitespaces())
    {
        if (str.testKeyword("newmtl"))
            break;

        if (str.parseKeyword("Ns"))
        {
            if (!str.parseFloat(outMat.m_specularExp))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for Ns", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("Ni"))
        {
            if (!str.parseFloat(outMat.m_opticalDensity))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for Ns", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("Ka"))
        {
            if (!ParseColor(contextName, str, outMat.m_colorAmbient))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for Ka", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("Kd"))
        {
            if (!ParseColor(contextName, str, outMat.m_colorDiffuse))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for Kd", contextName, str.line());
                continue;
            }

        }
        else if (str.parseKeyword("Ks"))
        {
            if (!ParseColor(contextName, str, outMat.m_colorSpecular))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for Ks", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("Ke"))
        {
            if (!ParseColor(contextName, str, outMat.m_colorEmissive))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for Ke", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("illum"))
        {
            if (!str.parseInt32(outMat.m_illumMode))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for Illum", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("Tr"))
        {
            float tr = 0.0f;
            if (!str.parseFloat(tr))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for Tr", contextName, str.line());
                continue;
            }

            outMat.m_dissolveCenter = 1.0f - tr;
            outMat.m_dissolveHalo = 1.0f - tr;
        }
        else if (str.parseKeyword("Tf"))
        {
            if (!ParseColor(contextName, str, outMat.m_colorTransmission))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for Tf", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("d"))
        {
            if (!str.parseFloat(outMat.m_dissolveCenter))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for d", contextName, str.line());
                continue;
            }

            outMat.m_dissolveHalo = outMat.m_dissolveCenter;

            if (str.parseKeyword("halo") || str.parseKeyword("-halo"))
            {
                if (str.parseFloat(outMat.m_dissolveHalo))
                {
                    TRACE_WARNING("{}({}): warning: Failed to parse value for d", contextName, str.line());
                    continue;
                }
            }
        }
        else if (str.parseKeyword("map_Ka"))
        {
            if (!ParseMaterialMap(contextName, str, outMat.m_mapAmbient))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for map_Ka", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("map_Kd"))
        {
            if (!ParseMaterialMap(contextName, str, outMat.m_mapDiffuse))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for map_Kd", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("map_Ke"))
        {
            if (!ParseMaterialMap(contextName, str, outMat.m_mapEmissive))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for map_Ke", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("map_Ks"))
        {
            if (!ParseMaterialMap(contextName, str, outMat.m_mapSpecular))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for map_Ks", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("map_d"))
        {
            if (!ParseMaterialMap(contextName, str, outMat.m_mapDissolve))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for map_d", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("map_bump"))
        {
            if (!ParseMaterialMap(contextName, str, outMat.m_mapBump))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for map_bump", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("map_normal"))
        {
            if (!ParseMaterialMap(contextName, str, outMat.m_mapNormal))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for map_normal", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("map_Krs"))
        {
            if (!ParseMaterialMap(contextName, str, outMat.m_mapRoughnessSpecularity))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for map_Krs", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("map_Kr"))
        {
            if (!ParseMaterialMap(contextName, str, outMat.m_mapRoughness))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for map_Kr", contextName, str.line());
                continue;
            }
        }
        else if (str.parseKeyword("map_Km"))
        {
            if (!ParseMaterialMap(contextName, str, outMat.m_mapMetallic))
            {
                TRACE_WARNING("{}({}): warning: Failed to parse value for map_Km", contextName, str.line());
                continue;
            }
        }
        else
        {
            str.parseTillTheEndOfTheLine();
        }
    }

    // parsed
    return true;
}

bool LoadFromBuffer(StringView contextName, const void* data, uint64_t dataSize, Array<Material>& outMaterials)
{
    StringParser parser(data, dataSize);

    while (parser.parseWhitespaces())
    {
        if (parser.parseKeyword("newmtl"))
        {
            StringView name;
            parser.parseString(name);

            Material mat;
            mat.m_name = StringBuf(name);

            if (ParseMaterial(contextName, parser, mat))
                outMaterials.pushBack(mat);
        }
        else
        {
            parser.parseTillTheEndOfTheLine();
        }
    }

    return true;
}

SourceAssetMTLPtr LoadFromBuffer(StringView contextName, const void* data, uint64_t dataSize)
{
    InplaceArray<Material, 128> materials;
    if (!LoadFromBuffer(contextName, data, dataSize, materials))
        return nullptr;

    return RefNew<SourceAssetMTL>(materials);
}
//--

SourceAssetMTLPtr LoadMaterials(StringView contextName, const void* data, uint64_t dataSize)
{
    return LoadFromBuffer(contextName, data, dataSize);
}

//--

END_BOOMER_NAMESPACE_EX(assets)
