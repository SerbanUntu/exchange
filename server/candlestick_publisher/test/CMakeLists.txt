add_executable(exchange-server-candlestick-test)
add_executable(exchange::server::candlestick::test ALIAS exchange-server-candlestick-test)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(exchange-server-candlestick-test
        PRIVATE ${SOURCES}
)

target_link_libraries(exchange-server-candlestick-test
        PRIVATE
        GTest::gtest_main
        exchange::server::candlestick
        exchange::server::common
        exchange::common
)

include(GoogleTest)
gtest_discover_tests(exchange-server-candlestick-test)
