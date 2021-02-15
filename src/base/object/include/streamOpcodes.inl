/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#ifdef STREAM_COMMAND_OPCODE

STREAM_COMMAND_OPCODE(Nop) // technically an invalid token, can be used as marker
STREAM_COMMAND_OPCODE(Compound) // start of compound object
STREAM_COMMAND_OPCODE(CompoundEnd) // end of compound object
STREAM_COMMAND_OPCODE(Array) // start of array
STREAM_COMMAND_OPCODE(ArrayEnd) // end of array
STREAM_COMMAND_OPCODE(SkipHeader) // start of skip block
STREAM_COMMAND_OPCODE(SkipLabel) // target (end) of skip block
STREAM_COMMAND_OPCODE(Property) // compound property
STREAM_COMMAND_OPCODE(DataRaw) // raw binary data
STREAM_COMMAND_OPCODE(DataTypeRef) // type reference
STREAM_COMMAND_OPCODE(DataName) // StringID
STREAM_COMMAND_OPCODE(DataObjectPointer) // reference to another object
STREAM_COMMAND_OPCODE(DataResourceRef) // reference to a resource (sync/async)
STREAM_COMMAND_OPCODE(DataInlineBuffer) // inlined data buffer (not stored in opcode space though)
STREAM_COMMAND_OPCODE(DataAsyncBuffer) // external asynchronously loaded buffer (not loaded/copied to opcode space)

#endif
