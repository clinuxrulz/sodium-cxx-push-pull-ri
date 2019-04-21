#ifndef _SODIUM_OPTIONAL_TRACE_H_
#define _SODIUM_OPTIONAL_TRACE_H_

#include "bacon_gc/gc.h"
#include "sodium/optional.h"

namespace bacon_gc {

    template <typename A>
    struct Trace<nonstd::optional<A>> {
        template <typename F>
        static void trace(const nonstd::optional<A>& a_op, F&& k) {
            if (a_op) {
                auto a = *a_op;
                Trace<A>::trace(a, k);
            }
        }
    };

}


#endif // _SODIUM_OPTIONAL_TRACE_H_
