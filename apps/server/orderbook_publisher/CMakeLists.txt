add_executable(orderbook-publisher)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(orderbook-publisher PRIVATE ${SOURCES})

target_link_libraries(orderbook-publisher PRIVATE exchange::server::orderbook)
