add_library(exchange-server-matching-engine STATIC)
add_library(exchange::server::matching_engine ALIAS exchange-server-matching-engine)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp)

target_sources(exchange-server-matching-engine
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADERS}
)

target_link_libraries(exchange-server-matching-engine
        PUBLIC
        exchange::common
        exchange::server::common
)
