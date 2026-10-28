add_library(exchange-server-matching-engine-fuzzing STATIC)
add_library(exchange::server::matching_engine::fuzzing ALIAS exchange-server-matching-engine-fuzzing)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.hpp)
set(SEEDS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/seeds")

target_sources(exchange-server-matching-engine-fuzzing
        PRIVATE ${SOURCES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADERS}
)

target_compile_definitions(exchange-server-matching-engine-fuzzing
        PRIVATE
        SEEDS_DIRECTORY="${SEEDS_DIRECTORY}"
)

target_link_libraries(exchange-server-matching-engine-fuzzing
        PUBLIC
        exchange::server::matching_engine
)

add_executable(exchange-server-matching-engine-write-seeds apps/main.cpp)
target_link_libraries(exchange-server-matching-engine-write-seeds PRIVATE exchange::server::matching_engine::fuzzing)

if(FUZZ)
    add_executable(exchange-server-matching-engine-fuzzer apps/fuzzer.cpp)
    target_link_options(exchange-server-matching-engine-fuzzer PRIVATE -fsanitize=fuzzer)
    add_test(NAME exchange-server-matching-engine-fuzzer-seeds
            COMMAND exchange-server-matching-engine-fuzzer -runs=0 ${SEEDS_DIRECTORY})
else()
    add_executable(exchange-server-matching-engine-fuzzer apps/run_seeds.cpp)
    add_test(NAME exchange-server-matching-engine-fuzzer-seeds
            COMMAND exchange-server-matching-engine-fuzzer ${SEEDS_DIRECTORY})
endif()
target_link_libraries(exchange-server-matching-engine-fuzzer PRIVATE exchange::server::matching_engine::fuzzing)
