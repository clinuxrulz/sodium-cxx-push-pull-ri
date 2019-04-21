#ifndef _SODIUM_PRIMITIVE_TRACE_H_
#define _SODIUM_PRIMITIVE_TRACE_H_

#include "bacon_gc/gc.h"

#define DEFINE_EMPTY_TRACE(TYPE) \
    template<> \
    struct Trace<TYPE> { \
        template <typename F> \
        static void trace(const TYPE& a, F&& k) { \
        } \
    };

namespace bacon_gc {
    DEFINE_EMPTY_TRACE(int)
}

#endif // _SODIUM_PRIMITIVE_TRACE_H_
