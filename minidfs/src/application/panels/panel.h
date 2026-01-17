#pragma once

#include "imgui.h"
#include <string>

namespace minidfs::panel {
    class Panel {
    public:
        virtual ~Panel() = default;
        virtual void render() = 0;
 
    };
};