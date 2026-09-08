add_executable(exchange-server-matching-engine-test)
add_executable(exchange::server::matching_engine::test ALIAS exchange-server-matching-engine-test)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(exchange-server-matching-engine-test
        PRIVATE ${SOURCES}
)

target_link_libraries(exchange-server-matching-engine-test
        PRIVATE
        GTest::gtest_main
        exchange::server::matching_engine
        exchange::server::common
        exchange::common
)

include(GoogleTest)
gtest_discover_tests(exchange-server-matching-engine-test)
