add_library(exchange-common STATIC)
add_library(exchange::common ALIAS exchange-common)
target_sources(exchange-common
        PRIVATE
        src/util/logging.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/common/util/logging.hpp
        include/common/model/order_request.hpp
        include/common/model/order_response.hpp
)

target_link_libraries(exchange-common
        PUBLIC
        spdlog::spdlog
)

add_subdirectory(test)