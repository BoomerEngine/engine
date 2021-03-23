/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "playerConnection.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGamePlayerConnection);
RTTI_END_TYPE();

IGamePlayerConnection::IGamePlayerConnection(IGamePlayerIdentity* identity)
    : m_identity(AddRef(identity))
{}

IGamePlayerConnection::~IGamePlayerConnection()
{}

//--
    
END_BOOMER_NAMESPACE()
