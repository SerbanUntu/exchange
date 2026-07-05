add_executable(exchange-client)
add_executable(exchange::client ALIAS exchange-client)
target_sources(exchange-client
        PRIVATE
        src/main.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/client/main.hpp
)

target_link_libraries(exchange-client
        PUBLIC
        exchange::common
        exchange::proto
        gRPC::grpc++
)

add_subdirectory(test)
