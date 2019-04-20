#include "sodium/impl/cell.h"

static void test() {
    sodium::impl::Cell<int> ca(1);
    auto cb = ca.map([](int a) { return a + 1; });
    {
        sodium::Lazy<int> a = sodium::Lazy<int>([]() { return 1; });
        auto b = a.map([](int x) { return x + 1; });
    }
}
