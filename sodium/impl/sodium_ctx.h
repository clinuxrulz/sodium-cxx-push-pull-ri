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
        A&& transaction(F f) {
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
        
        void propergate() {
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
            // TODO:
            --this->_transaction_depth;
            // TODO:
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
