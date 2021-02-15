/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"

namespace base
{
    namespace script
    {
        //---

        static float HalfToFloat(uint16_t a)
        {
            return Float16Helper::Decompress(a);
        }

        static uint16_t FloatToHalf(float a)
        {
            return Float16Helper::Compress(a);
        }

        static float LerpF(float a, float b, float f)
        {
            return a + (b-a) * f;
        }

        static double LerpD(double a, double b, double f)
        {
            return a + (b-a) * f;
        }

        static TYPE_TLS base::FastRandState GRand;

        static float RandOne()
        {
            return GRand.unit();
        }

        static float RandRangeF(float a, float b)
        {
            return a + (b-a) * GRand.unit();
        }

        static int RandRangeI(int a, int b)
        {
            if (abs(a-b) <= 1)
                return a;

            return a + GRand.range(b - a);
        }

        static int RandRangeUnique(int a, int b, int prev)
        {
            if (abs(a-b) <= 1)
                return a;

            for (;;)
            {
                auto val  = RandRangeI(a,b);
                if (val != prev)
                    return val;
            }
        }

        static float Sin(float a)
        {
            return sin(a);
        }

        static float Asin(float a)
        {
            return asin(a);
        }

        static float Cos(float a)
        {
            return cos(a);
        }

        static float Acos(float a)
        {
            return acos(a);
        }

        static float Tan(float a)
        {
            return tan(a);
        }

        static float Atan(float x, float y)
        {
            return atan2(x,y);
        }

        static float Exp(float a)
        {
            return exp(a);
        }

        static float Pow(float a, float x)
        {
            return pow(a, x);
        }

        static float Ln(float a)
        {
            if (a <= 0.0f)
                return 0.0f;
            return ::log(a);
        }

        static float Log2F(float a)
        {
            if (a <= 0.0f)
                return 0.0f;
            return log2(a);
        }

        static uint32_t Log2U32(uint32_t a)
        {
            return base::FloorLog2(a);
        }

        static uint32_t Log2U64(uint64_t a)
        {
            return base::FloorLog2(a);
        }

        static uint32_t NextPow2U32(uint32_t a)
        {
            return base::NextPow2(a);
        }

        static uint64_t NextPow2U64(uint64_t a)
        {
            return base::NextPow2(a);
        }

        static float Log10(float a)
        {
            if (a <= 0.0f)
                return 0.0f;
            return log10(a);
        }

        static float Sinh(float a)
        {
            return sinh(a);
        }

        static float Cosh(float a)
        {
            return cosh(a);
        }

        static float Tanh(float a)
        {
            return tanh(a);
        }

        static float Sqrt(float a)
        {
            if (a < 0.0f) return 0.0f;
            return sqrt(a);
        }

        static float CeilF(float a)
        {
            return ceil(a);
        }

        static double CeilD(double a)
        {
            return ceil(a);
        }

        static float FloorF(float a)
        {
            return floor(a);
        }

        static double FloorD(double a)
        {
            return floor(a);
        }

        static float RoundF(float a)
        {
            return round(a);
        }

        static double RoundD(double a)
        {
            return round(a);
        }

        static float TruncF(float a)
        {
            return trunc(a);
        }

        static double TruncD(double a)
        {
            return trunc(a);
        }

        RTTI_GLOBAL_FUNCTION(HalfToFloat, "Core.HalfToFloat");
        RTTI_GLOBAL_FUNCTION(FloatToHalf, "Core.FloatToHalf");
        RTTI_GLOBAL_FUNCTION(LerpF, "Core.LerpF");
        RTTI_GLOBAL_FUNCTION(LerpD, "Core.LerpD");
        RTTI_GLOBAL_FUNCTION(RandOne, "Core.RandOne");
        RTTI_GLOBAL_FUNCTION(RandRangeF, "Core.RandRangeF");
        RTTI_GLOBAL_FUNCTION(RandRangeI, "Core.RandRangeI");
        RTTI_GLOBAL_FUNCTION(RandRangeUnique, "Core.RandRangeUnique");
        RTTI_GLOBAL_FUNCTION(Sin, "Core.Sin");
        RTTI_GLOBAL_FUNCTION(Asin, "Core.Asin");
        RTTI_GLOBAL_FUNCTION(Cos, "Core.Cos");
        RTTI_GLOBAL_FUNCTION(Acos, "Core.Acos");
        RTTI_GLOBAL_FUNCTION(Tan, "Core.Tan");
        RTTI_GLOBAL_FUNCTION(Atan, "Core.Atan");
        RTTI_GLOBAL_FUNCTION(Exp, "Core.Exp");
        RTTI_GLOBAL_FUNCTION(Pow, "Core.Pow");
        RTTI_GLOBAL_FUNCTION(Ln, "Core.Ln");
        RTTI_GLOBAL_FUNCTION(Log2F, "Core.Log2F");
        RTTI_GLOBAL_FUNCTION(Log2U32, "Core.Log2U32");
        RTTI_GLOBAL_FUNCTION(Log2U64, "Core.Log2U64");
        RTTI_GLOBAL_FUNCTION(NextPow2U32, "Core.NextPow2U32");
        RTTI_GLOBAL_FUNCTION(NextPow2U64, "Core.NextPow2U64");
        RTTI_GLOBAL_FUNCTION(Log10, "Core.Log10");
        RTTI_GLOBAL_FUNCTION(Sinh, "Core.Sinh");
        RTTI_GLOBAL_FUNCTION(Cosh, "Core.Cosh");
        RTTI_GLOBAL_FUNCTION(Tanh, "Core.Tanh");
        RTTI_GLOBAL_FUNCTION(Sqrt, "Core.Sqrt");
        RTTI_GLOBAL_FUNCTION(CeilF, "Core.CeilF");
        RTTI_GLOBAL_FUNCTION(CeilD, "Core.CeilD");
        RTTI_GLOBAL_FUNCTION(FloorF, "Core.FloorF");
        RTTI_GLOBAL_FUNCTION(FloorD, "Core.FloorD");
        RTTI_GLOBAL_FUNCTION(RoundF, "Core.RoundF");
        RTTI_GLOBAL_FUNCTION(RoundD, "Core.RoundD");
        RTTI_GLOBAL_FUNCTION(TruncF, "Core.TruncF");
        RTTI_GLOBAL_FUNCTION(TruncD, "Core.TruncD");

        //---

    } // script
} // base