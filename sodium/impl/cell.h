#ifndef _SODIUM_IMPL_CELL_H_
#define _SODIUM_IMPL_CELL_H_

#include "bacon_gc/gc.h"
#include "sodium/lazy.h"
#include "sodium/listener.h"
#include "sodium/optional.h"
#include "sodium/optional_util.h"
#include "sodium/impl/sodium_ctx.h"
#include "sodium/impl/node.h"
#include "sodium/impl/stream.h"

namespace sodium::impl {

    template <typename A>
    struct CellData {
        Node node;
        sodium::impl::Stream<A> stream;
        sodium::Lazy<A> value;

        CellData(Node node, sodium::impl::Stream<A> stream, sodium::Lazy<A> value)
        : node(node), stream(stream), value(value) {
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
                    [](SodiumCtx& sodium_ctx, Node& node) {},
                    std::vector<bacon_gc::Node*>(),
                    std::vector<Node>(),
                    []() {},
                    "Cell::pure"
                ),
                sodium::impl::Stream<A>(),
                sodium::Lazy<A>([=]() { return value; })
            );
            this->data = bacon_gc::Gc<CellData<A>>(data2);
        }

        /*
        template <typename FN>
        Cell<typename std::result_of<FN(A)>::type> map(FN f) const {
            typedef typename std::result_of<FN(A)>::type B;
            CellData<B>* data2 = new CellData<A>(
                Node(),
                this->data->value.map(f),
                nonstd::nullopt
            );
            auto update2 = [=]() {
                if (this->data->next_value_op) {
                    data2->next_value_op = nonstd::optional<sodium::Lazy<B>>(this->data->next_value_op.value().map(f));
                    return true;
                }
                return false;
            };
            auto update = [=](SodiumCtx& sodium_ctx, Node& node) {
                if (update2()) {
                    sodium_ctx.mark_dependents_dirty(node);
                }
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
            update2();
            return Cell<B>(bacon_gc::Gc<CellData<B>>(data2));
        }

        template <typename B, typename FN>
        Cell<typename std::result_of<FN(A,B)>::type> lift2(Cell<B> const& cb, FN f) const {
            typedef typename std::result_of<FN(A,B)>::type C;
            CellData<B>* data2 = new CellData<B>(
                Node(),
                this->data->value.lift2(cb.data->value, f),
                nonstd::nullopt
            );
            auto update2 = [=]() {
                data2->next_value_op = nonstd::optional<Lazy<C>>(
                    this->next_value_or_value().lift2(
                        cb.next_value_or_value(),
                        f
                    )
                );
            };
            auto update = [=](SodiumCtx& sodium_ctx, Node& node) {
                update2();
                sodium_ctx.mark_dependents_dirty(node);
            };
            std::vector<Node> dependencies;
            dependencies.push_back(this->data->node);
            dependencies.push_back(cb.data->node);
            Node node = node_new(
                update,
                std::vector<bacon_gc::Node*>(),
                dependencies,
                []() {},
                "Cell::lift2"
            );
            data2->node = node;
            update2();
            return Cell<B>(bacon_gc::Gc<CellData<B>>(data2));
        }

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
        }*/
    };

}

namespace bacon_gc {

    template <typename A>
    struct Trace<sodium::impl::CellData<A>> {
        template <typename F>
        static void trace(const sodium::impl::CellData<A>& a, F&& k) {
            Trace<sodium::impl::Node>::trace(a.node, k);
            Trace<sodium::impl::Stream<A>>::trace(a.stream, k);
            Trace<sodium::Lazy<A>>::trace(a.value, k);
        }
    };
}

#endif // _SODIUM_IMPL_CELL_H_
