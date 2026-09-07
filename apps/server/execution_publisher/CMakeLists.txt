add_executable(execution-publisher)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(execution-publisher PRIVATE ${SOURCES})

target_link_libraries(execution-publisher PRIVATE exchange::server::execution)
