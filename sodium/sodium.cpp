#include "sodium/sodium.h"
#include "bacon_gc/gc.h"
#include <functional>
#include <queue>
#include <unordered_set>
#include <vector>
#include <mutex>
#include "sodium/optional.h"

namespace sodium {

    template <typename A>
    class Lazy {
    public:

        template <typename F>
        Lazy(F k): k(k) {}

        A operator()() {
            if (!value_op) {
                value_op = k();
            }
            return *value_op;
        }

        template <typename F>
        Lazy<typename std::result_of<F(A)>::type> map(F f) {
            typedef typename std::result_of<F(A)>::type B;
            Lazy<A>& self = *this;
            return Lazy<B>([=] { return f(self()); });
        }

    private:
        nonstd::optional<Lazy<A>> value_op;
        std::function<A()> k;
    };

    template <typename A>
    class Latch {
    public:
        template <typename F>
        Latch(F k) {
            this->value_op = nonstd::nullopt;
            this->k = std::function<Lazy<A>>(k);
        }

        void reset() {
            value_op = nonstd::nullopt;
        }

        Lazy<A> operator()() {
            if (!value_op) {
                value_op = k();
            }
            return *value_op;
        }

    private:
        nonstd::optional<Lazy<A>> value_op;
        std::function<Lazy<A>()> k;
    };

    class Node;

    struct NodeData {
        int id;
        int rank;
        std::function<bool()> update;
        std::vector<bacon_gc::Gc<NodeData>> dependencies;
        std::vector<bacon_gc::GcWeak<NodeData>> targets;
        std::vector<std::function<void()>> clean_ups;
    };

    NodeData null_node_data = {
        id: -1,
        rank: 0,
        update: std::function<bool()>([] { return false; }),
        dependencies: std::vector<bacon_gc::Gc<NodeData>>(),
        targets: std::vector<bacon_gc::GcWeak<NodeData>>(),
        clean_ups: std::vector<std::function<void()>>()
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
                    bacon_gc::Gc<NodeData>& n = *it;
                    for (int i = n->targets.size(); i >= 0; --i) {
                        bacon_gc::GcWeak<NodeData> target = n->targets[i];
                        bacon_gc::Gc<NodeData> target2 = target.lock();
                        if (target2 && target2->id == this->data->id) {
                            n->targets.erase(n->targets.begin() + i);
                        }
                    }
                }
            }));
        }

        bacon_gc::Gc<NodeData> node_data() const {
            return data;
        }
    };

    Node null_node(bacon_gc::Gc<NodeData>(null_node_data));

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

    template <typename A>
    struct StreamData {
        bacon_gc::Gc<NodeData> node;
        bacon_gc::Gc<Latch<nonstd::optional<A>>> latch;
    };

    template <typename A>
    class Stream {
    public:
        Stream(): Stream(null_node, bacon_gc::Gc<Latch<nonstd::optional<A>>>(Latch<nonstd::optional<A>>([] { return nonstd::nullopt; }))) {}

        Stream(NodeData node, bacon_gc::Gc<Latch<nonstd::optional<A>>> latch)
        : data(bacon_gc::Gc<StreamData<A>>({ node, latch }))
        {
        }

        template <typename F>
        Stream<typename std::result_of<F(A)>::type> map(const F& f) {
            typedef typename std::result_of<F(A)>::type B;
            Stream<A>& self = *this;
            bacon_gc::Gc<Latch<nonstd::optional<A>>> latch(
                Latch<nonstd::optional<A>>(
                    [=]() {
                        return self->data->latch().map(f);
                    }
                )
            );
            std::vector<bacon_gc::Gc<NodeData>> dependencies;
            dependencies.push_back(data->node);
            return Stream<B>(
                Node(
                    dependencies,
                    [=] {
                        latch->reset();
                        return true;
                    }
                ),
                latch
            );
        }

    private:
        bacon_gc::Gc<StreamData<A>> data;
    };

} // end namespace sodium


static void test() {
    sodium::Stream<int> sa; // sa = never
    sodium::Stream<int> sb = sa.map([](int a) { return a + 1; });
}
