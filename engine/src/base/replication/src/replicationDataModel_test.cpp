/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "replicationBitWriter.h"
#include "replicationBitReader.h"
#include "replicationDataModel.h"
#include "replicationDataModelRepository.h"
#include "replicationRttiExtensions.h"

#include "build.h"
#include "base/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(DataModelTest);

using namespace base;
using namespace base::replication;

namespace test
{
    //---

    class ReplicationTestObject : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ReplicationTestObject, IObject);

    public:
        ReplicationTestObject()
        {}
    };

    RTTI_BEGIN_TYPE_CLASS(ReplicationTestObject);
    RTTI_END_TYPE();

    struct TestReplicatedStruct_Simple
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_Simple);

    public:
        bool m_bit = false;
        uint8_t m_unsigned = 0;
        short m_signed = 0;
        float m_float = 0;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_Simple);
        RTTI_PROPERTY(m_bit).metadata<replication::SetupMetadata>("b");
        RTTI_PROPERTY(m_unsigned).metadata<replication::SetupMetadata>("u:6");
        RTTI_PROPERTY(m_signed).metadata<replication::SetupMetadata>("s:12");
        RTTI_PROPERTY(m_float).metadata<replication::SetupMetadata>("f:10,-1,1");
    RTTI_END_TYPE();

    //---

    struct TestReplicatedStruct_InnerStructPacked
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_InnerStructPacked);

    public:
        TestReplicatedStruct_Simple m_struct;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_InnerStructPacked);
        RTTI_PROPERTY(m_struct).metadata<replication::SetupMetadata>("");
    RTTI_END_TYPE();

    //---

    struct TestReplicatedStruct_String
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_String);

    public:
        StringBuf m_str;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_String);
        RTTI_PROPERTY(m_str).metadata<replication::SetupMetadata>();
    RTTI_END_TYPE();

    //---

    struct TestReplicatedStruct_StringLimited
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_StringLimited);

    public:
        StringBuf m_str;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_StringLimited);
        RTTI_PROPERTY(m_str).metadata<replication::SetupMetadata>("maxLength:10");
    RTTI_END_TYPE();

    //---

    struct TestReplicatedStruct_StringID
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_StringID);

    public:
        StringID m_name;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_StringID);
        RTTI_PROPERTY(m_name).metadata<replication::SetupMetadata>();
    RTTI_END_TYPE();

    //---

    struct TestReplicatedStruct_TypeRef
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_TypeRef);

    public:
        SpecificClassType<IObject> m_type;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_TypeRef);
        RTTI_PROPERTY(m_type).metadata<replication::SetupMetadata>();
    RTTI_END_TYPE();

    //---

    /*struct TestReplicatedStruct_ResRef
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_ResRef);

    public:
        res::Ref<base::res::IResource> m_path;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_ResRef);
        RTTI_PROPERTY(m_path).metadata<replication::SetupMetadata>();
    RTTI_END_TYPE();*/

    //---

    struct TestReplicatedStruct_ObjectPtr
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_ObjectPtr);

    public:
        ObjectPtr m_obj;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_ObjectPtr);
        RTTI_PROPERTY(m_obj).metadata<replication::SetupMetadata>();
    RTTI_END_TYPE();

    //---

    struct TestReplicatedStruct_WeakObjectPtr
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_WeakObjectPtr);

    public:
        ObjectWeakPtr m_obj;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_WeakObjectPtr);
        RTTI_PROPERTY(m_obj).metadata<replication::SetupMetadata>();
    RTTI_END_TYPE();

    //---

    struct TestVector3
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestVector3);

    public:
        float x,y,z;

        TestVector3()
            : x(0), y(0), z(0)
        {}

        float squareLength() const
        {
            return x*x + y*y + z*z;
        }

        float dot(const TestVector3& v) const
        {
            return (x*v.x) + (y*v.y) + (z*v.z);
        }

        void normalize()
        {
            auto len = squareLength();
            if (len > 0.000001f)
            {
                len = 1.0f / std::sqrt(len);
                x *= len;
                y *= len;
                z *= len;
            }
        }
    };

    RTTI_BEGIN_TYPE_STRUCT(TestVector3);
        RTTI_PROPERTY(x).metadata<replication::SetupMetadata>();
        RTTI_PROPERTY(y).metadata<replication::SetupMetadata>();
        RTTI_PROPERTY(z).metadata<replication::SetupMetadata>();
    RTTI_END_TYPE();

    //--

    struct TestReplicatedStruct_InnerStruct
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_InnerStruct);

    public:
        TestVector3 m_pos;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_InnerStruct);
        RTTI_PROPERTY(m_pos).metadata<replication::SetupMetadata>();
    RTTI_END_TYPE();

    //--

    struct TestReplicatedStruct_Pos
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_Pos);

    public:
        TestVector3 m_pos;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_Pos);
        RTTI_PROPERTY(m_pos).metadata<replication::SetupMetadata>("pos");
    RTTI_END_TYPE();

    //--

    struct TestReplicatedStruct_DeltaPos
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_DeltaPos);

    public:
        TestVector3 m_pos;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_DeltaPos);
        RTTI_PROPERTY(m_pos).metadata<replication::SetupMetadata>("delta,10");
    RTTI_END_TYPE();

    //--

    struct TestReplicatedStruct_Normal
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_Normal);

    public:
        TestVector3 m_pos;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_Normal);
        RTTI_PROPERTY(m_pos).metadata<replication::SetupMetadata>("normal");
    RTTI_END_TYPE();

    //--

    struct TestReplicatedStruct_Dir
    {
    RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_Dir);

    public:
        TestVector3 m_pos;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_Dir);
        RTTI_PROPERTY(m_pos).metadata<replication::SetupMetadata>("dir");
    RTTI_END_TYPE();

    //--

    struct TestReplicatedStruct_PitchYaw
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_PitchYaw);

    public:
        TestVector3 m_pos;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_PitchYaw);
        RTTI_PROPERTY(m_pos).metadata<replication::SetupMetadata>("pitchYaw");
    RTTI_END_TYPE();

    //--

    struct TestReplicatedStruct_Angles
    {
    RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_Angles);

    public:
        TestVector3 m_pos;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_Angles);
        RTTI_PROPERTY(m_pos).metadata<replication::SetupMetadata>("angles");
    RTTI_END_TYPE();

    //--

    struct TestReplicatedStruct_Arrays
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_Arrays);

    public:
        Array<bool> m_bools;
        Array<float> m_floats;
        Array<StringBuf> m_strings;
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_Arrays);
        RTTI_PROPERTY(m_bools).metadata<replication::SetupMetadata>("b,maxCount:10");
        RTTI_PROPERTY(m_floats).metadata<replication::SetupMetadata>("f:10,-10,10,maxCount:10");
        RTTI_PROPERTY(m_strings).metadata<replication::SetupMetadata>("maxCount:5");
    RTTI_END_TYPE();

    //--

    struct TestReplicatedStruct_TreeNode
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TestReplicatedStruct_TreeNode);

    public:
        bool m_used = false;
        float m_value = 0.0f;
        Array<TestReplicatedStruct_TreeNode> m_children;

        void insert(float value)
        {
            if (!m_used)
            {
                m_value = value;
                m_children.resize(2);
                m_used = true;
                return;
            }

            if (value < m_value)
                m_children[0].insert(value);
            if (value > m_value)
                m_children[1].insert(value);
        }

        void dump(Array<float>& outArray) const
        {
            if (m_used)
            {
                m_children[0].dump(outArray);
                outArray.pushBack(m_value);
                m_children[1].dump(outArray);
            }
			else
			{
				DEBUG_CHECK(m_children.empty());
				DEBUG_CHECK(m_value == 0.0f);
			}
        }
    };

    RTTI_BEGIN_TYPE_STRUCT(TestReplicatedStruct_TreeNode);
        RTTI_PROPERTY(m_used).metadata<replication::SetupMetadata>("b");
        RTTI_PROPERTY(m_value).metadata<replication::SetupMetadata>("f:10,0,1");
        RTTI_PROPERTY(m_children).metadata<replication::SetupMetadata>("maxCount:2");
    RTTI_END_TYPE();

    //---

    class LocalKnowledgeBase : public IDataModelResolver, public IDataModelMapper
    {
    public:
        LocalKnowledgeBase()
        {
            clear();
        }

        void clear()
        {
            m_nextObjectID = 1;
            m_stringMap.clear();
            m_strings.clear();
            m_pathMap.clear();
            m_paths.clear();
            m_objectMap.clear();
            m_objectReverseMap.clear();
            m_strings.pushBack("");
            m_paths.pushBack("");
        }

        virtual DataMappedID mapString(StringView<char> txt) override final
        {
            if (txt.empty())
                return 0;

            DataMappedID id = 0;
            StringBuf str(txt);
            if (m_stringMap.find(str, id))
                return id;

            id = (DataMappedID)m_strings.size();
            m_strings.pushBack(str);
            m_stringMap[str] = id;
            return id;
        }

        virtual DataMappedID mapPath(StringView<char> path, const char* pathSeparators) override final
        {
            if (path.empty())
                return 0;

            DataMappedID id = 0;
            StringBuf str(path);
            if (m_pathMap.find(str, id))
                return id;

            id = (DataMappedID)m_paths.size();
            m_paths.pushBack(str);
            m_pathMap[str] = id;
            return id;
        }

        virtual DataMappedID mapObject(const IObject* obj) override final
        {
            DataMappedID id = 0;
            m_objectMap.find(obj, id);
            return id;
        }

        virtual bool resolveString(DataMappedID id, IFormatStream& ret) override final
        {
            if (id && id < m_strings.size())
                ret << m_strings[id];
            return true;
        }

        virtual bool resolvePath(DataMappedID id, const char* pathSeparator, IFormatStream& ret) override final
        {
            if (id && id < m_paths.size())
                ret << m_paths[id];
            return true;
        }

        virtual bool resolveObject(DataMappedID id, ObjectPtr& outPtr) override final
        {
            if (!id)
            {
                outPtr = ObjectPtr();
                return true;
            }

            const IObject* ptr = nullptr;
            m_objectReverseMap.find(id, ptr);
            outPtr = AddRef(ptr);
            return true;
        }

        //--

        HashMap<StringBuf, DataMappedID> m_stringMap;
        Array<StringBuf> m_strings;

        HashMap<StringBuf, DataMappedID> m_pathMap;
        Array<StringBuf> m_paths;

        HashMap<const IObject*, DataMappedID> m_objectMap;
        HashMap<DataMappedID, const IObject*> m_objectReverseMap;

        DataMappedID m_nextObjectID;

        DataMappedID allocObjectID()
        {
            return m_nextObjectID++;
        }

        void attachObject(DataMappedID id, ObjectPtr ptr)
        {
            m_objectMap[ptr.get()] = id;
            m_objectReverseMap[id] = ptr.get();
        }

        void detachObject(DataMappedID id)
        {
            const IObject* obj = nullptr;
            if (m_objectReverseMap.find(id, obj))
            {
                m_objectMap.remove(obj);
                m_objectReverseMap.remove(id);
            }
        }
    };

    struct TransferTest
    {
        TransferTest(test::LocalKnowledgeBase& kb)
            : m_knowledge(kb)
        {}

        template< typename T >
        void transfer(const T& input, T& output)
        {
            // create data model
            DataModelRepository rep;
            auto model  = rep.buildModelForType(T::GetStaticClass());
            ASSERT_TRUE(model);

            // encode
            BitWriter w;
            model->encodeFromNativeData(&input, m_knowledge, w);

            // decode
            BitReader r(w.data(), w.bitSize());
            model->decodeToNativeData(&output, m_knowledge, r);

            ASSERT_EQ(r.bitPos(), w.bitSize());
        }

        test::LocalKnowledgeBase& m_knowledge;
    };

} // test

