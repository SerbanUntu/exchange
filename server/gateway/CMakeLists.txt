add_library(exchange-server-gateway STATIC)
add_library(exchange::server::gateway ALIAS exchange-server-gateway)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp)

target_sources(exchange-server-gateway
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADERS}
)

target_link_libraries(exchange-server-gateway
        PUBLIC
        exchange::common
        exchange::server::common
        exchange::proto
        gRPC::grpc++
)

add_subdirectory(test)
