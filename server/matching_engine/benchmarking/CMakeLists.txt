add_executable(exchange-server-matching-engine-benchmark main.cpp)

target_link_libraries(exchange-server-matching-engine-benchmark
        PRIVATE
        exchange::server::matching_engine
        exchange::server::common
        exchange::common
)
