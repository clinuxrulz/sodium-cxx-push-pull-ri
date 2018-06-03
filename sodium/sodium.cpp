#include "sodium/sodium.h"

namespace sodium {
    NodeData null_node_data = {
        id: -1,
        rank: 0,
        update: std::function<bool()>([] { return false; }),
        dependencies: std::vector<bacon_gc::Gc<NodeData>>(),
        targets: std::vector<bacon_gc::GcWeak<NodeData>>(),
        clean_ups: std::vector<std::function<void()>>()
    };

    void update() {
        // TODO
    }

    int next_id() {
        return with_sodium_ctx([](SodiumCtx& sodium_ctx) {
            int id = sodium_ctx.next_id++;
            return id;
        });
    }

    int calc_rank(std::vector<bacon_gc::Gc<NodeData>> dependencies) {
        int rank = 0;
        for (std::vector<bacon_gc::Gc<NodeData>>::iterator it = dependencies.begin(); it != dependencies.end(); ++it) {
            bacon_gc::Gc<NodeData>& depends_on = (*it);
            if (depends_on->rank >= rank) {
                rank = depends_on->rank + 1;
            }
        }
        return rank;
    }
}

/*
    struct NodeData {
        int id;
        int rank;
        std::function<bool()> update;
        std::vector<bacon_gc::Gc<NodeData>> dependencies;
        std::vector<bacon_gc::GcWeak<NodeData>> targets;
        std::vector<std::function<void()>> clean_ups;
    };

template <typename A>
struct StreamData {
    Node node;
    bacon_gc::Gc<Latch<nonstd::optional<A>>> latch;
};
namespace bacon_gc {
    template <>
    struct Trace<MyObj1> {
        template <typename F>
        static void trace(const MyObj1& a, F&& k) {
            if (a.next) {
                Trace<Gc<MyObj1>>::trace(a.next, k);
            }
        }
    };
}
*/