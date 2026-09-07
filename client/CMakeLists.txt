add_library(exchange-client STATIC)
add_library(exchange::client ALIAS exchange-client)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp)

target_sources(exchange-client
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADERS}
)

target_link_libraries(exchange-client
        PUBLIC
        exchange::common
        exchange::proto
        gRPC::grpc++
)

add_subdirectory(test)
