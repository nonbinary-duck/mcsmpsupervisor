#pragma once
#ifndef H___PROJID___SRC_UTILS
#define H___PROJID___SRC_UTILS 1

#include <string>
#include <chrono>
#include <cstdint>
#include <iostream>


namespace utils
{
    // Give neater names to the forced-width int data types
    // least type defines smallest possible type that is larger than or equal to x (i.e. in 9-bit systems, a 64 would be 72 bits wide)
    // Scoped as to not interfere with other sources
    typedef uint_least64_t    tulong;
    typedef uint_least32_t    tuint;
    typedef uint_least16_t    tushort;
    typedef uint_least8_t     tuchar;

    typedef int_least64_t     tslong;
    typedef int_least32_t     tsint;
    typedef int_least16_t     tsshort;
    typedef int_least8_t      tschar;
    
} // End namespace utils

#endif // H___PROJID___SRC_UTILS