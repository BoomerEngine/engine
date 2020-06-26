/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "uiDataBox.h"

namespace ui
{
    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDataBox);
        RTTI_METADATA(ElementClassNameMetadata).name("DataBox");
    RTTI_END_TYPE();

    IDataBox::IDataBox()
        : m_readValid(false)
        , m_readOnly(false)
    {
        hitTest(HitTestState::Enabled);

        // whenever the value changes in the binding refresh the ui representation of it
        /*m_bindingObserver.OnDataChanged = [this](base::StringID)
        {
            handleValueChange(true);
        };*/
    }

    void IDataBox::bind(const base::DataProxyPtr& data, const base::StringBuf& path, bool readOnly)
    {
        if (m_observerToken)
        {
            m_data->unregisterObserver(m_observerToken);
            m_observerToken = nullptr;
        }

        m_data = data;
        m_path = path;
        m_readOnly = readOnly;
        handleValueChange();

        if (data)
            m_observerToken = m_data->registerObserver(m_path, this);
    }

    IDataBox::~IDataBox()
    {
        if (m_observerToken)
        {
            m_data->unregisterObserver(m_observerToken);
            m_observerToken = nullptr;
        }
    }

    void IDataBox::dataProxyValueChanged(base::StringView<char> fullPath, bool parentNotification)
    {
        if (m_path == fullPath)
            handleValueChange();
    }

    void IDataBox::enterEdit()
    {
        focus();
    }

    void IDataBox::cancelEdit()
    {
        handleValueChange();
    }

    void IDataBox::handleValueChange()
    {

    }

    bool IDataBox::canExpandChildren() const
    {
        return false;
    }

    //---

    bool IDataBox::readValue(void* data, const base::Type dataType)
    {
        m_readValid = m_data->read(0, m_path, data, dataType);

        // TODO: more

        return m_readValid;
    }

    bool IDataBox::writeValue(const void* data, const base::Type dataType)
    {
        bool written = true;

        for (uint32_t i = 0; i < m_data->size(); ++i)
            written &= m_data->write(i, m_path, data, dataType);

        // TODO: undo/redo ?

        return written;
    }

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDataBoxFactory);
    RTTI_END_TYPE();

    IDataBoxFactory::~IDataBoxFactory()
    {}

    //---

    class DataBoxRegistry : public base::ISingleton
    {
        DECLARE_SINGLETON(DataBoxRegistry);

    public:
        DataBoxRegistry()
        {
            base::InplaceArray<base::SpecificClassType<IDataBoxFactory>, 64> dataBoxClasses;
            RTTI::GetInstance().enumClasses(dataBoxClasses);

            m_templates.reserve(dataBoxClasses.size());
            for (const auto classPtr : dataBoxClasses)
                m_templates.pushBack(classPtr->createPointer<IDataBoxFactory>());
        }

        virtual void deinit() override
        {
            m_templates.clearPtr();
        }

        DataBoxPtr create(const base::rtti::DataViewInfo& info) const
        {
            for (const auto* ptr : m_templates)
                if (auto ret = ptr->tryCreate(info))
                    return ret;

            return nullptr;
        }

    public:
        base::Array<IDataBoxFactory*> m_templates;
    };

    DataBoxPtr IDataBox::CreateForType(const base::rtti::DataViewInfo& info)
    {
        return DataBoxRegistry::GetInstance().create(info);
    }

    //---

} // ui