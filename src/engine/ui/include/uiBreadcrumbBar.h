/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\navigation #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

class BreadcrumbBar;
class BreadcrumbContainer;
class IBreadcrumb;
typedef RefPtr<IBreadcrumb> BreadcrumbPtr;

/// bread crumb supplier
/// TODO: replace with model/view
class ENGINE_UI_API IBreadcrumb : public IElement
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
    virtual void listChildren(Array<StringBuf>& outNames) const = 0;

    /// create the breadcrumb with given name
    virtual BreadcrumbPtr createChild(const StringBuf& breadcrumbName) const = 0;
};

/// simple breadcrumb bar
class ENGINE_UI_API BreadcrumbBar : public IElement
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
    bool navigate(uint32_t parentBreadcrumb, const Array<StringBuf>& childrenNames, bool activateNewBreadcrumb);

public:
    typedef RefPtr<BreadcrumbContainer> BreadcrumbContainerPtr;
    typedef Array<BreadcrumbContainerPtr> TBreadCrumbs;

    TBreadCrumbs m_breadcrumbs;
};

END_BOOMER_NAMESPACE_EX(ui)
