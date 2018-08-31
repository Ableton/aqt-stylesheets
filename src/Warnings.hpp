/*
Copyright (c) 2014 Ableton AG, Berlin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

// clang-format off

#pragma once

#if defined(__clang__)

  #if __has_warning("-Wcomma")
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_COMMA \
    _Pragma("clang diagnostic ignored \"-Wcomma\"")
  #else
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_COMMA
  #endif

  #if __has_warning("-Wdouble-promotion")
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_DOUBLE_PROMOTION \
    _Pragma("clang diagnostic ignored \"-Wdouble-promotion\"")
  #else
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_DOUBLE_PROMOTION
  #endif

  #if __has_warning("-Wexpansion-to-defined")
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_EXPANSION_TO_DEFINED \
    _Pragma("clang diagnostic ignored \"-Wexpansion-to-defined\"")
  #else
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_EXPANSION_TO_DEFINED
  #endif

  #if __has_warning("-Winconsistent-missing-destructor-override")
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_INCONSISTENT_MISSING_DESTRUCTOR_OVERRIDE \
    _Pragma("clang diagnostic ignored \"-Winconsistent-missing-destructor-override\"")
  #else
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_INCONSISTENT_MISSING_DESTRUCTOR_OVERRIDE
  #endif

  #if __has_warning("-Wredundant-parens")
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_REDUNDANT_PARENS \
    _Pragma("clang diagnostic ignored \"-Wredundant-parens\"")
  #else
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_REDUNDANT_PARENS
  #endif

  #if __has_warning("-Wreserved-id-macro")
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_RESERVED_ID_MACRO \
    _Pragma("clang diagnostic ignored \"-Wreserved-id-macro\"")
  #else
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_RESERVED_ID_MACRO
  #endif

  #if __has_warning("-Wundefined-func-template")
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_UNDEFINED_FUNC_TEMPLATE \
    _Pragma("clang diagnostic ignored \"-Wundefined-func-template\"")
  #else
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_UNDEFINED_FUNC_TEMPLATE
  #endif

  #if __has_warning("-Wunused-local-typedef")
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_UNUSED_LOCAL_TYPEDEF \
    _Pragma("clang diagnostic ignored \"-Wunused-local-typedef\"")
  #else
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_UNUSED_LOCAL_TYPEDEF
  #endif

  #if __has_warning("-Wzero-as-null-pointer-constant")
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_ZERO_AS_NULL_POINTER_CONSTANT \
    _Pragma("clang diagnostic ignored \"-Wzero-as-null-pointer-constant\"")
  #else
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_ZERO_AS_NULL_POINTER_CONSTANT
  #endif

  #define SUPPRESS_WARNINGS \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wconditional-uninitialized\"") \
    _Pragma("clang diagnostic ignored \"-Wconversion\"") \
    _Pragma("clang diagnostic ignored \"-Wcovered-switch-default\"") \
    _Pragma("clang diagnostic ignored \"-Wdeprecated\"") \
    _Pragma("clang diagnostic ignored \"-Wdisabled-macro-expansion\"") \
    _Pragma("clang diagnostic ignored \"-Wdocumentation-unknown-command\"") \
    _Pragma("clang diagnostic ignored \"-Wdocumentation\"") \
    _Pragma("clang diagnostic ignored \"-Wexit-time-destructors\"") \
    _Pragma("clang diagnostic ignored \"-Wextra-semi\"") \
    _Pragma("clang diagnostic ignored \"-Wfloat-equal\"") \
    _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"") \
    _Pragma("clang diagnostic ignored \"-Wheader-hygiene\"") \
    _Pragma("clang diagnostic ignored \"-Wmissing-noreturn\"") \
    _Pragma("clang diagnostic ignored \"-Wmissing-prototypes\"") \
    _Pragma("clang diagnostic ignored \"-Wold-style-cast\"") \
    _Pragma("clang diagnostic ignored \"-Wpadded\"") \
    _Pragma("clang diagnostic ignored \"-Wshadow\"") \
    _Pragma("clang diagnostic ignored \"-Wshift-sign-overflow\"") \
    _Pragma("clang diagnostic ignored \"-Wshorten-64-to-32\"") \
    _Pragma("clang diagnostic ignored \"-Wsign-compare\"") \
    _Pragma("clang diagnostic ignored \"-Wsign-conversion\"") \
    _Pragma("clang diagnostic ignored \"-Wswitch-enum\"") \
    _Pragma("clang diagnostic ignored \"-Wundef\"") \
    _Pragma("clang diagnostic ignored \"-Wundefined-reinterpret-cast\"") \
    _Pragma("clang diagnostic ignored \"-Wunreachable-code\"") \
    _Pragma("clang diagnostic ignored \"-Wunused-parameter\"") \
    _Pragma("clang diagnostic ignored \"-Wused-but-marked-unused\"") \
    _Pragma("clang diagnostic ignored \"-Wweak-vtables\"") \
    ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_COMMA \
    ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_DOUBLE_PROMOTION \
    ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_EXPANSION_TO_DEFINED \
    ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_INCONSISTENT_MISSING_DESTRUCTOR_OVERRIDE \
    ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_REDUNDANT_PARENS \
    ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_RESERVED_ID_MACRO \
    ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_UNDEFINED_FUNC_TEMPLATE \
    ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_UNUSED_LOCAL_TYPEDEF \
    ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_ZERO_AS_NULL_POINTER_CONSTANT \
    /**/

  #define RESTORE_WARNINGS \
    _Pragma("clang diagnostic pop")

#elif defined(_MSC_VER)

  #define SUPPRESS_WARNINGS \
    __pragma(warning(push, 0)) \
    __pragma(warning(disable: 4244))

  #define RESTORE_WARNINGS \
    __pragma(warning(pop))

#elif defined(__GNUC__)

  #define SUPPRESS_WARNINGS _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"") \
    _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")

  #define RESTORE_WARNINGS _Pragma("GCC diagnostic pop")

#else

  #define SUPPRESS_WARNINGS
  #define RESTORE_WARNINGS

#endif
