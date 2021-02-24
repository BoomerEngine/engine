/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\navigation #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE(ui)

class BreadcrumbBar;
class BreadcrumbContainer;
class IBreadcrumb;
typedef base::RefPtr<IBreadcrumb> BreadcrumbPtr;

/// bread crumb supplier
/// TODO: replace with model/view
class BASE_UI_API IBreadcrumb : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(IBreadcrumb, IElement);

public:
    virtual ~IBreadcrumb();

    /// can we activate this breadcrumb with click ?
    virtual bool canClick() const;

    /// is this a terminal breadcrumb (no children)
    virtual bool hasChildren() const;

    /// handle activation of the breadcrumb
    /// NOTE: should return true if the navigation is accepted or false if it's not accepted
    virtual bool activate() = 0;

    /// get list of possible child breadcrumbs
    virtual void listChildren(base::Array<base::StringBuf>& outNames) const = 0;

    /// create the breadcrumb with given name
    virtual BreadcrumbPtr createChild(const base::StringBuf& breadcrumbName) const = 0;
};

/// simple breadcrumb bar
class BASE_UI_API BreadcrumbBar : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(BreadcrumbBar, IElement);

public:
    BreadcrumbBar();
    virtual ~BreadcrumbBar();

    /// reset content
    void reset();

    /// set the root breadcrumb
    void root(const BreadcrumbPtr& breadcrumb);

    /// navigate to a given child in n-th bread crumb
    bool navigate(uint32_t parentBreadcrumb, const base::Array<base::StringBuf>& childrenNames, bool activateNewBreadcrumb);

public:
    typedef base::RefPtr<BreadcrumbContainer> BreadcrumbContainerPtr;
    typedef base::Array<BreadcrumbContainerPtr> TBreadCrumbs;

    TBreadCrumbs m_breadcrumbs;
};

END_BOOMER_NAMESPACE(ui)