add_executable(exchange-server-common-test)
add_executable(exchange::server::common::test ALIAS exchange-server-common-test)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(exchange-server-common-test
        PRIVATE ${SOURCES}
)

target_link_libraries(exchange-server-common-test
        PRIVATE
        GTest::gtest_main
        exchange::server::common
)

include(GoogleTest)
gtest_discover_tests(exchange-server-common-test)