TEST(DataModel, CompiledSimple)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_Simple::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(4, model->fields().size());

    ASSERT_EQ(DataModelFieldType::Packed, model->fields()[0].m_type);
    ASSERT_EQ(DataModelFieldType::Packed, model->fields()[1].m_type);
    ASSERT_EQ(DataModelFieldType::Packed, model->fields()[2].m_type);
    ASSERT_EQ(DataModelFieldType::Packed, model->fields()[3].m_type);
}

TEST(DataModel, CompileString)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_String::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::StringBuf, model->fields()[0].m_type);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

TEST(DataModel, CompileStringLimited)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_StringLimited::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::StringBuf, model->fields()[0].m_type);
    ASSERT_EQ(10, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

TEST(DataModel, CompileStringID)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_StringID::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::StringID, model->fields()[0].m_type);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

TEST(DataModel, CompileTypeRef)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_TypeRef::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::TypeRef, model->fields()[0].m_type);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

/*TEST(DataModel, CompileResourceRef)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_ResRef::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::ResourceRef, model->fields()[0].m_type);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}
*/

TEST(DataModel, CompileObjectPtr)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_ObjectPtr::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::ObjectPtr, model->fields()[0].m_type);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

TEST(DataModel, CompileWeakObjectPtr)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_WeakObjectPtr::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::WeakObjectPtr, model->fields()[0].m_type);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

