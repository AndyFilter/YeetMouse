//
// FixPointCS
//
// Copyright(c) Jere Sanisalo, Petri Kero
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY INT64_C(C)AIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER INT64_C(DEA)INGS IN THE
// SOFTWARE.
//

//
// GENERATED FILE!!!
//
// Generated from Fixed64.cs, part of the FixPointCS project (MIT license).
//
#pragma once
#ifndef __FIXED64_H
#define __FIXED64_H

// Include numeric types
#include <linux/types.h>
#include "FixedUtil.h"
#include <linux/kernel.h>


// If FP_ASSERT is not custom-defined, then use the standard one
//#ifndef FP_ASSERT
//#   include <assert.h>
//#   define FP_ASSERT(x) assert(x)
//#endif

// If FP_CUSTOM_INVALID_ARGS is defined, then the used is expected to implement the following functions in
// the FixedUtil namespace:
//  void InvalidArgument(const char* funcName, const char* argName, FP_INT argValue);
//  void InvalidArgument(const char* funcName, const char* argName, FP_INT argValue1, FP_INT argValue2);
//	void InvalidArgument(const char* funcName, const char* argName, FP_LONG argValue);
//	void InvalidArgument(const char* funcName, const char* argName, FP_LONG argValue1, FP_LONG argValue2);
// These functions should handle the cases for invalid arguments in any desired way (assert, exception, log, ignore etc).
//#define FP_CUSTOM_INVALID_ARGS

typedef int32_t FP_INT;
typedef uint32_t FP_UINT;
typedef int64_t FP_LONG;
typedef uint64_t FP_ULONG;

static_assert(sizeof(FP_INT) == 4, "Wrong bytesize for FP_INT");
static_assert(sizeof(FP_UINT) == 4, "Wrong bytesize for FP_UINT");
static_assert(sizeof(FP_LONG) == 8, "Wrong bytesize for FP_LONG");
static_assert(sizeof(FP_ULONG) == 8, "Wrong bytesize for FP_ULONG");


static const FP_INT FP64_Shift = 32;
static const FP_LONG FractionMask =
        (1ll << FP64_Shift) - 1; // Space before INT64_C(1) needed because of hacky C++ code generator
static const FP_LONG IntegerMask = ~FractionMask;

// Constants
static const FP_LONG Zero = 0ll;
static const FP_LONG Neg1 = -1ll << FP64_Shift;
static const FP_LONG One = 1ll << FP64_Shift;
static const FP_LONG Two = 2ll << FP64_Shift;
static const FP_LONG Three = 3ll << FP64_Shift;
static const FP_LONG Four = 4ll << FP64_Shift;
static const FP_LONG Half = One >> 1;
static const FP_LONG Pi = 13493037705ll; //(FP_LONG)(Math.PI * 65536.0) << 16;
static const FP_LONG Pi2 = 26986075409ll;
static const FP_LONG PiHalf = 6746518852ll;
static const FP_LONG E = 11674931555ll;

static const FP_LONG MinValue = LLONG_MIN;
static const FP_LONG MaxValue = LLONG_MAX;

// Private constants
static const FP_LONG RCP_LN2 = 0x171547652ll; // 1.0 / log(2.0) ~= 1.4426950408889634
static const FP_LONG RCP_LOG2_E = 2977044471ll;  // 1.0 / log2(e) ~= 0.6931471805599453
static const FP_INT RCP_HALF_PI = 683565276; // 1.0 / (4.0 * 0.5 * Math.PI);  // the 4.0 factor converts directly to s2.30

/// <summary>
/// Converts an integer to a fixed-point value.
/// </summary>
static FP_LONG FP64_FromInt(FP_INT v) {
    return (FP_LONG) v << FP64_Shift;
}

/// <summary>
/// Converts a double to a fixed-point value.
/// </summary>
static FP_LONG FP64_FromDouble(double v) {
    return (FP_LONG) (v * 4294967296.0);
}

#define C0NST_FP64_FromDouble(v) ((FP_LONG) (v * 4294967296.0))

/// <summary>
/// Converts a float to a fixed-point value.
/// </summary>
static FP_LONG FP64_FromFloat(float v) {
    return (FP_LONG) (v * 4294967296.0f);
}

/// <summary>
/// Converts a fixed-point value into an integer by rounding it up to nearest integer.
/// </summary>
static FP_INT FP64_CeilToInt(FP_LONG v) {
    return (FP_INT) ((v + (One - 1)) >> FP64_Shift);
}

/// <summary>
/// Converts a fixed-point value into an integer by rounding it down to nearest integer.
/// </summary>
static FP_INT FP64_FloorToInt(FP_LONG v) {
    return (FP_INT) (v >> FP64_Shift);
}

/// <summary>
/// Converts a fixed-point value into an integer by rounding it to nearest integer.
/// </summary>
static FP_INT FP64_RoundToInt(FP_LONG v) {
    return (FP_INT) ((v + Half) >> FP64_Shift);
}

/// <summary>
/// Converts a fixed-point value into a double.
/// </summary>
// static double FP64_ToDouble(FP_LONG v) {
//     return (double) v * (1.0 / 4294967296.0);
// }

/// <summary>
/// Converts a FP value into a float.
/// </summary>
// static float FP64_ToFloat(FP_LONG v) {
//     return (float) v * (1.0f / 4294967296.0f);
// }

/// <summary>
/// Converts the value to a human readable string.
/// </summary>

/// <summary>
/// Returns the absolute (positive) value of x.
/// </summary>
static FP_LONG FP64_Abs(FP_LONG x) {
    // \note fails with LONG_MIN
    FP_LONG mask = x >> 63;
    return (x + mask) ^ mask;
}

/// <summary>
/// Negative absolute value (returns -abs(x)).
/// </summary>
static FP_LONG FP64_Nabs(FP_LONG x) {
    return -FP64_Abs(x);
}

/// <summary>
/// Round up to nearest integer.
/// </summary>
static FP_LONG FP64_Ceil(FP_LONG x) {
    return (x + FractionMask) & IntegerMask;
}

/// <summary>
/// Round down to nearest integer.
/// </summary>
static FP_LONG FP64_Floor(FP_LONG x) {
    return x & IntegerMask;
}

/// <summary>
/// Round to nearest integer.
/// </summary>
static FP_LONG FP64_Round(FP_LONG x) {
    return (x + Half) & IntegerMask;
}

/// <summary>
/// Returns the fractional part of x. Equal to 'x - floor(x)'.
/// </summary>
static FP_LONG FP64_Fract(FP_LONG x) {
    return x & FractionMask;
}

/// <summary>
/// Returns the minimum of the two values.
/// </summary>
static FP_LONG FP64_Min(FP_LONG a, FP_LONG b) {
    return (a < b) ? a : b;
}

