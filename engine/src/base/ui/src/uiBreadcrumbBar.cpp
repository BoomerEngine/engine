/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\navigation #]
***/

#include "build.h"
#include "uiBreadcrumbBar.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiWindowPopup.h"

namespace ui
{

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IBreadcrumb);
        RTTI_METADATA(ElementClassNameMetadata).name("Breadcrumb");
    RTTI_END_TYPE();

    IBreadcrumb::~IBreadcrumb()
    {}

    bool IBreadcrumb::canClick() const
    {
        return true;
    }

    bool IBreadcrumb::hasChildren() const
    {
        return true;
    }

    //---

    class BreadcrumbContainer : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(BreadcrumbContainer, IElement);

    public:
        BreadcrumbContainer(BreadcrumbBar* bar, const BreadcrumbPtr& breadcrumb, uint32_t index)
            : m_bar(bar)
            , m_breadcrumb(breadcrumb)
            , m_index(index)
        {
            layoutMode(LayoutMode::Horizontal);

            {
                auto breadcrumbButton = createNamedChild<Button>("content"_id);
                breadcrumbButton->attachChild(breadcrumb);

                breadcrumbButton->bind(EVENT_CLICKED, this) = [](BreadcrumbContainer* container)
                {
                    base::Array<base::StringBuf> childPath;
                    if (container->m_breadcrumb->canClick() && container->m_breadcrumb->activate())
                    {
                        container->m_bar->navigate(container->m_index, childPath, true);
                    }
                };
            }

            if (breadcrumb->hasChildren())
            {
                auto navigationButton = createNamedChild<Button>("navigation"_id);
                navigationButton->createNamedChild<TextLabel>("BreadcrumbNavigationIcon"_id);

                navigationButton->bind(EVENT_CLICKED) = [this]()
                {
                    // collect the navigation elements
                    base::InplaceArray<base::StringBuf, 20> childrenNames;
                    m_breadcrumb->listChildren(childrenNames);

                    // if we have some children to navigate to create the menu
                    if (!childrenNames.empty())
                    {
                        auto popup = base::CreateSharedPtr<PopupWindow>();

                        /*for (const auto& childName : childrenNames)
                        {
                            popup->addItem().caption(childName.c_str()).OnClick = [selfWeakRef, childName](UI_CALLBACK)
                            {
                                auto container = selfWeakRef.lock();
                                if (container)
                                {
                                    base::Array<base::StringBuf> childPath;
                                    childPath.pushBack(childName);

                                    container->m_bar->navigate(container->m_index, childPath, true);
                                }
                            };
                        }*/

                        popup->show(this);
                    }
                };
            }
        }

        INLINE const BreadcrumbPtr& breadcrumb() const
        {
            return m_breadcrumb;
        }

    private:
        BreadcrumbBar* m_bar;
        BreadcrumbPtr m_breadcrumb;
        uint32_t m_index;
    };

    RTTI_BEGIN_TYPE_NATIVE_CLASS(BreadcrumbContainer);
        RTTI_METADATA(ElementClassNameMetadata).name("BreadcrumbContainer");
    RTTI_END_TYPE();    

    //---

    RTTI_BEGIN_TYPE_CLASS(BreadcrumbBar);
        RTTI_METADATA(ElementClassNameMetadata).name("BreadcrumbBar");
    RTTI_END_TYPE();

    BreadcrumbBar::BreadcrumbBar()
    {
        layoutMode(LayoutMode::Horizontal);
    }

    BreadcrumbBar::~BreadcrumbBar()
    {}

    void BreadcrumbBar::reset()
    {
        removeAllChildren();
        m_breadcrumbs.clear();
    }

    bool BreadcrumbBar::navigate(uint32_t parentBreadcrumb, const base::Array<base::StringBuf>& childrenNames, bool activateNewBreadcrumb)
    {
        // empty
        if (m_breadcrumbs.empty())
            return false;

        // get the starting breadcrumb
        ASSERT(parentBreadcrumb < m_breadcrumbs.size());
        auto startBreadCrumb = m_breadcrumbs[parentBreadcrumb]->breadcrumb();

        // create the breadcrumbs to add
        base::Array<BreadcrumbPtr> newBreadcrumbs;
        for (const auto& childName : childrenNames)
        {
            auto nextBreadCrumb = startBreadCrumb->createChild(childName);
            if (!nextBreadCrumb)
                return false;

            newBreadcrumbs.pushBack(nextBreadCrumb);
            startBreadCrumb = nextBreadCrumb;
        }

        // get the new breadcrumb to activate
        if (activateNewBreadcrumb)
        {
            auto newTopBreadcrumb = childrenNames.empty() ? m_breadcrumbs[parentBreadcrumb]->breadcrumb() : newBreadcrumbs.back();
            if (!newTopBreadcrumb->activate())
                return false;
        }

        // remove the existing objects
        for (uint32_t i= parentBreadcrumb+1; i<m_breadcrumbs.size(); ++i)
            detachChild(m_breadcrumbs[i]);
        m_breadcrumbs.resize(parentBreadcrumb+1);

        // add new wrapper
        for (const auto& newBreadCrumb : newBreadcrumbs)
        {
            auto ptr = base::CreateSharedPtr<BreadcrumbContainer>(this, newBreadCrumb, m_breadcrumbs.size());
            m_breadcrumbs.pushBack(ptr);
            attachChild(ptr);
        }

        return true;
    }

    void BreadcrumbBar::root(const BreadcrumbPtr& breadcrumb)
    {
        // remove all bread crumbs
        m_breadcrumbs.clear();

        // create wrapper
        if (breadcrumb)
        {
            auto ptr = base::CreateSharedPtr<BreadcrumbContainer>(this, breadcrumb, 0);
            m_breadcrumbs.pushBack(ptr);
            attachChild(ptr);
        }
    }

    //---

} // ui
