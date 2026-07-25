add_executable(exchange-server-matching-engine)
add_executable(exchange::server::matching_engine ALIAS exchange-server-matching-engine)

target_sources(exchange-server-matching-engine
        PRIVATE
        src/main.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/server/matching_engine/main.hpp
)

target_link_libraries(exchange-server-matching-engine
        PUBLIC
        exchange::common
        exchange::server::common
)
