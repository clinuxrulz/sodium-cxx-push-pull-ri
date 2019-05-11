#ifndef _SODIUM_OPTIONAL_UTIL_H_
#define _SODIUM_OPTIONAL_UTIL_H_

#include "sodium/optional.h"

template <typename A, typename B, typename FN>
nonstd::optional<typename std::result_of<FN(A,B)>::type> optional_lift2(nonstd::optional<A> oa, nonstd::optional<B> ob, FN fn) {
    typedef typename std::result_of<FN(A,B)>::type C;
    if (oa) {
        if (ob) {
            return nonstd::optional<C>(oa(), ob());
        }
    }
    return nonstd::nullopt;
}

#endif // _SODIUM_OPTIONAL_UTIL_H_
