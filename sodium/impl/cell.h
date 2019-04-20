#ifndef _SODIUM_IMPL_CELL_H_
#define _SODIUM_IMPL_CELL_H_

#include "bacon_gc/gc.h"
#include "sodium/lazy.h"
#include "sodium/optional.h"
#include "sodium/impl/sodium_ctx.h"
#include "sodium/impl/node.h"

namespace sodium::impl {

    template <typename A>
    struct CellData {
        Node node;
        sodium::Lazy<A> value;
        nonstd::optional<sodium::Lazy<A>> next_value_op;
    };

    template <typename A>
    class Cell {
    public:
        bacon_gc::Gc<CellData<A>> data;

        Cell(bacon_gc::Gc<CellData<A>> data): data(data) {}

        Cell(A value) {
            CellData<A>* data2 = new CellData<A>();
            data2->node = node_new(
                []() { return false; },
                std::vector<bacon_gc::Node*>(),
                std::vector<Node>(),
                []() {},
                "Cell::pure"
            );
            data2->value = value;
            data2->next_value_op = nonstd::nullopt;
            this->data = bacon_gc::Gc<CellData<A>>(data2);
        }

        template <typename FN>
        Cell<typename std::result_of<FN(A)>::type> map(FN f) {
            typedef typename std::result_of<FN(A)>::type B;
            CellData<B>* data2 = new CellData<A>();
            auto update = [=]() {
                if (this->data->next_value_op) {
                    data2->next_value_op = nonstd::optional<B>(this->data->next_value_op.value().map(f));
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
            data2->value = this->data->value.map(f);
            update();
            return Cell<B>(data2);
        }
    };

}

#endif // _SODIUM_IMPL_CELL_H_
