/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\profiling #]
***/

#pragma once

namespace base
{
    namespace profiler
    {

        /// color of the profiling block
        namespace colors
        {
            // Google Material Design colors
            // See https://material.google.com/style/color.html

            constexpr uint32_t Red50   = 0xffffebee;
            constexpr uint32_t Red100  = 0xffffcdd2;
            constexpr uint32_t Red200  = 0xffef9a9a;
            constexpr uint32_t Red300  = 0xffe57373;
            constexpr uint32_t Red400  = 0xffef5350;
            constexpr uint32_t Red500  = 0xfff44336;
            constexpr uint32_t Red600  = 0xffe53935;
            constexpr uint32_t Red700  = 0xffd32f2f;
            constexpr uint32_t Red800  = 0xffc62828;
            constexpr uint32_t Red900  = 0xffb71c1c;
            constexpr uint32_t RedA100 = 0xffff8a80;
            constexpr uint32_t RedA200 = 0xffff5252;
            constexpr uint32_t RedA400 = 0xffff1744;
            constexpr uint32_t RedA700 = 0xffd50000;

            constexpr uint32_t Pink50   = 0xfffce4ec;
            constexpr uint32_t Pink100  = 0xfff8bbd0;
            constexpr uint32_t Pink200  = 0xfff48fb1;
            constexpr uint32_t Pink300  = 0xfff06292;
            constexpr uint32_t Pink400  = 0xffec407a;
            constexpr uint32_t Pink500  = 0xffe91e63;
            constexpr uint32_t Pink600  = 0xffd81b60;
            constexpr uint32_t Pink700  = 0xffc2185b;
            constexpr uint32_t Pink800  = 0xffad1457;
            constexpr uint32_t Pink900  = 0xff880e4f;
            constexpr uint32_t PinkA100 = 0xffff80ab;
            constexpr uint32_t PinkA200 = 0xffff4081;
            constexpr uint32_t PinkA400 = 0xfff50057;
            constexpr uint32_t PinkA700 = 0xffc51162;

            constexpr uint32_t Purple50   = 0xfff3e5f5;
            constexpr uint32_t Purple100  = 0xffe1bee7;
            constexpr uint32_t Purple200  = 0xffce93d8;
            constexpr uint32_t Purple300  = 0xffba68c8;
            constexpr uint32_t Purple400  = 0xffab47bc;
            constexpr uint32_t Purple500  = 0xff9c27b0;
            constexpr uint32_t Purple600  = 0xff8e24aa;
            constexpr uint32_t Purple700  = 0xff7b1fa2;
            constexpr uint32_t Purple800  = 0xff6a1b9a;
            constexpr uint32_t Purple900  = 0xff4a148c;
            constexpr uint32_t PurpleA100 = 0xffea80fc;
            constexpr uint32_t PurpleA200 = 0xffe040fb;
            constexpr uint32_t PurpleA400 = 0xffd500f9;
            constexpr uint32_t PurpleA700 = 0xffaa00ff;

            constexpr uint32_t DeepPurple50   = 0xffede7f6;
            constexpr uint32_t DeepPurple100  = 0xffd1c4e9;
            constexpr uint32_t DeepPurple200  = 0xffb39ddb;
            constexpr uint32_t DeepPurple300  = 0xff9575cd;
            constexpr uint32_t DeepPurple400  = 0xff7e57c2;
            constexpr uint32_t DeepPurple500  = 0xff673ab7;
            constexpr uint32_t DeepPurple600  = 0xff5e35b1;
            constexpr uint32_t DeepPurple700  = 0xff512da8;
            constexpr uint32_t DeepPurple800  = 0xff4527a0;
            constexpr uint32_t DeepPurple900  = 0xff311b92;
            constexpr uint32_t DeepPurpleA100 = 0xffb388ff;
            constexpr uint32_t DeepPurpleA200 = 0xff7c4dff;
            constexpr uint32_t DeepPurpleA400 = 0xff651fff;
            constexpr uint32_t DeepPurpleA700 = 0xff6200ea;

