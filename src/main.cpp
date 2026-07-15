#include "config.h"
#include <iostream>
#include <optional>
#include <system_error>
#include <slim/command_line_handler.h>
#include <slim/common/exception.h>
#include <slim/common/log.h>
#include <slim/configuration_handler.h>
#include <slim/slim.h>
#include <slim/slim_v8.h>
int main(int argc, char *argv[]) {
    using namespace slim::common;
    try {
        slim::configuration_handler::set_startup_logging(argc, argv);
        log::trace(log::Message(__func__, "begins",__FILE__, __LINE__));
        auto v8_command_line_arguments = slim::command_line::parse(argc, argv);
        log::trace(log::Message(__func__, "parsed command line",__FILE__, __LINE__));
        for(auto&& argument_string : v8_command_line_arguments) {
		    log::debug(log::Message(__func__,"v8 command line argument => " + argument_string,__FILE__,__LINE__));	
	    }
        slim::v_8::initialize(v8_command_line_arguments);
        log::trace(log::Message(__func__, "initialized v8",__FILE__, __LINE__));
        slim::configuration_handler::load();
        slim::configuration_handler::disable_startup_logging();
        log::trace(log::Message(__func__, "loaded configuration",__FILE__, __LINE__));       
        log::debug(log::Message(__func__, "calling slim::start()",__FILE__, __LINE__));
        slim::start();
        log::debug(log::Message(__func__, "slim::start() ended",__FILE__, __LINE__));
    }
    catch (const std::bad_optional_access& error) {
        log::error(log::Message(__func__, error.what(),__FILE__, __LINE__));
    }
    catch(const std::string error) {
        log::error(log::Message(__func__, error,__FILE__, __LINE__));
    }
    catch(const std::error_code error_code) {
        log::error(log::Message(__func__, error_code.message(),__FILE__, __LINE__));
    }
    catch(const slim::common::SlimFileException& error) {
        std::string error_message = error.message + ", path => " + error.path;
        log::error(log::Message(error.call, error_message,__FILE__, __LINE__));
    }
    catch(const slim::common::SlimException& error) {
        log::error(log::Message(error.call, error.message,__FILE__, __LINE__));
    }
    catch (const std::invalid_argument& error) {
        log::error(log::Message(__func__, error.what(),__FILE__, __LINE__));
    }
    catch (const std::runtime_error& error) {
        log::error(log::Message(__func__, error.what(),__FILE__, __LINE__));
    }
    catch (const std::exception& error) {
        log::error(log::Message(__func__, error.what(),__FILE__, __LINE__));
    }
    catch(...) {
        log::error(log::Message(__func__, "caught unknown exception",__FILE__, __LINE__));
    }
    slim::v_8::tear_down();
    log::trace(log::Message(__func__,"exiting",__FILE__, __LINE__));
    return 0;
}