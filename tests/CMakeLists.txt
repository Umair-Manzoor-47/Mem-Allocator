include(FetchContent)
FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.11
)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(doctest)

add_executable(tests
        test_linear_allocator.cpp
        test_tracking_allocator.cpp
        test_stack_allocator.cpp
        test_pool_allocator.cpp
)
target_link_libraries(tests PRIVATE alloc doctest::doctest)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
enable_testing()
doctest_discover_tests(tests)