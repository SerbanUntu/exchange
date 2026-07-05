add_executable(exchange-common-test)
add_executable(exchange::common::test ALIAS exchange-common-test)
target_sources(exchange-common-test
        PRIVATE
        util/logging.cpp
)

target_link_libraries(exchange-common-test
        PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(exchange-common-test)
