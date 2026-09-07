add_executable(client)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(client PRIVATE ${SOURCES})

target_link_libraries(client PRIVATE exchange::client)
