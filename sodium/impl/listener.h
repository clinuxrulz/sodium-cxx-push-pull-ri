#ifndef _SODIUM_IMPL_LISTENER_H_
#define _SODIUM_IMPL_LISTENER_H_

#include "bacon_gc/gc.h"
#include "sodium/impl/node.h"

namespace sodium::impl {

    class ListenerData {
    public:
        Node node;

        ListenerData(Node node): node(node) {}
    };

    class Listener {
    public:
        bacon_gc::Gc<ListenerData> data;

        Listener(Node node) {
            ListenerData* data = new ListenerData(node);
            this->data = bacon_gc::Gc<ListenerData>(data);
        }

        void unlisten() {
            this->data->node = Node();
        }
    };
};

namespace bacon_gc {

    template<>
    struct Trace<sodium::impl::Listener> {
        template <typename F>
        static void trace(const sodium::impl::Listener& a, F&& k) {
            Trace<bacon_gc::Gc<sodium::impl::ListenerData>>::trace(a.data, k);
        }
    };

    template<>
    struct Trace<sodium::impl::ListenerData> {
        template <typename F>
        static void trace(const sodium::impl::ListenerData& a, F&& k) {
            Trace<sodium::impl::Node>::trace(a.node, k);
        }
    };
}

#endif // _SODIUM_IMPL_LISTENER_H_