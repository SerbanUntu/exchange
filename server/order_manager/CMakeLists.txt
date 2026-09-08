add_library(exchange-server-order-manager STATIC)
add_library(exchange::server::order_manager ALIAS exchange-server-order-manager)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp)

target_sources(exchange-server-order-manager
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADERS}
)

target_link_libraries(exchange-server-order-manager
        PUBLIC
        exchange::common
        exchange::server::common
)
