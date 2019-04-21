#include "test_sodium.h"
#include <sodium/sodium.h>
#include <sodium/primitive_trace.h>
#include <sodium/impl/cell.h>

#include <cppunit/ui/text/TestRunner.h>
#include <stdio.h>
#include <ctype.h>
#include <iostream>

using namespace std;
using namespace sodium;

void test_sodium::tearDown()
{
    bacon_gc::collect_cycles();
}

void test_sodium::map() {
    StreamSink<int> ssa;
    Stream<int> sa = ssa.stream();
    Stream<int> sb = sa.map([](int a) { return a + 1; });
    ssa.send(1);
    ssa.send(2);
    ssa.send(3);
}

void test_sodium::cell_map() {
    sodium::impl::Cell<int> ca(1);
    auto cb = ca.map([](int a) { return a + 1; });
}

// Just here for type-checking templates at compile time.
void test_sodium::lazy_map() {
    sodium::Lazy<int> a = sodium::Lazy<int>([]() { return 1; });
    auto b = a.map([](int x) { return x + 1; });
}

int main(int argc, char* argv[]) {
    CppUnit::TextUi::TestRunner runner;
    runner.addTest( test_sodium::suite() );
    bool success = runner.run();
    return success ? 0 : 1;
}
