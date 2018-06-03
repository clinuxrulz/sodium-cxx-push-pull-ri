#ifndef _TEST_SODIUM_H_
#define _TEST_SODIUM_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <sodium/sodium.h>
#include <string>

class test_sodium : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(test_sodium);
    // sodium tests
    CPPUNIT_TEST(map);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void tearDown();

    void map();
};

#endif