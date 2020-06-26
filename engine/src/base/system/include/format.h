/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\output #]
***/

#pragma once

namespace base
{

    //----

    /// formatting stream
    class BASE_SYSTEM_API IFormatStream : public base::NoCopy
    {
    public:
        virtual ~IFormatStream();

        // append number of chars to the stream
        virtual IFormatStream& append(const char* str, uint32_t len = INDEX_MAX) = 0;

        // append wide-char stream
        virtual IFormatStream& append(const wchar_t* str, uint32_t len = INDEX_MAX);

        //---

        // append numbers in various formats
        // NOTE: those functions are not meant to be called directly
        IFormatStream& appendNumber(char val);
        IFormatStream& appendNumber(short val);
        IFormatStream& appendNumber(int val);
        IFormatStream& appendNumber(int64_t val);
        IFormatStream& appendNumber(uint8_t val);
        IFormatStream& appendNumber(uint16_t val);
        IFormatStream& appendNumber(uint32_t val);
        IFormatStream& appendNumber(uint64_t val);
        IFormatStream& appendNumber(float val);
        IFormatStream& appendNumber(double val);

        // append hex number
        IFormatStream& appendHexNumber(const void* data, uint32_t size);

        // append hex data blob
        IFormatStream& appendHexBlock(const void* data, uint32_t size, uint32_t blockSize);

        // append number with precisions
        IFormatStream& appendPreciseNumber(double val, uint32_t numDigits);

        // append base64 block
        IFormatStream& appendBase64(const void* data, uint32_t size);

        // append URL-encoded string
        IFormatStream& appendUrlEscaped(const char* data, uint32_t length);

        // append C-escaped string
        IFormatStream& appendCEscaped(const char* data, uint32_t length, bool willBeSentInQuotes);

        //---

        // append any printable type to stream, uses the printers
        template< typename T >
        INLINE IFormatStream& appendT(T& data);

        // append any printable type to stream, uses the printers
        template< typename... Args>
        INLINE IFormatStream& appendf(const char* pos, const Args&... args)
        {
            innerFormatter(pos, args...);
            return *this;
        }

        //---

        // add single character
        INLINE IFormatStream& appendch(char ch)
        {
            char str[2] = {ch,0};
            return append(str);
        }

        // append padding string
        IFormatStream& appendPadding(char ch, uint32_t count);

        //---

        // stream like writer
        template< typename T >
        INLINE friend IFormatStream& operator << (IFormatStream& s, const T& value) { return s.appendT(value); }

        //---

        // default NULL stream
        static IFormatStream& NullStream();

    protected:
        // consume and print the string buffer, stop on format argument {}
        bool consumeFormatString(const char*& pos);

        //--

        INLINE void innerFormatter(const char*& pos)
        {
            while (consumeFormatString(pos))
            {
                append("<undefined>");
                innerFormatter(pos);
            }
        }

        template< typename T, typename... Args>
        INLINE void innerFormatter(const char*& pos, const T& value, const Args&... args)
        {
            while (consumeFormatString(pos))
            {
                appendT(value);
                innerFormatter(pos, args...);
            }
        }
    };

    //----

    template<typename T, typename Enable = void>
    struct PrintableConverter
    {
    };

    template<typename T>
    struct has_csrt_method
    {
    private:
        typedef char YesType[1];
        typedef char NoType[2];

        template <typename C> static YesType& test( decltype(&C::c_str) ) ;
        template <typename C> static NoType& test(...);


    public:
        enum { value = sizeof(test<T>(0)) == sizeof(YesType) };
    };

    template<typename T>
    struct has_print_method
    {
    private:
        typedef char YesType[1];
        typedef char NoType[2];

        template <typename C> static YesType& test( decltype(&C::print) ) ;
        template <typename C> static NoType& test(...);


    public:
        enum { value = sizeof(test<T>(0)) == sizeof(YesType) };
    };

    template<typename T>
    struct has_Print_method
    {
    private:
        typedef char YesType[1];
        typedef char NoType[2];

        template <typename C> static YesType& test( decltype(&C::Print) ) ;
        template <typename C> static NoType& test(...);


    public:
        enum { value = sizeof(test<T>(0)) == sizeof(YesType) };
    };

