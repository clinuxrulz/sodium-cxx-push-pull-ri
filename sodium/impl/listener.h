#ifndef _SODIUM_IMPL_LISTENER_H_
#define _SODIUM_IMPL_LISTENER_H_

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

#endif // _SODIUM_IMPL_LISTENER_H_