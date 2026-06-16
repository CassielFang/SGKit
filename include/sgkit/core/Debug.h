#pragma once

#include <cstdio>
#include <cstdlib>

// SGK_ASSERT — Debug only; compiles out in Release (NDEBUG defined).
// SGK_DEBUG_ONLY — wrap code that should only run in Debug.
#ifdef _DEBUG
    #define SGK_ASSERT(cond, msg)                                              \
        do {                                                                   \
            if (!(cond)) {                                                     \
                std::fprintf(stderr, "SGKit ASSERT: %s\n  %s:%d\n",            \
                             msg, __FILE__, __LINE__);                         \
                std::abort();                                                  \
            }                                                                  \
        } while (0)
    #define SGK_DEBUG_ONLY(code) code
#else
    #define SGK_ASSERT(cond, msg) ((void)0)
    #define SGK_DEBUG_ONLY(code)
#endif