TEST(DataModel, CompileInnerStruct)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_InnerStruct::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::Struct, model->fields()[0].m_type);

    auto innerModel  = rep.buildModelForType(test::TestVector3::GetStaticClass());
    ASSERT_EQ(innerModel, model->fields()[0].m_structModel);
    ASSERT_EQ(PackingMode::Default, model->fields()[0].m_packing.m_mode);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

TEST(DataModel, CompilePosStruct)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_Pos::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::Packed, model->fields()[0].m_type);
    ASSERT_EQ(PackingMode::Position, model->fields()[0].m_packing.m_mode);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

TEST(DataModel, CompileDeltaPosStruct)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_DeltaPos::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::Packed, model->fields()[0].m_type);
    ASSERT_EQ(PackingMode::DeltaPosition, model->fields()[0].m_packing.m_mode);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

TEST(DataModel, CompileNormalStruct)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_Normal::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::Packed, model->fields()[0].m_type);
    ASSERT_EQ(PackingMode::NormalFull, model->fields()[0].m_packing.m_mode);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

TEST(DataModel, CompileDirStruct)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_Dir::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::Packed, model->fields()[0].m_type);
    ASSERT_EQ(PackingMode::NormalRough, model->fields()[0].m_packing.m_mode);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

TEST(DataModel, CompilePitchYawStruct)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_PitchYaw::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::Packed, model->fields()[0].m_type);
    ASSERT_EQ(PackingMode::PitchYaw, model->fields()[0].m_packing.m_mode);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

