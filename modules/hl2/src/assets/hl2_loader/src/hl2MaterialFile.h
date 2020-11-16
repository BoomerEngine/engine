/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"

#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/stringParser.h"
#include "base/io/include/ioFileHandle.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "base/parser/include/textParser.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"

namespace hl2
{
    namespace vmt
    {
        class MaterialParameters
        {
        public:
            MaterialParameters();

            // get parameter by name
            INLINE const base::StringBuf& param(const char* name) const
            {
                auto ret  = m_values.find(base::StringBuf(name));
                return ret ? *ret : base::StringBuf::EMPTY();
            }

            // do we have param
            INLINE bool hasParam(const char* name) const
            {
                auto ret  = m_values.find(base::StringBuf(name));
                return ret;
            }

            // set parameter
            INLINE void param(const base::StringBuf& name, const base::StringBuf& value)
            {
                if (!name.empty())
                {
                    if (value.empty())
                        m_values.remove(name);
                    else
                        m_values[name] = value;
                }
            }

            void dump(uint32_t depth, base::StringBuilder& buf) const
            {
                for (auto pair : m_values.pairs())
                {
                    buf.appendPadding(' ', depth*2);
                    buf.appendf("\"{}\" \"{}\"\n", pair.key, pair.value);
                }
            }

        private:
            base::HashMap<base::StringBuf, base::StringBuf> m_values;

        };

        struct MaterialGroup : public MaterialParameters
        {
            base::StringBuf m_name;
            base::Array<MaterialGroup> m_groups;

            base::Array<const MaterialGroup*> m_overrideGroups;

            bool parseGroup(base::parser::TextParser& p);
            void dump(uint32_t depth, base::StringBuilder& buf) const;
            void seutpOverrideGroups();

            INLINE const MaterialGroup* findSubGroup(const char* name) const
            {
                for (auto& subGroup : m_groups)
                    if (subGroup.m_name == name)
                        return &subGroup;
                return nullptr;
            }

            INLINE const base::StringBuf& param(const char* name) const
            {
                // try the override for DX9
                for (auto subGroup  : m_overrideGroups)
                {
                    auto& ret = subGroup->param(name);
                    if (!ret.empty())
                        return ret;
                }

                // get local param
                return MaterialParameters::param(name);
            }

            INLINE void param(const base::StringBuf& name, const base::StringBuf& value)
            {
                MaterialParameters::param(name, value);
            }

            INLINE int paramInt(const char* name, int defaultValue=0) const
            {
                // try the override for DX9
                for (auto subGroup  : m_overrideGroups)
                {
                    auto& ret = subGroup->param(name);
                    if (!ret.empty())
                    {
                        int val = 0;
                        if (base::MatchResult::OK == ret.view().match(val))
                            return val;
                    }
                }

                auto& ret = MaterialParameters::param(name);
                if (!ret.empty())
                {
                    int val = 0;
                    if (base::MatchResult::OK == ret.view().match(val))
                        return val;
                }

                return defaultValue;
            }

            INLINE float paramFloat(const char* name, float defaultValue=0) const
            {
                // try the override for DX9
                for (auto subGroup  : m_overrideGroups)
                {
                    auto& ret = subGroup->param(name);
                    if (!ret.empty())
                    {
                        int val = 0;
                        if (base::MatchResult::OK == ret.view().match(val))
                            return val;
                    }
                }

                auto& ret = MaterialParameters::param(name);
                if (!ret.empty())
                {
                    float val = 0;
                    if (base::MatchResult::OK == ret.view().match(val))
                        return val;
                }

                return defaultValue;
            }

            INLINE bool hasParam(const char* name) const
            {
                for (auto subGroup  : m_overrideGroups)
                    if (subGroup->hasParam(name))
                        return true;

                // get local param
                return MaterialParameters::hasParam(name);
            }

        };

        struct MaterialDocument : public MaterialGroup
        {
            bool parseDocument(base::parser::TextParser& p);
            bool createTextureParamFromPath(const base::StringBuf& name, const base::StringBuf& path, base::res::IResourceCookerInterface& cooker, base::VariantTable& outParamInfos);
            bool createTextureParam(const char* name, base::res::IResourceCookerInterface& cooker, base::VariantTable& outParamInfos);
            bool createFloatParam(const char* hname, base::res::IResourceCookerInterface& cooker, base::VariantTable& outParamInfos, float defaultValue);

            static base::UniquePtr<MaterialDocument> ParseFromData(base::res::IResourceCookerInterface& cooker, const base::Buffer& rawContent);
        };

    } // vmt
} // hl2