            constexpr uint32_t Indigo50   = 0xffe8eaf6;
            constexpr uint32_t Indigo100  = 0xffc5cae9;
            constexpr uint32_t Indigo200  = 0xff9fa8da;
            constexpr uint32_t Indigo300  = 0xff7986cb;
            constexpr uint32_t Indigo400  = 0xff5c6bc0;
            constexpr uint32_t Indigo500  = 0xff3f51b5;
            constexpr uint32_t Indigo600  = 0xff3949ab;
            constexpr uint32_t Indigo700  = 0xff303f9f;
            constexpr uint32_t Indigo800  = 0xff283593;
            constexpr uint32_t Indigo900  = 0xff1a237e;
            constexpr uint32_t IndigoA100 = 0xff8c9eff;
            constexpr uint32_t IndigoA200 = 0xff536dfe;
            constexpr uint32_t IndigoA400 = 0xff3d5afe;
            constexpr uint32_t IndigoA700 = 0xff304ffe;

            constexpr uint32_t Blue50   = 0xffe3f2fd;
            constexpr uint32_t Blue100  = 0xffbbdefb;
            constexpr uint32_t Blue200  = 0xff90caf9;
            constexpr uint32_t Blue300  = 0xff64b5f6;
            constexpr uint32_t Blue400  = 0xff42a5f5;
            constexpr uint32_t Blue500  = 0xff2196f3;
            constexpr uint32_t Blue600  = 0xff1e88e5;
            constexpr uint32_t Blue700  = 0xff1976d2;
            constexpr uint32_t Blue800  = 0xff1565c0;
            constexpr uint32_t Blue900  = 0xff0d47a1;
            constexpr uint32_t BlueA100 = 0xff82b1ff;
            constexpr uint32_t BlueA200 = 0xff448aff;
            constexpr uint32_t BlueA400 = 0xff2979ff;
            constexpr uint32_t BlueA700 = 0xff2962ff;

            constexpr uint32_t LightBlue50   = 0xffe1f5fe;
            constexpr uint32_t LightBlue100  = 0xffb3e5fc;
            constexpr uint32_t LightBlue200  = 0xff81d4fa;
            constexpr uint32_t LightBlue300  = 0xff4fc3f7;
            constexpr uint32_t LightBlue400  = 0xff29b6f6;
            constexpr uint32_t LightBlue500  = 0xff03a9f4;
            constexpr uint32_t LightBlue600  = 0xff039be5;
            constexpr uint32_t LightBlue700  = 0xff0288d1;
            constexpr uint32_t LightBlue800  = 0xff0277bd;
            constexpr uint32_t LightBlue900  = 0xff01579b;
            constexpr uint32_t LightBlueA100 = 0xff80d8ff;
            constexpr uint32_t LightBlueA200 = 0xff40c4ff;
            constexpr uint32_t LightBlueA400 = 0xff00b0ff;
            constexpr uint32_t LightBlueA700 = 0xff0091ea;

            constexpr uint32_t Cyan50   = 0xffe0f7fa;
            constexpr uint32_t Cyan100  = 0xffb2ebf2;
            constexpr uint32_t Cyan200  = 0xff80deea;
            constexpr uint32_t Cyan300  = 0xff4dd0e1;
            constexpr uint32_t Cyan400  = 0xff26c6da;
            constexpr uint32_t Cyan500  = 0xff00bcd4;
            constexpr uint32_t Cyan600  = 0xff00acc1;
            constexpr uint32_t Cyan700  = 0xff0097a7;
            constexpr uint32_t Cyan800  = 0xff00838f;
            constexpr uint32_t Cyan900  = 0xff006064;
            constexpr uint32_t CyanA100 = 0xff84ffff;
            constexpr uint32_t CyanA200 = 0xff18ffff;
            constexpr uint32_t CyanA400 = 0xff00e5ff;
            constexpr uint32_t CyanA700 = 0xff00b8d4;

