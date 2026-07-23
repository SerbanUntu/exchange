add_executable(exchange-server-gateway-test)
add_executable(exchange::server::gateway::test ALIAS exchange-server-gateway-test)
target_sources(exchange-server-gateway-test
        PRIVATE
        main.cpp
)

target_link_libraries(exchange-server-gateway-test
        PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(exchange-server-gateway-test)
