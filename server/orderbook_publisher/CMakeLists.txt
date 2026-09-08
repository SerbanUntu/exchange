add_library(exchange-server-orderbook STATIC)
add_library(exchange::server::orderbook ALIAS exchange-server-orderbook)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp)

target_sources(exchange-server-orderbook
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADERS}
)

target_link_libraries(exchange-server-orderbook
        PUBLIC
        exchange::common
        exchange::server::common
)