TEST(DataModel, CompileAnglesStruct)
{
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_Angles::GetStaticClass());
    ASSERT_TRUE(model);
    ASSERT_EQ(1, model->fields().size());
    ASSERT_EQ(DataModelFieldType::Packed, model->fields()[0].m_type);
    ASSERT_EQ(PackingMode::AllAngles, model->fields()[0].m_packing.m_mode);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxLength);
    ASSERT_EQ(0, model->fields()[0].m_packing.m_maxCount);
}

//--

TEST(DataModel, EncodeSimple)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_Simple::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_Simple s;

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    uint32_t expectedDataSize = 0;
    for (auto& field : model->fields())
        expectedDataSize += field.m_packing.calcBitCount();

    ASSERT_EQ(expectedDataSize, w.bitSize());
}

TEST(DataModel, EncodeWritesEmptyString)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_String::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_String s;
    s.m_str = "";

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = 1; // just the empty string marker
    ASSERT_EQ(expectedBits, w.bitSize());
}

TEST(DataModel, EncodeWritesUnlimitedString)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_String::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_String s;
    s.m_str = "Ala ma kota! This string should not be cut!";

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = 16 + s.m_str.length() * 8;
    ASSERT_EQ(expectedBits, w.bitSize());
}

TEST(DataModel, EncodeWritesLimitedString)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_StringLimited::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_StringLimited s;
    s.m_str = "Ala ma kota! This string should be cut!";

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = 8 + 10 * 8;
    ASSERT_EQ(expectedBits, w.bitSize());
}

TEST(DataModel, EncodeWritesStringIDEmpty)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_StringID::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_StringID s;
    s.m_name = StringID();

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = 5;
    ASSERT_EQ(expectedBits, w.bitSize());

    ASSERT_EQ(1, knowledge.m_paths.size());
    ASSERT_EQ(1, knowledge.m_strings.size());
}

TEST(DataModel, EncodeWritesStringIDIntoKnowledgeDBase)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_StringID::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_StringID s;
    s.m_name = StringID("Ala ma kota");

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = 5;
    ASSERT_EQ(expectedBits, w.bitSize());

    ASSERT_EQ(1, knowledge.m_paths.size());
    ASSERT_EQ(2, knowledge.m_strings.size());
    ASSERT_STREQ("Ala ma kota", knowledge.m_strings[1].c_str());
}

TEST(DataModel, EncodeWritesStringIDIntoKnowledgeDBase_OnlyOnce)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_StringID::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_StringID s;
    s.m_name = StringID("Ala ma kota");

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = 10;
    ASSERT_EQ(expectedBits, w.bitSize());

    ASSERT_EQ(1, knowledge.m_paths.size());
    ASSERT_EQ(2, knowledge.m_strings.size());
    ASSERT_STREQ("Ala ma kota", knowledge.m_strings[1].c_str());
}

//--

TEST(DataModel, EncodeWritesTypeRefEmpty)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_TypeRef::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_TypeRef s;

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    ASSERT_EQ(1, knowledge.m_strings.size());
    ASSERT_EQ(1, knowledge.m_paths.size());
}

TEST(DataModel, EncodeWritesTypeRefOnce)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_TypeRef::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_TypeRef s;
    s.m_type = IObject::GetStaticClass();

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    ASSERT_EQ(1, knowledge.m_strings.size());
    ASSERT_EQ(2, knowledge.m_paths.size());
    ASSERT_STREQ("base::IObject", knowledge.m_paths[1].c_str());
}

TEST(DataModel, EncodeWritesTypeRefOnlyOnce)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_TypeRef::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_TypeRef s;
    s.m_type = IObject::GetStaticClass();

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);
    model->encodeFromNativeData(&s, knowledge, w);

    ASSERT_EQ(1, knowledge.m_strings.size());
    ASSERT_EQ(2, knowledge.m_paths.size());
    ASSERT_STREQ("base::IObject", knowledge.m_paths[1].c_str());
}

//--

/*TEST(DataModel, EncodeWritesPathEmpty)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_ResRef::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_ResRef s;

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    ASSERT_EQ(1, knowledge.m_strings.size());
    ASSERT_EQ(1, knowledge.m_paths.size());
}

TEST(DataModel, EncodeWritesPathWhenSet)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_ResRef::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_ResRef s;
    //s.m_path.set(res::ResourceKey("dupa.txt", res::ITextResource::GetStaticClass()));

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    ASSERT_EQ(1, knowledge.m_strings.size());
    ASSERT_EQ(3, knowledge.m_paths.size());
    ASSERT_STREQ("base::res::ITextResource", knowledge.m_paths[1].c_str());
    ASSERT_STREQ("dupa.txt", knowledge.m_paths[2].c_str());
}

TEST(DataModel, EncodeWritesPathWhenSetOnlyOnce)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_ResRef::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_ResRef s;
    //s.m_path.set(res::ResourceKey("dupa.txt", res::ITextResource::GetStaticClass()));

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);
    model->encodeFromNativeData(&s, knowledge, w);

    ASSERT_EQ(1, knowledge.m_strings.size());
    ASSERT_EQ(3, knowledge.m_paths.size());
    ASSERT_STREQ("base::res::ITextResource", knowledge.m_paths[1].c_str());
    ASSERT_STREQ("dupa.txt", knowledge.m_paths[2].c_str());
}
*/

