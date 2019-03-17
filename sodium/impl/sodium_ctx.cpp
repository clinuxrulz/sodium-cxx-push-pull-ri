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
    SodiumCtx __sodium_ctx;
    std::mutex __sodium_ctx_mutex;
}
