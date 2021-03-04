/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\box #]
***/

#include "build.h"
#include "mathShapes.h"

#include <Windows.h>

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_STRUCT(Box);
    RTTI_BIND_NATIVE_COMPARE(Box);
    RTTI_PROPERTY(min).editable();
    RTTI_PROPERTY(max).editable();
RTTI_END_TYPE();

//--

static Box ZERO_B(Vector3(0,0,0), Vector3(0,0,0));
static Box EMPTY_B(Vector3(VERY_LARGE_FLOAT, VERY_LARGE_FLOAT, VERY_LARGE_FLOAT), Vector3(-VERY_LARGE_FLOAT,-VERY_LARGE_FLOAT,-VERY_LARGE_FLOAT));
static Box UNIT_B(Vector3(0,0,0), Vector3(1,1,1));

const Box& Box::ZERO()
{
    return ZERO_B;
}

const Box& Box::EMPTY()
{
    return EMPTY_B;
}

const Box& Box::UNIT()
{
    return UNIT_B;
}

//--

bool Box::intersect(const Vector3& origin, const Vector3& direction, float maxLength /*= VERY_LARGE_FLOAT*/, float* outEnterDistFromOrigin /*= nullptr*/, Vector3* outEntryPoint /*= nullptr*/, Vector3* outEntryNormal /*= nullptr*/) const
{
    if (outEntryNormal)
    {
        Plane planes[6];
        planes[0].n = -Vector3::EX();
        planes[1].n = Vector3::EX();
        planes[2].n = -Vector3::EY();
        planes[3].n = Vector3::EY();
        planes[4].n = -Vector3::EZ();
        planes[5].n = Vector3::EZ();

        planes[0].d = min.x;
        planes[1].d = -max.x;
        planes[2].d = min.y;
        planes[3].d = -max.y;
        planes[4].d = min.z;
        planes[5].d = -max.z;

        return IntersectPlaneList(planes, 6, origin, direction, maxLength, outEnterDistFromOrigin, outEntryPoint, outEntryNormal);
    }

    // r.dir is unit direction vector of ray
    auto fracX = 1.0f / direction.x;
    auto fracY = 1.0f / direction.y;
    auto fracZ = 1.0f / direction.z;

    // lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
    // r.org is origin of ray
    float t1 = (min.x - origin.x) * fracX;
    float t2 = (max.x - origin.x) * fracX;
    float t3 = (min.y - origin.y) * fracY;
    float t4 = (max.y - origin.y) * fracY;
    float t5 = (min.z - origin.z) * fracZ;
    float t6 = (max.z - origin.z) * fracZ;

    float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));
    if (tmax < 0)
        return false;

    if (tmin > tmax)
        return false;

    if (tmin > maxLength)
        return false;

    if (outEnterDistFromOrigin)
        *outEnterDistFromOrigin = tmin;

    if (outEntryPoint)
        *outEntryPoint = origin + (tmin * direction); // TODO: project on box

    return true;
}

//--

class IReflectionInitProxy
{
public:
    IReflectionInitProxy();
    virtual ~IReflectionInitProxy();

    virtual const char* typeName() const = 0;
    virtual void createType(IType*& outType) const = 0;
    virtual void initType(IType* type) const = 0;

    static void PopList(Array<IReflectionInitProxy*>& outNew);

private:
    static IReflectionInitProxy* RunListOp(const std::function<IReflectionInitProxy*(IReflectionInitProxy*&)>& op);

    IReflectionInitProxy* next = nullptr;
};

IReflectionInitProxy::IReflectionInitProxy()
{
    RunListOp([this](IReflectionInitProxy*& list)
        {
            next = list;
            list = this;
            return list;
        });        
}

IReflectionInitProxy::~IReflectionInitProxy()
{

}

void IReflectionInitProxy::PopList(Array<IReflectionInitProxy*>& outNew)
{
    RunListOp([&outNew](IReflectionInitProxy*& list)
        {
            auto val = list;
            list = nullptr;

            while (val)
            {
                outNew.pushBack(val);
                auto next = val->next;
                val->next = nullptr;
                val = val->next;
            }

            return nullptr;
        });
}

IReflectionInitProxy* IReflectionInitProxy::RunListOp(const std::function<IReflectionInitProxy* (IReflectionInitProxy*&)>& op)
{
    static SpinLock ListLock;
    static IReflectionInitProxy* List = nullptr;

    auto lock = CreateLock(ListLock);
    return op(List);
}

//--

template<typename D>
struct ReflectionAutomaticRegister {
private:
    struct Exec {
        Exec() { 
            D::do_it();
        }
    };

    // will force instantiation of definition of static member
    template<Exec&> struct ref_it { };

    static Exec register_object;
    static ref_it<register_object> referrer;
};

template<typename D> typename ReflectionAutomaticRegister<D>::Exec ReflectionAutomaticRegister<D>::register_object;

//--

class TestRegister : public IReflectionInitProxy
{
public:
    TestRegister()
    {
        TRACE_INFO("Dupa registered!");
    }
    virtual const char* typeName() const { return "Dupa"; }
    virtual void createType(IType*& outType) const { outType = new EnumType("crap"_id, 1, 1, false); }
    virtual void initType(IType* type) const { }
};

ReflectionAutomaticRegister<TestRegister> GTestRegister;

class CrapRegister : public ReflectionAutomaticRegister<CrapRegister>
{
public:
    static void do_it()
    {
        TRACE_INFO("Dupa registered!");
    }
};

CrapRegister GCrapRegister;

void __cdecl TestMe()
{
    TRACE_INFO("TestMe!");
}

struct ForceFunctionToBeLinked
{
    ForceFunctionToBeLinked(const void* p) { SetLastError(PtrToInt(p)); }
};

ForceFunctionToBeLinked forceTestMe(TestMe);

//--

void TestNewRegistering()
{
    InplaceArray<IReflectionInitProxy*, 10> newRegistrants;
    IReflectionInitProxy::PopList(newRegistrants);

    TRACE_INFO("Size: {}", newRegistrants.size());
}

//--

END_BOOMER_NAMESPACE()
