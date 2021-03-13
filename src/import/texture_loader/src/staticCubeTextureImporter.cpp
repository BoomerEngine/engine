/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "staticCubeTextureImporter.h"

#include "core/io/include/fileHandle.h"
#include "core/image/include/image.h"
#include "core/image/include/imageUtils.h"
#include "core/app/include/localServiceContainer.h"
#include "core/resource/include/resource.h"
#include "core/containers/include/inplaceArray.h"
#include "core/image/include/imageView.h"
#include "core/image/include/freeImageLoader.h"
#include "core/resource/include/tags.h"
#include "engine/texture/include/staticTextureCube.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//---

RTTI_BEGIN_TYPE_CLASS(StaticTextureCubeCompressionFace);
    RTTI_PROPERTY(suffix).editable("File name suffix for automatic suck in").overriddable();
    RTTI_PROPERTY(swapXY).editable("Should we swap X and Y").overriddable();
    RTTI_PROPERTY(flipX).editable("Should we flip the texture in the X direction").overriddable();
    RTTI_PROPERTY(flipY).editable("Should we flip the texture in the Y direction").overriddable();
RTTI_END_TYPE();

//---

RTTI_BEGIN_TYPE_ENUM(StaticTextureCubeImportSpace);
RTTI_ENUM_OPTION(SpaceYUp);
RTTI_ENUM_OPTION(SpaceZUp)
RTTI_END_TYPE();

//---

RTTI_BEGIN_TYPE_CLASS(StaticTextureCubeCompressionConfiguration);
    RTTI_CATEGORY("Faces");
    RTTI_PROPERTY(m_space).editable("General cubemap orientation").overriddable();
    RTTI_PROPERTY(m_facePX).editable("+X face settings").overriddable();
    RTTI_PROPERTY(m_faceNX).editable("-X face settings").overriddable();
    RTTI_PROPERTY(m_facePY).editable("+Y face settings").overriddable();
    RTTI_PROPERTY(m_faceNY).editable("-Y face settings").overriddable();
    RTTI_PROPERTY(m_facePZ).editable("+Z face settings").overriddable();
    RTTI_PROPERTY(m_faceNZ).editable("-Z face settings").overriddable();
RTTI_END_TYPE();

StaticTextureCubeCompressionConfiguration::StaticTextureCubeCompressionConfiguration()
{
    m_faceNX.suffix = "-NX;_nx";
    m_faceNY.suffix = "-NY;_ny";
    m_faceNZ.suffix = "-NZ;_nz";
    m_facePX.suffix = "-PX;_px";
    m_facePY.suffix = "-PY;_py";
    m_facePZ.suffix = "-PZ;_pz";
}

//--

RTTI_BEGIN_TYPE_CLASS(StaticCubeTextureFromImageImporter);
    RTTI_METADATA(ResourceImportedClassMetadata).addClass<StaticTextureCube>();
    RTTI_METADATA(ResourceSourceFormatMetadata).addSourceExtensions("bmp;dds;png;jpg;jpeg;jp2;jpx;tga;tif;tiff;hdr;exr;ppm;pbm;psd;xbm;nef;xpm;gif;webp");
    RTTI_METADATA(ResourceImporterConfigurationClassMetadata).configurationClass<StaticTextureCubeCompressionConfiguration>();
RTTI_END_TYPE();

StaticCubeTextureFromImageImporter::StaticCubeTextureFromImageImporter()
{}

static StringView GetTextureSuffix(StringView path)
{
    auto fileName = path.afterLastOrFull("/").beforeFirstOrFull(".");
    if (fileName.empty())
        return StringBuf::EMPTY();

    return fileName.afterLast("_");
}

static bool MatchSuffix(StringView pattern, StringView name, StringView& outCoreStem)
{
    InplaceArray<StringView, 10> parts;
    pattern.slice(";", false, parts);

    for (const auto part : parts)
    {
        if (name.endsWithNoCase(part))
        {
            outCoreStem = name.leftPart(name.length() - part.length());
            return true;
        }
    }

    return false;
}

struct CubemapImportSetup
{
    struct Face
    {
        StaticTextureCubeCompressionFace settings;
        RefPtr<image::FreeImageLoadedData> loaded;
        RefPtr<ImageCompressedResult> compressed;
        StringBuf importPath;
    };

    InplaceArray<Face, 6> faces; // ordered as in physical resource
};

static bool FindFaces(const IResourceImporterInterface& importer, CubemapImportSetup& setup)
{
    const auto& importPath = importer.queryImportPath();

    // find the core file name (before cube face suffix)
    StringView coreFileStem;
    {
        auto importFileStem = importPath.view().fileStem();
        for (const auto& face : setup.faces)
            if (MatchSuffix(face.settings.suffix, importFileStem, coreFileStem))
                break;

        if (!coreFileStem)
        {
            TRACE_ERROR("Unable to determine cube side from file name '{}'", importPath);
            return false;
        }
    }

    // find import resources for all cube faces
    {
        const auto importBasePath = importPath.view().baseDirectory();
        const auto importExtension = importPath.view().extensions();
        for (auto& face : setup.faces)
        {
            InplaceArray<StringView, 10> parts;
            face.settings.suffix.view().slice(";", false, parts);

            for (const auto suffix : parts)
            {
                const StringBuf testPath = TempString("{}{}{}.{}", importBasePath, coreFileStem, suffix, importExtension);
                if (importer.checkSourceFile(testPath))
                {
                    face.importPath = testPath;
                    TRACE_INFO("Found cube face at '{}'", testPath);
                    break;
                }
            }

            if (!face.importPath)
            {
                TRACE_ERROR("Missing cubemap face when importing cubemap from '{}'", importPath);
                return false;
            }
        }
    }

    return true;
}

