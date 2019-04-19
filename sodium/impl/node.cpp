#include "node.h"

namespace sodium::impl {

    void _node_ensure_bigger_than2(Node* node, int rank, std::unordered_set<int>* visited) {
        if (visited->find(node->data->id) != visited->end()) {
            return;
        }
        visited->insert(node->data->id);
        if (node->data->rank < rank) {
            return;
        }
        node->data->rank = rank + 1;
        for (auto it = node->data->dependents.begin(); it != node->data->dependents.end(); ++it) {
            auto& dependent = *it;
            auto dependent2 = dependent.data.lock();
            if (dependent2) {
                Node dependent3 = { dependent2 };
                _node_ensure_bigger_than2(&dependent3, rank, visited);
            }
        }
    }

    void node_ensure_bigger_than(Node* node, int rank) {
        with_sodium_ctx_void([](SodiumCtx sodium_ctx) {
            sodium_ctx.schedule_update_sort();
        });
        std::unordered_set<int> visited;
        _node_ensure_bigger_than2(node, rank, &visited);
    }

}
