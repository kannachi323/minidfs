#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "svg_loader.h"




namespace minidfs {
    enum class FontID {
        DEFAULT,
        ROBOTO_SMALL,
        ROBOTO_LARGE
    };

    struct ImageTexture {
        GLuint id;
        int width;
        int height;
    };

    class AssetManager {
    public:
        // Singleton Access
        static AssetManager& get();

        void load_fonts();

        // Load a CSS theme file into memory
        void load_themes();

        // Get an icon (Loads and caches if not found)
        SVGTexture& get_svg_texture(const std::string& name, int size = 24);

        ImageTexture& get_image_texture(const std::string& path);

        // In AssetManager.h
        const std::string& get_current_theme() const;

        ImFont* get_font(const FontID& fond_id) const;

        // Cleanup GPU resources
        void shutdown();

    private:
        AssetManager() = default;
        ~AssetManager() = default;

        std::string current_theme_;
        std::unordered_map<FontID, ImFont*> fonts_;
        std::unordered_map<std::string, SVGTexture> svg_textures_;
        std::unordered_map<std::string, ImageTexture> image_textures_;
    };
}