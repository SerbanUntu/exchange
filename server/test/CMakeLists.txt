add_executable(exchange-server-test)
add_executable(exchange::server::test ALIAS exchange-server-test)
target_sources(exchange-server-test
        PRIVATE
        main.cpp
)

target_link_libraries(exchange-server-test
        PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(exchange-server-test)
