#pragma once

// OpenGL must be first!
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Standard library
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>

#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "minidfs.grpc.pb.h"
#include "minidfs.pb.h"
#include "dfs/server/minidfs_impl.h"
#include "dfs/client/minidfs_client.h"

