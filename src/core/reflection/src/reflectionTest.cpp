/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection\test #]
***/

#include "build.h"
#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

struct DupaStruct
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(DupaStruct);

    float re = 0;
    float im = 0;

    INLINE DupaStruct() {};
    INLINE DupaStruct(float a, float b) : re(a), im(b) {};

    float Mag() const
    {
        return std::sqrtf(re * re + im * im);
    }

    DupaStruct operator+(const DupaStruct& b) const
    {
        return DupaStruct(re + b.re, im + b.im);
    }

    DupaStruct& operator+=(const DupaStruct& b)
    {
        re += b.re;
        im += b.im;
        return *this;
    }
};

RTTI_BEGIN_TYPE_STRUCT(DupaStruct);
RTTI_NATIVE_CLASS_FUNCTION(Mag);
RTTI_NATIVE_CLASS_FUNCTION(operator+);
//RTTI_NATIVE_CLASS_FUNCTION(operator+=);
RTTI_END_TYPE();

class Dupa : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(Dupa, IObject);

public:
    Dupa() {};

private:
    int m_x;
    float m_y;
        
    Array<int> m_arrX;
    Array<ObjectPtr> m_arrY;

    void TestP0()
    {
    }

    int TestRetP0()
    {
        return 42;
    }

    void TestP1(int x)
    {
    }

    float TestRetP1(int x)
    {
        return (float)x*x;
    }

    void TestP2(int x, int y)
    {
    }

    bool TestRetP2(int x, int y)
    {
        return x != y;
    }

    void TestP3(int x, const StringBuf& y, ObjectPtr z)
    {
    }

    float TestRetP3(int x, int y, int z)
    {
        return (float)x + y + z;
    }

    void TestP4(int x, const StringBuf& y, ObjectPtr z, float a)
    {
    }

    ObjectPtr TestRetP4(int x, int y, int z, int w)
    {
        return nullptr;
    }

    void TestP5(int x, const StringBuf& y, ObjectPtr z, float a, bool b)
    {
    }

    bool TestRetP5(int x, int y, int z, int w, int k)
    {
        return false;
    }

    static bool IsDupa()
    {
        return false;
    }
};

RTTI_BEGIN_TYPE_CLASS(Dupa);
    RTTI_PROPERTY(m_x);
    RTTI_PROPERTY(m_y).name("Dupa");
    RTTI_PROPERTY(m_arrX);
    RTTI_PROPERTY(m_arrY);
    RTTI_NATIVE_CLASS_FUNCTION(TestP0);
    RTTI_NATIVE_CLASS_FUNCTION(TestRetP0);
    RTTI_NATIVE_CLASS_FUNCTION(TestP1);
    RTTI_NATIVE_CLASS_FUNCTION(TestRetP1);
    RTTI_NATIVE_CLASS_FUNCTION(TestP2);
    RTTI_NATIVE_CLASS_FUNCTION(TestRetP2);
    RTTI_NATIVE_CLASS_FUNCTION(TestP3);
    RTTI_NATIVE_CLASS_FUNCTION(TestRetP3);
    RTTI_NATIVE_CLASS_FUNCTION(TestP4);
    RTTI_NATIVE_CLASS_FUNCTION(TestRetP4);
    RTTI_NATIVE_CLASS_FUNCTION(TestP5);
    RTTI_NATIVE_CLASS_FUNCTION(TestRetP5);
    RTTI_NATIVE_STATIC_FUNCTION(IsDupa);
RTTI_END_TYPE();

// TODO: test extracted reflection info

END_BOOMER_NAMESPACE_EX(test)