TEST(DataModel, EncodeWritesObjectId)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_ObjectPtr::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_ObjectPtr s;

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    ASSERT_EQ(1, knowledge.m_strings.size());
    ASSERT_EQ(1, knowledge.m_paths.size());
}

TEST(DataModel, EncodeWritesWeakObjectId)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_WeakObjectPtr::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_WeakObjectPtr s;

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    ASSERT_EQ(1, knowledge.m_strings.size());
    ASSERT_EQ(1, knowledge.m_paths.size());
}

TEST(DataModel, EncodeWritesPos)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_Pos::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_Pos s;
    s.m_pos.x = 1.0f;
    s.m_pos.y = 2.0f;
    s.m_pos.z = 3.0f;

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = FieldPacking::POSITION_X_BITS + FieldPacking::POSITION_Y_BITS + FieldPacking::POSITION_Z_BITS;
    ASSERT_EQ(expectedBits, w.bitSize());
}

TEST(DataModel, EncodeWritesDeltaPos)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_DeltaPos::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_DeltaPos s;
    s.m_pos.x = 1.0f;
    s.m_pos.y = 2.0f;
    s.m_pos.z = 3.0f;

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = model->fields()[0].m_packing.calcBitCount();
    ASSERT_EQ(expectedBits, w.bitSize());
}

TEST(DataModel, EncodeWritesNormal)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_Normal::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_Normal s;
    s.m_pos.x = 1.0f;
    s.m_pos.y = 2.0f;
    s.m_pos.z = 3.0f;

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = model->fields()[0].m_packing.calcBitCount();
    ASSERT_EQ(expectedBits, w.bitSize());
}

TEST(DataModel, EncodeWritesDir)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_Dir::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_Dir s;
    s.m_pos.x = 1.0f;
    s.m_pos.y = 2.0f;
    s.m_pos.z = 3.0f;

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = model->fields()[0].m_packing.calcBitCount();
    ASSERT_EQ(expectedBits, w.bitSize());
}

TEST(DataModel, EncodePitchYaw)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_PitchYaw::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_PitchYaw s;
    s.m_pos.x = 1.0f;
    s.m_pos.y = 2.0f;
    s.m_pos.z = 3.0f;

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = model->fields()[0].m_packing.calcBitCount();
    ASSERT_EQ(expectedBits, w.bitSize());
}

TEST(DataModel, EncodeAngles)
{
    test::LocalKnowledgeBase knowledge;
    DataModelRepository rep;

    auto model  = rep.buildModelForType(test::TestReplicatedStruct_Angles::GetStaticClass());
    ASSERT_TRUE(model);

    test::TestReplicatedStruct_Angles s;
    s.m_pos.x = 1.0f;
    s.m_pos.y = 2.0f;
    s.m_pos.z = 3.0f;

    BitWriter w;
    model->encodeFromNativeData(&s, knowledge, w);

    auto expectedBits = model->fields()[0].m_packing.calcBitCount();
    ASSERT_EQ(expectedBits, w.bitSize());
}

//--

TEST(DataModelTransmit, SimpleFalse)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Simple s, out;
    transfer.transfer(s, out);

    ASSERT_EQ(false, out.m_bit);
}

TEST(DataModelTransmit, SimpleTrue)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Simple s, out;
    s.m_bit = true;
    transfer.transfer(s, out);

    ASSERT_EQ(true, out.m_bit);
}

TEST(DataModelTransmit, SimpleUnsignedInRange)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Simple s, out;
    s.m_unsigned = 10;
    transfer.transfer(s, out);

    ASSERT_EQ(s.m_unsigned, out.m_unsigned);
}

TEST(DataModelTransmit, SimpleUnsignedClampToRange)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Simple s, out;
    s.m_unsigned = 255;
    transfer.transfer(s, out);

    ASSERT_EQ(63, out.m_unsigned);
}

TEST(DataModelTransmit, SimpleSignedInRangePositive)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Simple s, out;
    s.m_signed = 100;
    transfer.transfer(s, out);

    ASSERT_EQ(s.m_signed, out.m_signed);
}

TEST(DataModelTransmit, SimpleSignedInRangeNegative)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Simple s, out;
    s.m_signed = -100;
    transfer.transfer(s, out);

    ASSERT_EQ(s.m_signed, out.m_signed);
}

TEST(DataModelTransmit, SimpleSignedClampedToRangeMin)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Simple s, out;
    s.m_signed = -4000;
    transfer.transfer(s, out);

    ASSERT_EQ(-2048, out.m_signed);
}

