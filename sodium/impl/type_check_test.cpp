#include "sodium/impl/cell.h"

static void test() {
    sodium::impl::Cell<int> ca(1);
    auto cb = ca.map([](int a) { return a + 1; });
}
