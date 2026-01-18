#pragma once


namespace minidfs::view {
    enum class ViewID {
        Auth,
        Login,
        FileExplorer,
        Settings,
        TS,
        None
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
}