// WIP: Translating from rust version: sodium/impl_/node.rs

#ifndef _SODIUM_IMPL_NODE_H_
#define _SODIUM_IMPL_NODE_H_

#include "bacon_gc/gc.h"
#include <functional>
#include <vector>

namespace sodium::impl {

    struct NodeData;

    struct Node {
        bacon_gc::Gc<NodeData> data;
    };

    struct WeakNode {
        bacon_gc::GcWeak<NodeData> data;
    };

    struct NodeData {
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

namespace std {
    template<>
    struct hash<sodium::impl::Node> {
        size_t operator()(const sodium::impl::Node& node) const {
            return node.data->id;
        }
    };

    template<>
    struct equal_to<sodium::impl::Node> {
        bool operator()(const sodium::impl::Node& lhs, const sodium::impl::Node& rhs) const {
            return lhs.data->id == rhs.data->id;
        }
    };

    template<>
    struct less<sodium::impl::Node> {
        bool operator()(const sodium::impl::Node& lhs, const sodium::impl::Node& rhs) const {
            return lhs.data->rank < rhs.data->rank;
        }
    };
}

#endif // _SODIUM_IMPL_NODE_H_
