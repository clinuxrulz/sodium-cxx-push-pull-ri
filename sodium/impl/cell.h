#ifndef _SODIUM_IMPL_CELL_H_
#define _SODIUM_IMPL_CELL_H_

#include "bacon_gc/gc.h"
#include "sodium/optional.h"
#include "sodium/impl/sodium_ctx.h"
#include "sodium/impl/node.h"

namespace sodium::impl {

    template <typename A>
    struct CellData {
        Node node;
        A value;
        nonstd::optional<A> next_value_op;
    };

    template <typename A>
    class Cell {
    public:
        bacon_gc::Gc<CellData<A>> data;

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
    };

}

#endif // _SODIUM_IMPL_CELL_H_
