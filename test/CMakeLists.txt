enable_testing()
find_package(GTest CONFIG REQUIRED)
add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE QtMarkdownParser GTest::gtest GTest::gtest_main)
add_test(NAME test_parser COMMAND test_parser)