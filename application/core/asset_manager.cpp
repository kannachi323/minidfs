#include "asset_manager.h"
#include "stb_image.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>

namespace minidfs {
    void AssetManager::shutdown() {
        for (auto& [name, texture] : svg_textures_) {
            unload_svg(texture);
        }
        svg_textures_.clear();
        for (auto& [name, texture] : image_textures_) {
            if (texture.id != 0) {
                glDeleteTextures(1, &texture.id);
            }
        }
        image_textures_.clear();
    }

    AssetManager& AssetManager::get() {
        static AssetManager instance;
        return instance;
    }

    void AssetManager::load_fonts() {
        ImGuiIO& io = ImGui::GetIO();
        fonts_[FontID::DEFAULT] = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", 18.0f);;
        fonts_[FontID::ROBOTO_SMALL] = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", 16.0f);
        fonts_[FontID::ROBOTO_LARGE] = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", 24.0f);
    }

    void AssetManager::load_themes() {
        std::ifstream file("assets/themes/default.css");
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            current_theme_ = buffer.str();
            
            // If theme changes, we must clear cache to re-render icons
            svg_textures_.clear(); 
        }
    }

    ImFont* AssetManager::get_font(const FontID& font_id) const {
        if (!fonts_.contains(font_id)) {
            return fonts_.at(FontID::DEFAULT);
        }
        return fonts_.at(font_id);
    }

    const std::string& AssetManager::get_current_theme() const {
        return current_theme_;
    }

    SVGTexture& AssetManager::get_svg_texture(const std::string& name, int size) {
        std::string key = name + ".svg";

        // 1. Check if it's already in the cache
        auto it = svg_textures_.find(key);
        if (it != svg_textures_.end()) {
            return it->second;
        }

        // 2. Try to load the requested icon
        std::string path = "assets/icons/" + key;

        std::cout << "Loading icon: " << path << std::endl;
        SVGTexture tex = load_svg(path, size, size);


        // 4. Cache whatever we ended up with (actual icon, fallback, or null)
        svg_textures_[key] = tex;
        return svg_textures_[key];
    }

    ImageTexture& AssetManager::get_image_texture(const std::string& path) {
        auto it = image_textures_.find(path);
        if (it != image_textures_.end()) {
            return it->second;
        }

        // Load the image
        int width, height, channels;
        unsigned char* image_data = stbi_load(path.c_str(), &width, &height, &channels, 4); // Force RGBA

        ImageTexture tex = { 0, 0, 0 };

        if (image_data == nullptr) {
			std::cout << "Failed to load image: " << path << std::endl;
            image_textures_[path] = tex;
            return image_textures_[path];
        }

        // Create OpenGL texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        // Setup filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Upload pixels to texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

        // Free image data
        stbi_image_free(image_data);

        // Cache and return
        tex.id = texture;
        tex.width = width;
        tex.height = height;

        image_textures_[path] = tex;
        return image_textures_[path];
    }
    

}