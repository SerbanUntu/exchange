add_executable(archiver-process)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(archiver-process PRIVATE ${SOURCES})

target_link_libraries(archiver-process PRIVATE exchange::server::archiver)
