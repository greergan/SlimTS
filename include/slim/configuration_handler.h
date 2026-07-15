#ifndef __SLIM__CONFIGURATION_HANDLER__H
#define __SLIM__CONFIGURATION_HANDLER__H
#include <string_view>
#include <slim/common/log.h>
namespace slim::configuration_handler {
	bool can_log(std::string_view _consumer, std::string_view _log_level, std::string_view _file, std::string_view _function);
	void disable_startup_logging();
	void load();
	void log_startup_tasks_debug(bool _bool);
	void log_startup_tasks_trace(bool _bool);
	void set_startup_logging(int argc, char* argv[]);
}
#endif