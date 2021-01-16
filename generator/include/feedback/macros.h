#pragma once

#define PRAGMA(x) _Pragma(#x)

#ifdef __GNUC__
#define GCC_DIAGNOSTIC(x) PRAGMA(GCC diagnostic x)
#else
#define GCC_DIAGNOSTIC(x) /**/
#endif

#define GCC_DIAGNOSTIC_POP  GCC_DIAGNOSTIC(pop)
#define GCC_DIAGNOSTIC_PUSH GCC_DIAGNOSTIC(push)

#ifdef _MSC_VER
#define MSC_WARNING(x) PRAGMA(warning(x))
#else
#define MSC_WARNING(x) /**/
#endif

#define MSC_WARNING_POP  MSC_WARNING(pop)
#define MSC_WARNING_PUSH MSC_WARNING(push)

#define COMPILER_WARNINGS_POP  GCC_DIAGNOSTIC_POP MSC_WARNING_POP
#define COMPILER_WARNINGS_PUSH GCC_DIAGNOSTIC_PUSH MSC_WARNING_PUSH
