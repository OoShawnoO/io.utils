cmake_minimum_required(VERSION 3.20)

project(io.utils)

set(CMAKE_CXX_STANDARD 11)

if(MSVC)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

add_subdirectory(3rdparty)

set(SOCKET_SOURCES
        src/Socket/Socket.cpp
)
set(FILESYSTEM_SOURCES
        src/FileSystem/FileSystem.cpp
)
include_directories(3rdparty/Mole)

find_package(GTest)

enable_testing()
add_executable(test_ test/main.cpp ${SOCKET_SOURCES} ${FILESYSTEM_SOURCES})

#add_library(Socket SHARED ${SOCKET_SOURCES})
#add_library(FileSystem SHARED ${FILESYSTEM_SOURCES})

target_link_libraries(test_ PRIVATE Mole)
target_link_libraries(test_ PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

add_test(test_ ${CMAKE_BINARY_DIR}/test_)