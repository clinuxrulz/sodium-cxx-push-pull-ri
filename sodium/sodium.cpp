#include "sodium/sodium.h"
#include "bacon_gc/gc.h"
#include <functional>
#include <queue>
#include <unordered_set>
#include <vector>
#include <mutex>
#include "sodium/optional.h"

namespace sodium {

    template <typename A, typename F>
    static nonstd::optional<typename std::result_of<F(A)>::type> optional_map(F f, nonstd::optional<A> ma) {
        if (ma) {
            return f(*ma);
        } else {
            return nonstd::nullopt;
        }
    }

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
        nonstd::optional<A> value_op;
        std::function<A()> k;
    };

    template <typename A>
    class Latch {
    public:
        template <typename F>
        Latch(F k) {
            this->value_op = nonstd::nullopt;
            this->k = std::function<Lazy<A>()>(k);
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

    class Node: public HasNodeData {
    public:
        bacon_gc::Gc<NodeData> data;

        Node(const bacon_gc::Gc<NodeData>& data): data(data) {}

        Node(): Node(std::vector<bacon_gc::Gc<NodeData>>(), std::function<bool()>([] { return false; })) {
        }

        Node(std::vector<bacon_gc::Gc<NodeData>> dependencies, std::function<bool()> update) {
            data->id = next_id();
            data->rank = calc_rank(dependencies);
            data->update = update;
            for (std::vector<bacon_gc::Gc<NodeData>>::iterator it = dependencies.begin(); it != dependencies.end(); ++it) {
                bacon_gc::Gc<NodeData>& dependency = (*it);
                data->dependencies.push_back(dependency);
                dependency->targets.push_back(data.downgrade());
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

    static Node null_node(bacon_gc::Gc<NodeData>(new NodeData(null_node_data)));

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

    struct Listener {};

    template <typename A>
    struct StreamData {
        Node node;
        bacon_gc::Gc<Latch<nonstd::optional<A>>> latch;
    };

    template <typename A>
    class Stream: public HasNodeData {
    public:
        Stream()
        : Stream(
            null_node,
            bacon_gc::Gc<Latch<nonstd::optional<A>>>(
                new Latch<nonstd::optional<A>>(
                    [] {
                        return Lazy<nonstd::optional<A>>([] { return nonstd::nullopt; });
                    }
                )
            )
        )
        {
        }

        Stream(Node node, bacon_gc::Gc<Latch<nonstd::optional<A>>> latch)
        : data(bacon_gc::Gc<StreamData<A>>(new StreamData<A>({ node, latch })))
        {
        }

        template <typename F>
        Stream<typename std::result_of<F(A)>::type> map(const F& f) {
            typedef typename std::result_of<F(A)>::type B;
            Stream<A>& self = *this;
            bacon_gc::Gc<Latch<nonstd::optional<A>>> latch(
                new Latch<nonstd::optional<A>>(
                    [=]() {
                        return Lazy<nonstd::optional<B>>([=] {
                            return optional_map(f, (*self.data->latch)()());
                        });
                    }
                )
            );
            std::vector<bacon_gc::Gc<NodeData>> dependencies;
            dependencies.push_back(data->node.node_data());
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

        bacon_gc::Gc<NodeData> node_data() const {
            return data->node.node_data();
        }
    private:
        bacon_gc::Gc<StreamData<A>> data;
    };

    template<typename A>
    struct StreamSinkData {
        Node node;
        nonstd::optional<A> value;
        bool value_will_reset;
        Stream<A> stream;
    };

    template<typename A>
    class StreamSink {
    public:
        StreamSink() {
            this.data = bacon_gc::Gc<StreamSinkData<A>>(new StreamSinkData<A>({
                node: null_node,
                value: nonstd::nullopt,
                value_will_reset: false,
                stream: Stream<A>(
                    null_node,
                    bacon_gc::Gc<Latch<nonstd::optional<A>>>(
                        [this] {
                            return Lazy<nonstd::optional<A>>([this] {
                                return this->data.value;
                            });
                        }
                    )
                )
            }));
        }

        /* TODO:
        public void send(A value) {
            transactionVoid(() -> {
                this._value = Option.some(value);
                if (!this._valueWillReset) {
                    this._valueWillReset = true;
                    last(() -> {
                        this._value = Option.none();
                        this._valueWillReset = false;
                    });
                }
                this._node.changed();
            });
        }*/

        Stream<A> stream() {
            return data->stream;
        }
    private:
        bacon_gc::Gc<StreamSinkData<A>> data;
    };

} // end namespace sodium


static void test() {
    sodium::Stream<int> sa; // sa = never
    sodium::Stream<int> sb = sa.map([](int a) { return a + 1; });
}
