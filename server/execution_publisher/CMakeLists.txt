add_library(exchange-server-execution STATIC)
add_library(exchange::server::execution ALIAS exchange-server-execution)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp)

target_sources(exchange-server-execution
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADERS}
)

target_link_libraries(exchange-server-execution
        PUBLIC
        exchange::common
        exchange::server::common
)
