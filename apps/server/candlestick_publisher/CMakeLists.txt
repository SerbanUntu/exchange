add_executable(candlestick-publisher)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(candlestick-publisher PRIVATE ${SOURCES})

target_link_libraries(candlestick-publisher PRIVATE exchange::server::candlestick)