TEST(DataModelTransmit, SimpleSignedClampedToRangeMax)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Simple s, out;
    s.m_signed = 4000;
    transfer.transfer(s, out);

    ASSERT_EQ(2047, out.m_signed);
}

TEST(DataModelTransmit, SimpleFloatInRange)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Simple s, out;
    s.m_float = 0.0f;
    transfer.transfer(s, out);

    ASSERT_NEAR(s.m_float, out.m_float, 2.0f / 1024.0f);
}

TEST(DataModelTransmit, SimpleFloatClampedToRangeMin)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Simple s, out;
    s.m_float = -2.0f;
    transfer.transfer(s, out);

    ASSERT_NEAR(-1.0f, out.m_float, 2.0f / 1024.0f);
}

TEST(DataModelTransmit, SimpleFloatClampedToRangeMax)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Simple s, out;
    s.m_float = 2.0f;
    transfer.transfer(s, out);

    ASSERT_NEAR(1.0f, out.m_float, 2.0f / 1024.0f);
}

TEST(DataModelTransmit, StringEmpty)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_String s, out;
    transfer.transfer(s, out);

    ASSERT_TRUE(out.m_str.empty());
}

TEST(DataModelTransmit, StringUnlimited)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_String s, out;
    s.m_str = "This string should not be cut by the replication, even though it's very long";
    transfer.transfer(s, out);

    ASSERT_STREQ(s.m_str.c_str(), out.m_str.c_str());
}

TEST(DataModelTransmit, StringLimited)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_StringLimited s, out;
    s.m_str = "This string should be cut";
    transfer.transfer(s, out);

    ASSERT_STREQ("This strin", out.m_str.c_str());
}

TEST(DataModelTransmit, StringIDEmpty)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_StringID s, out;
    transfer.transfer(s, out);

    ASSERT_TRUE(out.m_name.empty());
}

TEST(DataModelTransmit, StringIDValue)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_StringID s, out;
    s.m_name = "StringIDValue"_id;
    transfer.transfer(s, out);

    ASSERT_STREQ(s.m_name.c_str(), out.m_name.c_str());
}

TEST(DataModelTransmit, FullStruct)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_InnerStruct s, out;
    s.m_pos.x = 3.1415926535f;
    s.m_pos.y = 2.7182818284f;
    s.m_pos.z = 1.41421356237f;

    transfer.transfer(s, out);

    EXPECT_EQ(s.m_pos.x, out.m_pos.x);
    EXPECT_EQ(s.m_pos.y, out.m_pos.y);
    EXPECT_EQ(s.m_pos.z, out.m_pos.z);
}

TEST(DataModelTransmit, PackedStruct)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_InnerStructPacked s, out;
    s.m_struct.m_bit = true;
    s.m_struct.m_signed = 666;
    s.m_struct.m_unsigned = 42;
    s.m_struct.m_float = 0.0f;

    transfer.transfer(s, out);

    EXPECT_EQ(s.m_struct.m_bit, out.m_struct.m_bit);
    EXPECT_EQ(s.m_struct.m_signed, out.m_struct.m_signed);
    EXPECT_EQ(s.m_struct.m_unsigned, out.m_struct.m_unsigned);
    EXPECT_NEAR(s.m_struct.m_float, out.m_struct.m_float, 1.0f / 255.0f);
}

TEST(DataModelTransmit, Position)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Pos s, out;
    s.m_pos.x = 100.0f;
    s.m_pos.y = 200.0f;
    s.m_pos.z = 300.0f;

    transfer.transfer(s, out);

    EXPECT_NEAR(s.m_pos.x, out.m_pos.x, FieldPacking::POSITION_X_QUANTIZATION.quantizationError());
    EXPECT_NEAR(s.m_pos.y, out.m_pos.y, FieldPacking::POSITION_Y_QUANTIZATION.quantizationError());
    EXPECT_NEAR(s.m_pos.z, out.m_pos.z, FieldPacking::POSITION_Z_QUANTIZATION.quantizationError());
}

TEST(DataModelTransmit, PositionClamp)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Pos s, out;
    s.m_pos.x = FieldPacking::POSITION_XY_RANGE + 100.0f;
    s.m_pos.y = -FieldPacking::POSITION_XY_RANGE - 100.0f;
    s.m_pos.z = FieldPacking::POSITION_Z_RANGE + 100.0f;

    transfer.transfer(s, out);

    EXPECT_NEAR(out.m_pos.x, FieldPacking::POSITION_XY_RANGE, FieldPacking::POSITION_X_QUANTIZATION.quantizationError());
    EXPECT_NEAR(out.m_pos.y, -FieldPacking::POSITION_XY_RANGE, FieldPacking::POSITION_Y_QUANTIZATION.quantizationError());
    EXPECT_NEAR(out.m_pos.z, FieldPacking::POSITION_Z_RANGE, FieldPacking::POSITION_Z_QUANTIZATION.quantizationError());
}

