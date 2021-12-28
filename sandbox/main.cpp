#define SGE_INCLUDE_MAIN
#include <sge.h>
namespace sandbox {
    class sandbox_app : public sge::application {
    public:
        sandbox_app() : application("Sandbox") { }

    protected:
        virtual void load_content() override {
            spdlog::info("load or something");
        }
        virtual void unload_content() override {
            spdlog::info("unload?");
        }
    };
}

sge::ref<sge::application> create_app_instance() {
    return sge::ref<sandbox::sandbox_app>::create();
};