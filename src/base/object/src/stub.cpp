/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: stubs #]
***/

#include "build.h"
#include "stub.h"

#include "base/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE(base)

//--

IStub::IStub()
{}

IStub::~IStub()
{}

void IStub::postLoad()
{}

//--

IStubReader::IStubReader(uint32_t version)
: m_version(version)
{}

IStubReader::~IStubReader()
{}

IStubWriter::~IStubWriter()
{}

//--

END_BOOMER_NAMESPACE(base)
