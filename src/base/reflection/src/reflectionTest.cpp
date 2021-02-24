/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection\test #]
***/

#include "build.h"
#include "base/object/include/object.h"

BEGIN_BOOMER_NAMESPACE(base::test)

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
};

RTTI_BEGIN_TYPE_CLASS(Dupa);
    RTTI_PROPERTY(m_x);
    RTTI_PROPERTY(m_y).name("Dupa");
    RTTI_PROPERTY(m_arrX);
    RTTI_PROPERTY(m_arrY);
    RTTI_FUNCTION("TestP0", TestP0);
    RTTI_FUNCTION("TestRetP0", TestRetP0);
    RTTI_FUNCTION("TestP1", TestP1);
    RTTI_FUNCTION("TestRetP1", TestRetP1);
    RTTI_FUNCTION("TestP2", TestP2);
    RTTI_FUNCTION("TestRetP2", TestRetP2);
    RTTI_FUNCTION("TestP3", TestP3);
    RTTI_FUNCTION("TestRetP3", TestRetP3);
    RTTI_FUNCTION("TestP4", TestP4);
    RTTI_FUNCTION("TestRetP4", TestRetP4);
    RTTI_FUNCTION("TestP5", TestP5);
    RTTI_FUNCTION("TestRetP5", TestRetP5);
RTTI_END_TYPE();

// TODO: test extracted reflection info

END_BOOMER_NAMESPACE(base::test)