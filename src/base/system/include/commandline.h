/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

namespace base
{

    /// the "base" commandline interface, used in initialization of the most basic systems like IO and fibers where the base::Commandline class is not avaiable
    /// NOTE: implementation is in "app"
    class BASE_SYSTEM_API IBaseCommandLine
    {
    public:
        virtual ~IBaseCommandLine();

        //! check if params is defined
        virtual bool hasParam(const char* param) const = 0;

        //! get single value (last defined value) of a parameter
        virtual const char* singleValueAnsiStr(const char* param) const = 0;

        //! get single value (last defined value) of parameter and parse it as an integer
        virtual int singleValueInt(const char* param, int defaultValue = 0) const = 0;

        //! get single value (last defined value) of parameter and parse it as a boolean
        virtual bool singleValueBool(const char* param, bool defaultValue = false) const = 0;

        //! get single value (last defined value) of parameter and parse it as a floating point value
        virtual float singleValueFloat(const char* param, float defaultValue = 0.0f) const = 0;
    };

} // base
