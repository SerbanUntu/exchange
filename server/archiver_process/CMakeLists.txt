add_library(exchange-server-archiver STATIC)
add_library(exchange::server::archiver ALIAS exchange-server-archiver)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp)

target_sources(exchange-server-archiver
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADERS}
)

target_link_libraries(exchange-server-archiver
        PUBLIC
        exchange::common
        exchange::server::common
)
