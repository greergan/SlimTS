#ifndef SLIM_H
#define SLIM_H
#include <stop_token>
namespace slim {
    void start();
    void stop();
    void version();
    std::stop_token get_stop_token();
}
#endif
