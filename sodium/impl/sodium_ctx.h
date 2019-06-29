#ifndef _SODIUM_IMPL_SODIUM_CTX_H_
#define _SODIUM_IMPL_SODIUM_CTX_H_

#include "bacon_gc/gc.h"
#include <functional>
#include <queue>
#include <unordered_set>
#include <vector>
#include <memory>
#include <mutex>
#include "sodium/optional.h"
#include "sodium/finally.h"

namespace sodium::impl {
    class SodiumCtx;

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
        std::function<void(SodiumCtx&,Node&)> update;
        std::vector<bacon_gc::Node*> update_dependencies;
        std::vector<Node> dependencies;
        std::vector<WeakNode> dependents;
        std::function<void()> cleanup;
        std::vector<std::function<void()>> additional_cleanups;
    };

}

namespace bacon_gc {

    template <>
    struct Trace<sodium::impl::Node> {
        template <typename F>
        static void trace(const sodium::impl::Node& a, F&& k) {
            Trace<bacon_gc::Gc<sodium::impl::NodeData>>::trace(a.data, k);
        }
    };

    template <>
    struct Trace<sodium::impl::NodeData> {
        template <typename F>
        static void trace(const sodium::impl::NodeData& a, F&& k) {
            for (auto it = a.update_dependencies.begin(); it != a.update_dependencies.end(); ++it) {
                auto update_dependency = *it;
                k(update_dependency);
            }
            for (auto it = a.dependencies.begin(); it != a.dependencies.end(); ++it) {
                auto dependency = *it;
                Trace<sodium::impl::Node>::trace(dependency, k);
            }
        }
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

namespace sodium::impl {

    struct SodiumCtx {
        unsigned int _next_id = 0;
        unsigned int _transaction_depth = 0;
        unsigned int _callback_depth = 0;
        std::priority_queue<Node> _to_be_updated;
        std::unordered_set<Node> _to_be_updated_set;
        bool _resort_required = false;
        std::vector<std::function<void()>> _pre_trans;
        std::vector<std::function<void()>> _post_trans;
        unsigned int _node_count = 0;
        std::unordered_set<Node> _keep_alive;

        unsigned int new_id() {
            return this->_next_id++;
        }

        void add_keep_alive(Node node) {
            this->_keep_alive.insert(node);
        }

        void remove_keep_alive(Node node) {
            this->_keep_alive.erase(node);
        }

        void inc_node_count() {
            ++this->_node_count;
        }

        void dec_node_count() {
            --this->_node_count;
        }

        int node_count() const {
            return this->_node_count;
        }

        void inc_callback_depth() {
            ++this->_callback_depth;
        }

        void dec_callback_depth() {
            --this->_callback_depth;
        }

        int callback_depth() const {
            return this->_callback_depth;
        }

        template <typename F>
        void pre(F f) {
            transaction_void([this,f]() {
                this->_pre_trans.push_back(std::function<void()>(f));
            });
        }

        template <typename F>
        void post(F f) {
            transaction_void([this,f]() {
                this->_post_trans.push_back(std::function<void()>(f));
            });
        }

        template <typename F>
        void transaction_void(F f) {
            transaction<int>([f]() { f(); return 0; });
        }

        template <typename A,typename F>
        A transaction(F f) {
            ++this->_transaction_depth;
            A result = f();
            --this->_transaction_depth;
            if (this->_transaction_depth == 0) {
                this->propergate();
            }
            return result;
        }

        void schedule_update_sort() {
            this->_resort_required = true;
        }

        void mark_dependents_dirty(sodium::impl::Node& node) {
            for (auto it = node.data->dependents.begin(); it != node.data->dependents.end(); ++it) {
                auto weakDependent = *it;
                auto dependent = weakDependent.data.lock();
                if (dependent) {
                    auto dependent2 = Node { data: dependent };
                    mark_dirty(dependent2);
                }
            }
        }

        void mark_dirty(sodium::impl::Node& node) {
            if (this->_to_be_updated_set.find(node) == this->_to_be_updated_set.end()) {
                this->_to_be_updated.push(node);
                this->_to_be_updated_set.insert(node);
            }
        }

        void propergate() {
            while (this->_pre_trans.size() != 0) {
                std::vector<std::function<void()>> pre_trans;
                this->_pre_trans.swap(pre_trans);
                for (auto it = pre_trans.begin(); it != pre_trans.end(); ++it) {
                    auto pre_trans2 = *it;
                    pre_trans2();
                }
            }
            if (this->_resort_required) {
                while (!this->_to_be_updated.empty()) {
                    this->_to_be_updated.pop();
                }
                for (auto it = this->_to_be_updated_set.begin(); it != this->_to_be_updated_set.end(); ++it) {
                    this->_to_be_updated.push(*it);
                }
                this->_resort_required = false;
            }
            ++this->_transaction_depth;
            while (!this->_to_be_updated.empty()) {
                auto node = this->_to_be_updated.top();
                this->_to_be_updated.pop();
                this->_to_be_updated_set.erase(node);
                node.data->update(*this, node);
            }
            --this->_transaction_depth;
            while (this->_post_trans.size() != 0) {
                auto post_trans = this->_post_trans;
                this->_post_trans.clear();
                for (auto it = post_trans.begin(); it != post_trans.end(); ++it) {
                    auto post_trans2 = *it;
                    post_trans2();
                }
            }
        }
    };

    extern SodiumCtx __sodium_ctx;
    extern std::mutex __sodium_ctx_mutex;

    template <typename R, typename F>
    R with_sodium_ctx(F&& k) {
        std::lock_guard<std::mutex> guard(__sodium_ctx_mutex);
        return k(__sodium_ctx);
    }

    template <typename F>
    void with_sodium_ctx_void(F&& k) {
        std::lock_guard<std::mutex> guard(__sodium_ctx_mutex);
        k(__sodium_ctx);
    }
}

#endif // _SODIUM_IMPL_SODIUM_CTX_H_
