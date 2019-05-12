#ifndef _SODIUM_LAZY_H_
#define _SODIUM_LAZY_H_

#include <vector>
#include "bacon_gc/gc.h"
#include "sodium/optional_trace.h"
#include "sodium/optional.h"

namespace sodium {

    template <typename A>
    struct LazyData {
        nonstd::optional<A> value_op;
        std::function<A()> k;
        std::vector<bacon_gc::Node*> gc_deps;
    };

    template <typename A>
    class Lazy {
    public:

        Lazy(const Lazy<A>& lazy): data(lazy.data) {
        }

        template <typename F>
        Lazy(F k) {
            this->data = bacon_gc::Gc<LazyData<A>>(new LazyData<A>());
            this->data->k = k;
        }

        template <typename F>
        Lazy(F k, std::vector<bacon_gc::Node*> gc_deps) {
            this->data->k = k;
            this->data->gc_deps = gc_deps;
        }

        Lazy<A>& operator=(const Lazy<A>& lazy) {
            this->data = lazy.data;
            return *this;
        }

        A operator()() const {
            if (!this->data->value_op) {
                this->data->value_op = (this->data->k)();
            }
            return *(this->data->value_op);
        }

        template <typename FN>
        Lazy<typename std::result_of<FN(A)>::type> map(FN f) const {
            typedef typename std::result_of<FN(A)>::type B;
            return Lazy<B>([=]() { return f((*this)()); });
        }

        template <typename B, typename FN>
        Lazy<typename std::result_of<FN(A,B)>::type> lift2(Lazy<B> lb, FN f) const {
            typedef typename std::result_of<FN(A,B)>::type C;
            return Lazy<C>([=]() { return f((*this)(), lb()); });
        }

    //private:
        bacon_gc::Gc<LazyData<A>> data;
    };

}

namespace bacon_gc {

    template <typename A>
    struct Trace<sodium::Lazy<A>> {
        template <typename F>
        static void trace(const sodium::Lazy<A>& a, F&& k) {
            Trace<bacon_gc::Gc<sodium::LazyData<A>>>::trace(a.data, k);
        }
    };

    template <typename A>
    struct Trace<sodium::LazyData<A>> {
        template <typename F>
        static void trace(const sodium::LazyData<A>& a, F&& k) {
            if (a.value_op) {
                A const& value = *a.value_op;
                Trace<A>::trace(value, k);
            }
            for (auto it = a.gc_deps.begin(); it != a.gc_deps.end(); ++it) {
                auto gc_dep = *it;
                k(gc_dep);
            }
        }
    };
}

#endif // _SODIUM_LAZY_H_
