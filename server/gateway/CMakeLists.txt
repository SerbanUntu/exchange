add_executable(exchange-server-gateway)
add_executable(exchange::server::gateway ALIAS exchange-server-gateway)
target_sources(exchange-server-gateway
        PRIVATE
        src/main.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/server/gateway/main.hpp
)

target_link_libraries(exchange-server-gateway
        PUBLIC
        exchange::common
        exchange::proto
        gRPC::grpc++
)

add_subdirectory(test)