            constexpr uint32_t Teal50   = 0xffe0f2f1;
            constexpr uint32_t Teal100  = 0xffb2dfdb;
            constexpr uint32_t Teal200  = 0xff80cbc4;
            constexpr uint32_t Teal300  = 0xff4db6ac;
            constexpr uint32_t Teal400  = 0xff26a69a;
            constexpr uint32_t Teal500  = 0xff009688;
            constexpr uint32_t Teal600  = 0xff00897b;
            constexpr uint32_t Teal700  = 0xff00796b;
            constexpr uint32_t Teal800  = 0xff00695c;
            constexpr uint32_t Teal900  = 0xff004d40;
            constexpr uint32_t TealA100 = 0xffa7ffeb;
            constexpr uint32_t TealA200 = 0xff64ffda;
            constexpr uint32_t TealA400 = 0xff1de9b6;
            constexpr uint32_t TealA700 = 0xff00bfa5;

            constexpr uint32_t Green50   = 0xffe8f5e9;
            constexpr uint32_t Green100  = 0xffc8e6c9;
            constexpr uint32_t Green200  = 0xffa5d6a7;
            constexpr uint32_t Green300  = 0xff81c784;
            constexpr uint32_t Green400  = 0xff66bb6a;
            constexpr uint32_t Green500  = 0xff4caf50;
            constexpr uint32_t Green600  = 0xff43a047;
            constexpr uint32_t Green700  = 0xff388e3c;
            constexpr uint32_t Green800  = 0xff2e7d32;
            constexpr uint32_t Green900  = 0xff1b5e20;
            constexpr uint32_t GreenA100 = 0xffb9f6ca;
            constexpr uint32_t GreenA200 = 0xff69f0ae;
            constexpr uint32_t GreenA400 = 0xff00e676;
            constexpr uint32_t GreenA700 = 0xff00c853;

            constexpr uint32_t LightGreen50   = 0xfff1f8e9;
            constexpr uint32_t LightGreen100  = 0xffdcedc8;
            constexpr uint32_t LightGreen200  = 0xffc5e1a5;
            constexpr uint32_t LightGreen300  = 0xffaed581;
            constexpr uint32_t LightGreen400  = 0xff9ccc65;
            constexpr uint32_t LightGreen500  = 0xff8bc34a;
            constexpr uint32_t LightGreen600  = 0xff7cb342;
            constexpr uint32_t LightGreen700  = 0xff689f38;
            constexpr uint32_t LightGreen800  = 0xff558b2f;
            constexpr uint32_t LightGreen900  = 0xff33691e;
            constexpr uint32_t LightGreenA100 = 0xffccff90;
            constexpr uint32_t LightGreenA200 = 0xffb2ff59;
            constexpr uint32_t LightGreenA400 = 0xff76ff03;
            constexpr uint32_t LightGreenA700 = 0xff64dd17;

            constexpr uint32_t Lime50   = 0xfff9ebe7;
            constexpr uint32_t Lime100  = 0xfff0f4c3;
            constexpr uint32_t Lime200  = 0xffe6ee9c;
            constexpr uint32_t Lime300  = 0xffdce775;
            constexpr uint32_t Lime400  = 0xffd4e157;
            constexpr uint32_t Lime500  = 0xffcddc39;
            constexpr uint32_t Lime600  = 0xffc0ca33;
            constexpr uint32_t Lime700  = 0xffafb42b;
            constexpr uint32_t Lime800  = 0xff9e9d24;
            constexpr uint32_t Lime900  = 0xff827717;
            constexpr uint32_t LimeA100 = 0xfff4ff81;
            constexpr uint32_t LimeA200 = 0xffeeff41;
            constexpr uint32_t LimeA400 = 0xffc6ff00;
            constexpr uint32_t LimeA700 = 0xffaeea00;