/// <summary>
/// Returns the maximum of the two values.
/// </summary>
static FP_LONG FP64_Max(FP_LONG a, FP_LONG b) {
    return (a > b) ? a : b;
}

/// <summary>
/// Returns the value clamped between min and max.
/// </summary>
static FP_LONG FP64_Clamp(FP_LONG a, FP_LONG min, FP_LONG max) {
    return (a > max) ? max : (a < min) ? min : a;
}

/// <summary>
/// Returns the sign of the value (-1 if negative, 0 if zero, 1 if positive).
/// </summary>
static FP_INT FP64_Sign(FP_LONG x) {
    // https://stackoverflow.com/questions/14579920/fast-sign-of-integer-in-c/14612418#14612418
    return (FP_INT) ((x >> 63) | (FP_LONG) (((FP_ULONG) -x) >> 63));
}

/// <summary>
/// Adds the two FP numbers together.
/// </summary>
static FP_LONG FP64_Add(FP_LONG a, FP_LONG b) {
    return a + b;
}

/// <summary>
/// Subtracts the two FP numbers from each other.
/// </summary>
static FP_LONG FP64_Sub(FP_LONG a, FP_LONG b) {
    return a - b;
}

/// <summary>
/// Multiplies two FP values together.
/// </summary>
static FP_LONG FP64_Mul(FP_LONG a, FP_LONG b) {
#if defined(__SIZEOF_INT128__) && !defined(__ppc64le__)
    return (FP_LONG)(((__int128_t)a * (__int128_t)b) >> FP64_Shift);
#endif
    FP_LONG ai = a >> FP64_Shift;
    FP_LONG af = (a & FractionMask);
    FP_LONG bi = b >> FP64_Shift;
    FP_LONG bf = (b & FractionMask);
    return LogicalShiftRight(af * bf, FP64_Shift) + ai * b + af * bi;
}

static FP_INT FP64_MulIntLongLow(FP_INT a, FP_LONG b) {
    //FP_ASSERT(a >= 0);
    FP_INT bi = (FP_INT) (b >> FP64_Shift);
    FP_LONG bf = b & FractionMask;
    return (FP_INT) LogicalShiftRight(a * bf, FP64_Shift) + a * bi;
}

static FP_LONG FP64_MulIntLongLong(FP_INT a, FP_LONG b) {
    //FP_ASSERT(a >= 0);
    FP_LONG bi = b >> FP64_Shift;
    FP_LONG bf = b & FractionMask;
    return LogicalShiftRight(a * bf, FP64_Shift) + a * bi;
}

/// <summary>
/// Linearly interpolate from a to b by t.
/// </summary>
static FP_LONG FP64_Lerp(FP_LONG a, FP_LONG b, FP_LONG t) {
    return FP64_Mul(b - a, t) + a;
}

static FP_INT FP64_Nlz(FP_ULONG x) {
    return __builtin_clzll(x); // Use the gcc built-in, it's much faster
}

// Divides a 128 bit int (u1:u0) by a 64 bit int (v)
static FP_LONG Div128_64(FP_LONG u1, FP_LONG u0, FP_LONG v) {
    FP_LONG result;
#if defined (__ppc64le__)
    FP_LONG q1;
    FP_LONG q2;
    FP_LONG minus_r1;
    FP_LONG r2;
    FP_LONG q2dv;
    FP_LONG total_r;
    __asm__(
        // Algorithm extracted from page 82 of OPF_PowerISA_v3.1C.pdf
        // https://wiki.raptorcs.com/w/images/5/5f/OPF_PowerISA_v3.1C.pdf
        // Algorithm step 1: q1 <- divdeu Dh, Dv
        // PPC64 'dide %[dest], %[num_high_part], %[divisor]'
        // this part uses the signed instruction set
        // which is the same case as the x86 part
        "divde %[q1], %[Dh], %[Dv]\n\t"

        // Algorithm step 3: q2 <- divwu Dl, Dv
        // PowerPC 'divdu %[dest], %[num_low_part], %[divisor]'
        // We use the unsigned version as the right part is always
        // positive by definition
        "divdu %[q2], %[Dl], %[Dv]\n\t"

        // For Algorithm step 2: r1 <- -(q1 × Dv)
        // This operation returns -r1 as this is just the mult
        // without changing the sign. we will use subf in the future
        "mulld %[minus_r1], %[q1], %[Dv]\n\t"

        // For Algorithm step 4: r2 <- Dl - (q2 × Dv)
        // First, calculate q2_prod_temp = q2 * Dv
        "mulld %[q2dv], %[q2], %[Dv]\n\t" // q2 * Dv

        // Then, r2 = Dl - q2dv
        // so this is still step 4
        "subf %[r2], %[q2dv], %[Dl]\n\t"

        // Algorithm step 5: Q <- q1 + q2
        "add %[result], %[q1], %[q2]\n\t"

        // Algorithm step 6: R <- r1 + r2
        // As per the provided assembly, R is calculated as r2 - (q1 * Dv).
	// Before we calculated q1 * Dv so now we subf to change the sign of minus_r1
        "subf %[total_r], %[minus_r1], %[r2]\n\t"

        // Algorithm step 7: if (R < r2) | (R >= Dv) then adjust Q and R.
        // PowerPC 'cmplw cr_field, %[val1], %[val2]' (compares val1 and val2,
        // sets cr_field)
        "cmpld cr0, %[total_r], %[r2]\n\t"
        // PowerPC 'blt target_cr_field, label' (branch if cr_field indicates
        // less-than)
        "blt cr0, 0f\n\t" // Branch to adjustment if R < r2 (label 0, forward)

        "cmpld cr0, %[total_r], %[Dv]\n\t"
        // If R < Dv, the condition (R >= Dv) is false. So, branch to skip
        // adjustment. If R >= Dv, it means R must be adjusted, so fall through
        // to adjustment code.
        "blt cr0, 1f\n\t" // Branch to skip adjustment if R < Dv (label 1,
                          // forward)

        "0:\n\t" // Adjustment label
                 // Q <- Q + 1
                 // PowerPC 'addi %[dest_src], %[src], immediate_val'
        "addi %[result], %[result], 1\n\t"
        // R <- R - Dv
        // "subf %[R_o], %[Dv_i], %[R_o]\n\t" // Computes: R_o = R_o - Dv_i
        // we don't use this part as we only need Q

        "1:\n\t" // Skip adjustment label / end of conditional adjustment

        // === Operand Constraints ===
        // Output Operands:
        // "+r": A register that is both read and written (read-write).
        // "=&r": A write-only register that is written before all inputs are
        // read.
        // c code doesn't need total_r as the remainder is only needed in the contidition
        // so total_r is also write-only
        : [result] "+r"(result), [total_r] "=&r"(total_r),
          [q1] "=&r"(q1), [q2] "=&r"(q2),
          [minus_r1] "=&r"(minus_r1), [r2] "=&r"(r2),
          [q2dv] "=&r"(q2dv)
        // Input Operands:
        // [Dh_i], [Dl_i], [Dv_i] are the inputs to the division.
        : [Dh] "r"(u1), [Dl] "r"(u0), [Dv] "r"(v)
        // Clobbered Registers:
        // cr0 (Condition Register field 0) is modified by 'cmplw' and used by 'blt'.
        : "cr0"        
     );

    return result;
#else
    uint64_t remainder;
    __asm__ (
            //"cqto\n\t"
            "idivq %[v]" : "=a"(result), "=d"(remainder) : [v] "r"(v), "a"(u0), "d"(u1)
    );
    return result;
#endif
}

