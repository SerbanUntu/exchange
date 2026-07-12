add_library(exchange-common STATIC)
add_library(exchange::common ALIAS exchange-common)
target_sources(exchange-common
        PRIVATE
        src/model/universe.cpp
        src/util/logging.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/common/model/universe.hpp
        include/common/util/logging.hpp
)

target_link_libraries(exchange-common
        PUBLIC
        spdlog::spdlog
        exchange::proto
)

add_subdirectory(test)