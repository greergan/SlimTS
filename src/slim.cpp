#include <cstdlib>
#include <filesystem>
#include <future>
#include <string>
#include "config.h"
#include <v8.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <slim/command_line_handler.h>
#include <slim/common/log.h>
#include <slim/common/memory/mapper.h>
#include <slim/configuration_handler.h>
#include <slim/module/import_specifier.h>
#include <slim/service/launcher.h>
#include <slim/slim.h>
#include <slim/slim_v8.h>
#include <slim/utilities.h>
namespace slim {
	using namespace slim::common;
	static void initialize_ssl();
}
static void slim::initialize_ssl() {
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();
}
void slim::start() {
	auto& script = slim::command_line::get_script_name();
	initialize_ssl();
	log::trace(log::Message(__func__,"begins => " + script,__FILE__, __LINE__));
	slim::service::launcher::marshal_resources();

	log::debug(log::Message(__func__,"creating => typescript_launch_stub",__FILE__, __LINE__));
	slim::module::specifier_stub typescript_launch_stub {
		"file:///slim/launchable_service/bin/typescript.mjs",
		slim::common::memory_mapper::read("launchable_service_bin", "file:///slim/launchable_service/bin/typescript.mjs"),
		true
	};
	log::debug(log::Message(__func__,"created => typescript_launch_stub",__FILE__, __LINE__));

	auto launch_typescript_future = std::async(std::launch::async, slim::service::launcher::launch, typescript_launch_stub);
	log::debug(log::Message(__func__,"launched => launch_typescript_future",__FILE__, __LINE__));

	auto launch_script_future = std::async(std::launch::async, slim::service::launcher::launch, script);
	log::debug(log::Message(__func__,std::format("launched => {}", script),__FILE__, __LINE__));

	if(launch_script_future.valid()) {
		log::debug(log::Message(__func__,"script future is valid",__FILE__, __LINE__));
		launch_script_future.get();
		log::debug(log::Message(__func__,std::format("script => {} => completed", script),__FILE__, __LINE__));
	}
	else {
		log::debug(log::Message(__func__,"future is not valid",__FILE__, __LINE__));
	}


	log::trace(log::Message(__func__,"ends => " + script,__FILE__, __LINE__));
}
void slim::version() {
	log::info("slim:  " VERSION);
	log::info("libv8:  " + std::string(v8::V8::GetVersion()));
}
