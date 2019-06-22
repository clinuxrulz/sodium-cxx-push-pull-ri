#ifndef _TEST_SODIUM_H_
#define _TEST_SODIUM_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>

class test_sodium : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(test_sodium);
    // sodium tests
    CPPUNIT_TEST(stream_map);
    CPPUNIT_TEST(cell_map);
    CPPUNIT_TEST(cell_lift2);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void tearDown();

    void map();

    void stream_map();

    void cell_map();

    void cell_lift2();

    void lazy_map();
};

#endif
