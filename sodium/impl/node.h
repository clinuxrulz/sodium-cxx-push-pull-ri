// WIP: Translating from rust version: sodium/impl_/node.rs

#ifndef _SODIUM_IMPL_NODE_H_
#define _SODIUM_IMPL_NODE_H_

#include <functional>
#include <vector>
#include <memory>
#include "bacon_gc/gc.h"
#include "sodium_ctx.h"

namespace sodium::impl {

    template <typename UPDATE, typename CLEANUP>
    static Node node_new(
        UPDATE update,
        std::vector<bacon_gc::Node*> update_dependencies,
        std::vector<Node> dependencies,
        CLEANUP cleanup,
        std::string desc
    ) {
        int id = with_sodium_ctx<int>([](SodiumCtx sodium_ctx) {
            sodium_ctx.inc_node_count();
            return sodium_ctx.new_id();
        });
        int rank = 0;
        for (auto it = dependencies.begin(); it != dependencies.end(); ++it) {
            auto& dependency = *it;
            if (rank <= dependency.data->rank) {
                rank = dependency.data->rank + 1;
            }
        }
        auto update2 = [=]() {
            with_sodium_ctx_void([](SodiumCtx sodium_ctx) {
                sodium_ctx.inc_callback_depth();
            });
            auto result = update();
            with_sodium_ctx_void([](SodiumCtx sodium_ctx) {
                sodium_ctx.dec_callback_depth();
            });
            return result;
        };
        std::shared_ptr<NodeData> _self;
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
            for (auto it = _self->additional_cleanups.begin(); it != _self->additional_cleanups.end(); ++it) {
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
        _self = node_data;
        return { data: bacon_gc::Gc<NodeData>(node_data) };
    }

}

#endif // _SODIUM_IMPL_NODE_H_