/// <summary>
/// Divides two FP values.
/// </summary>
static FP_LONG FP64_DivPrecise(FP_LONG arg_a, FP_LONG arg_b) {
#ifdef __SIZEOF_INT128__
    return Div128_64(arg_a >> FP64_Shift, arg_a << FP64_Shift, arg_b);
    //return (FP_LONG)(((__int128)arg_a << FP64_Shift) / (__int128)arg_b);
#endif
    // From http://www.hackersdelight.org/hdcodetxt/divlu.c.txt

    FP_LONG sign_dif = arg_a ^ arg_b;

    static const FP_ULONG b = 0x100000000ll; // Number base (32 bits)
    FP_ULONG abs_arg_a = (FP_ULONG) ((arg_a < 0) ? -arg_a : arg_a);
    FP_ULONG u1 = abs_arg_a >> 32;
    FP_ULONG u0 = abs_arg_a << 32;
    FP_ULONG v = (FP_ULONG) ((arg_b < 0) ? -arg_b : arg_b);

    // Overflow?
    if (u1 >= v) {
        //rem = 0;
        return 0x7fffffffffffffffll;
    }

    // FP64_Shift amount for norm
    FP_INT s = FP64_Nlz(v); // 0 <= s <= 63
    v = v << s; // Normalize the divisor
    FP_ULONG vn1 = v >> 32; // Break the divisor into two 32-bit digits
    FP_ULONG vn0 = v & 0xffffffffll;

    FP_ULONG un32 = (u1 << s) | (u0 >> (64 - s)) & (FP_ULONG) ((FP_LONG) -s >> 63);
    FP_ULONG un10 = u0 << s; // FP64_Shift dividend left

    FP_ULONG un1 = un10 >> 32; // Break the right half of dividend into two digits
    FP_ULONG un0 = un10 & 0xffffffffll;

    // Compute the first quotient digit, q1
    FP_ULONG q1 = un32 / vn1;
    FP_ULONG rhat = un32 - q1 * vn1;
    do {
        if ((q1 >= b) || ((q1 * vn0) > (b * rhat + un1))) {
            q1 = q1 - 1;
            rhat = rhat + vn1;
        } else break;
    } while (rhat < b);

    FP_ULONG un21 = un32 * b + un1 - q1 * v; // Multiply and subtract

    // Compute the second quotient digit, q0
    FP_ULONG q0 = un21 / vn1;
    rhat = un21 - q0 * vn1;
    do {
        if ((q0 >= b) || ((q0 * vn0) > (b * rhat + un0))) {
            q0 = q0 - 1;
            rhat = rhat + vn1;
        } else break;
    } while (rhat < b);

    // Calculate the remainder
    // FP_ULONG r = (un21 * b + un0 - q0 * v) >> s;
    // rem = (FP_LONG)r;

    FP_ULONG ret = q1 * b + q0;
    return (sign_dif < 0) ? -(FP_LONG) ret : (FP_LONG) ret;
}

/// <summary>
/// Calculates division approximation.
/// </summary>
static FP_LONG FP64_Div(FP_LONG a, FP_LONG b) {
#if defined(__SIZEOF_INT128__) && !defined(__ppc64le__)// Same as precise, because it's fast and precise
    /// This instruction might not work, because you cannot link C library when compiling for kernel, and
    /// this uses __idiv3 instruction
    return (FP_LONG)(((__int128)a << FP64_Shift) / (__int128)b);
#endif
    if (b == MinValue || b == 0) {
        InvalidArgument("Fixed64::Div", "b", b);
        return 0;
    }

    // Handle negative values.
    FP_INT sign = (b < 0) ? -1 : 1;
    b *= sign;

    // Normalize input into [1.0, 2.0( range (convert to s2.30).
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) b);
    FP_INT n = (FP_INT) ShiftRight(b, offset + 2);
    static const FP_INT ONE = (1 << 30);
    //FP_ASSERT(n >= ONE);

    // Polynomial approximation.
    FP_INT res = RcpPoly4Lut8(n - ONE);

    // Apply exponent, convert back to s32.32.
    FP_LONG y = FP64_MulIntLongLong(res, a) << 2;
    return ShiftRight(sign * y, offset);
}

/// <summary>
/// Calculates division approximation.
/// </summary>
static FP_LONG FP64_DivFast(FP_LONG a, FP_LONG b) {
    if (b == MinValue || b == 0) {
        InvalidArgument("Fixed64::DivFast", "b", b);
        return 0;
    }

    // Handle negative values.
    FP_INT sign = (b < 0) ? -1 : 1;
    b *= sign;

    // Normalize input into [1.0, 2.0( range (convert to s2.30).
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) b);
    FP_INT n = (FP_INT) ShiftRight(b, offset + 2);
    static const FP_INT ONE = (1 << 30);
    //FP_ASSERT(n >= ONE);

    // Polynomial approximation.
    FP_INT res = RcpPoly6(n - ONE);

    // Apply exponent, convert back to s32.32.
    FP_LONG y = FP64_MulIntLongLong(res, a) << 2;
    return ShiftRight(sign * y, offset);
}

/// <summary>
/// Calculates division approximation.
/// </summary>
static FP_LONG FP64_DivFastest(FP_LONG a, FP_LONG b) {
    if (b == MinValue || b == 0) {
        InvalidArgument("Fixed64::DivFastest", "b", b);
        return 0;
    }

    // Handle negative values.
    FP_INT sign = (b < 0) ? -1 : 1;
    b *= sign;

    // Normalize input into [1.0, 2.0( range (convert to s2.30).
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) b);
    FP_INT n = (FP_INT) ShiftRight(b, offset + 2);
    static const FP_INT ONE = (1 << 30);
    //FP_ASSERT(n >= ONE);

    // Polynomial approximation.
    FP_INT res = RcpPoly4(n - ONE);

    // Apply exponent, convert back to s32.32.
    FP_LONG y = FP64_MulIntLongLong(res, a) << 2;
    return ShiftRight(sign * y, offset);
}

/// <summary>
/// Divides two FP values and returns the modulus.
/// </summary>
static FP_LONG FP64_Mod(FP_LONG a, FP_LONG b) {
    if (b == 0) {
        InvalidArgument("Fixed64::Mod", "b", b);
        return 0;
    }

    return a % b;
}

