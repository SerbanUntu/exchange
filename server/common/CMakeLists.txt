add_library(exchange-server-common STATIC)
add_library(exchange::server::common ALIAS exchange-server-common)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp)

target_sources(exchange-server-common
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADERS}
)

target_link_libraries(exchange-server-common
        PUBLIC
        exchange::common
        boost::boost
)

add_subdirectory(test)
