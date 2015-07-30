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

  #if __has_warning("-Wunused-local-typedef")
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_UNUSED_LOCAL_TYPEDEF \
    _Pragma("clang diagnostic ignored \"-Wunused-local-typedef\"")
  #else
    #define ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_UNUSED_LOCAL_TYPEDEF
  #endif

  #define SUPPRESS_WARNINGS \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wconversion\"") \
    _Pragma("clang diagnostic ignored \"-Wconditional-uninitialized\"") \
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
    _Pragma("clang diagnostic ignored \"-Wunreachable-code\"") \
    _Pragma("clang diagnostic ignored \"-Wunused-parameter\"") \
    _Pragma("clang diagnostic ignored \"-Wused-but-marked-unused\"") \
    _Pragma("clang diagnostic ignored \"-Wweak-vtables\"") \
    ABL_PRAGMA_CLANG_DIAGNOSTIC_IGNORED_UNUSED_LOCAL_TYPEDEF

  #define RESTORE_WARNINGS \
    _Pragma("clang diagnostic pop")

#elif defined(_MSC_VER)

  /**
  * C4100: 'identifier' : unreferenced formal parameter
  * C4127: conditional expression is constant
  * C4244: 'conversion' conversion from 'type1' to 'type2', possible loss of data
  * C4251: 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
  * C4365: 'action' : conversion from 'type_1' to 'type_2', signed/unsigned mismatch
  * C4388: signed/unsigned mismatch
  * C4555: expression has no effect; expected expression with side-effect
  * C4619: #pragma warning : there is no warning number 'number'
  * C4628: digraphs not supported with -Ze. Character sequence 'digraph' not interpreted as alternate token for 'char'
  * C4640: 'instance' : construction of local static object is not thread-safe
  * C4668: 'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives'
  * C4800: 'type' : forcing value to bool 'true' or 'false' (performance warning)
  * C4826: Conversion from 'type1 ' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
  */
  #define SUPPRESS_WARNINGS \
    __pragma(warning(push)) \
    __pragma(warning(disable: 4100)) \
    __pragma(warning(disable: 4127)) \
    __pragma(warning(disable: 4244)) \
    __pragma(warning(disable: 4251)) \
    __pragma(warning(disable: 4365)) \
    __pragma(warning(disable: 4388)) \
    __pragma(warning(disable: 4555)) \
    __pragma(warning(disable: 4619)) \
    __pragma(warning(disable: 4628)) \
    __pragma(warning(disable: 4640)) \
    __pragma(warning(disable: 4668)) \
    __pragma(warning(disable: 4800)) \
    __pragma(warning(disable: 4826))

  #define RESTORE_WARNINGS \
    __pragma(warning(pop))

#else

  #define SUPPRESS_WARNINGS
  #define RESTORE_WARNINGS

#endif