/// <summary>
/// Calculates the square root of the given number.
/// </summary>
static FP_LONG FP64_SqrtPrecise(FP_LONG a) {
    // Adapted from https://github.com/chmike/fpsqrt
    if (a <= 0) {
        if (a < 0)
            InvalidArgument("Fixed64::SqrtPrecise", "a", a);
        return 0;
    }

    FP_ULONG r = (FP_ULONG) a;
    FP_ULONG b = 0x4000000000000000ll;
    FP_ULONG q = 0ll;
    while (b > 0x40ll) {
        FP_ULONG t = q + b;
        if (r >= t) {
            r -= t;
            q = t + b;
        }
        r <<= 1;
        b >>= 1;
    }
    q >>= 16;
    return (FP_LONG) q;
}

static FP_LONG FP64_Sqrt(FP_LONG x) {
    // Return 0 for all non-positive values.
    if (x <= 0) {
        if (x < 0)
            InvalidArgument("Fixed64::Sqrt", "x", x);
        return 0;
    }

    // Constants (s2.30).
    static const FP_INT ONE = (1 << 30);
    static const FP_INT SQRT2 = 1518500249; // sqrt(2.0)

    // Normalize input into [1.0, 2.0( range (as s2.30).
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    //FP_ASSERT(n >= ONE);
    FP_INT y = SqrtPoly3Lut8(n - ONE);

    // Divide offset by 2 (to get sqrt), compute adjust value for odd exponents.
    FP_INT adjust = ((offset & 1) != 0) ? SQRT2 : ONE;
    offset = offset >> 1;

    // Apply exponent, convert back to s32.32.
    FP_LONG yr = (FP_LONG) Qmul30(adjust, y) << 2;
    return (offset >= 0) ? (yr << offset) : (yr >> -offset);
}

static FP_LONG FP64_SqrtFast(FP_LONG x) {
    // Return 0 for all non-positive values.
    if (x <= 0) {
        if (x < 0)
            InvalidArgument("Fixed64::SqrtFast", "x", x);
        return 0;
    }

    // Constants (s2.30).
    static const FP_INT ONE = (1 << 30);
    static const FP_INT SQRT2 = 1518500249; // sqrt(2.0)

    // Normalize input into [1.0, 2.0( range (as s2.30).
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    //FP_ASSERT(n >= ONE);
    FP_INT y = SqrtPoly4(n - ONE);

    // Divide offset by 2 (to get sqrt), compute adjust value for odd exponents.
    FP_INT adjust = ((offset & 1) != 0) ? SQRT2 : ONE;
    offset = offset >> 1;

    // Apply exponent, convert back to s32.32.
    FP_LONG yr = (FP_LONG) Qmul30(adjust, y) << 2;
    return (offset >= 0) ? (yr << offset) : (yr >> -offset);
}

static FP_LONG FP64_SqrtFastest(FP_LONG x) {
    // Return 0 for all non-positive values.
    if (x <= 0) {
        if (x < 0)
            InvalidArgument("Fixed64::SqrtFastest", "x", x);
        return 0;
    }

    // Constants (s2.30).
    static const FP_INT ONE = (1 << 30);
    static const FP_INT SQRT2 = 1518500249; // sqrt(2.0)

    // Normalize input into [1.0, 2.0( range (as s2.30).
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    //FP_ASSERT(n >= ONE);
    FP_INT y = SqrtPoly3(n - ONE);

    // Divide offset by 2 (to get sqrt), compute adjust value for odd exponents.
    FP_INT adjust = ((offset & 1) != 0) ? SQRT2 : ONE;
    offset = offset >> 1;

    // Apply exponent, convert back to s32.32.
    FP_LONG yr = (FP_LONG) Qmul30(adjust, y) << 2;
    return (offset >= 0) ? (yr << offset) : (yr >> -offset);
}

/// <summary>
/// Calculates the reciprocal square root.
/// </summary>
static FP_LONG FP64_RSqrt(FP_LONG x) {
    // Return 0 for invalid values
    if (x <= 0) {
        InvalidArgument("Fixed64::RSqrt", "x", x);
        return 0;
    }

    // Constants (s2.30).
    static const FP_INT ONE = (1 << 30);
    static const FP_INT HALF_SQRT2 = 759250125; // 0.5 * sqrt(2.0)

    // Normalize input into [1.0, 2.0( range (as s2.30).
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    //FP_ASSERT(n >= ONE);
    FP_INT y = RSqrtPoly3Lut16(n - ONE);

    // Divide offset by 2 (to get sqrt), compute adjust value for odd exponents.
    FP_INT adjust = ((offset & 1) != 0) ? HALF_SQRT2 : ONE;
    offset = offset >> 1;

    // Apply exponent, convert back to s32.32.
    FP_LONG yr = (FP_LONG) Qmul30(adjust, y) << 2;
    return (offset >= 0) ? (yr >> offset) : (yr << -offset);
}

/// <summary>
/// Calculates the reciprocal square root.
/// </summary>
static FP_LONG FP64_RSqrtFast(FP_LONG x) {
    // Return 0 for invalid values
    if (x <= 0) {
        InvalidArgument("Fixed64::RSqrtFast", "x", x);
        return 0;
    }

    // Constants (s2.30).
    static const FP_INT ONE = (1 << 30);
    static const FP_INT HALF_SQRT2 = 759250125; // 0.5 * sqrt(2.0)

    // Normalize input into [1.0, 2.0( range (as s2.30).
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    //FP_ASSERT(n >= ONE);
    FP_INT y = RSqrtPoly5(n - ONE);

    // Divide offset by 2 (to get sqrt), compute adjust value for odd exponents.
    FP_INT adjust = ((offset & 1) != 0) ? HALF_SQRT2 : ONE;
    offset = offset >> 1;

    // Apply exponent, convert back to s32.32.
    FP_LONG yr = (FP_LONG) Qmul30(adjust, y) << 2;
    return (offset >= 0) ? (yr >> offset) : (yr << -offset);
}

/// <summary>
/// Calculates the reciprocal square root.
/// </summary>
static FP_LONG FP64_RSqrtFastest(FP_LONG x) {
    // Return 0 for invalid values
    if (x <= 0) {
        InvalidArgument("Fixed64::RSqrtFastest", "x", x);
        return 0;
    }

    // Constants (s2.30).
    static const FP_INT ONE = (1 << 30);
    static const FP_INT HALF_SQRT2 = 759250125; // 0.5 * sqrt(2.0)

    // Normalize input into [1.0, 2.0( range (as s2.30).
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    //FP_ASSERT(n >= ONE);
    FP_INT y = RSqrtPoly3(n - ONE);

    // Divide offset by 2 (to get sqrt), compute adjust value for odd exponents.
    FP_INT adjust = ((offset & 1) != 0) ? HALF_SQRT2 : ONE;
    offset = offset >> 1;

    // Apply exponent, convert back to s32.32.
    FP_LONG yr = (FP_LONG) Qmul30(adjust, y) << 2;
    return (offset >= 0) ? (yr >> offset) : (yr << -offset);
}

