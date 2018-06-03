add_executable( test_sodium test_sodium.cpp )
target_link_libraries( test_sodium cppunit libsodium )
add_test( test_sodium test_sodium )
