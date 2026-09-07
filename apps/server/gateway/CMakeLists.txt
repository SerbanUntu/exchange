add_executable(gateway)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(gateway PRIVATE ${SOURCES})

target_link_libraries(gateway PRIVATE exchange::server::gateway)
