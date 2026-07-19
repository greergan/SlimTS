#include <print>
#include <future>
#include <string>
#include "config.h"
#include <v8.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <slim/command_line_handler.h>
#include <slim/service/launcher.h>
#include <slim/slim.h>

namespace slim {
namespace {
static void initialize_ssl() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}
} // namespace

void start() {
    initialize_ssl();
	auto launch_script_future = std::async(std::launch::async, slim::service::launcher::launch, slim::command_line::get_script_name());
	launch_script_future.get();
}
void version() {
    std::println("slim:  {}", VERSION);
	std::println("libv8: {} ", v8::V8::GetVersion());
}
} // namespace slim