            constexpr uint32_t Yellow50   = 0xfffffde7;
            constexpr uint32_t Yellow100  = 0xfffff9c4;
            constexpr uint32_t Yellow200  = 0xfffff59d;
            constexpr uint32_t Yellow300  = 0xfffff176;
            constexpr uint32_t Yellow400  = 0xffffee58;
            constexpr uint32_t Yellow500  = 0xffffeb3b;
            constexpr uint32_t Yellow600  = 0xfffdd835;
            constexpr uint32_t Yellow700  = 0xfffbc02d;
            constexpr uint32_t Yellow800  = 0xfff9a825;
            constexpr uint32_t Yellow900  = 0xfff57f17;
            constexpr uint32_t YellowA100 = 0xffffff8d;
            constexpr uint32_t YellowA200 = 0xffffff00;
            constexpr uint32_t YellowA400 = 0xffffea00;
            constexpr uint32_t YellowA700 = 0xffffd600;

            constexpr uint32_t Amber50   = 0xfffff8e1;
            constexpr uint32_t Amber100  = 0xffffecb3;
            constexpr uint32_t Amber200  = 0xffffe082;
            constexpr uint32_t Amber300  = 0xffffd54f;
            constexpr uint32_t Amber400  = 0xffffca28;
            constexpr uint32_t Amber500  = 0xffffc107;
            constexpr uint32_t Amber600  = 0xffffb300;
            constexpr uint32_t Amber700  = 0xffffa000;
            constexpr uint32_t Amber800  = 0xffff8f00;
            constexpr uint32_t Amber900  = 0xffff6f00;
            constexpr uint32_t AmberA100 = 0xffffe57f;
            constexpr uint32_t AmberA200 = 0xffffd740;
            constexpr uint32_t AmberA400 = 0xffffc400;
            constexpr uint32_t AmberA700 = 0xffffab00;

            constexpr uint32_t Orange50   = 0xfffff3e0;
            constexpr uint32_t Orange100  = 0xffffe0b2;
            constexpr uint32_t Orange200  = 0xffffcc80;
            constexpr uint32_t Orange300  = 0xffffb74d;
            constexpr uint32_t Orange400  = 0xffffa726;
            constexpr uint32_t Orange500  = 0xffff9800;
            constexpr uint32_t Orange600  = 0xfffb8c00;
            constexpr uint32_t Orange700  = 0xfff57c00;
            constexpr uint32_t Orange800  = 0xffef6c00;
            constexpr uint32_t Orange900  = 0xffe65100;
            constexpr uint32_t OrangeA100 = 0xffffd180;
            constexpr uint32_t OrangeA200 = 0xffffab40;
            constexpr uint32_t OrangeA400 = 0xffff9100;
            constexpr uint32_t OrangeA700 = 0xffff6d00;

            constexpr uint32_t DeepOrange50   = 0xfffbe9e7;
            constexpr uint32_t DeepOrange100  = 0xffffccbc;
            constexpr uint32_t DeepOrange200  = 0xffffab91;
            constexpr uint32_t DeepOrange300  = 0xffff8a65;
            constexpr uint32_t DeepOrange400  = 0xffff7043;
            constexpr uint32_t DeepOrange500  = 0xffff5722;
            constexpr uint32_t DeepOrange600  = 0xfff4511e;
            constexpr uint32_t DeepOrange700  = 0xffe64a19;
            constexpr uint32_t DeepOrange800  = 0xffd84315;
            constexpr uint32_t DeepOrange900  = 0xffbf360c;
            constexpr uint32_t DeepOrangeA100 = 0xffff9e80;
            constexpr uint32_t DeepOrangeA200 = 0xffff6e40;
            constexpr uint32_t DeepOrangeA400 = 0xffff3d00;
            constexpr uint32_t DeepOrangeA700 = 0xffdd2c00;

