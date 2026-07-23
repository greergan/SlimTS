#pragma once
#include <v8.h>
#include <functional>
#include <mutex>
#include <queue>
#include <semaphore>
#include <unordered_map>

namespace slim::isolate_wake {

    struct IsolateWake {
        std::mutex mutex;
        std::queue<std::function<void()>> tasks;
        std::binary_semaphore semaphore{0};
    };

    IsolateWake& register_isolate(v8::Isolate*);
    void unregister_isolate(v8::Isolate*);
    void post(v8::Isolate*, std::function<void()> task);
    void signal(v8::Isolate*);
    void drain(v8::Isolate*);

} // namespace slim::isolate_wake
