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


#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "minidfs.grpc.pb.h"
#include "minidfs.pb.h"
#include "minidfs_impl.h"
#include "minidfs_client.h"

#include "panel.h"
#include "panel_ui.h"
#include "ui_registry.h"
#include "app_view_registry.h"
#include "file_explorer_panel.h"
#include "file_sidebar_panel.h"
#include "navbar_panel.h"
#include "worker_pool.h"
#include "minidfs_client.h"
#include "app_view.h"
#include "file_explorer_view.h"
#include "application.h"   
#include "asset_manager.h" 