/// <summary>
/// Calculates reciprocal approximation.
/// </summary>
static FP_LONG FP64_Rcp(FP_LONG x) {
    if (x == MinValue || x == 0) {
        InvalidArgument("Fixed64::Rcp", "x", x);
        return 0;
    }

    // Handle negative values.
    FP_INT sign = (x < 0) ? -1 : 1;
    x *= sign;

    // Normalize input into [1.0, 2.0( range (convert to s2.30).
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) ShiftRight(x, offset + 2);
    static const FP_INT ONE = (1 << 30);
    //FP_ASSERT(n >= ONE);

    // Polynomial approximation.
    FP_INT res = RcpPoly4Lut8(n - ONE);
    FP_LONG y = (FP_LONG) (sign * res) << 2;

    // Apply exponent, convert back to s32.32.
    return ShiftRight(y, offset);
}

/// <summary>
/// Calculates reciprocal approximation.
/// </summary>
static FP_LONG FP64_RcpFast(FP_LONG x) {
    if (x == MinValue || x == 0) {
        InvalidArgument("Fixed64::RcpFast", "x", x);
        return 0;
    }

    // Handle negative values.
    FP_INT sign = (x < 0) ? -1 : 1;
    x *= sign;

    // Normalize input into [1.0, 2.0( range (convert to s2.30).
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) ShiftRight(x, offset + 2);
    static const FP_INT ONE = (1 << 30);
    //FP_ASSERT(n >= ONE);

    // Polynomial approximation.
    FP_INT res = RcpPoly6(n - ONE);
    FP_LONG y = (FP_LONG) (sign * res) << 2;

    // Apply exponent, convert back to s32.32.
    return ShiftRight(y, offset);
}

/// <summary>
/// Calculates reciprocal approximation.
/// </summary>
static FP_LONG FP64_RcpFastest(FP_LONG x) {
    if (x == MinValue || x == 0) {
        InvalidArgument("Fixed64::RcpFastest", "x", x);
        return 0;
    }

    // Handle negative values.
    FP_INT sign = (x < 0) ? -1 : 1;
    x *= sign;

    // Normalize input into [1.0, 2.0( range (convert to s2.30).
    static const FP_INT ONE = (1 << 30);
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) ShiftRight(x, offset + 2);
    //FP_INT n = (FP_INT)(((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);

    // Polynomial approximation.
    FP_INT res = RcpPoly4(n - ONE);
    FP_LONG y = (FP_LONG) (sign * res) << 2;

    // Apply exponent, convert back to s32.32.
    return ShiftRight(y, offset);
}

/// <summary>
/// Calculates the base 2 exponent.
/// </summary>
static FP_LONG FP64_Exp2(FP_LONG x) {
    // Handle values that would under or overflow.
    if (x >= 32 * One) return MaxValue;
    if (x <= -32 * One) return 0;

    // Compute exp2 for fractional part.
    FP_INT k = (FP_INT) ((x & FractionMask) >> 2);
    FP_LONG y = (FP_LONG) Exp2Poly5(k) << 2;

    // Combine integer and fractional result, and convert back to s32.32.
    FP_INT intPart = (FP_INT) (x >> FP64_Shift);
    return (intPart >= 0) ? (y << intPart) : (y >> -intPart);
}

/// <summary>
/// Calculates the base 2 exponent.
/// </summary>
static FP_LONG FP64_Exp2Fast(FP_LONG x) {
    // Handle values that would under or overflow.
    if (x >= 32 * One) return MaxValue;
    if (x <= -32 * One) return 0;

    // Compute exp2 for fractional part.
    FP_INT k = (FP_INT) ((x & FractionMask) >> 2);
    FP_LONG y = (FP_LONG) Exp2Poly4(k) << 2;

    // Combine integer and fractional result, and convert back to s32.32.
    FP_INT intPart = (FP_INT) (x >> FP64_Shift);
    return (intPart >= 0) ? (y << intPart) : (y >> -intPart);
}

/// <summary>
/// Calculates the base 2 exponent.
/// </summary>
static FP_LONG Exp2Fastest(FP_LONG x) {
    // Handle values that would under or overflow.
    if (x >= 32 * One) return MaxValue;
    if (x <= -32 * One) return 0;

    // Compute exp2 for fractional part.
    FP_INT k = (FP_INT) ((x & FractionMask) >> 2);
    FP_LONG y = (FP_LONG) Exp2Poly3(k) << 2;

    // Combine integer and fractional result, and convert back to s32.32.
    FP_INT intPart = (FP_INT) (x >> FP64_Shift);
    return (intPart >= 0) ? (y << intPart) : (y >> -intPart);
}

static FP_LONG FP64_Exp(FP_LONG x) {
    // e^x == 2^(x / ln(2))
    return FP64_Exp2(FP64_Mul(x, RCP_LN2));
}

static FP_LONG FP64_ExpFast(FP_LONG x) {
    // e^x == 2^(x / ln(2))
    return FP64_Exp2Fast(FP64_Mul(x, RCP_LN2));
}

static FP_LONG FP64_ExpFastest(FP_LONG x) {
    // e^x == 2^(x / ln(2))
    return Exp2Fastest(FP64_Mul(x, RCP_LN2));
}

// Natural logarithm (base e).
static FP_LONG FP64_Log(FP_LONG x) {
    // Return 0 for invalid values
    if (x <= 0) {
        InvalidArgument("Fixed64::Log", "x", x);
        return 0;
    }

    // Normalize value to range [1.0, 2.0( as s2.30 and extract exponent.
    static const FP_INT ONE = (1 << 30);
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    //FP_ASSERT(n >= ONE);
    FP_LONG y = (FP_LONG) LogPoly5Lut8(n - ONE) << 2;

    // Combine integer and fractional parts (into s32.32).
    return (FP_LONG) offset * RCP_LOG2_E + y;
}

static FP_LONG FP64_LogFast(FP_LONG x) {
    // Return 0 for invalid values
    if (x <= 0) {
        InvalidArgument("Fixed64::LogFast", "x", x);
        return 0;
    }

    // Normalize value to range [1.0, 2.0( as s2.30 and extract exponent.
    static const FP_INT ONE = (1 << 30);
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    //FP_ASSERT(n >= ONE);
    FP_LONG y = (FP_LONG) LogPoly3Lut8(n - ONE) << 2;

    // Combine integer and fractional parts (into s32.32).
    return (FP_LONG) offset * RCP_LOG2_E + y;
}

