#ifndef _SODIUM_LAZY_H_
#define _SODIUM_LAZY_H_

#include "sodium/optional.h"

namespace sodium {

    template <typename A>
    class Lazy {
    public:

        template <typename F>
        Lazy(F k): k(k) {}

        A operator()() {
            if (!value_op) {
                value_op = k();
            }
            return *value_op;
        }

        template <typename F>
        Lazy<typename std::result_of<F(A)>::type> map(F f) {
            typedef typename std::result_of<F(A)>::type B;
            Lazy<A>& self = *this;
            return Lazy<B>([=] { return f(self()); });
        }

    private:
        nonstd::optional<A> value_op;
        std::function<A()> k;
    };

}

#endif // _SODIUM_LAZY_H_
