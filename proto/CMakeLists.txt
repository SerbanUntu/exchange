file(GLOB_RECURSE PROTO_FILES CONFIGURE_DEPENDS *.proto)

add_library(exchange-proto ${PROTO_FILES})
add_library(exchange::proto ALIAS exchange-proto)
target_link_libraries(exchange-proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)
target_include_directories(exchange-proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(TARGET exchange-proto LANGUAGE cpp)
protobuf_generate(
        TARGET exchange-proto
        LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
)
