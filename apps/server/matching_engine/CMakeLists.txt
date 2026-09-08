add_executable(matching-engine)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(matching-engine PRIVATE ${SOURCES})

target_link_libraries(matching-engine PRIVATE exchange::server::matching_engine)
