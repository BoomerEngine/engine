// Scintilla source code edit control
// [# filter: scintilla #]
// [# pch: disabled #]
/** @file LexerNoExceptions.cxx
 ** A simple lexer with no state which does not throw exceptions so can be used in an external lexer.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "LexerModule.h"
#include "LexerBase.h"
#include "LexerNoExceptions.h"

using namespace Scintilla;

Sci_Position SCI_METHOD LexerNoExceptions::PropertySet(const char *key, const char *val) {
	return LexerBase::PropertySet(key, val);
}

Sci_Position SCI_METHOD LexerNoExceptions::WordListSet(int n, const char *wl) {
    return LexerBase::WordListSet(n, wl);
}

void SCI_METHOD LexerNoExceptions::Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *pAccess) {
    Accessor astyler(pAccess, &props);
    Lexer(startPos, lengthDoc, initStyle, pAccess, astyler);
    astyler.Flush();
}

void SCI_METHOD LexerNoExceptions::Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *pAccess) {
    Accessor astyler(pAccess, &props);
    Folder(startPos, lengthDoc, initStyle, pAccess, astyler);
    astyler.Flush();
}
