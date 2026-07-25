add_executable(exchange-server-orderbook)
add_executable(exchange::server::orderbook ALIAS exchange-server-orderbook)

target_sources(exchange-server-orderbook
        PRIVATE
        src/main.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/server/orderbook_publisher/main.hpp
)

target_link_libraries(exchange-server-orderbook
        PUBLIC
        exchange::common
        exchange::server::common
)
