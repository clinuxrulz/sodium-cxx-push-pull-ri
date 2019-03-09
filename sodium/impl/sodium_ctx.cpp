#include "bacon_gc/gc.h"
#include <functional>
#include <queue>
#include <unordered_set>
#include <vector>
#include <memory>
#include <mutex>
#include "sodium/optional.h"
#include "sodium/finally.h"
#include "sodium/impl/node.h"
#include "sodium/impl/sodium_ctx.h"

namespace sodium::impl {

    typedef struct SodiumCtx {
        unsigned int next_id = 0;
        unsigned int transaction_depth = 0;
        unsigned int callback_depth = 0;
        std::priority_queue<Node> to_be_updated;
        std::unordered_set<Node> to_be_updated_set;
        bool resort_required = false;
        std::vector<std::function<void()>> pre_trans;
        std::vector<std::function<void()>> post_trans;
        unsigned int node_count = 0;
        std::unordered_set<Node> keep_alive;
    };

    static SodiumCtx __sodium_ctx;
    static std::mutex __sodium_ctx_mutex;

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

    unsigned int new_id() {
        return with_sodium_ctx<unsigned int>([](SodiumCtx& sodium_ctx) {
            return sodium_ctx.next_id++;
        });
    }

    void add_keep_alive(Node node) {
        with_sodium_ctx_void([node](SodiumCtx& sodium_ctx) {
            sodium_ctx.keep_alive.insert(node);
        });
    }

    void remove_keep_alive(Node node) {
        with_sodium_ctx_void([node](SodiumCtx& sodium_ctx) {
            sodium_ctx.keep_alive.erase(node);
        });
    }
}
