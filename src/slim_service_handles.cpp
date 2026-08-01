#include <atomic>
#include <slim/service/handles.h>

namespace slim::service::handles {
    std::atomic<int> active_handles{0};
}

int slim::service::handles::count() {
    return active_handles;
}

void slim::service::handles::decrement() {
    active_handles--;
}

void slim::service::handles::increment() {
    active_handles++;
}
