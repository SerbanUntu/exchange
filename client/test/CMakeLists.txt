add_executable(exchange-client-test)
add_executable(exchange::client::test ALIAS exchange-client-test)
target_sources(exchange-client-test
        PRIVATE
        main.cpp
)

target_link_libraries(exchange-client-test
        PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(exchange-client-test)