TEST(DataModelTransmit, DeltaPosition)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_DeltaPos s, out;
    s.m_pos.x = 1.0f;
    s.m_pos.y = 2.0f;
    s.m_pos.z = 3.0f;

    transfer.transfer(s, out);

    auto packing = Quantization(FieldPacking::DELTA_POSITION_BITS, -10.0f, 10.0f);
    EXPECT_NEAR(s.m_pos.x, out.m_pos.x, packing.quantizationError());
    EXPECT_NEAR(s.m_pos.y, out.m_pos.y, packing.quantizationError());
    EXPECT_NEAR(s.m_pos.z, out.m_pos.z, packing.quantizationError());
}

TEST(DataModelTransmit, DeltaPositionClamped)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_DeltaPos s, out;
    s.m_pos.x = 20.0f;
    s.m_pos.y = -20.0f;
    s.m_pos.z = std::numeric_limits<float>::infinity();

    transfer.transfer(s, out);

    auto packing = Quantization(FieldPacking::DELTA_POSITION_BITS, -10.0f, 10.0f);
    EXPECT_NEAR(out.m_pos.x, 10.0f, packing.quantizationError());
    EXPECT_NEAR(out.m_pos.y, -10.0f, packing.quantizationError());
    EXPECT_NEAR(out.m_pos.z, 10.0f, packing.quantizationError());
}

TEST(DataModelTransmit, Normal)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Normal s, out;
    s.m_pos.x = 1.0f;
    s.m_pos.y = 1.0f;
    s.m_pos.z = 1.0f;

    transfer.transfer(s, out);

    auto packing = FieldPacking::NORMAL_X_QUANTIZATION;
    auto normalError = packing.quantizationError() * packing.quantizationError();

    s.m_pos.normalize();

    EXPECT_NEAR(out.m_pos.squareLength(), 1.0f, normalError);
    EXPECT_NEAR(out.m_pos.dot(s.m_pos), 1.0f, normalError);
}

TEST(DataModelTransmit, RoughNormal)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Dir s, out;
    s.m_pos.x = 1.0f;
    s.m_pos.y = 1.0f;
    s.m_pos.z = 1.0f;

    transfer.transfer(s, out);

    auto packing = FieldPacking::FAST_NORMAL_X_QUANTIZATION;
    auto normalError = packing.quantizationError() * packing.quantizationError();

    s.m_pos.normalize();

    EXPECT_NEAR(out.m_pos.squareLength(), 1.0f, normalError);
    EXPECT_NEAR(out.m_pos.dot(s.m_pos), 1.0f, normalError);
}

TEST(DataModelTransmit, PitchYawNormal)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_PitchYaw s, out;
    s.m_pos.x = 100.0f;
    s.m_pos.y = 200.0f;

    transfer.transfer(s, out);

    EXPECT_NEAR(out.m_pos.x, s.m_pos.x, FieldPacking::FULL_PITCH_QUANTIZATION.quantizationError());
    EXPECT_NEAR(out.m_pos.y, s.m_pos.y, FieldPacking::FULL_YAW_QUANTIZATION.quantizationError());
}

TEST(DataModelTransmit, PitchYawWraps)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_PitchYaw s, out;
    s.m_pos.x = 100.0f + 360.0f;
    s.m_pos.y = 200.0f - 360.0f;

    transfer.transfer(s, out);

    EXPECT_NEAR(out.m_pos.x, 100.0f, FieldPacking::FULL_PITCH_QUANTIZATION.quantizationError());
    EXPECT_NEAR(out.m_pos.y, 200.0f, FieldPacking::FULL_YAW_QUANTIZATION.quantizationError());
}

TEST(DataModelTransmit, AnglesNormal)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Angles s, out;
    s.m_pos.x = 100.0f;
    s.m_pos.y = 200.0f;
    s.m_pos.z = 300.0f;

    transfer.transfer(s, out);

    EXPECT_NEAR(out.m_pos.x, s.m_pos.x, FieldPacking::SMALL_PITCH_QUANTIZATION.quantizationError());
    EXPECT_NEAR(out.m_pos.y, s.m_pos.y, FieldPacking::SMALL_YAW_QUANTIZATION.quantizationError());
    EXPECT_NEAR(out.m_pos.z, s.m_pos.z, FieldPacking::SMALL_ROLL_QUANTIZATION.quantizationError());
}

TEST(DataModelTransmit, AnglesNormalWraps)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Angles s, out;
    s.m_pos.x = 100.0f + 360.0f;
    s.m_pos.y = 200.0f - 360.0f;
    s.m_pos.z = 300.0f + 3600.0f;

    transfer.transfer(s, out);

    EXPECT_NEAR(out.m_pos.x, 100.0f, FieldPacking::SMALL_PITCH_QUANTIZATION.quantizationError());
    EXPECT_NEAR(out.m_pos.y, 200.0f, FieldPacking::SMALL_YAW_QUANTIZATION.quantizationError());
    EXPECT_NEAR(out.m_pos.z, 300.0f, FieldPacking::SMALL_ROLL_QUANTIZATION.quantizationError());
}