            constexpr uint32_t Brown50  = 0xffefebe9;
            constexpr uint32_t Brown100 = 0xffd7ccc8;
            constexpr uint32_t Brown200 = 0xffbcaaa4;
            constexpr uint32_t Brown300 = 0xffa1887f;
            constexpr uint32_t Brown400 = 0xff8d6e63;
            constexpr uint32_t Brown500 = 0xff795548;
            constexpr uint32_t Brown600 = 0xff6d4c41;
            constexpr uint32_t Brown700 = 0xff5d4037;
            constexpr uint32_t Brown800 = 0xff4e342e;
            constexpr uint32_t Brown900 = 0xff3e2723;

            constexpr uint32_t Grey50  = 0xfffafafa;
            constexpr uint32_t Grey100 = 0xfff5f5f5;
            constexpr uint32_t Grey200 = 0xffeeeeee;
            constexpr uint32_t Grey300 = 0xffe0e0e0;
            constexpr uint32_t Grey400 = 0xffbdbdbd;
            constexpr uint32_t Grey500 = 0xff9e9e9e;
            constexpr uint32_t Grey600 = 0xff757575;
            constexpr uint32_t Grey700 = 0xff616161;
            constexpr uint32_t Grey800 = 0xff424242;
            constexpr uint32_t Grey900 = 0xff212121;

            constexpr uint32_t BlueGrey50  = 0xffeceff1;
            constexpr uint32_t BlueGrey100 = 0xffcfd8dc;
            constexpr uint32_t BlueGrey200 = 0xffb0bec5;
            constexpr uint32_t BlueGrey300 = 0xff90a4ae;
            constexpr uint32_t BlueGrey400 = 0xff78909c;
            constexpr uint32_t BlueGrey500 = 0xff607d8b;
            constexpr uint32_t BlueGrey600 = 0xff546e7a;
            constexpr uint32_t BlueGrey700 = 0xff455a64;
            constexpr uint32_t BlueGrey800 = 0xff37474f;
            constexpr uint32_t BlueGrey900 = 0xff263238;

            constexpr uint32_t Black  = 0xff000000;
            constexpr uint32_t White  = 0xffffffff;
            constexpr uint32_t Null   = 0x00000000;


            constexpr uint32_t Red         = Red500;
            constexpr uint32_t DarkRed     = Red900;
            constexpr uint32_t Coral       = Red200;
            constexpr uint32_t RichRed     = 0xffff0000;
            constexpr uint32_t Pink        = Pink500;
            constexpr uint32_t Rose        = PinkA100;
            constexpr uint32_t Purple      = Purple500;
            constexpr uint32_t Magenta     = PurpleA200;
            constexpr uint32_t DarkMagenta = PurpleA700;
            constexpr uint32_t DeepPurple  = DeepPurple500;
            constexpr uint32_t Indigo      = Indigo500;
            constexpr uint32_t Blue        = Blue500;
            constexpr uint32_t DarkBlue    = Blue900;
            constexpr uint32_t RichBlue    = 0xff0000ff;
            constexpr uint32_t LightBlue   = LightBlue500;
            constexpr uint32_t SkyBlue     = LightBlueA100;
            constexpr uint32_t Navy        = LightBlue800;
            constexpr uint32_t Cyan        = Cyan500;
            constexpr uint32_t DarkCyan    = Cyan900;
            constexpr uint32_t Teal        = Teal500;
            constexpr uint32_t DarkTeal    = Teal900;
            constexpr uint32_t Green       = Green500;
            constexpr uint32_t DarkGreen   = Green900;
            constexpr uint32_t RichGreen   = 0xff00ff00;
            constexpr uint32_t LightGreen  = LightGreen500;
            constexpr uint32_t Mint        = LightGreen900;
            constexpr uint32_t Lime        = Lime500;
            constexpr uint32_t Olive       = Lime900;
            constexpr uint32_t Yellow      = Yellow500;
            constexpr uint32_t RichYellow  = YellowA200;
            constexpr uint32_t Amber       = Amber500;
            constexpr uint32_t Gold        = Amber300;
            constexpr uint32_t PaleGold    = AmberA100;
            constexpr uint32_t Orange      = Orange500;
            constexpr uint32_t Skin        = Orange100;
            constexpr uint32_t DeepOrange  = DeepOrange500;
            constexpr uint32_t Brick       = DeepOrange900;
            constexpr uint32_t Brown       = Brown500;
            constexpr uint32_t DarkBrown   = Brown900;
            constexpr uint32_t CreamWhite  = Orange50;
            constexpr uint32_t Wheat       = Amber100;
            constexpr uint32_t Grey        = Grey500;
            constexpr uint32_t Dark        = Grey900;
            constexpr uint32_t Silver      = Grey300;
            constexpr uint32_t BlueGrey    = BlueGrey500;

