#ifndef _SODIUM_SODIUM_H_
#define _SODIUM_SODIUM_H_

#include "bacon_gc/gc.h"
#include <functional>
#include <queue>
#include <unordered_set>
#include <vector>
#include <mutex>
#include "sodium/optional.h"
#include "sodium/finally.h"

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

        const std::vector<bacon_gc::Node*>* dependencies() const {
            return &this->_dependencies;
        }

    private:
        nonstd::optional<Lazy<A>> value_op;
        std::function<Lazy<A>()> k;
        std::vector<bacon_gc::Node*> _dependencies;
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

    extern NodeData null_node_data;

    int next_id();

    class HasNodeData {
    public:
        virtual ~HasNodeData() {};

        virtual bacon_gc::Gc<NodeData> node_data() const = 0;
    };

    int calc_rank(std::vector<bacon_gc::Gc<NodeData>> dependencies);

    class Node: public HasNodeData {
    public:
        bacon_gc::Gc<NodeData> data;

        Node(const bacon_gc::Gc<NodeData>& data): data(data) {}

        Node(): Node(std::vector<bacon_gc::Gc<NodeData>>(), std::function<bool()>([] { return false; })) {
        }

        Node(std::vector<bacon_gc::Gc<NodeData>> dependencies, std::function<bool()> update) {
            data = bacon_gc::Gc<NodeData>(new NodeData());
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

        void changed() {
            // TODO
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

    class Listener;

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

    template <typename F>
    typename std::result_of<F(SodiumCtx&)>::type with_sodium_ctx(F f) {
        std::lock_guard<std::mutex> guard(_sodium_ctx_mutex);
        return f(_sodium_ctx);
    }

    template <typename F>
    void with_sodium_ctx_void(F f) {
        std::lock_guard<std::mutex> guard(_sodium_ctx_mutex);
        f(_sodium_ctx);
    }

    void update();

    template<typename K>
    typename std::result_of<K()>::type transaction(K code) {
        return with_finally(
            [=] {
                with_sodium_ctx([=](SodiumCtx& sodium_ctx) {
                    ++sodium_ctx.transaction_depth;
                });
                return code();
            },
            [] {
                int transaction_depth = with_sodium_ctx([](SodiumCtx& sodium_ctx) {
                    --sodium_ctx.transaction_depth;
                    return sodium_ctx.transaction_depth;
                });
                if (transaction_depth == 0) {
                    update();
                }
            }
        );
    }

    template<typename K>
    void transaction_void(K code) {
        with_finally_void(
            [=] {
                with_sodium_ctx([=](SodiumCtx& sodium_ctx) {
                    ++sodium_ctx.transaction_depth;
                });
                code();
            },
            [] {
                int transaction_depth = with_sodium_ctx([](SodiumCtx& sodium_ctx) {
                    --sodium_ctx.transaction_depth;
                    return sodium_ctx.transaction_depth;
                });
                if (transaction_depth == 0) {
                    update();
                }
            }
        );
    }

    template<typename K>
    void last(K code) {
        with_sodium_ctx_void([=](SodiumCtx& sodium_ctx) {
            sodium_ctx.last.push_back(std::function<void()>(code));
        });
    }

    class Listener {
    private:
        std::function<void()> _unlisten;
    public:
        template <typename F>
        Listener(F unlisten): _unlisten(std::function<void()>(unlisten)) {}
    };

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

        const bacon_gc::Gc<StreamData<A>>* _data() const {
            return &this->data;
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
            this->data = bacon_gc::Gc<StreamSinkData<A>>(new StreamSinkData<A>({
                node: null_node,
                value: nonstd::nullopt,
                value_will_reset: false,
                stream: Stream<A>(
                    null_node,
                    bacon_gc::Gc<Latch<nonstd::optional<A>>>(
                        new Latch<nonstd::optional<A>>([this] {
                            return Lazy<nonstd::optional<A>>([this] {
                                return this->data->value;
                            });
                        })
                    )
                )
            }));
        }

        void send(A value) {
            transaction_void([=] {
                StreamSinkData<A>& data = *this->data;
                data.value = value;
                if (!data.value_will_reset) {
                    data.value_will_reset = true;
                    last([this]() {
                        StreamSinkData<A>& data = *this->data;
                        data.value = nonstd::nullopt;
                        data.value_will_reset = false;
                    });
                }
                data.node.changed();
            });
        }

        Stream<A> stream() {
            return data->stream;
        }
    private:
        bacon_gc::Gc<StreamSinkData<A>> data;
    };

} // end namespace sodium

namespace bacon_gc {

    template <typename A>
    struct Trace<sodium::Latch<A>> {
        template <typename F>
        static void trace(const sodium::Latch<A>& a, F&& k) {
            const std::vector<Node*>& dependencies = *a.dependencies();
            for (std::vector<Node*>::const_iterator it = dependencies.cbegin(); it != dependencies.cend(); ++it) {
                k(*it);
            }
        }
    };

    template <>
    struct Trace<sodium::NodeData> {
        template <typename F>
        static void trace(const sodium::NodeData& a, F&& k) {
            for (std::vector<Gc<sodium::NodeData>>::const_iterator it = a.dependencies.cbegin(); it != a.dependencies.cend(); ++it) {
                Trace<Gc<sodium::NodeData>>::trace(*it, k);
            }
        }
    };

    template <typename A>
    struct Trace<sodium::StreamData<A>> {
        template <typename F>
        static void trace(const sodium::StreamData<A>& a, F&& k) {
            Trace<Gc<sodium::Latch<nonstd::optional<A>>>>::trace(a.latch, k);
        }
    };

    template <typename A>
    struct Trace<sodium::StreamSinkData<A>> {
        template <typename F>
        static void trace(const sodium::StreamSinkData<A>& a, F&& k) {
            Trace<Gc<sodium::StreamData<A>>>::trace(*a.stream._data(), k);
        }
    };
}

namespace sodium {

    template <typename... Args> inline void unused(Args&&...) {}

} // end namespace sodium

#endif // _SODIUM_SODIUM_H_
