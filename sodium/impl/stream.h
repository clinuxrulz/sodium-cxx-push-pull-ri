#ifndef _SODIUM_IMPL_STREAM_H_
#define _SODIUM_IMPL_STREAM_H_

#include "bacon_gc/gc.h"
#include "sodium/lazy.h"
#include "sodium/optional.h"
#include "sodium/optional_util.h"
#include "sodium/impl/listener.h"
#include "sodium/impl/node.h"
#include "sodium/impl/sodium_ctx.h"

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
            this->data = bacon_gc::Gc<StreamData<A>>(data2);
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
                    const Lazy<A>& firing = *this->data->firing_op;
                    data2->firing_op = nonstd::optional<Lazy<B>>(firing.map(f));
                    with_sodium_ctx_void([=](SodiumCtx& sodium_ctx) {
                        sodium_ctx.post([=]() {
                            data2->firing_op = nonstd::nullopt;
                        });
                    });
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

        /*
        template <typename PRED>
        Stream<A> filter(PRED pred) const {
            StreamData<A>* data2 = new StreamData<B>(
                Node(),
                nonstd::nullopt
            );
            auto update = [=]() {
                if (this->data->firing_op) {
                    auto firing = this->data->firing_op.value();
                }
                return false;
            };
        }*/

        template <typename CALLBACK>
        Listener listen_weak(CALLBACK callback) const {
            auto update = [=]() {
                if (this->data->firing_op) {
                    const Lazy<A>& firing = *this->data->firing_op;
                    callback(firing());
                    return false;
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
                "Stream::listen"
            );
        }
    };
}

namespace bacon_gc {

    template <typename A>
    struct Trace<sodium::impl::Stream<A>> {
        template <typename F>
        static void trace(const sodium::impl::Stream<A>& a, F&& k) {
            Trace<bacon_gc::Gc<sodium::LazyData<A>>>::trace(a.data, k);
        }
    };

    template <typename A>
    struct Trace<sodium::impl::StreamData<A>> {
        template <typename F>
        static void trace(const sodium::impl::StreamData<A>& a, F&& k) {
            if (a.firing_op) {
                sodium::Lazy<A> const& firing = *a.firing_op;
                Trace<sodium::Lazy<A>>::trace(firing, k);
            }
            Trace<sodium::impl::Node>::trace(a.node, k);
        }
    };
}

#endif // _SODIUM_IMPL_STREAM_H_