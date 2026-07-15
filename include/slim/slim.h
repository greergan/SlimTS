#ifndef __SLIM__H
#define __SLIM__H
#include <functional>
#include <string>
namespace slim {
    using _network_listener_function = std::function<void(char* request_pointer)>;

    void start();
    void version(void);
}
#endif