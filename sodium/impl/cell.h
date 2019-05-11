#ifndef _SODIUM_IMPL_CELL_H_
#define _SODIUM_IMPL_CELL_H_

#include "bacon_gc/gc.h"
#include "sodium/lazy.h"
#include "sodium/listener.h"
#include "sodium/optional.h"
#include "sodium/impl/sodium_ctx.h"
#include "sodium/impl/node.h"

namespace sodium::impl {

    template <typename A>
    struct CellData {
        Node node;
        sodium::Lazy<A> value;
        nonstd::optional<sodium::Lazy<A>> next_value_op;

        CellData(Node node, sodium::Lazy<A> value, nonstd::optional<sodium::Lazy<A>> next_value_op)
        : node(node), value(value), next_value_op(next_value_op) {
        }
    };

    template <typename A>
    class Cell {
    public:
        bacon_gc::Gc<CellData<A>> data;

        Cell(bacon_gc::Gc<CellData<A>> data): data(data) {}

        Cell(A value) {
            CellData<A>* data2 = new CellData<A>(
                node_new(
                    []() { return false; },
                    std::vector<bacon_gc::Node*>(),
                    std::vector<Node>(),
                    []() {},
                    "Cell::pure"
                ),
                sodium::Lazy<A>([=]() { return value; }),
                nonstd::nullopt
            );
            this->data = bacon_gc::Gc<CellData<A>>(data2);
        }

        template <typename FN>
        Cell<typename std::result_of<FN(A)>::type> map(FN f) const {
            typedef typename std::result_of<FN(A)>::type B;
            CellData<B>* data2 = new CellData<A>(
                Node(),
                this->data->value.map(f),
                nonstd::nullopt
            );
            auto update = [=]() {
                if (this->data->next_value_op) {
                    data2->next_value_op = nonstd::optional<sodium::Lazy<B>>(this->data->next_value_op.value().map(f));
                    return true;
                }
                return false;
            };
            std::vector<Node> dependencies;
            dependencies.push_back(this->data->node);
            Node node = node_new(
                update,
                std::vector<bacon_gc::Node*>(),
                dependencies,
                []() {},
                "Cell::map"
            );
            data2->node = node;
            update();
            return Cell<B>(data2);
        }

        //template <typename B, typename FN>
        //Cell<typename std::result_of<FN(A,B)>::type> lift2(Cell<B> const& cb)

        template <typename FN>
        Listener listen_weak(FN f) const {
            auto update = [=]() {
                if (this->data->next_value_op) {
                    f(this->data->next_value_op.value()());
                    return true;
                }
                return false;
            };
            std::vector<Node> dependencies;
            dependencies.push_back(this->data->node);
            Node node = node_new(
                update,
                std::vector<bacon_gc::Node*>(),
                dependencies,
                []() {},
                "Cell::listen"
            );
            nonstd::optional<Node> keep_alive_op = nonstd::optional<Node>(node);
            with_sodium_ctx_void([=](SodiumCtx& sodium_ctx) {
                sodium_ctx.pre([=]() {
                    if (this->data->next_value_op) {
                        f(this->data->next_value_op.value()());
                    } else {
                        f(this->data->value());
                    }
                });
            });
            return Listener([=]() mutable {
                keep_alive_op = nonstd::nullopt;
            });
        }
    };

}

namespace bacon_gc {

    template <typename A>
    struct Trace<sodium::impl::CellData<A>> {
        template <typename F>
        static void trace(const sodium::impl::CellData<A>& a, F&& k) {
            Trace<sodium::impl::Node>::trace(a.node, k);
            Trace<sodium::Lazy<A>>::trace(a.value, k);
            if (a.next_value_op) {
                auto next_value = *a.next_value_op;
                Trace<sodium::Lazy<A>>::trace(next_value, k);
            }
        }
    };
}

#endif // _SODIUM_IMPL_CELL_H_
