#pragma once

#include <memory>
#include <unordered_map>
#include <mutex>

namespace minidfs::view {
    enum class ViewID {
        Auth,
        Login,
        FileExplorer,
        Settings,
        Workspace,
        Activity,
        Devices,
        Default
    };

    class AppView {
    public:
        AppView() = default;
        virtual ~AppView() = default;

        virtual ViewID get_view_id() = 0;
        virtual void render() = 0;

    protected:
        ViewID view_id;
    };

    class ViewRegistry {
        
    public:
        void init_default_view();
        void register_view(ViewID id, std::unique_ptr<AppView> view);
        void switch_view(ViewID id);
        void render_current_view();
        ViewID get_current_view_id() const;

        static ViewRegistry& get();

    private:
        std::unordered_map<ViewID, std::unique_ptr<AppView>> views_;
        AppView* current_view_ = nullptr;
        ViewID current_view_id_ = ViewID::Default;
        
        mutable std::mutex mutex_;
    };

    // Public API functions
    void register_view(ViewID id, std::unique_ptr<AppView> view);
    void switch_view(ViewID id);
    void render_current_view();
    ViewID get_current_view_id();
}