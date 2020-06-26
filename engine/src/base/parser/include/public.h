/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_parser_glue.inl"

namespace base
{
    namespace parser
    {

        ///----

        /// type of token found
        enum class TextTokenType : uint8_t
        {
            Invalid=0,

            String, // double quoted string
            Name, // single quoted string
            IntNumber, // an integer numerical value, NOTE: no sign is allowed, may end with "i" or "I"
            UnsignedNumber, // an unsigned numerical value, we know because it ended with "u" or "U"
            FloatNumber, // a floating point numerical value, NOTE: no sign is allowed, may end with "f"
            Char, // a single character from table of simple characters, usually used for {} [] () , + - etc
            Keyword, // a recognized keyword, ie. function, shader, etc whatever was registered in the language
            Identifier, // an identifier (a-z, A-Z, 0-9, _)
            Preprocessor, // a content of perprocessor line, usually beings with # and is eatean till the end
        };

        ///----

        class Location;
        class Token;
        class TokenList;

        class ICommentEater;
        class ILanguageDefinition;
        class IErrorReporter;

        class SimpleLanguageDefinitionBuilder;

        class TextParser;

        //-----

        /// error reporter for the text parser
        class BASE_PARSER_API IErrorReporter : public base::NoCopy
        {
        public:
            virtual ~IErrorReporter();
            virtual void reportError(const Location& loc, StringView<char> message) = 0;
            virtual void reportWarning(const Location& loc, StringView<char> message) = 0;

            //--

            static IErrorReporter& GetDevNull();
            static IErrorReporter& GetDefault();
        };

        //-----

        // a handler for including files
        class BASE_PARSER_API IIncludeHandler : public base::NoCopy
        {
        public:
            virtual ~IIncludeHandler();

            //----

            // load content of local include
            // "global" include in the <> not ""
            // path - requested path
            // referencePath - path of the file we are calling the #include from 
            // outContent - produced content of the file
            // outPath - reference path for extracted text
            virtual bool loadInclude(bool global, StringView<char> path, StringView<char> referencePath, Buffer& outContent, StringBuf& outPath) = 0;


            //--

            /// get the default "NoIncludes" handler
            static IIncludeHandler& GetEmptyHandler();
        };

        //-----

    } // parser
} // base
