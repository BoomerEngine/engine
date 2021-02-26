/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "core/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

struct StubOpcode;

/// list of opcodes, can be linked together to form opcode chain
class OpcodeList
{
public:
    INLINE OpcodeList() {};
    INLINE OpcodeList(const OpcodeList& other) = default;
    INLINE OpcodeList(OpcodeList&& other) = default;
    INLINE OpcodeList& operator=(const OpcodeList& other) = default;
    INLINE OpcodeList& operator=(OpcodeList&& other) = default;

    INLINE OpcodeList(StubOpcode* op)
        : m_head(op)
        , m_tail(op)
    {}

    //--

    // boolean op (for easier testing)
    INLINE operator bool() const { return m_head != nullptr; }

    // get head of the block
    INLINE StubOpcode* operator->() const { return m_head; }

    // get head of the block
    INLINE StubOpcode* operator*() const { return m_head; }

    // get the head of the block
    INLINE StubOpcode* head() const { return m_head; }

    // get the tail of the block
    INLINE StubOpcode* tail() const { return m_tail; }

    //--

    // get number of opcodes
    uint32_t size() const;

    // print to text stream
    void print(IFormatStream& f) const;

    // extract list into a table
    void extract(Array<const StubOpcode*>& outOpcodes) const;


    //--

    // merge code blocks
    // NOTE: this is destructive operation on the current list
    static void Glue(OpcodeList& a, const OpcodeList& b);
    static OpcodeList Glue(std::initializer_list<const OpcodeList> blocks);

private:
    StubOpcode* m_head = nullptr;
    StubOpcode*	m_tail = nullptr;
};

/// opcode generator - takes the FunctionNode tree and generates StubOpcode list
class OpcodeGenerator : public NoCopy
{
public:
    OpcodeGenerator(mem::LinearAllocator& storageMem);

    // generate opcodes for given node
    // NOTE: this is a recursive function in nature
    OpcodeList generateOpcodes(IErrorHandler& err, const FunctionNode* node, Array<const FunctionScope*>& activeScopes);

    // make a single opcode
    OpcodeList makeOpcode(const FunctionNode* source, Opcode op);

    // make a single opcode with location
    OpcodeList makeOpcode(const StubLocation& location, Opcode op);

private:
    mem::LinearAllocator& m_mem;

    bool m_emitBreakpoints;

    OpcodeList generateInnerOpcodes(IErrorHandler& err, const FunctionNode* node, Array<const FunctionScope*>& activeScopes);
    OpcodeList generateScopeVariableDestructors(IErrorHandler& err, const FunctionNode* node, const Array<const FunctionScope*>& activeScopes, FunctionScope* targetScope);

    bool reportError(IErrorHandler& err, const StubLocation& location, StringView txt);
    void reportWarning(IErrorHandler& err, const StubLocation& location, StringView txt);

    bool reportError(IErrorHandler& err, const FunctionNode* node, StringView txt);
    void reportWarning(IErrorHandler& err, const FunctionNode* node, StringView txt);
};

//---

END_BOOMER_NAMESPACE_EX(script)
