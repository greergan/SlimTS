#include "config.h"
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <optional>
#include <string_view>
#include <system_error>
#include <vector>
#include <slim/command_line_handler.h>
#include <slim/common/exception.h>
#include <slim/common/io/error_codes.h>
#include <slim/configuration_handler.h>
#include <slim/slim.h>
#include <slim/slim_v8.h>
namespace {
void on_stop(int) {
    std::cerr << "SIGINT/SIGTERM received" << std::endl;
    slim::stop();
}
void on_reload(int) {
    std::cerr << "SIGHUP received" << std::endl;
    slim::configuration_handler::load();
}
void setup_signals() {
    struct sigaction sa{};
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = on_stop;
    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sa.sa_handler = on_reload;
    sigaction(SIGHUP, &sa, nullptr);
}
static constexpr const char* daemon_sentinel = "--daemon-child";
} // namespace
int main(int argc, char *argv[]) {
    using namespace slim::common;
    // check for daemon sentinel before anything else — Go runtime must not
    // be used in the parent before fork(); re-exec gives the child a clean
    // Go runtime by starting a fresh process image after fork()/setsid()
    bool is_daemon_child = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == daemon_sentinel) {
            is_daemon_child = true;
            for (int j = i; j < argc - 1; ++j) argv[j] = argv[j + 1];
            --argc;
            break;
        }
    }
    bool v8_initialized = false;
    try {
        auto v8_command_line_arguments = slim::command_line::parse(argc, argv);
        slim::configuration_handler::load();
        if (slim::configuration_handler::is_daemon() && !is_daemon_child) {
            pid_t pid = fork();
            if (pid < 0) throw std::runtime_error("fork() failed");
            if (pid > 0) return 0; // parent exits
            if (setsid() < 0) {
                throw std::system_error(errno, std::generic_category(), "setsid() failed");
            }
            // re-exec self with sentinel; child gets a clean Go runtime
            std::vector<char*> new_argv(argv, argv + argc);
            new_argv.push_back(const_cast<char*>(daemon_sentinel));
            new_argv.push_back(nullptr);
            execv("/proc/self/exe", new_argv.data());
            // execv only returns on failure
            throw std::system_error(errno, std::generic_category(), "execv() failed");
        }
        slim::v_8::initialize(v8_command_line_arguments);
        v8_initialized = true;
        setup_signals();
        do {
            slim::start();
        } while (slim::is_restart_requested());
    }
    catch (const std::bad_optional_access& error) {
        std::cerr << "Exception caught\n";
        std::cerr << error.what() << std::endl;
    }
    catch (const std::string& error) {
        std::cerr << "Exception caught\n";
        std::cerr << error << std::endl;
    }
    catch (const std::error_code& error_code) {
        std::cerr << "Exception caught\n";
        std::cerr << error_code.message() << std::endl;
    }
    catch (const slim::common::io::IOException& error) {
        std::cerr << "Exception caught\n";
        std::cerr << error.what() << std::endl;
    }
    catch (const slim::common::SlimFileException& error) {
        std::cerr << "Exception caught\n";
        std::cerr << error.message << ", path => " << error.path << std::endl;
    }
    catch (const slim::common::SlimException& error) {
        std::cerr << "Exception caught\n";
        std::cerr << error.message << std::endl;
    }
    catch (const std::invalid_argument& error) {
        std::cerr << "Exception caught\n";
        std::cerr << error.what() << std::endl;
    }
    catch (const std::runtime_error& error) {
        std::cerr << "Exception caught\n";
        std::cerr << error.what() << std::endl;
    }
    catch (const std::exception& error) {
        std::cerr << "Exception caught\n";
        std::cerr << error.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Exception caught\n";
        std::cerr << "caught unknown exception" << std::endl;
    }
    if (v8_initialized) slim::v_8::tear_down();
    return 0;
}
