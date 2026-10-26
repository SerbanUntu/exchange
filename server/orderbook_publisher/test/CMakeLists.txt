add_executable(exchange-server-orderbook-test)
add_executable(exchange::server::orderbook::test ALIAS exchange-server-orderbook-test)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(exchange-server-orderbook-test
        PRIVATE ${SOURCES}
)

target_link_libraries(exchange-server-orderbook-test
        PRIVATE
        GTest::gtest_main
        exchange::server::orderbook
        exchange::server::common
        exchange::common
)

include(GoogleTest)
gtest_discover_tests(exchange-server-orderbook-test)
