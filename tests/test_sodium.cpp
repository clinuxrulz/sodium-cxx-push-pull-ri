#include "test_sodium.h"
#include <sodium/lazy.h>
#include <sodium/primitive_trace.h>
#include <sodium/impl/cell.h>
#include <sodium/impl/stream.h>
#include <sodium/impl/stream_sink.h>

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

void test_sodium::stream_map() {
    sodium::impl::StreamSink<int> ss;
    sodium::impl::Stream<int> s = ss.stream();
    sodium::impl::Stream<int> s2 = s.map([](int a) { return a + 1; });
    sodium::impl::Listener l = s2.listen_weak([](int a) {
        cout << a << endl;
    });
    ss.send(1);
    ss.send(2);
    ss.send(3);
}

void test_sodium::cell_map() {
    sodium::impl::Cell<int> ca(1);
    auto cb = ca.map([](int a) { return a + 2; });
    //cb.listen_weak([](int a) {
    //    cout << a << endl;
    //});
}

void test_sodium::cell_lift2() {
    sodium::impl::Cell<int> ca(1);
    sodium::impl::Cell<int> cb(2);
    sodium::impl::Cell<int> cc = ca.lift2(cb, [](int a, int b) { return a + b; });
    //cc.listen_weak([](int a) {
    //    cout << a << endl;
    //});
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
