add_library(exchange-server-candlestick STATIC)
add_library(exchange::server::candlestick ALIAS exchange-server-candlestick)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp)

target_sources(exchange-server-candlestick
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADERS}
)

target_link_libraries(exchange-server-candlestick
        PUBLIC
        exchange::common
        exchange::server::common
)
