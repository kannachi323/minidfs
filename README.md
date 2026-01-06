# MiniDFS

Minimal, educational distributed file system built in modern C++ using gRPC. It provides a simple server with file-locking and streaming I/O plus a cross-platform ImGui desktop client to browse, upload, download, and delete files under a mount directory.

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![CMake](https://img.shields.io/badge/CMake-%3E%3D3.10-brightgreen)
![gRPC](https://img.shields.io/badge/gRPC-C%2B%2B-orange)
![ImGui](https://img.shields.io/badge/ImGui-UI-lightgrey)
![vcpkg](https://img.shields.io/badge/vcpkg-deps-success)

## Overview

- **Server**: `MiniDFSService` over gRPC with file list, locks, upload (stream), download (stream), delete, and server-push updates.
- **Client (GUI)**: ImGui desktop app (`minidfs_client`) connecting to `localhost:50051` with a file explorer panel for browsing and actions.
- **Library**: `minidfs` static library encapsulates `MiniDFSClient`, `MiniDFSImpl`, `FileManager`, and `PubSubManager`.
- **Protocols**: Protobuf and gRPC service definitions in [proto/minidfs.proto](proto/minidfs.proto) (Auth proto exists but is experimental).

## Why It’s Useful

- **Clear primitives**: Read/write locks, chunked streaming I/O, server push notifications.
- **Small surface area**: Easy to read and extend for learning distributed systems concepts.
- **Cross-platform UI**: ImGui-based client; Windows code included, macOS code scaffolded.

## Getting Started

### Prerequisites

- CMake >= 3.10
- A C++20 compiler (MSVC/Clang/GCC)
- vcpkg with the following ports installed: `protobuf`, `grpc`, `abseil`, `glfw3`, `opengl`, `gtest`, `openssl`

### Configure and Build (Windows)

1. Install vcpkg if you don’t have it:

```powershell
git clone https://github.com/microsoft/vcpkg.git $env:USERPROFILE\vcpkg
$env:USERPROFILE\vcpkg\bootstrap-vcpkg.bat
```

2. Configure and build with the vcpkg toolchain:

```powershell
cd c:\Users\mtcco\projects\minidfs
cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=$env:USERPROFILE\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release
```

This generates the gRPC/protobuf sources and builds these targets:
- `minidfs` (static library)
- `minidfs_server` (server executable)
- `minidfs_client` (ImGui desktop client)
- `minidfs_tests` (unit tests)

### Run the Server

```powershell
build\bin\Release\minidfs_server.exe minidfs
```

- The optional argument sets the server mount directory; default is `minidfs` created in the current working directory.

### Run the Client (GUI)

```powershell
build\bin\Release\minidfs_client.exe
```

- Connects to `localhost:50051`.
- Client mount path defaults to `minidfs`. Use the File Explorer panel to navigate, upload, open, and delete files.

### Programmatic Usage (C++)

Minimal example using `MiniDFSClient`:

```cpp
#include <grpcpp/grpcpp.h>
#include "dfs/minidfs_client.h"

int main() {
	auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
	MiniDFSClient client(channel, "minidfs");

	const std::string client_id = "client1";

	// Upload a local file to the server mount
	client.StoreFile(client_id, "minidfs/example.txt");

	// Download a file from the server to the client mount
	client.FetchFile(client_id, "minidfs/example.txt");

	// List files under a directory
	minidfs::ListFilesRes res;
	client.ListFiles("minidfs", &res);
}
```

See [tests/minidfs_single_client_tests.cpp](tests/minidfs_single_client_tests.cpp) for more examples.

## Project Structure

- `dfs/`: `MiniDFSClient`, `MiniDFSImpl`, `FileManager`, `PubSubManager`
- `proto/`: Protocol definitions
- `proto_src/`: Generated C++ sources (created by CMake) for protobuf/gRPC
- `application/`: ImGui client app (panels, layout, platform code)
- `cmake/`: Build setup scripts for proto, library, server, app, tests
- `tests/`: GoogleTest suites for file manager and client–server interactions

## Testing

Build `minidfs_tests` and run:

```powershell
build\bin\Release\minidfs_tests.exe
```

See additional notes in [TESTS.md](TESTS.md).

## Help & Support

- Read the service definitions in [proto/minidfs.proto](proto/minidfs.proto).
- Check UI panels and workflow in [application/panels](application/panels).
- Open an issue or discussion in your fork if you need help.

## Contributing & Maintainers

- Maintainer: mattdev727@gmail.com
- Contributions are welcome. Please read [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) for guidelines.

## License

MIT License. See [LICENSE](LICENSE).