//--

TEST(DataModelTransmit, TypeRefEmpty)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_TypeRef s, out;

    transfer.transfer(s, out);

    EXPECT_EQ(out.m_type, s.m_type);
}

TEST(DataModelTransmit, TypeRefClass)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_TypeRef s, out;
    s.m_type = IObject::GetStaticClass();

    transfer.transfer(s, out);

    EXPECT_EQ(out.m_type, s.m_type);
}

/*TEST(DataModelTransmit, TypeRefNonClassType)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_TypeRef s, out;
    s.type = reflection::GetTypeObject< Array<ObjectPtr> >();

    transfer.transfer(s, out);

    EXPECT_EQ(out.type, s.type);
}*/

//--

/*TEST(DataModelTransmit, ResourceRefEmpty)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_ResRef s, out;

    transfer.transfer(s, out);

    EXPECT_EQ(out.m_path.key(), s.m_path.key());
}

TEST(DataModelTransmit, ResourceRef)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_ResRef s, out;
    //s.m_path.set(res::ResourceKey("dupa.txt", base::res::IResource::GetStaticClass()));

    transfer.transfer(s, out);

    EXPECT_EQ(out.m_path.key(), s.m_path.key());
}*/

//--

TEST(DataModelTransmit, ObjectPtrNull)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_ObjectPtr s, out;

    transfer.transfer(s, out);

    EXPECT_EQ(out.m_obj.get(), s.m_obj.get());
}

TEST(DataModelTransmit, ObjectPtrRegistered)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_ObjectPtr s, out;

    s.m_obj = CreateSharedPtr<test::ReplicationTestObject>();
    knowledge.attachObject(11, s.m_obj);

    transfer.transfer(s, out);

    EXPECT_EQ(out.m_obj.get(), s.m_obj.get());
}

//--

TEST(DataModelTransmit, ObjectWeakPtrNull)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_WeakObjectPtr s, out;

    transfer.transfer(s, out);

    EXPECT_EQ(out.m_obj.unsafe(), s.m_obj.unsafe());
}

TEST(DataModelTransmit, ObjectWeakPtrRegistered)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_WeakObjectPtr s, out;

    auto obj = CreateSharedPtr<test::ReplicationTestObject>();
    s.m_obj = obj;

    knowledge.attachObject(11, obj);

    transfer.transfer(s, out);

    EXPECT_EQ(out.m_obj.unsafe(), s.m_obj.unsafe());
}

//--

TEST(DataModelTransmit, Arrays)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Arrays s, out;
    for (uint32_t i=0; i<8; ++i)
        s.m_bools.pushBack(i & 1);
    for (uint32_t i=0; i<8; ++i)
        s.m_floats.pushBack(-2.0f + i / 4.0f);
    s.m_strings.pushBack("Ala");
    s.m_strings.pushBack("ma");
    s.m_strings.pushBack("kota");

    transfer.transfer(s, out);

    ASSERT_EQ(out.m_bools.size(), s.m_bools.size());
    ASSERT_EQ(out.m_floats.size(), s.m_floats.size());
    ASSERT_EQ(out.m_strings.size(), s.m_strings.size());

    for (uint32_t i=0; i<out.m_bools.size(); ++i)
        EXPECT_EQ(out.m_bools[i], s.m_bools[i]);

    for (uint32_t i=0; i<out.m_floats.size(); ++i)
        EXPECT_NEAR(out.m_floats[i], s.m_floats[i], 20.0f / 1024.0f);

    for (uint32_t i=0; i<out.m_strings.size(); ++i)
        EXPECT_STREQ(out.m_strings[i].c_str(), s.m_strings[i].c_str());
}

TEST(DataModelTransmit, ArraysLimit)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_Arrays s, out;
    for (uint32_t i=0; i<20; ++i)
        s.m_bools.pushBack(i & 1);

    transfer.transfer(s, out);

    ASSERT_EQ(10, out.m_bools.size());

    for (uint32_t i=0; i<out.m_bools.size(); ++i)
        EXPECT_EQ(out.m_bools[i], s.m_bools[i]);
}

//--

TEST(DataModelTransmit, TreeOfStructures)
{
    test::LocalKnowledgeBase knowledge;
    test::TransferTest transfer(knowledge);

    test::TestReplicatedStruct_TreeNode s, out;

    srand(0);
    for (uint32_t i=0; i<50; ++i)
    {
        auto value = rand() / (float)RAND_MAX;
        s.insert(value);
    }

    transfer.transfer(s, out);

    Array<float> orgValues, transferedValues;
    s.dump(orgValues);
    out.dump(transferedValues);

    for (uint32_t i=0; i<std::min(orgValues.size(), transferedValues.size()); ++i)
        EXPECT_NEAR(orgValues[i], transferedValues[i], 1.0f / 1024.0f);

	EXPECT_EQ(orgValues.size(), transferedValues.size());
}