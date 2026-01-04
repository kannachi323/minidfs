#pragma once

#include "imgui.h"
#include "registry.h"
#include <string>

namespace minidfs {
    class Layer {
    public:
        virtual ~Layer() = default;
        virtual void render() = 0;
 
    };
};