add_executable(exchange-server-candlestick)
add_executable(exchange::server::candlestick ALIAS exchange-server-candlestick)

target_sources(exchange-server-candlestick
        PRIVATE
        src/main.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/server/candlestick_publisher/main.hpp
)

target_link_libraries(exchange-server-candlestick
        PUBLIC
        exchange::common
        exchange::server::common
)