    template<typename T>
    struct PrintableConverter<T, typename std::enable_if<has_print_method<T>::value>::type >
    {
        static void Print(IFormatStream& s, const T& val)
        {
            val.print(s);
        }
    };

    template<typename T>
    struct PrintableConverter<T, typename std::enable_if<has_csrt_method<T>::value && !has_print_method<T>::value>::type >
    {
        static void Print(IFormatStream& s, const T& val)
        {
            s.append(val.c_str());
        }
    };

    template<> struct PrintableConverter<bool>
    {
        static void Print(IFormatStream& s, bool val)
        {
            s << (val ? "true" : "false");
        }
    };

    template<> struct PrintableConverter<char>
    {
        static void Print(IFormatStream& s, char val)
        {
            s.appendNumber(val);
        }
    };

    template<> struct PrintableConverter<short>
    {
        static void Print(IFormatStream& s, short val)
        {
            s.appendNumber(val);
        }
    };

    template<> struct PrintableConverter<int>
    {
        static void Print(IFormatStream& s, int val)
        {
            s.appendNumber(val);
        }
    };

    template<> struct PrintableConverter<int64_t>
    {
        static void Print(IFormatStream& s, int64_t val)
        {
            s.appendNumber(val);
        }
    };

    template<> struct PrintableConverter<uint8_t>
    {
        static void Print(IFormatStream& s, uint8_t val)
        {
            s.appendNumber(val);
        }
    };

    template<> struct PrintableConverter<uint16_t>
    {
        static void Print(IFormatStream& s, uint16_t val)
        {
            s.appendNumber(val);
        }
    };

    template<> struct PrintableConverter<uint32_t>
    {
        static void Print(IFormatStream& s, uint32_t val)
        {
            s.appendNumber(val);
        }
    };

    template<> struct PrintableConverter<uint64_t>
    {
        static void Print(IFormatStream& s, uint64_t val)
        {
            s.appendNumber(val);
        }
    };

    template<> struct PrintableConverter<long unsigned int>
    {
        static void Print(IFormatStream& s, long unsigned val)
        {
            s.appendNumber((uint64_t)val);
        }
    };

    template<> struct PrintableConverter<long  int>
    {
        static void Print(IFormatStream& s, long int val)
        {
            s.appendNumber((int64_t)val);
        }
    };

    template<> struct PrintableConverter<float>
    {
        static void Print(IFormatStream& s, float val)
        {
            s.appendNumber(val);
        }
    };

    template<> struct PrintableConverter<double>
    {
        static void Print(IFormatStream& s, double val)
        {
            s.appendNumber(val);
        }
    };

    template<> struct PrintableConverter<const char* const>
    {
        static void Print(IFormatStream& s, const char* val)
        {
            s.append(val);
        }
    };

    template<> struct PrintableConverter<const wchar_t* const>
    {
        static void Print(IFormatStream& s, const wchar_t* val)
        {
            s.append(val);
        }
    };

    template<> struct PrintableConverter<const char*>
    {
        static void Print(IFormatStream& s, const char* val)
        {
            s.append(val);
        }
    };

    template<> struct PrintableConverter<const wchar_t*>
    {
        static void Print(IFormatStream& s, const wchar_t* val)
        {
            s.append(val);
        }
    };

    template<> struct PrintableConverter<char*>
    {
        static void Print(IFormatStream& s, const char* val)
        {
            s.append(val);
        }
    };

    template<> struct PrintableConverter<wchar_t*>
    {
        static void Print(IFormatStream& s, const wchar_t* val)
        {
            s.append(val);
        }
    };

    template<int N>
    struct PrintableConverter<char[N]>
    {
        static void Print(IFormatStream& s, const char* val)
        {
            if (N > 1)
                s.append(val, N-1);
        }
    };

    template<int N>
    struct PrintableConverter<wchar_t[N]>
    {
        static void Print(IFormatStream& s, const wchar_t* val)
        {
            if (N > 1)
                s.append(val, N-1);
        }
    };

    template<typename T>
    struct PrintableConverter<T*>
    {
        static void Print(IFormatStream& s, T* ptr)
        {
            s.append("(");
            if (!ptr)
                s.append("null");
            else
                s.appendHexNumber(&ptr, sizeof(ptr));

            if (!std::is_same<T, void>::value)
                s.append(", type=").append(typeid(T).name());

            s.append(")");
        }
    };

