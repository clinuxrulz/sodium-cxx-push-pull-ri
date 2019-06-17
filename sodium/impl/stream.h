#ifndef _SODIUM_IMPL_STREAM_H_
#define _SODIUM_IMPL_STREAM_H_

#include "bacon_gc/gc.h"
#include "sodium/lazy.h"
#include "sodium/listener.h"
#include "sodium/optional.h"
#include "sodium/optional_util.h"
#include "sodium/impl/sodium_ctx.h"
#include "sodium/impl/node.h"

namespace sodium::impl {
    template <typename A>
    struct StreamData {
        Node node;
        nonstd::optional<sodium::Lazy<A>> firing_op;

        StreamData(Node node, nonstd::optional<sodium::Lazy<A>> firing_op)
        : node(node), firing_op(firing_op)
        {}
    };

    template <typename A>
    class Stream {
    public:
        bacon_gc::Gc<StreamData<A>> data;

        Stream(bacon_gc::Gc<StreamData<A>> data): data(data) {}

        Stream() {
            StreamData<A>* data2 = new StreamData<A>(
                node_new(
                    []() { return false; },
                    std::vector<bacon_gc::Node*>(),
                    std::vector<Node>(),
                    []() {},
                    "Stream::never"
                ),
                nonstd::nullopt
            );
        }

        template <typename FN>
        Stream<typename std::result_of<FN(A)>::type> map(FN f) const {
            typedef typename std::result_of<FN(A)>::type B;
            StreamData<B>* data2 = new StreamData<B>(
                Node(),
                nonstd::nullopt
            );
            auto update = [=]() {
                if (this->data->firing_op) {
                    data2->firing_op = this->data->firing_op.map([=](A& firing) { return f(firing); });
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
                "Stream::map"
            );
            data2->node = node;
            update();
            return Stream<B>(bacon_gc::Gc<StreamData<B>>(data2));
        }
    };
}

#endif // _SODIUM_IMPL_STREAM_H_