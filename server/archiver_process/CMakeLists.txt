add_executable(exchange-server-archiver)
add_executable(exchange::server::archiver ALIAS exchange-server-archiver)

target_sources(exchange-server-archiver
        PRIVATE
        src/main.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/server/archiver_process/main.hpp
)

target_link_libraries(exchange-server-archiver
        PUBLIC
        exchange::common
        exchange::server::common
)
