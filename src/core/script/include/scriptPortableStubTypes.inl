/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: stubs #]
***/

#ifndef DECLARE_STUB_TYPE
	#define	DECLARE_STUB_TYPE(x) struct Stub##x;
#endif

DECLARE_STUB_TYPE(Module)
DECLARE_STUB_TYPE(ModuleImport)
DECLARE_STUB_TYPE(File)
DECLARE_STUB_TYPE(TypeName)
DECLARE_STUB_TYPE(TypeDecl)
DECLARE_STUB_TYPE(TypeRef)
DECLARE_STUB_TYPE(Class)
DECLARE_STUB_TYPE(Constant)
DECLARE_STUB_TYPE(ConstantValue)
DECLARE_STUB_TYPE(Enum)
DECLARE_STUB_TYPE(EnumOption)
DECLARE_STUB_TYPE(Property)
DECLARE_STUB_TYPE(Function)
DECLARE_STUB_TYPE(FunctionArg)
DECLARE_STUB_TYPE(Opcode)

#undef DECLARE_STUB_TYPE