static bool DeduceCubeSize(const IResourceImporterInterface& importer, const CubemapImportSetup& setup, uint32_t& outSize)
{
    outSize = setup.faces[0].loaded->width;

    for (const auto& face : setup.faces)
    {
        if (face.loaded->width != face.loaded->height)
        {
            TRACE_ERROR("All cubemap faces should be square");
            return false;
        }

        if (face.loaded->width != outSize)
        {
            TRACE_ERROR("All cubemap faces should be of the same size");
            return false;
        }
    }

    return true;
}

static void Flip(bool& flag)
{
    flag = !flag;
}

ResourcePtr StaticCubeTextureFromImageImporter::importResource(IResourceImporterInterface& importer) const
{
    // get compression configuration
    auto config = importer.queryConfigration<StaticTextureCubeCompressionConfiguration>();

    // prepare cube setup
    CubemapImportSetup cubeSetup;
    cubeSetup.faces.resize(6);
    cubeSetup.faces[(int)ImageCubeFace::PositiveX].settings = config->m_facePX;
    cubeSetup.faces[(int)ImageCubeFace::NegativeX].settings = config->m_faceNX;
    cubeSetup.faces[(int)ImageCubeFace::PositiveY].settings = config->m_facePY;
    cubeSetup.faces[(int)ImageCubeFace::NegativeY].settings = config->m_faceNY;
    cubeSetup.faces[(int)ImageCubeFace::PositiveZ].settings = config->m_facePZ;
    cubeSetup.faces[(int)ImageCubeFace::NegativeZ].settings = config->m_faceNZ;

    // apply "space" config
    if (config->m_space == StaticTextureCubeImportSpace::SpaceYUp)
    {
        Flip(cubeSetup.faces[0].settings.flipY);
        Flip(cubeSetup.faces[0].settings.swapXY);
        Flip(cubeSetup.faces[1].settings.flipX);
        Flip(cubeSetup.faces[1].settings.swapXY);
        Flip(cubeSetup.faces[2].settings.flipX);
        Flip(cubeSetup.faces[2].settings.flipY);
        Flip(cubeSetup.faces[4].settings.flipX);
        Flip(cubeSetup.faces[4].settings.flipY);

        std::swap(cubeSetup.faces[2].settings.suffix, cubeSetup.faces[4].settings.suffix);
        std::swap(cubeSetup.faces[3].settings.suffix, cubeSetup.faces[5].settings.suffix);
    }

    // find assets
    if (!FindFaces(importer, cubeSetup))
        return nullptr;

    // load textures
    std::atomic<bool> hasErrors = false;
    RunFiberForFeach<CubemapImportSetup::Face>("LoadCubeFaces", cubeSetup.faces, 6, [&hasErrors, &importer](CubemapImportSetup::Face& face)
        {
            // load the source data into memory
            if (auto bufferData = importer.loadSourceFileContent(face.importPath))
            {
                if (auto imageData = image::LoadImageWithFreeImage(bufferData.data(), bufferData.size()))
                {
                    face.loaded = imageData;

                    if (face.settings.swapXY)
                        image::SwapXY(imageData->view());

                    if (face.settings.flipX)
                        image::FlipX(imageData->view());

                    if (face.settings.flipY)
                        image::FlipY(imageData->view());
                }
                else
                {
                    TRACE_ERROR("Unable to load image from '{}'", face.importPath);
                    hasErrors = true;
                }
            }
            else
            {
                TRACE_ERROR("Unable to load file content from '{}'", face.importPath);
                hasErrors = true;
            }
        });

    // something not loaded
    if (hasErrors)
        return nullptr;

    // check that all faces are square and of valid size
    uint32_t cubeSize = 0;
    if (!DeduceCubeSize(importer, cubeSetup, cubeSize))
        return nullptr;

    // compress all cube images
    // NOTE: no fibers here since fibers are used internally for compression, there's no point
    Array<RefPtr<ImageCompressedResult>> compressedFaces;
    for (auto& face : cubeSetup.faces)
    {
        auto settings = config->loadSettings();
        settings.m_compressDataBuffer = false; // not needed since we will repack it

        face.compressed = CompressImage(face.loaded->view(), settings, importer);
        if (!face.compressed)
        {
            TRACE_ERROR("Compression error for face from '{}'", face.importPath);
            return nullptr;
        }

        compressedFaces.pushBack(face.compressed);
    }

    // bind final data
    const auto mergedData = MergeCompressedImages(compressedFaces, ImageViewType::ViewCube, true);
    if (!mergedData)
        return nullptr;

    IStaticTexture::Setup setup;
    setup.data = std::move(mergedData->data);
    setup.mips = std::move(mergedData->mips);
    setup.info = mergedData->info;

    return RefNew<StaticTextureCube>(std::move(setup));
}

//---
    
END_BOOMER_NAMESPACE_EX(assets)
