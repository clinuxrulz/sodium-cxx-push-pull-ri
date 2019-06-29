// WIP: Translating from rust version: sodium/impl_/node.rs

#ifndef _SODIUM_IMPL_NODE_H_
#define _SODIUM_IMPL_NODE_H_

#include <functional>
#include <unordered_set>
#include <vector>
#include <memory>
#include "bacon_gc/gc.h"
#include "sodium/impl/sodium_ctx.h"

namespace sodium::impl {

    template <typename UPDATE, typename CLEANUP>
    static Node node_new(
        UPDATE update,
        std::vector<bacon_gc::Node*> update_dependencies,
        std::vector<Node> dependencies,
        CLEANUP cleanup,
        std::string desc
    ) {
        unsigned int id = with_sodium_ctx<unsigned int>([](SodiumCtx sodium_ctx) {
            sodium_ctx.inc_node_count();
            return sodium_ctx.new_id();
        });
        unsigned int rank = 0;
        for (auto it = dependencies.begin(); it != dependencies.end(); ++it) {
            auto& dependency = *it;
            if (rank <= dependency.data->rank) {
                rank = dependency.data->rank + 1;
            }
        }
        auto update2 = [=](SodiumCtx& sodium_ctx, Node& node2) {
            sodium_ctx.inc_callback_depth();
            update(sodium_ctx, node2);
            sodium_ctx.dec_callback_depth();
        };
        std::shared_ptr<NodeData*> _self;
        auto cleanup2 = [=]() {
            cleanup();
            for (auto it = dependencies.begin(); it != dependencies.end(); ++it) {
                auto& dependency = *it;
                for (auto it2 = dependency.data->dependents.begin(); it2 != dependency.data->dependents.end(); ++it2) {
                    auto& dependent = *it2;
                    auto dependent2 = dependent.data.lock();
                    if (dependent2) {
                        if (dependent2->id == id) {
                            dependency.data->dependents.erase(it2);
                            break;
                        }
                    }
                }
            }
            for (auto it = (*_self)->additional_cleanups.begin(); it != (*_self)->additional_cleanups.end(); ++it) {
                auto& cleanup = *it;
                cleanup();
            }
        };
        NodeData* node_data = new NodeData;
        node_data->id = id;
        node_data->rank = rank;
        node_data->update = update2;
        node_data->update_dependencies = update_dependencies;
        node_data->dependencies = dependencies;
        node_data->cleanup = cleanup2;
        NodeData** tmp = new NodeData*;
        *tmp = node_data;
        _self = std::shared_ptr<NodeData*>(tmp);
        Node node = Node { data : bacon_gc::Gc<NodeData>(node_data) };
        auto weak_node = WeakNode { data: node.data.downgrade() };
        for (auto it = dependencies.begin(); it != dependencies.end(); ++it) {
            auto& dependency = *it;
            dependency.data->dependents.push_back(weak_node);
        }
        return node;
    }

    template <typename CLEANUP>
    static void node_add_cleanup(Node* node, CLEANUP cleanup) {
        node->data->additional_cleanups.push_back(cleanup);
    }

    void node_ensure_bigger_than(Node* node, int rank);
}

#endif // _SODIUM_IMPL_NODE_H_
