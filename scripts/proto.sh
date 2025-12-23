PROTOC=/opt/homebrew/bin/protoc
GRPC_CPP_PLUGIN=/opt/homebrew/bin/grpc_cpp_plugin

$PROTOC -I=proto \
    --cpp_out=proto_src \
    --grpc_out=proto_src \
    --plugin=protoc-gen-grpc=$GRPC_CPP_PLUGIN \
    proto/minidfs.proto
