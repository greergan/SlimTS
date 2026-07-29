#ifndef SLIM_H
#define SLIM_H
#include <stop_token>
namespace slim {
    bool is_restart_requested();
    void restart();
    void start();
    void stop();
    void version();
    std::stop_token get_stop_token();
}
#endif
