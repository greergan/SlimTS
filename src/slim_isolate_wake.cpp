#include <functional>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <v8.h>
#include <slim/isolate_wake.h>

namespace slim::isolate_wake {
    static std::mutex registry_mutex;
    static std::unordered_map<v8::Isolate*, IsolateWake> registry;

    IsolateWake& register_isolate(v8::Isolate* isolate) {
        std::lock_guard lock(registry_mutex);
        auto [it, inserted] = registry.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple());
        return it->second;
    }

    void unregister_isolate(v8::Isolate* isolate) {
        std::lock_guard lock(registry_mutex);
        registry.erase(isolate);
    }

    void post(v8::Isolate* isolate, std::function<void()> task) {
        std::lock_guard lock(registry_mutex);
        auto it = registry.find(isolate);
        if (it == registry.end()) return;
        {
            std::lock_guard task_lock(it->second.mutex);
            it->second.tasks.push(std::move(task));
        }
        it->second.semaphore.release();
    }

    void signal(v8::Isolate* isolate) {
        std::lock_guard lock(registry_mutex);
        auto it = registry.find(isolate);
        if (it == registry.end()) return;
        it->second.semaphore.release();
    }

    void drain(v8::Isolate* isolate) {
        std::queue<std::function<void()>> local;
        {
            std::lock_guard lock(registry_mutex);
            auto it = registry.find(isolate);
            if (it == registry.end()) return;
            std::lock_guard task_lock(it->second.mutex);
            std::swap(local, it->second.tasks);
        }

        if (local.empty()) { return; }

        while (!local.empty()) {
            local.front()();
            local.pop();
        }
    }
} // namespace slim::isolate_wake
