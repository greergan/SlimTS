#include <print>
#include <future>
#include <csignal>
#include <stop_token>
#include <string>
#include "config.h"
#include <v8.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <slim/command_line_handler.h>
#include <slim/runtime.h>
#include <slim/service/launcher.h>
#include <slim/slim.h>
#include <slim/common/log.h>
using namespace slim::common;
namespace slim {
namespace {
static std::stop_source stop_source;
static void initialize_ssl() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}
} // namespace
void stop() {
    log::debug(log::Message(__func__, "stopping", __FILE__, __LINE__));
    stop_source.request_stop();
}
std::stop_token get_stop_token() {
    return stop_source.get_token();
}
void start() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    initialize_ssl();
    const auto& script_name = slim::command_line::get_script_name();
    if(script_name.empty()) {
        std::println("usage: slimts [options] <script>");
        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
        return;
    }
    log::debug(log::Message(__func__, std::format("launching script => {}", script_name), __FILE__, __LINE__));
    slim::runtime::instance().start();
    std::signal(SIGINT,  [](int){ slim::stop(); });
    std::signal(SIGTERM, [](int){ slim::stop(); });
    auto launch_script_future = std::async(std::launch::async, slim::service::launcher::launch, script_name);
    launch_script_future.get();
    slim::runtime::instance().stop();
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}
void version() {
    std::println("slim:  {}", VERSION);
    std::println("libv8: {} ", v8::V8::GetVersion());
}
} // namespace slim
