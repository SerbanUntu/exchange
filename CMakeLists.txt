cmake_minimum_required(VERSION 4.2)

project(exchange)

# After project() call to prevent from being overwritten by Conan to C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(gRPC REQUIRED)     # gRPC::grpc++
find_package(protobuf REQUIRED) # protobuf::protobuf
find_package(spdlog REQUIRED)   # spdlog::spdlog
find_package(GTest REQUIRED)    # GTest::gtest_main
find_package(Boost)             # boost::boost
find_package(TBB)               # onetbb::onetbb
find_package(concurrentqueue)   # concurrentqueue::concurrentqueue
find_package(CURL)              # CURL::libcurl
find_package(nlohmann_json)     # nlohmann_json::nlohmann_json
find_package(uwebsockets)       # uwebsockets::uwebsockets

enable_testing()

add_subdirectory(common)
add_subdirectory(proto)
add_subdirectory(client)
add_subdirectory(server)
