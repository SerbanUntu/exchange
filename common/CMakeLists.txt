add_library(exchange-common STATIC)
add_library(exchange::common ALIAS exchange-common)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp)

target_sources(exchange-common
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADERS}
)

target_link_libraries(exchange-common
        PUBLIC
        spdlog::spdlog
        exchange::proto
)

add_subdirectory(test)
