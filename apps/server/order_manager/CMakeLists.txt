add_executable(order-manager)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

target_sources(order-manager PRIVATE ${SOURCES})

target_link_libraries(order-manager PRIVATE exchange::server::order_manager)
