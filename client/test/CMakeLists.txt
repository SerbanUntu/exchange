add_executable(exchange-client-test)
add_executable(exchange::client::test ALIAS exchange-client-test)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(exchange-client-test
        PRIVATE ${SOURCES}
)

target_link_libraries(exchange-client-test
        PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(exchange-client-test)