static FP_LONG FP64_LogFastest(FP_LONG x) {
    // Return 0 for invalid values
    if (x <= 0) {
        InvalidArgument("Fixed64::LogFastest", "x", x);
        return 0;
    }

    // Normalize value to range [1.0, 2.0( as s2.30 and extract exponent.
    static const FP_INT ONE = (1 << 30);
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    //FP_ASSERT(n >= ONE);
    FP_LONG y = (FP_LONG) LogPoly5(n - ONE) << 2;

    // Combine integer and fractional parts (into s32.32).
    return (FP_LONG) offset * RCP_LOG2_E + y;
}

static FP_LONG FP64_Log2(FP_LONG x) {
    // Return 0 for invalid values
    if (x <= 0) {
        InvalidArgument("Fixed64::Log2", "x", x);
        return 0;
    }

    // Normalize value to range [1.0, 2.0( as s2.30 and extract exponent.
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);

    // Polynomial approximation of mantissa.
    static const FP_INT ONE = (1 << 30);
    //FP_ASSERT(n >= ONE);
    FP_LONG y = (FP_LONG) Log2Poly4Lut16(n - ONE) << 2;

    // Combine integer and fractional parts (into s32.32).
    return ((FP_LONG) offset << FP64_Shift) + y;
}

static FP_LONG FP64_Log2Fast(FP_LONG x) {
    // Return 0 for invalid values
    if (x <= 0) {
        InvalidArgument("Fixed64::Log2Fast", "x", x);
        return 0;
    }

    // Normalize value to range [1.0, 2.0( as s2.30 and extract exponent.
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);

    // Polynomial approximation of mantissa.
    static const FP_INT ONE = (1 << 30);
    //FP_ASSERT(n >= ONE);
    FP_LONG y = (FP_LONG) Log2Poly3Lut16(n - ONE) << 2;

    // Combine integer and fractional parts (into s32.32).
    return ((FP_LONG) offset << FP64_Shift) + y;
}

static FP_LONG FP64_Log2Fastest(FP_LONG x) {
    // Return 0 for invalid values
    if (x <= 0) {
        InvalidArgument("Fixed64::Log2Fastest", "x", x);
        return 0;
    }

    // Normalize value to range [1.0, 2.0( as s2.30 and extract exponent.
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);

    // Polynomial approximation of mantissa.
    static const FP_INT ONE = (1 << 30);
    //FP_ASSERT(n >= ONE);
    FP_LONG y = (FP_LONG) Log2Poly5(n - ONE) << 2;

    // Combine integer and fractional parts (into s32.32).
    return ((FP_LONG) offset << FP64_Shift) + y;
}

/// <summary>
/// Calculates x to the power of the exponent.
/// </summary>
static FP_LONG FP64_Pow(FP_LONG x, FP_LONG exponent) {
    // Return 0 for invalid values
    if (x <= 0) {
        if (x < 0)
            InvalidArgument("Fixed64::Pow", "x", x);
        return 0;
    }

    return FP64_Exp(FP64_Mul(exponent, FP64_Log(x)));
}

/// <summary>
/// Calculates x to the power of the exponent.
/// </summary>
static FP_LONG FP64_PowFast(FP_LONG x, FP_LONG exponent) {
    // Return 0 for invalid values
    if (x <= 0) {
        if (x < 0)
            InvalidArgument("Fixed64::PowFast", "x", x);
        return 0;
    }

    return FP64_ExpFast(FP64_Mul(exponent, FP64_LogFast(x)));
}

/// <summary>
/// Calculates x to the power of the exponent.
/// </summary>
static FP_LONG FP64_PowFastest(FP_LONG x, FP_LONG exponent) {
    // Return 0 for invalid values
    if (x <= 0) {
        if (x < 0)
            InvalidArgument("Fixed64::PowFastest", "x", x);
        return 0;
    }

    return FP64_ExpFastest(FP64_Mul(exponent, FP64_LogFastest(x)));
}

static FP_INT FP64_UnitSin(FP_INT z) {
    // See: http://www.coranac.com/2009/07/sines/

    // Handle quadrants 1 and 2 by mirroring the [1, 3] range to [-1, 1] (by calculating 2 - z).
    // The if condition uses the fact that for the quadrants of interest are 0b01 and 0b10 (top two bits are different).
    if ((z ^ (z << 1)) < 0)
        z = (1 << 31) - z;

    // Now z is in range [-1, 1].
    //static const FP_INT ONE = (1 << 30);
    //FP_ASSERT((z >= -ONE) && (z <= ONE));

    // Polynomial approximation.
    FP_INT zz = Qmul30(z, z);
    FP_INT res = Qmul30(SinPoly4(zz), z);

    // Return s2.30 value.
    return res;
}

static FP_INT FP64_UnitSinFast(FP_INT z) {
    // See: http://www.coranac.com/2009/07/sines/

    // Handle quadrants 1 and 2 by mirroring the [1, 3] range to [-1, 1] (by calculating 2 - z).
    // The if condition uses the fact that for the quadrants of interest are 0b01 and 0b10 (top two bits are different).
    if ((z ^ (z << 1)) < 0)
        z = (1 << 31) - z;

    // Now z is in range [-1, 1].
    //static const FP_INT ONE = (1 << 30);
    //FP_ASSERT((z >= -ONE) && (z <= ONE));

    // Polynomial approximation.
    FP_INT zz = Qmul30(z, z);
    FP_INT res = Qmul30(SinPoly3(zz), z);

    // Return s2.30 value.
    return res;
}

static FP_INT FP64_UnitSinFastest(FP_INT z) {
    // See: http://www.coranac.com/2009/07/sines/

    // Handle quadrants 1 and 2 by mirroring the [1, 3] range to [-1, 1] (by calculating 2 - z).
    // The if condition uses the fact that for the quadrants of interest are 0b01 and 0b10 (top two bits are different).
    if ((z ^ (z << 1)) < 0)
        z = (1 << 31) - z;

    // Now z is in range [-1, 1].
    //static const FP_INT ONE = (1 << 30);
    //FP_ASSERT((z >= -ONE) && (z <= ONE));

    // Polynomial approximation.
    FP_INT zz = Qmul30(z, z);
    FP_INT res = Qmul30(SinPoly2(zz), z);

    // Return s2.30 value.
    return res;
}

static FP_LONG FP64_Sin(FP_LONG x) {
    // Map [0, 2pi] to [0, 4] (as s2.30).
    // This also wraps the values into one period.
    FP_INT z = FP64_MulIntLongLow(RCP_HALF_PI, x);

    // Compute sine and convert to s32.32.
    return (FP_LONG) FP64_UnitSin(z) << 2;
}