    //----

    template< typename T >
    struct HexWrap
    {
        HexWrap(const T& data)
            : data_(data)
        {}

        void print(IFormatStream& s) const
        {
            s.appendHexNumber((const unsigned char*)&data_, sizeof(T));
        }

        const T& data_;
    };

    //----

    template< typename T >
    struct PrecWrap
    {
        PrecWrap(const T& data, uint32_t digits)
            : data_(data)
            , digits_(digits)
        {}

        void print(IFormatStream& s) const
        {
            s.appendPreciseNumber(data_, digits_);
        }

        const uint32_t digits_;
        const T& data_;
    };

    //----

    template< typename T >
    struct MemSizeWrap
    {
        MemSizeWrap(const T& data)
            : data_(data)
        {}

        void print(IFormatStream& s) const
        {
            static const uint64_t GB_SIZE = 1ULL << 30;
            static const uint64_t MB_SIZE = 1ULL << 20;
            static const uint64_t KB_SIZE = 1ULL << 10;

            if (data_ >= GB_SIZE)
                s.appendPreciseNumber((double)data_ / (double)GB_SIZE, 3).append(" GB");
            else if (data_ >= MB_SIZE)
                s.appendPreciseNumber((double)data_ / (double)MB_SIZE, 3).append(" MB");
            else if (data_ >= KB_SIZE)
                s.appendPreciseNumber((double)data_ / (double)KB_SIZE, 3).append(" KB");
            else
                s.appendNumber((uint32_t)data_).append(" B");
        }

        const T& data_;
    };

    //----

    template< typename T >
    struct TimeWrap
    {
        TimeWrap(const T& data)
            : data_(data)
        {}

        void print(IFormatStream& s) const
        {
            static const double MIN = 60.0f;
            static const double SEC = 1.0f;
            static const double MS = 0.0001f;
            static const double US = 0.0000001f;
            static const double NS = 0.0000001f;

            if (data_ >= MIN)
                s.appendPreciseNumber((double)data_ / MIN, 2).append(" min");
            else if (data_ >= SEC)
                s.appendPreciseNumber((double)data_ / SEC, 2).append(" s");
            else if (data_ >= MS)
                s.appendPreciseNumber((double)data_ * 1000.0, 3).append(" ms");
            else if (data_ >= US)
                s.appendPreciseNumber((double)data_ * 1000000.0, 3).append(" us");
            else
                s.appendPreciseNumber((double)data_ * 1000000000.0, 3).append(" ns");
        }

        const T& data_;
    };

    //----

    template< typename T >
    struct IndirectWrap
    {
        IndirectWrap(const T* data)
            : data_(data)
        {}

        void print(IFormatStream& s) const
        {
            if (data_)
                data_->print(s);
            else
                s << "null";
        }

        const T* data_;
    };

    //----

    template< typename T >
    INLINE IFormatStream& IFormatStream::appendT(T& data)
    {
        //static_assert(has_Print_method<PrintableConverter<T>>::value, "There is no printer for this type");
        PrintableConverter<typename std::remove_cv<T>::type>::Print(*this, data);
        return *this;
    }

    //----

    // used to create nice foldable (in Notepad++) blocks in the log
    class FoldableFormatBlock : public NoCopy
    {
    public:
        FoldableFormatBlock(base::IFormatStream& f, const char* txt = "")
            : m_log(f)
        {
            m_log << "[#" << txt << "\n";
        }

        ~FoldableFormatBlock()
        {
            m_log << "#]\n";
        }

    private:
        base::IFormatStream& m_log;
    };

} // base

//--

template< typename T >
base::HexWrap<T> Hex(const T& data)
{
    return base::HexWrap<T>(data);
}

//--

template< typename T >
base::PrecWrap<T> Prec(const T& data, int digits)
{
    return base::PrecWrap<T>(data, digits);
}

//--

template< typename T >
base::MemSizeWrap<T> MemSize(const T& data)
{
    return base::MemSizeWrap<T>(data);
}

//--

template< typename T >
base::TimeWrap<T> TimeInterval(const T& data)
{
    return base::TimeWrap<T>(data);
}

//--

template< typename T >
base::IndirectWrap<T> IndirectPrint(const T* data)
{
    return base::IndirectWrap<T>(data);
}

//--

