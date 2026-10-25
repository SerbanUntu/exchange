cmake_minimum_required(VERSION 4.2)

project(exchange)

# After project() call to prevent from being overwritten by Conan to C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prevent min/max macros from clashing with std
if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

set(OPTIMIZATION_LEVEL "" CACHE STRING "Release optimization level (0, 1, 2 or 3)")
set_property(CACHE OPTIMIZATION_LEVEL PROPERTY STRINGS "" 0 1 2 3)
if(NOT OPTIMIZATION_LEVEL STREQUAL "")
    if(NOT OPTIMIZATION_LEVEL MATCHES "^[0-3]$")
        message(FATAL_ERROR "OPTIMIZATION_LEVEL must be 0, 1, 2 or 3, got '${OPTIMIZATION_LEVEL}'")
    endif()
    if(MSVC)
        set(_msvc_opt_flags "/Od" "/O1" "/O2 /Ob2" "/O2 /Ob2")
        list(GET _msvc_opt_flags ${OPTIMIZATION_LEVEL} _opt_flag)
        set(CMAKE_CXX_FLAGS_RELEASE "${_opt_flag} /DNDEBUG")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "-O${OPTIMIZATION_LEVEL} -DNDEBUG")
    endif()
endif()

find_package(gRPC REQUIRED)     # gRPC::grpc++
find_package(protobuf REQUIRED) # protobuf::protobuf
find_package(spdlog REQUIRED)   # spdlog::spdlog
find_package(GTest REQUIRED)    # GTest::gtest_main
find_package(Boost REQUIRED)    # boost::boost
find_package(aeron REQUIRED)    # aeron::aeron
find_package(TBB)               # onetbb::onetbb
find_package(concurrentqueue)   # concurrentqueue::concurrentqueue
find_package(CURL)              # CURL::libcurl
find_package(nlohmann_json)     # nlohmann_json::nlohmann_json
find_package(uwebsockets)       # uwebsockets::uwebsockets

enable_testing()

add_subdirectory(proto)
add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(apps)
