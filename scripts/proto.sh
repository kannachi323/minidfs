protoc -I=proto \
    --cpp_out=proto_src \
    --grpc_out=proto_src \
    --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
    proto/minidfs.proto