static FP_LONG FP64_SinFast(FP_LONG x) {
    // Map [0, 2pi] to [0, 4] (as s2.30).
    // This also wraps the values into one period.
    FP_INT z = FP64_MulIntLongLow(RCP_HALF_PI, x);

    // Compute sine and convert to s32.32.
    return (FP_LONG) FP64_UnitSinFast(z) << 2;
}

static FP_LONG FP64_SinFastest(FP_LONG x) {
    // Map [0, 2pi] to [0, 4] (as s2.30).
    // This also wraps the values into one period.
    FP_INT z = FP64_MulIntLongLow(RCP_HALF_PI, x);

    // Compute sine and convert to s32.32.
    return (FP_LONG) FP64_UnitSinFastest(z) << 2;
}

static FP_LONG FP64_Cos(FP_LONG x) {
    return FP64_Sin(x + PiHalf);
}

static FP_LONG FP64_CosFast(FP_LONG x) {
    return FP64_SinFast(x + PiHalf);
}

static FP_LONG FP64_CosFastest(FP_LONG x) {
    return FP64_SinFastest(x + PiHalf);
}

static FP_LONG FP64_Tan(FP_LONG x) {
    FP_INT z = FP64_MulIntLongLow(RCP_HALF_PI, x);
    FP_LONG sinX = (FP_LONG) FP64_UnitSin(z) << 32;
    FP_LONG cosX = (FP_LONG) FP64_UnitSin(z + (1 << 30)) << 32;
    return FP64_Div(sinX, cosX);
}

static FP_LONG FP64_TanFast(FP_LONG x) {
    FP_INT z = FP64_MulIntLongLow(RCP_HALF_PI, x);
    FP_LONG sinX = (FP_LONG) FP64_UnitSinFast(z) << 32;
    FP_LONG cosX = (FP_LONG) FP64_UnitSinFast(z + (1 << 30)) << 32;
    return FP64_DivFast(sinX, cosX);
}

static FP_LONG FP64_TanFastest(FP_LONG x) {
    FP_INT z = FP64_MulIntLongLow(RCP_HALF_PI, x);
    FP_LONG sinX = (FP_LONG) FP64_UnitSinFastest(z) << 32;
    FP_LONG cosX = (FP_LONG) FP64_UnitSinFastest(z + (1 << 30)) << 32;
    return FP64_DivFastest(sinX, cosX);
}

static FP_INT FP64_Atan2Div(FP_LONG y, FP_LONG x) {
    //FP_ASSERT(y >= 0 && x > 0 && x >= y);

    // Normalize input into [1.0, 2.0( range (convert to s2.30).
    static const FP_INT ONE = (1 << 30);
    //static const FP_INT HALF = (1 << 29);
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    FP_INT k = n - ONE;

    // Polynomial approximation of reciprocal.
    FP_INT oox = RcpPoly4Lut8(k);
    //FP_ASSERT(oox >= HALF && oox <= ONE);

    // Apply exponent and multiply.
    FP_LONG yr = (offset >= 0) ? (y >> offset) : (y << -offset);
    return Qmul30((FP_INT) (yr >> 2), oox);
}

static FP_LONG FP64_Atan2(FP_LONG y, FP_LONG x) {
    // See: https://www.dsprelated.com/showarticle/1052.php

    if (x == 0) {
        if (y > 0) return PiHalf;
        if (y < 0) return -PiHalf;

        //InvalidArgument("Fixed64::Atan2", "y, x", y, x);
        return 0;
    }

    // \note these round negative numbers slightly
    FP_LONG nx = x ^ (x >> 63);
    FP_LONG ny = y ^ (y >> 63);
    FP_LONG negMask = ((x ^ y) >> 63);

    if (nx >= ny) {
        FP_INT k = FP64_Atan2Div(ny, nx);
        FP_INT z = AtanPoly5Lut8(k);
        FP_LONG angle = negMask ^ ((FP_LONG) z << 2);
        if (x > 0) return angle;
        if (y >= 0) return angle + Pi;
        return angle - Pi;
    } else {
        FP_INT k = FP64_Atan2Div(nx, ny);
        FP_INT z = AtanPoly5Lut8(k);
        FP_LONG angle = negMask ^ ((FP_LONG) z << 2);
        return ((y > 0) ? PiHalf : -PiHalf) - angle;
    }
}

static FP_INT FP64_Atan2DivFast(FP_LONG y, FP_LONG x) {
    //FP_ASSERT(y >= 0 && x > 0 && x >= y);

    // Normalize input into [1.0, 2.0( range (convert to s2.30).
    static const FP_INT ONE = (1 << 30);
    //static const FP_INT HALF = (1 << 29);
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    FP_INT k = n - ONE;

    // Polynomial approximation.
    FP_INT oox = RcpPoly6(k);
    //FP_ASSERT(oox >= HALF && oox <= ONE);

    // Apply exponent and multiply.
    FP_LONG yr = (offset >= 0) ? (y >> offset) : (y << -offset);
    return Qmul30((FP_INT) (yr >> 2), oox);
}

static FP_LONG FP64_Atan2Fast(FP_LONG y, FP_LONG x) {
    // See: https://www.dsprelated.com/showarticle/1052.php

    if (x == 0) {
        if (y > 0) return PiHalf;
        if (y < 0) return -PiHalf;

        //InvalidArgument("Fixed64::Atan2Fast", "y, x", y, x);
        return 0;
    }

    // \note these round negative numbers slightly
    FP_LONG nx = x ^ (x >> 63);
    FP_LONG ny = y ^ (y >> 63);
    FP_LONG negMask = ((x ^ y) >> 63);

    if (nx >= ny) {
        FP_INT k = FP64_Atan2DivFast(ny, nx);
        FP_INT z = AtanPoly3Lut8(k);
        FP_LONG angle = negMask ^ ((FP_LONG) z << 2);
        if (x > 0) return angle;
        if (y >= 0) return angle + Pi;
        return angle - Pi;
    } else {
        FP_INT k = FP64_Atan2DivFast(nx, ny);
        FP_INT z = AtanPoly3Lut8(k);
        FP_LONG angle = negMask ^ ((FP_LONG) z << 2);
        return ((y > 0) ? PiHalf : -PiHalf) - angle;
    }
}

static FP_INT FP64_Atan2DivFastest(FP_LONG y, FP_LONG x) {
    //FP_ASSERT(y >= 0 && x > 0 && x >= y);

    // Normalize input into [1.0, 2.0( range (convert to s2.30).
    static const FP_INT ONE = (1 << 30);
    //static const FP_INT HALF = (1 << 29);
    FP_INT offset = 31 - FP64_Nlz((FP_ULONG) x);
    FP_INT n = (FP_INT) (((offset >= 0) ? (x >> offset) : (x << -offset)) >> 2);
    FP_INT k = n - ONE;

    // Polynomial approximation.
    FP_INT oox = RcpPoly4(k);
    //FP_ASSERT(oox >= HALF && oox <= ONE);

    // Apply exponent and multiply.
    FP_LONG yr = (offset >= 0) ? (y >> offset) : (y << -offset);
    return Qmul30((FP_INT) (yr >> 2), oox);
}

