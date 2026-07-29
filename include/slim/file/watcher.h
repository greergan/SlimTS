#pragma once
#include <functional>
#include <string_view>
namespace slim::file::watcher {
    void add(std::string_view path);
    void clear();
    void on_change(std::function<void()> restart);
    void stop();
    void watch();

    class Watcher {
    public:
        explicit Watcher(std::function<void()> restart) {
            on_change(std::move(restart));
        }
        ~Watcher() {
            stop();
            clear();
        }
        Watcher(const Watcher&) = delete;
        Watcher& operator=(const Watcher&) = delete;
    };
}
