cmake_minimum_required( VERSION 2.8 )

set( CMAKE_INSTALL_PREFIX /usr/local )

project(sodium)
include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-std=c++11" SUPPORTS_CXX11)
if( SUPPORTS_CXX11 )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    check_cxx_compiler_flag("-std=c++0x" SUPPORTS_CXX0X)
    if( SUPPORTS_CXX0X )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
        message("C++11 is not supported")
        add_definitions(-DSODIUM_NO_CXX11)
    endif()
endif()

# set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wno-deprecated-declarations" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated-declarations -g" )

set( SODIUM_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sodium )

set( GC_CXX_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gc-cxx/bacon_gc )
set( GC_CXX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gc-cxx )

set( CMAKE_INCLUDE_CURRENT_DIR ON )

set(SODIUM_INCLUDE_FILES
    ${SODIUM_BASE_DIR}/sodium.h
    ${SODIUM_BASE_DIR}/impl/cell.h
    ${SODIUM_BASE_DIR}/impl/node.h
    ${SODIUM_BASE_DIR}/impl/sodium_ctx.h
    )

set(SODIUM_SOURCE_FILES
    ${SODIUM_BASE_DIR}/sodium.cpp
    ${SODIUM_BASE_DIR}/impl/node.cpp
    ${SODIUM_BASE_DIR}/impl/sodium_ctx.cpp
    )

set(GC_CXX_INCLUDE_FILES
    ${GC_CXX_BASE_DIR}/gc.h
    )

set(GC_CXX_SOURCE_FILES
    ${GC_CXX_BASE_DIR}/gc.cpp
    )

file(
    COPY ${SODIUM_INCLUDE_FILES} 
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/sodium
    )

file(
    COPY ${GC_CXX_INCLUDE_FILES}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/bacon_gc
    )

include_directories( ${CMAKE_SOURCE_DIR} ${GC_CXX_INCLUDE_DIR} )

set( ALL_SOURCES ${SODIUM_INCLUDE_FILES} ${SODIUM_SOURCE_FILES} ${GC_CXX_INCLUDE_FILES} ${GC_CXX_SOURCE_FILES} )

add_library( sodium SHARED ${ALL_SOURCES} )
add_library( libsodium STATIC ${ALL_SOURCES} )

set_target_properties( libsodium PROPERTIES OUTPUT_NAME sodium )

# Testing

enable_testing()
add_subdirectory(tests)

# INSTALL
# -------

install (
    TARGETS sodium libsodium
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

# Install header files
install (
    FILES ${SODIUM_INCLUDE_FILES}
    DESTINATION include/sodium
    )

