#pragma once
extern sge::ref<sge::application> create_app_instance();
int32_t main(int32_t argc, const char** argv) {
    auto app = create_app_instance();
    sge::application::set(app);
    app->init();

    // todo: run

    app->shutdown();
    sge::application::set(nullptr);
    return 0;
}