// WIP: Translating from rust version: sodium/impl_/node.rs

#ifndef _SODIUM_IMPL_NODE_H_
#define _SODIUM_IMPL_NODE_H_

#include "bacon_gc/gc.h"
#include "functional"
#include "vector"

namespace sodium::impl {

    typedef struct NodeData;

    typedef struct Node {
        bacon_gc::Gc<NodeData> data;
    };

    typedef struct WeakNode {
        bacon_gc::GcWeak<NodeData> data;
    };

    typedef struct NodeData {
        unsigned int id;
        unsigned int rank;
        std::function<void()> update;
        std::vector<bacon_gc::Node> update_dependencies;
        std::vector<Node> dependencies;
        std::vector<WeakNode> dependents;
        std::function<void()> cleanup;
        std::vector<std::function<void()>> additional_cleanups;
    };

}

#endif // _SODIUM_IMPL_NODE_H_
