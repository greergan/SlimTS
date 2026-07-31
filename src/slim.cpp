#include <print>
#include <filesystem>
#include <future>
#include <stop_token>
#include <string>
#include "config.h"
#include <v8.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <slim/configuration_handler.h>
#include <slim/file/watcher.h>
#include <slim/runtime.h>
#include <slim/service/launcher.h>
#include <slim/slim.h>
#include <slim/slim_v8.h>
#include <slim/common/log.h>
#include <libtsgo.h>

namespace slim {
namespace {
using namespace slim::common;
static std::stop_source stop_source;
static std::once_flag ssl_init_flag;
static bool restart_requested = false;

static void initialize_ssl() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}
} // namespace

std::stop_token get_stop_token() {
    return stop_source.get_token();
}

bool is_restart_requested() {
    return restart_requested;
}

void restart() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    restart_requested = true;
    slim::stop();
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}

void start() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    restart_requested = false;
    stop_source = std::stop_source{};
    std::call_once(ssl_init_flag, initialize_ssl);
    auto script_name = slim::configuration_handler::get_script_name();
    if(script_name.empty()) {
        std::println("usage: slimts [options] <script>");
        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
        return;
    }
    load_types_dir((char*)(std::filesystem::current_path().string() + "/types").c_str());
    if(slim::configuration_handler::is_watching()) {
		slim::file::watcher::watch_dir(std::filesystem::current_path().string() + "/types");
    }
    log::debug(log::Message(__func__, "starting runtime instance", __FILE__, __LINE__));
    slim::runtime::instance().start();
    log::debug(log::Message(__func__, std::format("launching script => {}", script_name), __FILE__, __LINE__));
    auto launch_script_future = std::async(std::launch::async, slim::service::launcher::launch, script_name);
    launch_script_future.get();
    slim::v_8::dispose_isolate(script_name);
    log::debug(log::Message(__func__, "stop runtime instance", __FILE__, __LINE__));
    slim::runtime::instance().stop();
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}

void stop() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    stop_source.request_stop();
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}

void version() {
    std::println("slimts: {}", VERSION);
    std::println("libv8:  {}", v8::V8::GetVersion());
    exit(0);
}
} // namespace slim
