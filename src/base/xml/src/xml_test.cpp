/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"
#include "base/xml/include/xmlUtils.h"
#include "base/xml/include/xmlDocument.h"

using namespace base;

const char* xmlSample =
"<?xml version=\"1.0\" standalone=\"yes\"?>"
"<doc>"
"<node x=\"a\"  y=\"b\">"
"<test/>"
"</node>"
"<node x=\"a\"  y=\"b\">"
"<test/>"
"</node>"
"</doc>";

TEST(XML, LoadFromText)
{
    auto doc  = xml::LoadDocument(xml::ILoadingReporter::GetDefault(), xmlSample);
    ASSERT_TRUE(!!doc);

    auto root  = doc->root();
    ASSERT_EQ(StringBuf("doc"), doc->nodeName(root));

    auto child  = doc->nodeFirstChild(root);
    ASSERT_TRUE(child != 0);

    ASSERT_EQ(StringBuf("node"), doc->nodeName(child));

    ASSERT_EQ(doc->nodeAttributeOfDefault(child, "x"), "a");
    ASSERT_EQ(doc->nodeAttributeOfDefault(child, "y"), "b");

    //--

    child = doc->nodeSibling(child);
    ASSERT_EQ(StringBuf("node"), doc->nodeName(child));

    ASSERT_EQ(doc->nodeAttributeOfDefault(child, "x"), "a");
    ASSERT_EQ(doc->nodeAttributeOfDefault(child, "y"), "b");

    //---


    child = doc->nodeSibling(child);
    ASSERT_TRUE(child == 0);
}

TEST(XML, CreateManual)
{
    auto doc  = xml::CreateDocument("doc");

    {
        auto id  = doc->createNode(doc->root(), "node");
        ASSERT_TRUE(id != 0);

        doc->nodeAttribute(id, "x", "a");
        doc->nodeAttribute(id, "y", "b");
    }

    {
        auto id  = doc->createNode(doc->root(), "node");
        ASSERT_TRUE(id != 0);

        doc->nodeAttribute(id, "x", "a");
        doc->nodeAttribute(id, "y", "b");
    }

    ASSERT_TRUE(!!doc);

    auto root  = doc->root();
    ASSERT_EQ(StringBuf("doc"), doc->nodeName(root));

    //---

    auto child  = doc->nodeFirstChild(root);
    ASSERT_TRUE(child != 0);

    ASSERT_EQ(doc->nodeAttributeOfDefault(child, "x"), "a");
    ASSERT_EQ(doc->nodeAttributeOfDefault(child, "y"), "b");

    ASSERT_EQ(StringBuf("node"), doc->nodeName(child));

    //---

    child = doc->nodeSibling(child);
    ASSERT_EQ(StringBuf("node"), doc->nodeName(child));

    ASSERT_EQ(doc->nodeAttributeOfDefault(child, "x"), "a");
    ASSERT_EQ(doc->nodeAttributeOfDefault(child, "y"), "b");

    //---

    child = doc->nodeSibling(child);
    ASSERT_TRUE(child == 0);

    //---
}

TEST(XML, Reparse)
{
    auto docA  = xml::LoadDocument(xml::ILoadingReporter::GetDefault(), xmlSample);
    StringBuilder txtA;
    docA->saveAsText(txtA, docA->root());

    auto docB  = xml::LoadDocument(xml::ILoadingReporter::GetDefault(), txtA.c_str());
    StringBuilder txtB;
    docA->saveAsText(txtB, docB->root());

    ASSERT_EQ(std::string(txtA.c_str()), std::string(txtB.c_str()));
}


