/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "utils.h"
#include "service.h"
#include "mainWindow.h"
#include "progressDialog.h"

#include "core/xml/include/xmlUtils.h"
#include "core/xml/include/xmlWrappers.h"
#include "core/fibers/include/backgroundJob.h"
#include "core/io/include/fileFormat.h"
#include "core/resource/include/metadata.h"
#include "core/containers/include/path.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

OpenSavePersistentData XMLSaveLoadDialogSettings;

bool SaveToXML(ui::IElement* owner, StringView category, const std::function<ObjectPtr()>& makeXMLFunc, StringBuf* currentFileNamePtr)
{
    DEBUG_CHECK_RETURN_EX_V(owner, "Cannot save without window", false);

    // add XML format
    InplaceArray<FileFormat, 1> formatList;
    formatList.emplaceBack("xml", "Extensible Markup Language file");

    // current file name
    StringBuf currentFileName;
    if (currentFileNamePtr)
        currentFileName = *currentFileNamePtr;
    else if (!XMLSaveLoadDialogSettings.lastSaveFileName.empty())
        currentFileName = XMLSaveLoadDialogSettings.lastSaveFileName;

    // ask for file path
    StringBuf selectedPath;
    const auto nativeHandle = owner->windowNativeHandle();
    if (!ShowFileSaveDialog(nativeHandle, currentFileName, formatList, selectedPath, XMLSaveLoadDialogSettings))
        return false;

    // extract file name
    if (currentFileNamePtr)
        *currentFileNamePtr = StringBuf(selectedPath.view().fileName());
    else
        XMLSaveLoadDialogSettings.lastSaveFileName = StringBuf(selectedPath.view().fileName());

    // get the object to save
    const auto object = makeXMLFunc();
    if (!object)
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLSave"_id, "No object to save");
        return false;
    }

    // compile into XML document
    const auto document = SaveObjectToXML(object);
    if (!document)
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLSave"_id, TempString("Failed to serialize '{}' into XML", object));
        return false;
    }

    // save on disk
    if (!xml::SaveDocument(document, selectedPath))
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLSave"_id, TempString("Failed to save XML to '{}'", selectedPath));
        return false;
    }

    // saved
    ui::PostWindowMessage(owner, ui::MessageType::Info, "XMLSave"_id, TempString("XML '{}' saved", selectedPath));
    return true;

}

bool SaveToXML(ui::IElement* owner, StringView category, const ObjectPtr& objectPtr, StringBuf* currentFileNamePtr)
{
    return SaveToXML(owner, category, [objectPtr]() { return objectPtr;  });
}

ObjectPtr LoadFromXML(ui::IElement* owner, StringView category, SpecificClassType<IObject> expectedObjectClass)
{
    DEBUG_CHECK_RETURN_EX_V(owner, "Cannot load without window", nullptr);

    // add XML format
    InplaceArray<FileFormat, 1> formatList;
    formatList.emplaceBack("xml", "Extensible Markup Language file");

    // ask for file path
    Array<StringBuf> selectedPaths;
    const auto nativeHandle = owner->windowNativeHandle();
    if (!ShowFileOpenDialog(nativeHandle, false, formatList, selectedPaths, XMLSaveLoadDialogSettings))
        return false;

    // load the document from the file
    const auto& loadPath = selectedPaths.front();
    auto& reporter = xml::ILoadingReporter::GetDefault();
    const auto document = xml::LoadDocument(reporter, loadPath);
    if (!document)
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, TempString("Failed to load XML from '{}'", loadPath));
        return nullptr;
    }

    // check the class without actual object loading
    const auto rootNode = xml::Node(document);
    if (const auto className = rootNode.attribute("class"))
    {
        const auto classType = RTTI::GetInstance().findClass(StringID::Find(className));
        if (!classType || classType->isAbstract() || !classType->is(expectedObjectClass))
        {
            ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, TempString("Incompatible object '{}' from in XML '{}'", className, loadPath));
            return nullptr;
        }
    }
    else
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, TempString("No object serialized in XML '{}'", loadPath));
        return nullptr;
    }

    // load object
    const auto obj = LoadObjectFromXML(document);
    if (!obj)
    {
        ui::PostWindowMessage(owner, ui::MessageType::Warning, "XMLLoad"_id, TempString("Failed to deserialize XML from '{}'", loadPath));
        return nullptr;
    }

    // loaded
    ui::PostWindowMessage(owner, ui::MessageType::Info, "XMLLoad"_id, TempString("Loaded '{}' from '{}'", obj->cls()->name(), loadPath));
    return obj;
}

//--

class ProgressJobAdaptor : public IBackgroundJob
{
public:
    ProgressJobAdaptor(RefPtr<ProgressDialog> dlg, const TLongJobFunc& func)
        : m_dlg(dlg)
        , m_func(func)
    {}

    virtual void cancel() override
    {
        m_dlg->signalCanceled();
    }

    virtual void run() override
    {
        m_func(*m_dlg);
        m_dlg->signalFinished();
    }

public:
    TLongJobFunc m_func;
    RefPtr<ProgressDialog> m_dlg;
};

void RunLongAction(ui::IElement* owner, ProgressDialog* dlg, const TLongJobFunc& func, StringView title, bool canCancel)
{
    RefPtr<ProgressDialog> dlgPtr(AddRef(dlg));

    if (!dlgPtr)
        dlgPtr = RefNew<ProgressDialog>(title ? title : "Operation in progress, please wait", canCancel, false);

    auto job = RefNew<ProgressJobAdaptor>(dlgPtr, func);

    RunBackground(job, "BackgroundJob"_id);

    // TODO: wait a small amount of time - 0.1, 0.2s before showing a dialog

    dlgPtr->runModal(owner);
}

//--

END_BOOMER_NAMESPACE_EX(ed)
