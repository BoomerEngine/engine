/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE(base::script)

//---

static void Log(const StringBuf& txt, const StringBuf& a, const StringBuf& b, const StringBuf& c, const StringBuf& d, const StringBuf& e)
{
    StringBuilder ret;
    ret.appendf(txt.c_str(), a, b, c, d, e);
#ifdef BUILD_RELEASE
    fprintf(stdout, "Script: %s\n", ret.c_str());
#else
    TRACE_INFO("Script: {}", ret.c_str());
#endif
    //*/
}

static StringBuf Format(const StringBuf& txt, const StringBuf& a, const StringBuf& b, const StringBuf& c, const StringBuf& d, const StringBuf& e)
{
    StringBuilder ret;
    ret.appendf(txt.c_str(), a, b, c, d, e);
    return ret.toString();
}

RTTI_GLOBAL_FUNCTION(Log, "Core.Log");
RTTI_GLOBAL_FUNCTION(Format, "Core.Format");

//---

END_BOOMER_NAMESPACE(base::script)