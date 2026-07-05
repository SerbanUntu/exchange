add_executable(exchange-server)
add_executable(exchange::server ALIAS exchange-server)
target_sources(exchange-server
        PRIVATE
        src/main.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/server/main.hpp
)

target_link_libraries(exchange-server
        PUBLIC
        exchange::common
        exchange::proto
        gRPC::grpc++
)

add_subdirectory(test)