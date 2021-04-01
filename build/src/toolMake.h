#pragma once

#include "utils.h"
#include "project.h"

//--

class ToolMake
{
public:
    ToolMake();
    int run(const char* argv0, const Commandline& cmdline);
};


//--