static FP_LONG FP64_Atan2Fastest(FP_LONG y, FP_LONG x) {
    // See: https://www.dsprelated.com/showarticle/1052.php

    if (x == 0) {
        if (y > 0) return PiHalf;
        if (y < 0) return -PiHalf;

        //InvalidArgument("Fixed64::Atan2Fastest", "y, x", y, x);
        return 0;
    }

    // \note these round negative numbers slightly
    FP_LONG nx = x ^ (x >> 63);
    FP_LONG ny = y ^ (y >> 63);
    FP_LONG negMask = ((x ^ y) >> 63);

    if (nx >= ny) {
        FP_INT z = FP64_Atan2DivFastest(ny, nx);
        FP_INT res = AtanPoly4(z);
        FP_LONG angle = negMask ^ ((FP_LONG) res << 2);
        if (x > 0) return angle;
        if (y >= 0) return angle + Pi;
        return angle - Pi;
    } else {
        FP_INT z = FP64_Atan2DivFastest(nx, ny);
        FP_INT res = AtanPoly4(z);
        FP_LONG angle = negMask ^ ((FP_LONG) res << 2);
        return ((y > 0) ? PiHalf : -PiHalf) - angle;
    }
}

static FP_LONG FP64_Asin(FP_LONG x) {
    // Return 0 for invalid values
    if (x < -One || x > One) {
        InvalidArgument("Fixed64::Asin", "x", x);
        return 0;
    }

    return FP64_Atan2(x, FP64_Sqrt(FP64_Mul(One + x, One - x)));
}

static FP_LONG FP64_AsinFast(FP_LONG x) {
    // Return 0 for invalid values
    if (x < -One || x > One) {
        InvalidArgument("Fixed64::AsinFast", "x", x);
        return 0;
    }

    return FP64_Atan2Fast(x, FP64_SqrtFast(FP64_Mul(One + x, One - x)));
}

static FP_LONG FP64_AsinFastest(FP_LONG x) {
    // Return 0 for invalid values
    if (x < -One || x > One) {
        InvalidArgument("Fixed64::AsinFastest", "x", x);
        return 0;
    }

    return FP64_Atan2Fastest(x, FP64_SqrtFastest(FP64_Mul(One + x, One - x)));
}

static FP_LONG FP64_Acos(FP_LONG x) {
    // Return 0 for invalid values
    if (x < -One || x > One) {
        InvalidArgument("Fixed64::Acos", "x", x);
        return 0;
    }

    return FP64_Atan2(FP64_Sqrt(FP64_Mul(One + x, One - x)), x);
}

static FP_LONG FP64_AcosFast(FP_LONG x) {
    // Return 0 for invalid values
    if (x < -One || x > One) {
        InvalidArgument("Fixed64::AcosFast", "x", x);
        return 0;
    }

    return FP64_Atan2Fast(FP64_SqrtFast(FP64_Mul(One + x, One - x)), x);
}

static FP_LONG FP64_AcosFastest(FP_LONG x) {
    // Return 0 for invalid values
    if (x < -One || x > One) {
        InvalidArgument("Fixed64::AcosFastest", "x", x);
        return 0;
    }

    return FP64_Atan2Fastest(FP64_SqrtFastest(FP64_Mul(One + x, One - x)), x);
}

static FP_LONG FP64_Atan(FP_LONG x) {
    return FP64_Atan2(x, One);
}

static FP_LONG FP64_AtanFast(FP_LONG x) {
    return FP64_Atan2Fast(x, One);
}

static FP_LONG FP64_AtanFastest(FP_LONG x) {
    return FP64_Atan2Fastest(x, One);
}

static const uint64_t FP_64_scales[10] = {
        /* 18 decimals is enough for full 64bit fixed point precision */
        1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
};

static char *FP_64_itoa_loop(char *buf, uint64_t scale, uint64_t value, int skip) {
    while (scale) {
        unsigned digit = (value / scale);

        if (!skip || digit || scale == 1) {
            skip = 0;
            *buf++ = '0' + digit;
            value %= scale;
        }

        scale /= 10;
    }
    return buf;
}

static void FP64_ToString(FP_LONG value, char *buf, int decimals) {
    uint64_t uvalue = (value >= 0) ? value : -value;
    if (value < 0)
        *buf++ = '-';

    /* Separate the integer and decimal parts of the value */
    uint64_t intpart = uvalue >> 32;
    uint64_t fracpart = uvalue & 0xFFFFFFFF;
    uint64_t scale = FP_64_scales[decimals & 7];
    fracpart = FP64_Mul(fracpart, scale);

    if (fracpart >= scale) {
        /* Handle carry from decimal part */
        intpart++;
        fracpart -= scale;
    }

    /* Format integer part */
    buf = FP_64_itoa_loop(buf, 1000000000, intpart, 1);

    /* Format decimal part (if any) */
    if (scale != 1) {
        *buf++ = '.';
        buf = FP_64_itoa_loop(buf, scale / 10, fracpart, 0);
    }

    *buf = '\0';
}

#include <linux/types.h>

#define _isdigit(c) (c >= '0' && c <= '9')
#define _isspace(c) (c == ' ' || c == '\t' || c == '\n')

// Returns the number of converted bytes
static int FP64_FromString(const char *buf, FP_LONG *val) {
    const char* start_pos = buf;
    while (_isspace(*buf))
        buf++;

    /* Decode the sign */
    short negative = (*buf == '-');
    if (*buf == '+' || *buf == '-')
        buf++;

    /* Decode the integer part */
    uint64_t intpart = 0;
    int count = 0;
    while (_isdigit(*buf)) {
        intpart *= 10;
        intpart += *buf++ - '0';
        count++;
    }

    if (count == 0 || count > 18 || intpart > 9223372036854775807LL || (!negative && intpart > 9223372036854775807LL))
        return false;

    FP_LONG value = intpart << FP64_Shift;

    /* Decode the decimal part */
    if (*buf == '.' || *buf == ',') {
        buf++;

        uint64_t fracpart = 0;
        uint64_t scale = 1;
        while (_isdigit((unsigned char) *buf) && scale < 1000000000000000000LL) {
            scale *= 10;
            fracpart *= 10;
            fracpart += *buf++ - '0';
        }

        value += FP64_DivPrecise(fracpart, scale);
    }

    /* Verify that there is no garbage left over */
    while (*buf != '\0' && *buf != ';') {
        if (!_isdigit((unsigned char) *buf) && !_isspace((unsigned char) *buf))
            return 0;

        buf++;
    }

    *val = negative ? -value : value;
    return buf - start_pos;
}


#undef FP_ASSERT

#endif // __FIXED64_H


