add_executable(exchange-server-execution)
add_executable(exchange::server::execution ALIAS exchange-server-execution)

target_sources(exchange-server-execution
        PRIVATE
        src/main.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/server/execution_publisher/main.hpp
)

target_link_libraries(exchange-server-execution
        PUBLIC
        exchange::common
        exchange::server::common
)
