add_executable(exchange-server-order-manager)
add_executable(exchange::server::order_manager ALIAS exchange-server-order-manager)

target_sources(exchange-server-order-manager
        PRIVATE
        src/main.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/server/order_manager/main.hpp
)

target_link_libraries(exchange-server-order-manager
        PUBLIC
        exchange::common
        exchange::server::common
)