            constexpr uint32_t Default = Wheat;
        };

        template <class ... TArgs>
        inline constexpr uint32_t ExtractColor(TArgs...);

        template <>
        inline constexpr uint32_t ExtractColor<>() {
            return colors::Default;
        }

        template <class T>
        inline constexpr uint32_t ExtractColor(T) {
            return colors::Default;
        }

        template <>
        inline constexpr uint32_t ExtractColor<uint32_t>(uint32_t _color) {
            return _color;
        }

        /// block information
        struct BASE_SYSTEM_API BlockInfo
        {
        public:
            BlockInfo(const char* name, const char* id, const char* file, uint32_t line, uint8_t lvl, uint32_t color);

            const char* m_name;
            uint8_t m_level;
            uint32_t m_color;
            const void* m_internal;
        };

        /// profiling block
        struct BASE_SYSTEM_API Block
        {
        public:
            Block(const BlockInfo& info);
            ~Block();

            // initialize profiler at given level
            static void Initialize(uint8_t level);

            // initialize profiler at disabled state
            static void InitializeDisabled();

            /// yield current profiling blocks, called when exiting fiber, will stop time capture on all blocks
            /// NOTE: this returns the active block at the place when fiber is yielded
            /// NOTE: this may be null if there's nothing yielded
            static Block* CaptureAndYield();

            /// resume previously yielded profiling blocks
            static void Resume(Block* block);

        private:
            Block* m_parent;
            const BlockInfo* m_info;
            bool m_enabled;
            bool m_started;
            uint64_t m_startTime;

            void start();
            void stop();

            void startChain();
            void stopChain();

            static uint8_t st_level;
        };

    } // profiler
} // base

//--

#if defined(NO_PROFILING)

    // profiling macros
    #define PC_SCOPE_LVL0(name, ...)
    #define PC_SCOPE_LVL1(name, ...)
    #define PC_SCOPE_LVL2(name, ...)
    #define PC_SCOPE_LVL3(name, ...)
    #define PC_EVENT(name)
    #define PC_DATA(name, val)

#else

    #define PC_UNIQUE_LINE_ID __FILE__ ":" STRINGIFICATION(__LINE__)

    #define PC_SCOPE(name, lvl, color) static thread_local base::profiler::BlockInfo __profiling_block_##name(#name, PC_UNIQUE_LINE_ID, __FILE__, __LINE__, lvl, color); \
            base::profiler::Block __profiling_##name(__profiling_block_##name)

    // profiling macros
    #define PC_SCOPE_LVL0(name, ...) PC_SCOPE(name, 0, base::profiler::ExtractColor(__VA_ARGS__))
    #define PC_SCOPE_LVL1(name, ...) PC_SCOPE(name, 1, base::profiler::ExtractColor(__VA_ARGS__))
    #define PC_SCOPE_LVL2(name, ...) PC_SCOPE(name, 2, base::profiler::ExtractColor(__VA_ARGS__))
    #define PC_SCOPE_LVL3(name, ...) PC_SCOPE(name, 3, base::profiler::ExtractColor(__VA_ARGS__))
    #define PC_EVENT(name)
    #define PC_DATA(name, val)

#endif
