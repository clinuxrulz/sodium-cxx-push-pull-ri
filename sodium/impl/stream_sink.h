#ifndef _SODIUM_IMPL_STREAM_SINK_H_
#define _SODIUM_IMPL_STREAM_SINK_H_

#include "bacon_gc/gc.h"
#include "sodium/lazy.h"
#include "sodium/listener.h"
#include "sodium/optional.h"
#include "sodium/optional_util.h"
#include "sodium/impl/sodium_ctx.h"
#include "sodium/impl/stream.h"
#include "sodium/impl/node.h"

namespace sodium::impl {

    template <typename A>
    class StreamSink {
    public:
        Stream<A> _stream;

        StreamSink() {
            StreamData<A>* data2 = new StreamData<A>(
                Node(),
                nonstd::nullopt
            );
            Node node = node_new(
                []() { return false; },
                std::vector<bacon_gc::Node*>(),
                std::vector<sodium::impl::Node>(),
                []() {},
                "StreamSink"
            );
            data2->node = node;
            this->_stream = Stream<A>(data2);
        }

        Stream<A> stream() {
            return this->_stream;
        }

        void send(A a) {
            with_sodium_ctx_void([=](SodiumCtx& sodium_ctx) {
                sodium_ctx.transaction_void([=]() {
                    bool wasFiring = this->_stream->data->firing_op;
                    this->_stream->data->firing_op = nonstd::optional<Lazy<A>>(Lazy<A>::pure(a));
                    if (!wasFiring) {
                        sodium_ctx.post([=]() {
                            this->_stream->data->firing_op = nonstd::nullopt;
                        });
                    }
                });
            });
        }
    };
}

#endif // _SODIUM_IMPL_STREAM_SINK_H_