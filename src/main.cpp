#include "config.h"
#include <iostream>
#include <optional>
#include <system_error>
#include <slim/command_line_handler.h>
#include <slim/common/exception.h>
#include <slim/configuration_handler.h>
#include <slim/slim.h>
#include <slim/slim_v8.h>

int main(int argc, char *argv[]) {
    using namespace slim::common;
    try {
        slim::configuration_handler::load();
        auto v8_command_line_arguments = slim::command_line::parse(argc, argv);
        slim::start();
    }
    catch (const std::bad_optional_access& error) {
        std::cerr << error.what() << std::endl;
    }
    catch(const std::string error) {
        std::cerr << error << std::endl;
    }
    catch(const std::error_code error_code) {
        std::cerr << error_code.message() << std::endl;
    }
    catch(const slim::common::SlimFileException& error) {
        std::cerr << error.message << ", path => " << error.path << std::endl;
    }
    catch(const slim::common::SlimException& error) {
        std::cerr << error.message << std::endl;
    }
    catch (const std::invalid_argument& error) {
        std::cerr << error.what() << std::endl;
    }
    catch (const std::runtime_error& error) {
        std::cerr << error.what() << std::endl;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
    }
    catch(...) {
        std::cerr << "caught unknown exception" << std::endl;
    }
    slim::v_8::tear_down();
    return 0;
}
