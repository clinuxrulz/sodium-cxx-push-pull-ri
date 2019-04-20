#ifndef _SODIUM_LAZY_H_
#define _SODIUM_LAZY_H_

#include "bacon_gc/gc.h"
#include "sodium/optional.h"

namespace sodium {

    template <typename A>
    struct LazyData {
        nonstd::optional<A> value_op;
        std::function<A()> k;
    };

    template <typename A>
    class Lazy {
    public:

        template <typename F>
        Lazy(F k) {
            this->data->k = k;
        }

        A operator()() {
            if (!this->data->value_op) {
                this->data->value_op = (this->data->k)();
            }
            return *(this->data->value_op);
        }

        template <typename F>
        Lazy<typename std::result_of<F(A)>::type> map(F f) {
            typedef typename std::result_of<F(A)>::type B;
            return Lazy<B>([=]() { return f((*this)()); });
        }

    private:
        bacon_gc::Gc<LazyData<A>> data;
    };

}

#endif // _SODIUM_LAZY_H_
