/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"
#include "hl2MaterialFile.h"
#include "base/parser/include/textParser.h"
#include "base/containers/include/stringParser.h"
#include "base/reflection/include/variantTable.h"

namespace hl2
{
    namespace vmt
    {

        ///--

        MaterialParameters::MaterialParameters()
        {}

        ///--

        bool MaterialGroup::parseGroup(base::parser::TextParser& p)
        {
            // parse group until we reach the end
            while (!p.parseKeyword("}"))
            {
                // get the name
                base::StringView nameToken;
                if (!p.parseString(nameToken, false, true))
                    return false;

                // a sub group
                auto name = base::StringBuf(nameToken).toLower();
                if (!name.beginsWith("$") && !name.beginsWith("%"))
                {
                    // we should have group data
                    if (p.parseKeyword("{", false, true))
                    {
                        auto &childGroup = m_groups.emplaceBack();
                        childGroup.m_name = name;

                        if (!childGroup.parseGroup(p))
                            return false;
                        continue;
                    }
                }
                else
                {
                    name = name.subString(1);
                }

                // get value for the parameter
                base::StringView value;
                if (p.parseString(value, true, false))
                {
                    param(base::StringBuf(name), base::StringBuf(value));
                    continue;
                }

                // try to get as number
                float floatValue = 0.0f;
                if (p.parseFloat(floatValue, true, false))
                {
                    param(base::StringBuf(name), base::StringBuf(base::TempString("{}", floatValue)));
                    continue;
                }

                // problem
                return p.error(base::TempString("No value for parameter '{}'", name));
            }

            // done
            return true;
        }

        void MaterialGroup::dump(uint32_t depth, base::StringBuilder& buf) const
        {
            buf.appendPadding(' ', depth*2);
            buf.appendf("\"{}\" {\n", m_name);

            for (auto& childGroup : m_groups)
                childGroup.dump(depth+2, buf);

            MaterialParameters::dump(depth+2, buf);

            buf.appendPadding(' ', depth*2);
            buf.append("}\n");
        }

        void MaterialGroup::seutpOverrideGroups()
        {
            const char* groupNames[] = {"_hdr_dx9", "_dx9", "_dx8"};
            for (uint32_t i=0; i<ARRAY_COUNT(groupNames); ++i)
            {
                auto fullGroupName = base::TempString("{}{}", m_name, groupNames[i]);
                auto overrideGroup  = findSubGroup(fullGroupName.c_str());
                if (overrideGroup)
                    m_overrideGroups.pushBack(overrideGroup);
            }
        }

        //--

        bool MaterialDocument::parseDocument(base::parser::TextParser& p)
        {
            // get the name
            base::StringView name;
            if (!p.parseString(name, false, true))
                return false;

            // we should have group data
            if (!p.parseKeyword("{", false, true))
                return false;

            // parse a group
            m_name = base::StringBuf(name);
            if (!MaterialGroup::parseGroup(p))
                return false;

            // fixup
            seutpOverrideGroups();
            return true;
        }

        bool MaterialDocument::createTextureParamFromPath(const base::StringBuf& name, const base::StringBuf& path, base::res::IResourceCookerInterface& cooker, base::VariantTable& outParamInfos)
        {
            /*base::res::Ref<rendering::content::StaticTexture> texRef;

            // format the HL2 compliant texture path
            if (!path.empty())
            {
                base::StringBuf hl2FullPath = base::TempString("materials/{}.vtf", path.toLower());

                // get the full engine path
                base::StringBuf fullPath;
                if (cooker.queryResolvedPath(hl2FullPath, cooker.queryResourcePath().path(), false, fullPath))
                {
                    // try to load the texture
                    auto textureResource = base::LoadResource<rendering::content::StaticTexture>(fullPath);

                    // bind, use loaded resource if possible
                    if (textureResource)
                    {
                        if (auto renderObject = textureResource->renderingObject())
                        {
                            outParamInfos.set<rendering::runtime::RuntimeObjectPtr>(name.c_str(), renderObject);
                            return true;
                        }
                    }
                }
            }*/

            return false;
        }

        bool MaterialDocument::createTextureParam(const char* name, base::res::IResourceCookerInterface& cooker, base::VariantTable& outParamInfos)
        {
            // get path for the param
            auto path = param(name);
            if (path.empty())
                return false;

            // define the param
            return createTextureParamFromPath(name, path.c_str(), cooker, outParamInfos);
        }

        bool MaterialDocument::createFloatParam(const char* name, base::res::IResourceCookerInterface& cooker, base::VariantTable& outParamInfos, float defaultValue)
        {
            float value = defaultValue;

            // get path for the param
            auto textValue  = param(name).c_str();
            if (textValue && *textValue)
                base::StringView(textValue).match(value);

            outParamInfos.setValue<float>(base::StringID(name), value);
            return true;
        }

        base::UniquePtr<MaterialDocument> MaterialDocument::ParseFromData(base::res::IResourceCookerInterface& cooker, const base::Buffer& rawContent)
        {
            // get the context name
            base::StringBuf sourceFileContextName;
            base::StringBuf contextName = base::StringBuf(cooker.queryResourcePath());
            cooker.queryContextName(contextName, sourceFileContextName);

            // CRAP
            base::Array<char> text;
            text.resize(rawContent.size() + 1);
            memcpy(text.data(), rawContent.data(), rawContent.size());
            text[rawContent.size()] = 0;
            TRACE_INFO("Code for {}: '{}'", contextName, text.typedData());

            // initialize parser
            base::parser::TextParser parser(sourceFileContextName, base::parser::IErrorReporter::GetDefault(), base::parser::ICommentEater::StandardComments());
            parser.reset(rawContent);
            //parser.reset(text.typedData(), text.typedData() + strlen(text.typedData()));

            // parse material document
            auto doc = base::CreateUniquePtr<vmt::MaterialDocument>();
            if (!doc->parseDocument(parser))
                return nullptr;

            // do we have a include ?
            if (doc->m_name == "patch")
            {
                TRACE_INFO("Material '{}' is a patch material", contextName.c_str());

                // get the actual material to include
                auto includePath = doc->param("include").toLower();
                if (includePath.empty())
                {
                    TRACE_ERROR("No include specified for patch material '{}'", contextName.c_str());
                    return nullptr;
                }

                // load the include
                TRACE_INFO("Material '{}' redirects to '{}'", contextName.c_str(), includePath.c_str());

                // get the root path
                auto rootPathToDepot = cooker.queryResourcePath().view().beforeFirst("/materials/");
                if (rootPathToDepot.empty())
                    rootPathToDepot = cooker.queryResourcePath().view().beforeFirst("/maps/");

                // get the include path to load
                auto includedMaterialPath = base::StringBuf(base::TempString("{}/{}", rootPathToDepot, includePath));

                // load the include into the buffer
                auto data = cooker.loadToBuffer(includedMaterialPath);
                if (!data)
                {
                    TRACE_ERROR("Unable to load data from included source file '{}'", includedMaterialPath);
                    return nullptr;
                }

                // process the include
                return ParseFromData(cooker, data);

            }

            return doc;
        }

    } // vm
} // hl2