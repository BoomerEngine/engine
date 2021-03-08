/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#ifdef SERIALIZATION_OPCODE

SERIALIZATION_OPCODE(Nop) // technically an invalid token, can be used as marker
SERIALIZATION_OPCODE(Compound) // start of compound object
SERIALIZATION_OPCODE(CompoundEnd) // end of compound object
SERIALIZATION_OPCODE(Array) // start of array
SERIALIZATION_OPCODE(ArrayEnd) // end of array
SERIALIZATION_OPCODE(SkipHeader) // start of skip block
SERIALIZATION_OPCODE(SkipLabel) // target (end) of skip block
SERIALIZATION_OPCODE(Property) // compound property
SERIALIZATION_OPCODE(DataRaw) // raw binary data
SERIALIZATION_OPCODE(DataTypeRef) // type reference
SERIALIZATION_OPCODE(DataName) // StringID
SERIALIZATION_OPCODE(DataObjectPointer) // reference to another object
SERIALIZATION_OPCODE(DataResourceRef) // reference to a resource (sync/async)
SERIALIZATION_OPCODE(DataInlineBuffer) // inlined data buffer (not stored in opcode space though)
SERIALIZATION_OPCODE(DataAsyncFileBuffer) // external asynchronously loaded buffer (not loaded/copied to opcode space)

#endif
