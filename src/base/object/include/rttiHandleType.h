/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#pragma once

#include "rttiType.h"

BEGIN_BOOMER_NAMESPACE(base::rtti)

/// Base type in the RTTI system
class BASE_OBJECT_API IHandleType : public IType
{
public:
    IHandleType(StringID name, ClassType pointedType);
    virtual ~IHandleType();

    /// get the class we are pointing to
    INLINE ClassType pointedClass() const { return m_pointedType; }

    //--
            
    /// get the pointed object
    virtual void readPointedObject(const void* data, ObjectPtr& outObject) const = 0;

    /// set the pointed object
    virtual void writePointedObject(void* data, const ObjectPtr& object) const = 0;

    /// is the handle pointing to null object (this is faster test than the full extraction)
    virtual bool isPointingToNull(const void* data) const = 0;

    //----

    virtual bool compare(const void* data1, const void* data2) const override final;
    virtual void copy(void* dest, const void* src) const override final;

    virtual void writeBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const override final;
    virtual void readBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const override final;

    virtual DataViewResult describeDataView(StringView viewPath, const void* viewData, DataViewInfo& outInfo) const override final;
    virtual DataViewResult readDataView(StringView viewPath, const void* viewData, void* targetData, Type targetType) const override final;
    virtual DataViewResult writeDataView(StringView viewPath, void* viewData, const void* sourceData, Type sourceType) const override final;
            
    virtual void writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const override final;
    virtual void readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const override final;

    virtual void printToText(IFormatStream& f, const void* data, uint32_t flags = 0) const override;
            
    //----

    // cast data to other handle type, checks the class type
    static bool CastHandle(const void* srcData, const IHandleType* srcHandleType, void* destData, const IHandleType* destHandleType);

public:
    ClassType m_pointedType;
};

//---

// a ref-keeping handle
class StrongHandleType : public IHandleType
{
public:
    StrongHandleType(ClassType pointedType);
    virtual ~StrongHandleType();

    virtual void construct(void* object) const override final;
    virtual void destruct(void* object) const override final;

    virtual void readPointedObject(const void* data, ObjectPtr& outObject) const override final;
    virtual void writePointedObject(void* data, const ObjectPtr& object) const override final;
    virtual bool isPointingToNull(const void* data) const override final;

    //---

    static const char* TypePrefix;
    static Type ParseType(StringParser& typeNameString, TypeSystem& typeSystem);
};

//--

// a non ref-keeping handle that will "expire" when original object gets deleted
class WeakHandleType : public IHandleType
{
public:
    WeakHandleType(ClassType pointedType);
    virtual ~WeakHandleType();

    virtual void construct(void* object) const override final;
    virtual void destruct(void* object) const override final;

    virtual void readPointedObject(const void* data, ObjectPtr& outObject) const override final;
    virtual void writePointedObject(void* data, const ObjectPtr& object) const override final;
    virtual bool isPointingToNull(const void* data) const override final;

    //---

    static const char* TypePrefix;
    static Type ParseType(StringParser& typeNameString, TypeSystem& typeSystem);

};

END_BOOMER_NAMESPACE(base::rtti)