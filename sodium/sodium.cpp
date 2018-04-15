#include "sodium/sodium.h"
#include "bacon_gc/gc.h"
#include <functional>
#include <queue>
#include <unordered_set>
#include <vector>
#include <mutex>

namespace sodium {

    class Node;

    struct NodeData {
        int id;
        int rank;
        std::function<bool()> update;
        std::vector<bacon_gc::Gc<NodeData>> dependencies;
        std::vector<bacon_gc::GcWeak<NodeData>> targets;
        std::vector<std::function<void()>> clean_ups;
    };

    static int next_id();

    class HasNodeData {
    public:
        virtual ~HasNodeData() {};

        virtual bacon_gc::Gc<NodeData> node_data() const = 0;
    };

    int calc_rank(std::vector<HasNodeData*> dependencies) {
        int rank = 0;
        for (std::vector<HasNodeData*>::iterator it = dependencies.begin(); it != dependencies.end(); ++it) {
            HasNodeData& depends_on = *(*it);
            if (depends_on.node_data()->rank >= rank) {
                rank = depends_on.node_data()->rank + 1;
            }
        }
        return rank;
    }

    class Node: public HasNodeData {
    public:
        bacon_gc::Gc<NodeData> data;

        Node(bacon_gc::Gc<NodeData>& data): data(data) {}

        Node(): Node(std::vector<HasNodeData*>(), std::function<bool()>([] { return false; })) {
        }

        Node(std::vector<HasNodeData*> dependencies, std::function<bool()> update) {
            data->id = next_id();
            data->rank = calc_rank(dependencies);
            data->update = update;
            for (std::vector<HasNodeData*>::iterator it = dependencies.begin(); it != dependencies.end(); ++it) {
                HasNodeData& dependency = *(*it);
                data->dependencies.push_back(dependency.node_data());
                dependency.node_data()->targets.push_back(data.downgrade());
            }
            data->clean_ups.push_back(std::function<void()>([this] {
                for (std::vector<bacon_gc::Gc<NodeData>>::iterator it = this->data->dependencies.begin(); it != this->data->dependencies.end(); ++it) {
                    /* TODO:
                    for (int i = dependence.node().targets.size()-1; i >= 0; --i) {
                        WeakReference<Node> target = dependence.node().targets.get(i);
                        Node target2 = target.get();
                        if (target2 == null || target2 == this) {
                            dependence.node().targets.remove(i);
                        }
                    }
                    */
                }
            }));
        }

        bacon_gc::Gc<NodeData> node_data() const {
            return data;
        }
    };

}

namespace std {
    template<>
    struct hash<sodium::Node> {
        size_t operator()(const sodium::Node& n) const {
            return hash<int>()(n.data->id);
        }
    };

    template<>
    struct equal_to<sodium::Node> {
        bool operator()(const sodium::Node& a, const sodium::Node& b) const {
            return a.data->id == b.data->id;
        }
    };

    template<>
    struct less<sodium::Node> {
        bool operator()(const sodium::Node& a, const sodium::Node& b) const {
            if (a.data->rank < b.data->rank) {
                return true;
            } else if (a.data->rank > b.data->rank) {
                return false;
            } else {
                return a.data->id < b.data->id;
            }
        }
    };
}

namespace sodium {

    struct Listener;

    struct SodiumCtx {
        int next_id;
        int transaction_depth;
        std::priority_queue<Node> queue;
        std::unordered_set<Node> entries;
        std::vector<Listener> keep_alive;
        std::vector<std::function<void()>> last;
    };

    static SodiumCtx _sodium_ctx;
    static std::mutex _sodium_ctx_mutex;

    template <typename A, typename F>
    A with_sodium_ctx(F f) {
        std::lock_guard<std::mutex> guard(_sodium_ctx_mutex);
        return f(_sodium_ctx);
    }

    static int next_id() {
        return with_sodium_ctx<int>([](SodiumCtx& sodium_ctx) {
            int id = sodium_ctx.next_id++;
            return id;
        });
    }

    class HasNode {
    public:
        virtual ~HasNode() {};

        virtual Node* node() = 0;
    };

    struct Listener {};

} // end namespace sodium
